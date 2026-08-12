#ifndef SEEDUI_SMARTGUIDES_H
#define SEEDUI_SMARTGUIDES_H

// Guias inteligentes (estilo CorelDRAW): durante o arraste de mover, alinham
// bordas e centros da seleção com a tela-base e com outros elementos visíveis,
// mostrando a linha-guia. A moldura da tela base tem snap FORTE (tolerância
// ampliada) — é o delimitador principal. Também preveem espaçamentos repetidos
// entre elementos vizinhos (guias de espaçamento). Fazem parte do auxílio de
// edição (M04) — não entram no projeto.ui.json. Header-only para o self-test
// M04 cobrir a matemática.

#include "Project.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace seedui
{
    namespace SmartGuides
    {
        // Retângulo de um elemento no início do arraste (espaço do projeto).
        struct Rect
        {
            float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
        };

        // Adiciona as coordenadas de bordas/centro de um elemento visível que
        // NÃO faz parte da seleção (a seleção move inteira: pai + filhos).
        inline void CollectCandidates(const Element& element,
                                      const std::vector<std::string>& selectedIds,
                                      std::vector<float>& xs,
                                      std::vector<float>& ys)
        {
            const bool selected = std::find(selectedIds.begin(), selectedIds.end(),
                                            element.id) != selectedIds.end();
            if (!element.visivel || selected) return;
            const float x = element.transformacao.value("x", 0.0f);
            const float y = element.transformacao.value("y", 0.0f);
            const float w = element.transformacao.value("largura", 160.0f);
            const float h = element.transformacao.value("altura", 32.0f);
            xs.push_back(x);
            xs.push_back(x + w * 0.5f);
            xs.push_back(x + w);
            ys.push_back(y);
            ys.push_back(y + h * 0.5f);
            ys.push_back(y + h);
            for (const Element& child : element.filhos)
                CollectCandidates(child, selectedIds, xs, ys);
        }

        // Reúne os retângulos (coordenadas absolutas) dos elementos visíveis
        // que NÃO fazem parte da seleção — base das guias de espaçamento.
        inline void CollectRects(const Element& element,
                                 const std::vector<std::string>& selectedIds,
                                 std::vector<Rect>& out)
        {
            if (!element.visivel) return;
            const bool selected = std::find(selectedIds.begin(), selectedIds.end(),
                                            element.id) != selectedIds.end();
            if (!selected)
            {
                Rect r;
                r.x = element.transformacao.value("x", 0.0f);
                r.y = element.transformacao.value("y", 0.0f);
                r.w = element.transformacao.value("largura", 160.0f);
                r.h = element.transformacao.value("altura", 32.0f);
                out.push_back(r);
            }
            for (const Element& child : element.filhos)
                CollectRects(child, selectedIds, out);
        }

        // Snap de uma GUIA arrastada da régua (estilo CorelDRAW): a guia
        // gruda nas laterais/centros das formas visíveis e na moldura da
        // tela-base — a régua vira ferramenta funcional de alinhamento, não
        // só estética. `horizontal` = guia horizontal (eixo Y: topo/centro/
        // base); senão guia vertical (eixo X: esquerda/centro/direita).
        // Devolve a posição encaixada (a própria `value` quando nada dentro
        // da tolerância) e `outSnapped` = true se houve encaixe.
        inline float SnapGuideToShapes(float value, bool horizontal,
                                       const Modo& mode,
                                       float frameW, float frameH,
                                       float tolerance, bool& outSnapped)
        {
            outSnapped = false;
            std::vector<float> xs;
            std::vector<float> ys;
            const std::vector<std::string> noSelection;
            for (const Element& e : mode.raiz)
                CollectCandidates(e, noSelection, xs, ys);
            const std::vector<float>& cands = horizontal ? ys : xs;
            float best = value;
            float bestDist = tolerance;
            for (const float cand : cands)
            {
                const float d = fabsf(cand - value);
                if (d < bestDist) { bestDist = d; best = cand; outSnapped = true; }
            }
            // Moldura da tela-base (0, centro, fim): snap forte, é o
            // delimitador principal — vence as formas em caso de empate.
            const float frameMax = horizontal ? frameH : frameW;
            const float frameCands[3] = { 0.0f, frameMax * 0.5f, frameMax };
            for (const float cand : frameCands)
            {
                const float d = fabsf(cand - value);
                if (d <= bestDist) { bestDist = d; best = cand; outSnapped = true; }
            }
            return best;
        }

        // Ajusta dx/dy para encaixar na candidata mais próxima (dentro da
        // tolerância, em unidades do projeto) e devolve a posição da guia.
        // A moldura da tela-base tem PRIORIDADE: tolerância ampliada (snap
        // forte) — o delimitador principal vence os demais snaps.
        inline void Apply(const Modo& mode,
                          const std::vector<Rect>& starts,
                          const std::vector<std::string>& selectedIds,
                          float& dx, float& dy,
                          float canvasW, float canvasH, float tolerance,
                          float& outGuideX, float& outGuideY)
        {
            outGuideX = -1.0f;
            outGuideY = -1.0f;
            if (starts.empty()) return;

            // Limites da seleção na posição candidata (dx, dy atuais).
            float selLeft = starts[0].x + dx, selTop = starts[0].y + dy;
            float selRight = starts[0].x + starts[0].w + dx;
            float selBottom = starts[0].y + starts[0].h + dy;
            for (const Rect& s : starts)
            {
                selLeft = std::min(selLeft, s.x + dx);
                selTop = std::min(selTop, s.y + dy);
                selRight = std::max(selRight, s.x + s.w + dx);
                selBottom = std::max(selBottom, s.y + s.h + dy);
            }
            const float selCx = (selLeft + selRight) * 0.5f;
            const float selCy = (selTop + selBottom) * 0.5f;

            // ---- Eixo X: moldura primeiro (tolerância FORTE, reforçada a
            // pedido do usuário: a tela base 1280x720 é a área de segurança
            // e deve "segurar" mais que qualquer outro snap).
            const float frameTol = tolerance * 5.0f;
            const float frameX[3] = { 0.0f, canvasW * 0.5f, canvasW };
            float bestDx = 0.0f, bestX = -1.0f, bestDist = frameTol;
            for (int i = 0; i < 3; ++i)
            {
                const float cand = frameX[i];
                const float dLeft = cand - selLeft;
                const float dCenter = cand - selCx;
                const float dRight = cand - selRight;
                if (fabsf(dLeft) < bestDist)   { bestDist = fabsf(dLeft);   bestDx = dLeft;   bestX = cand; }
                if (fabsf(dCenter) < bestDist) { bestDist = fabsf(dCenter); bestDx = dCenter; bestX = cand; }
                if (fabsf(dRight) < bestDist)  { bestDist = fabsf(dRight);  bestDx = dRight;  bestX = cand; }
            }
            if (bestX < 0.0f)
            {
                std::vector<float> xs;
                std::vector<float> unused;
                xs.reserve(16);
                for (const Element& e : mode.raiz)
                    CollectCandidates(e, selectedIds, xs, unused);
                bestDist = tolerance;
                for (const float cand : xs)
                {
                    const float dLeft = cand - selLeft;
                    const float dCenter = cand - selCx;
                    const float dRight = cand - selRight;
                    if (fabsf(dLeft) < bestDist)   { bestDist = fabsf(dLeft);   bestDx = dLeft;   bestX = cand; }
                    if (fabsf(dCenter) < bestDist) { bestDist = fabsf(dCenter); bestDx = dCenter; bestX = cand; }
                    if (fabsf(dRight) < bestDist)  { bestDist = fabsf(dRight);  bestDx = dRight;  bestX = cand; }
                }
            }
            if (bestX >= 0.0f) { dx += bestDx; outGuideX = bestX; }

            // ---- Eixo Y: topo, centro ou base (moldura primeiro).
            const float frameY[3] = { 0.0f, canvasH * 0.5f, canvasH };
            float bestDy = 0.0f, bestY = -1.0f, bestDistY = frameTol;
            for (int i = 0; i < 3; ++i)
            {
                const float cand = frameY[i];
                const float dTop = cand - selTop;
                const float dCenter = cand - selCy;
                const float dBottom = cand - selBottom;
                if (fabsf(dTop) < bestDistY)    { bestDistY = fabsf(dTop);    bestDy = dTop;    bestY = cand; }
                if (fabsf(dCenter) < bestDistY) { bestDistY = fabsf(dCenter); bestDy = dCenter; bestY = cand; }
                if (fabsf(dBottom) < bestDistY) { bestDistY = fabsf(dBottom); bestDy = dBottom; bestY = cand; }
            }
            if (bestY < 0.0f)
            {
                std::vector<float> ys;
                std::vector<float> unused;
                ys.reserve(16);
                for (const Element& e : mode.raiz)
                    CollectCandidates(e, selectedIds, unused, ys);
                bestDistY = tolerance;
                for (const float cand : ys)
                {
                    const float dTop = cand - selTop;
                    const float dCenter = cand - selCy;
                    const float dBottom = cand - selBottom;
                    if (fabsf(dTop) < bestDistY)    { bestDistY = fabsf(dTop);    bestDy = dTop;    bestY = cand; }
                    if (fabsf(dCenter) < bestDistY) { bestDistY = fabsf(dCenter); bestDy = dCenter; bestY = cand; }
                    if (fabsf(dBottom) < bestDistY) { bestDistY = fabsf(dBottom); bestDy = dBottom; bestY = cand; }
                }
            }
            if (bestY >= 0.0f) { dy += bestDy; outGuideY = bestY; }
        }

        // Linha do preview de espaçamento (Shift no arraste): `horizontal` =
        // guia longa de topo/base (ou tick horizontal de coluna); senão tick
        // vertical de espaço entre peças de uma fileira.
        struct GuideLine
        {
            bool horizontal = false;
            float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
        };

        struct GuideLabel
        {
            float x = 0.0f, y = 0.0f;
            float value = 0.0f;
            bool highlight = false; // rótulo do espaço PREVISTO (engatado)
        };

        // Preview de espaçamento estilo CorelDRAW (Shift durante o mover):
        // em vez de linhas longas, pequenos TRAÇOS nos cantos das laterais de
        // cada objeto da fileira alinhada (mesmos topo e base) — traços
        // horizontais curtos nos cantos das laterais esquerda/direita,
        // delimitando cada peça sem poluir. Idem para colunas verticais
        // (mesmos lados, traços verticais nos cantos de topo/base). Rótulos
        // com a distância de cada espaço. Só orientação visual — não entra
        // no projeto.ui.json.
        inline void ComputeSpacingPreview(Modo& mode,
                                          const std::vector<std::string>& selectedIds,
                                          float tolerance,
                                          float markSize,
                                          float highlightValue,
                                          std::vector<GuideLine>& lines,
                                          std::vector<GuideLabel>& labels)
        {
            lines.clear();
            labels.clear();
            float selL = FLT_MAX, selT = FLT_MAX, selR = -FLT_MAX, selB = -FLT_MAX;
            bool have = false;
            for (const std::string& id : selectedIds)
            {
                if (const Element* el = Project::ResolverId(mode, id))
                {
                    if (!el->visivel) continue;
                    const float x = el->transformacao.value("x", 0.0f);
                    const float y = el->transformacao.value("y", 0.0f);
                    const float w = el->transformacao.value("largura", 160.0f);
                    const float h = el->transformacao.value("altura", 32.0f);
                    selL = std::min(selL, x);
                    selT = std::min(selT, y);
                    selR = std::max(selR, x + w);
                    selB = std::max(selB, y + h);
                    have = true;
                }
            }
            if (!have || selR <= selL || selB <= selT) return;
            const Rect selRect{ selL, selT, selR - selL, selB - selT };

            std::vector<Rect> others;
            others.reserve(16);
            for (const Element& e : mode.raiz)
                CollectRects(e, selectedIds, others);

            // Fileira horizontal: traços nos cantos das laterais esquerda e
            // direita de cada objeto (delimitação de cada peça).
            {
                std::vector<Rect> row = { selRect };
                for (const Rect& o : others)
                    if (fabsf(o.y - selT) <= tolerance &&
                        fabsf(o.y + o.h - selB) <= tolerance)
                        row.push_back(o);
                if (row.size() >= 2)
                {
                    std::sort(row.begin(), row.end(),
                              [](const Rect& a, const Rect& b) { return a.x < b.x; });
                    for (const Rect& r : row)
                    {
                        lines.push_back({ true, r.x - markSize, r.y, r.x, r.y });
                        lines.push_back({ true, r.x - markSize, r.y + r.h, r.x, r.y + r.h });
                        lines.push_back({ true, r.x + r.w, r.y, r.x + r.w + markSize, r.y });
                        lines.push_back({ true, r.x + r.w, r.y + r.h, r.x + r.w + markSize, r.y + r.h });
                    }
                    for (size_t i = 1; i < row.size(); ++i)
                    {
                        const float gap = row[i].x - (row[i - 1].x + row[i - 1].w);
                        if (gap > 2.0f)
                            labels.push_back({ row[i - 1].x + row[i - 1].w + gap * 0.5f,
                                               selT - 7.0f, gap,
                                               highlightValue > 0.0f &&
                                               fabsf(gap - highlightValue) <= 0.5f });
                    }
                }
            }

            // Coluna vertical: traços nos cantos das laterais de topo/base.
            {
                std::vector<Rect> col = { selRect };
                for (const Rect& o : others)
                    if (fabsf(o.x - selL) <= tolerance &&
                        fabsf(o.x + o.w - selR) <= tolerance)
                        col.push_back(o);
                if (col.size() >= 2)
                {
                    std::sort(col.begin(), col.end(),
                              [](const Rect& a, const Rect& b) { return a.y < b.y; });
                    for (const Rect& r : col)
                    {
                        lines.push_back({ false, r.x, r.y - markSize, r.x, r.y });
                        lines.push_back({ false, r.x + r.w, r.y - markSize, r.x + r.w, r.y });
                        lines.push_back({ false, r.x, r.y + r.h, r.x, r.y + r.h + markSize });
                        lines.push_back({ false, r.x + r.w, r.y + r.h, r.x + r.w, r.y + r.h + markSize });
                    }
                    for (size_t i = 1; i < col.size(); ++i)
                    {
                        const float gap = col[i].y - (col[i - 1].y + col[i - 1].h);
                        if (gap > 2.0f)
                            labels.push_back({ selR + 7.0f,
                                               col[i - 1].y + col[i - 1].h + gap * 0.5f, gap,
                                               highlightValue > 0.0f &&
                                               fabsf(gap - highlightValue) <= 0.5f });
                    }
                }
            }
        }

        // Guias de espaçamento (M04): se o espaço entre a seleção e um vizinho
        // coincide com um espaçamento que JÁ aparece entre dois outros
        // elementos adjacentes DA MESMA FILEIRA/COLUNA, a seleção é puxada
        // para replicar esse espaço e duas linhas-guia delimitam o espaço
        // repetido. Referências fora da fileira (em outra altura/coluna) NÃO
        // poluem a previsão: o alvo é sempre o espaçamento do contexto em que
        // a peça está sendo encaixada. O ímã também detecta CRUZAMENTO: se o
        // gap pulou por cima de um alvo entre dois frames (arrasto rápido),
        // ele ainda engata no alvo — nunca "passa batido". Devolve as posições
        // (projeto) das guias; -1 = sem encaixe no eixo.
        inline void ApplySpacing(const Modo& mode,
                                 const std::vector<Rect>& starts,
                                 const std::vector<std::string>& selectedIds,
                                 float& dx, float& dy,
                                 float tolerance,
                                 float prevDx, float prevDy,
                                 float& outGapX1, float& outGapX2,
                                 float& outGapY1, float& outGapY2)
        {
            outGapX1 = outGapX2 = outGapY1 = outGapY2 = -1.0f;
            if (starts.empty()) return;

            auto selBounds = [&starts](float ox, float oy)
            {
                float l = starts[0].x + ox, t = starts[0].y + oy;
                float r = starts[0].x + starts[0].w + ox;
                float b = starts[0].y + starts[0].h + oy;
                for (const Rect& s : starts)
                {
                    l = std::min(l, s.x + ox);
                    t = std::min(t, s.y + oy);
                    r = std::max(r, s.x + s.w + ox);
                    b = std::max(b, s.y + s.h + oy);
                }
                return std::array<float, 4>{ l, t, r, b };
            };
            const auto cur = selBounds(dx, dy);
            const float selLeft = cur[0], selTop = cur[1];
            const float selRight = cur[2], selBottom = cur[3];
            const auto prev = selBounds(prevDx, prevDy);
            const float prevLeft = prev[0], prevTop = prev[1];
            const float prevRight = prev[2], prevBottom = prev[3];

            std::vector<Rect> others;
            others.reserve(16);
            for (const Element& e : mode.raiz)
                CollectRects(e, selectedIds, others);
            if (others.size() < 2) return; // precisa de vizinhos para prever

            // Remove retângulos ANINHADOS (grupo/container + filhos duplicam a
            // geometria e geram alvos espúrios de espaçamento). Fica o maior.
            auto dedupe = [](const std::vector<Rect>& rects)
            {
                std::vector<Rect> out;
                for (const Rect& r : rects)
                {
                    bool nested = false;
                    for (const Rect& o : rects)
                    {
                        if (&r == &o) continue;
                        if (r.x >= o.x - 0.01f && r.y >= o.y - 0.01f &&
                            r.x + r.w <= o.x + o.w + 0.01f &&
                            r.y + r.h <= o.y + o.h + 0.01f)
                        { nested = true; break; }
                    }
                    if (!nested) out.push_back(r);
                }
                return out;
            };

            // Espaçamentos entre pares adjacentes (sem sobreposição). Os
            // alvos são os VALORES EXATOS da referência (sem arredondar a
            // 0.5) — uma referência de 32.4 prevê 32.4, e o encaixe é exato:
            // a peça pousa com o MESMO espaçamento do lado de referência.
            auto collectGaps = [](const std::vector<Rect>& rects, bool vertical)
            {
                std::vector<float> keys;
                std::vector<Rect> sorted = rects;
                if (vertical)
                    std::sort(sorted.begin(), sorted.end(),
                              [](const Rect& a, const Rect& b) { return a.y < b.y; });
                else
                    std::sort(sorted.begin(), sorted.end(),
                              [](const Rect& a, const Rect& b) { return a.x < b.x; });
                for (size_t i = 1; i < sorted.size(); ++i)
                {
                    const float gap = vertical
                        ? sorted[i].y - (sorted[i - 1].y + sorted[i - 1].h)
                        : sorted[i].x - (sorted[i - 1].x + sorted[i - 1].w);
                    if (gap >= 0.0f) keys.push_back(gap);
                }
                std::sort(keys.begin(), keys.end());
                return keys;
            };
            // Melhor alvo de espaçamento: dentro da tolerância OU cruzado
            // entre o frame anterior e o atual (arrasto rápido não pula a
            // zona do ímã). Devolve -1 se não houver alvo; o erro EFETIVO
            // (0 para cruzamento) sai em outErr — é ele que decide a disputa
            // entre vizinhos, não o erro bruto do gap atual.
            auto matchGap = [](const std::vector<float>& keys,
                               float gap, float prevGap, float tolerance,
                               float& outErr)
            {
                float best = -1.0f;
                float bestErr = tolerance;
                for (float target : keys)
                {
                    float err = fabsf(gap - target);
                    const bool crossed = (prevGap - target) * (gap - target) < 0.0f &&
                                         fabsf(gap - prevGap) > 0.001f;
                    if (crossed) err = 0.0f;
                    if (err < bestErr) { bestErr = err; best = target; }
                }
                outErr = bestErr;
                return best;
            };

            // Fileira (eixo X): vizinhos que sobrepõem a faixa vertical da
            // seleção; referências = gaps adjacentes DESTA fileira.
            std::vector<Rect> row;
            for (const Rect& o : others)
                if (o.y < selBottom + tolerance && o.y + o.h > selTop - tolerance)
                    row.push_back(o);
            row = dedupe(row);
            const std::vector<float> rowKeys = collectGaps(row, false);

            // Coluna (eixo Y): vizinhos que sobrepõem a faixa horizontal da
            // seleção; referências = gaps adjacentes DESTA coluna.
            std::vector<Rect> col;
            for (const Rect& o : others)
                if (o.x < selRight + tolerance && o.x + o.w > selLeft - tolerance)
                    col.push_back(o);
            col = dedupe(col);
            const std::vector<float> colKeys = collectGaps(col, true);

            // Eixo X: lado direito da seleção -> borda esquerda de um vizinho
            // da MESMA fileira.
            {
                float best = tolerance;
                float bestGap = -1.0f, bestCand = -1.0f;
                for (const Rect& o : row)
                {
                    const float gap = o.x - selRight;
                    const float prevGap = o.x - prevRight;
                    if (gap < -tolerance && (prevGap - gap) <= 0.0f) continue;
                    float matchErr = tolerance;
                    const float target = matchGap(rowKeys, gap, prevGap, tolerance,
                                                  matchErr);
                    if (target < 0.0f) continue;
                    if (matchErr < best) { best = matchErr; bestGap = target; bestCand = o.x; }
                }
                if (bestCand >= 0.0f)
                {
                    // Seleção à ESQUERDA do vizinho: gap = o.x - selRight;
                    // para gap == bestGap, dx += (o.x - selRight) - bestGap.
                    dx += (bestCand - selRight) - bestGap;
                    outGapX1 = bestCand - bestGap;
                    outGapX2 = bestCand;
                }
            }
            // Lado esquerdo da seleção -> borda direita de um vizinho.
            if (outGapX1 < 0.0f)
            {
                float best = tolerance;
                float bestGap = -1.0f, bestCand = -1.0f;
                for (const Rect& o : row)
                {
                    const float right = o.x + o.w;
                    const float gap = selLeft - right;
                    const float prevGap = prevLeft - right;
                    if (gap < -tolerance && (prevGap - gap) <= 0.0f) continue;
                    float matchErr = tolerance;
                    const float target = matchGap(rowKeys, gap, prevGap, tolerance,
                                                  matchErr);
                    if (target < 0.0f) continue;
                    if (matchErr < best) { best = matchErr; bestGap = target; bestCand = right; }
                }
                if (bestCand >= 0.0f)
                {
                    dx += bestGap - (selLeft - bestCand);
                    outGapX1 = bestCand;
                    outGapX2 = bestCand + bestGap;
                }
            }

            // Eixo Y: seleção abaixo de um vizinho (base -> topo da seleção).
            {
                float best = tolerance;
                float bestGap = -1.0f, bestCand = -1.0f;
                for (const Rect& o : col)
                {
                    const float gap = selTop - (o.y + o.h);
                    const float prevGap = prevTop - (o.y + o.h);
                    if (gap < -tolerance && (prevGap - gap) <= 0.0f) continue;
                    float matchErr = tolerance;
                    const float target = matchGap(colKeys, gap, prevGap, tolerance,
                                                  matchErr);
                    if (target < 0.0f) continue;
                    if (matchErr < best) { best = matchErr; bestGap = target; bestCand = o.y + o.h; }
                }
                if (bestCand >= 0.0f)
                {
                    dy += bestGap - (selTop - bestCand);
                    outGapY1 = bestCand;
                    outGapY2 = bestCand + bestGap;
                }
            }
            // Eixo Y: seleção acima de um vizinho (base da seleção -> topo).
            if (outGapY1 < 0.0f)
            {
                float best = tolerance;
                float bestGap = -1.0f, bestCand = -1.0f;
                for (const Rect& o : col)
                {
                    const float gap = o.y - selBottom;
                    const float prevGap = o.y - prevBottom;
                    if (gap < -tolerance && (prevGap - gap) <= 0.0f) continue;
                    float matchErr = tolerance;
                    const float target = matchGap(colKeys, gap, prevGap, tolerance,
                                                  matchErr);
                    if (target < 0.0f) continue;
                    if (matchErr < best) { best = matchErr; bestGap = target; bestCand = o.y; }
                }
                if (bestCand >= 0.0f)
                {
                    // Seleção ACIMA do vizinho: gap = o.y - selBottom;
                    // para gap == bestGap, dy += (o.y - selBottom) - bestGap.
                    dy += (bestCand - selBottom) - bestGap;
                    outGapY1 = bestCand - bestGap;
                    outGapY2 = bestCand;
                }
            }
        }
    }
}

#endif // SEEDUI_SMARTGUIDES_H
