#include "Annotations.h"

#include "Theme.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <cstring>

namespace seedui
{
    namespace
    {
        constexpr float kLabelW = 240.0f;
        constexpr float kLabelMinH = 68.0f;
        constexpr float kPopupW = 360.0f;
        constexpr float kPopupH = 202.0f; // edição individual compacta; exportação é global na barra inferior
        constexpr float kIconPickerH = 210.0f; // altura extra do seletor de ícones (SeedNotas)
        constexpr float kFontRowH = 26.0f;     // linha do tamanho da fonte

        float ClampFontSize(float v)
        {
            return std::max(8.0f, std::min(64.0f, v));
        }

        float BodyFontSize(const Annotation& a)
        {
            return ClampFontSize(a.fontSize <= 0.0f ? 13.0f : a.fontSize);
        }

        // Fonte de alta resolução para o texto das anotações (SeedNotas
        // registra; SeedUI deixa nulo e usa a fonte atual do ImGui).
        ImFont* gAnnotHighResFont = nullptr;

        ImFont* BodyFont()
        {
            return gAnnotHighResFont ? gAnnotHighResFont : ImGui::GetFont();
        }

        const ImU32 kAnnotColors[] = {
            IM_COL32(241, 196, 15, 255),   // amarelo
            IM_COL32(236, 72, 153, 255),   // magenta
            IM_COL32(34, 211, 238, 255),   // ciano
            IM_COL32(46, 204, 113, 255),   // verde
            IM_COL32(168, 85, 247, 255),   // violeta
        };
        constexpr int kColorCount = 5;

        float PopupHeight(const AnnotationState& st)
        {
            return kPopupH + kFontRowH +
                   (st.iconPicker ? kIconPickerH : 0.0f);
        }

        bool PointInRect(const ImVec2& p, const ImVec2& a, const ImVec2& b)
        {
            return p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y;
        }

        bool RectsOverlap(const ImVec2& a0, const ImVec2& a1,
                          const ImVec2& b0, const ImVec2& b1)
        {
            return a0.x < b1.x && a1.x > b0.x && a0.y < b1.y && a1.y > b0.y;
        }

        // Alça de redimensionamento da CAIXA DE SELEÇÃO sob o mouse:
        // 1=sup.esq, 2=sup.dir, 3=inf.esq, 4=inf.dir, 5=esq, 6=dir, 7=sup, 8=inf.
        int AreaResizeHandle(const Annotation& a, const ImVec2& mouse)
        {
            const float tol = 10.0f;
            const float l = a.min.x, t = a.min.y, r = a.max.x, b = a.max.y;
            if (fabsf(mouse.x - l) <= tol && fabsf(mouse.y - t) <= tol) return 1;
            if (fabsf(mouse.x - r) <= tol && fabsf(mouse.y - t) <= tol) return 2;
            if (fabsf(mouse.x - l) <= tol && fabsf(mouse.y - b) <= tol) return 3;
            if (fabsf(mouse.x - r) <= tol && fabsf(mouse.y - b) <= tol) return 4;
            if (fabsf(mouse.x - l) <= tol && mouse.y >= t && mouse.y <= b) return 5;
            if (fabsf(mouse.x - r) <= tol && mouse.y >= t && mouse.y <= b) return 6;
            if (mouse.x >= l && mouse.x <= r && fabsf(mouse.y - t) <= tol) return 7;
            if (mouse.x >= l && mouse.x <= r && fabsf(mouse.y - b) <= tol) return 8;
            return 0;
        }

        void ArrowHead(ImDrawList* dl, const ImVec2& to, const ImVec2& dir, ImU32 col, float size)
        {
            const float d0 = -dir.y * 0.5f;
            const float d1 = dir.x * 0.5f;
            dl->AddLine(to, ImVec2(to.x - dir.x * size + d0 * size, to.y - dir.y * size + d1 * size), col, 1.5f);
            dl->AddLine(to, ImVec2(to.x - dir.x * size - d0 * size, to.y - dir.y * size - d1 * size), col, 1.5f);
        }
    }

    void AnnotationsSetHighResFont(ImFont* font)
    {
        gAnnotHighResFont = font;
    }

    float AnnotationsLabelHeight(const Annotation& a)
    {
        const char* preview = a.text.empty() ? "Clique para editar..." : a.text.c_str();
        const float bodySize = BodyFontSize(a);
        const float w = std::max(100.0f, a.labelW);
        ImFont* font = BodyFont();
        float bodyH = 0.0f;
        if (font)
            bodyH = font->CalcTextSizeA(bodySize, FLT_MAX,
                                        w - 16.0f, preview).y;
        // 6 (topo) + 12 (título) + 8 (espaço) + corpo + 6 (base)
        const float autoH = std::max(kLabelMinH, 32.0f + bodyH);
        if (a.labelH > 0.0f) return std::max(autoH, a.labelH);
        return autoH;
    }

    AnnotationsPopupRect AnnotationsPopupRectFor(const AnnotationState& st,
                                                 const Annotation& annotation,
                                                 const ImVec2& viewportSize)
    {
        const float popupH = PopupHeight(st);
        const float popupW = kPopupW;
        const ImVec2 label = annotation.label;
        const float lh = AnnotationsLabelHeight(annotation);
        const float lw = std::max(100.0f, annotation.labelW);
        const ImVec2 a0 = annotation.min;
        const ImVec2 a1 = annotation.max;

        // Candidatos em ordem de preferência. O escolhido deve caber na
        // janela E NÃO cobrir nem a área anotada nem o rótulo — assim a caixa
        // de seleção continua visível e a digitação fica legível.
        const float cx = std::max(0.0f, std::min(viewportSize.x - popupW, label.x));
        const float belowY = label.y + lh + 8.0f;
        const float aboveY = label.y - popupH - 8.0f;
        const float bottomY = std::max(0.0f, viewportSize.y - popupH);
        const float midY = std::max(0.0f, std::min(viewportSize.y - popupH,
                                                   (a0.y + a1.y) * 0.5f - popupH * 0.5f));
        struct Cand { float x, y; };
        const Cand cands[] = {
            { cx, belowY },          // abaixo do rótulo
            { cx, aboveY },          // acima do rótulo
            { cx, bottomY },         // abaixo do rótulo, ancorado na borda inferior
            { a1.x + 14.0f, midY },  // à direita da área
            { a0.x - popupW - 14.0f, midY }, // à esquerda da área
        };

        const auto clears = [&](float x, float y)
        {
            if (x + popupW > a0.x && x < a1.x &&
                y + popupH > a0.y && y < a1.y)
                return false; // cobre a área anotada
            if (x + popupW > label.x && x < label.x + lw &&
                y + popupH > label.y && y < label.y + lh)
                return false; // cobre o rótulo
            return true;
        };

        for (const Cand& c : cands)
        {
            float x = std::max(0.0f, std::min(viewportSize.x - popupW, c.x));
            float y = std::max(0.0f, std::min(viewportSize.y - popupH, c.y));
            if (clears(x, y))
            {
                AnnotationsPopupRect r;
                r.min = ImVec2(x, y);
                r.max = ImVec2(x + popupW, y + popupH);
                return r;
            }
        }

        // Fallback: abaixo do rótulo, dentro da janela (pode encostar — não
        // há outro espaço livre).
        float x = std::max(0.0f, std::min(viewportSize.x - popupW, label.x));
        float y = std::max(0.0f, std::min(viewportSize.y - popupH, belowY));
        AnnotationsPopupRect r;
        r.min = ImVec2(x, y);
        r.max = ImVec2(x + popupW, y + popupH);
        return r;
    }

    void AnnotationsAdd(AnnotationState& st, const ImVec2& min, const ImVec2& max, const char* text)
    {
        Annotation a;
        a.id = st.nextId++;
        a.min = min;
        a.max = max;
        a.text = text ? text : "";
        a.label = ImVec2(max.x + 14.0f, max.y - AnnotationsLabelHeight(a) - 8.0f);
        a.color = kAnnotColors[(a.id - 1) % kColorCount];
        st.items.push_back(a);
        st.selected = (int)st.items.size() - 1;
        st.editedIndex = -1;
        TraceLog(LOG_INFO, "ANNOT add id=%d area %.0f,%.0f %.0fx%.0f",
                 a.id, a.min.x, a.min.y, a.max.x - a.min.x, a.max.y - a.min.y);
    }

    void AnnotationsUpdate(AnnotationState& st, bool toolActive, const ImVec2& viewportSize)
    {
        if (!toolActive)
        {
            st.creating = false;
            st.dragging = -1;
            st.dragMode = 0;
            return;
        }

        // Coordenadas GLOBAIS (janela do SeedUI inteira): a anotação pode cobrir
        // menus, ícones da barra lateral, painéis e o canvas.
        const ImVec2 mouse = ImGui::GetMousePos();
        // O canvas usa um InvisibleButton que fica ativo durante o arraste;
        // depender de IsWindowHovered bloqueava falsamente as anotações.
        const bool hovered = PointInRect(mouse, ImVec2(0.0f, 0.0f), viewportSize);

        // O popup da anotação precisa receber clique e teclado. Sem esta
        // exclusão, a captura global desmarcava a anotação antes do campo focar.
        if (!st.creating && st.dragging < 0 && st.selected >= 0 &&
            st.selected < (int)st.items.size())
        {
            const AnnotationsPopupRect popup =
                AnnotationsPopupRectFor(st, st.items[st.selected], viewportSize);
            if (PointInRect(mouse, popup.min, popup.max)) return;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered)
        {
            TraceLog(LOG_INFO, "ANNOT click %.0f,%.0f toolActive=%d",
                     mouse.x, mouse.y, (int)toolActive);
            int hitLabel = -1;
            int hitRect = -1;
            int hitHandle = 0;
            for (int i = (int)st.items.size() - 1; i >= 0; --i)
            {
                const Annotation& a = st.items[i];
                const float w = std::max(100.0f, a.labelW);
                if (PointInRect(mouse, a.label,
                                ImVec2(a.label.x + w,
                                       a.label.y + AnnotationsLabelHeight(a))))
                {
                    hitLabel = i;
                    break;
                }
                // Alças de redimensionamento ANTES do interior: uma alça
                // pode estar até 10px FORA do retângulo (metade do quadrado
                // fica para fora) — sem isto, clicar na alça criava uma nova
                // seleção em vez de redimensionar.
                // (1=sup.esq, 2=sup.dir, 3=inf.esq, 4=inf.dir,
                //  5=esq, 6=dir, 7=sup, 8=inf)
                hitHandle = AreaResizeHandle(a, mouse);
                if (hitHandle != 0)
                {
                    hitRect = i;
                    break;
                }
                if (PointInRect(mouse, a.min, a.max))
                {
                    hitRect = i;
                    break;
                }
            }

            if (hitHandle != 0 && hitRect >= 0)
            {
                // Redimensionar a caixa de seleção pela alça
                st.selected = hitRect;
                st.dragging = hitRect;
                st.dragMode = 2 + hitHandle; // 3..10
                const Annotation& ha = st.items[hitRect];
                const float cx = (ha.min.x + ha.max.x) * 0.5f;
                const float cy = (ha.min.y + ha.max.y) * 0.5f;
                switch (hitHandle)
                {
                    case 1: st.dragOffset = ImVec2(mouse.x - ha.min.x, mouse.y - ha.min.y); break;
                    case 2: st.dragOffset = ImVec2(mouse.x - ha.max.x, mouse.y - ha.min.y); break;
                    case 3: st.dragOffset = ImVec2(mouse.x - ha.min.x, mouse.y - ha.max.y); break;
                    case 4: st.dragOffset = ImVec2(mouse.x - ha.max.x, mouse.y - ha.max.y); break;
                    case 5: st.dragOffset = ImVec2(mouse.x - ha.min.x, mouse.y - cy); break;
                    case 6: st.dragOffset = ImVec2(mouse.x - ha.max.x, mouse.y - cy); break;
                    case 7: st.dragOffset = ImVec2(mouse.x - cx, mouse.y - ha.min.y); break;
                    case 8: st.dragOffset = ImVec2(mouse.x - cx, mouse.y - ha.max.y); break;
                }
            }
            else if (hitLabel >= 0)
            {
                st.selected = hitLabel;
                st.dragging = hitLabel;
                st.dragMode = 2; // mover o rótulo (a caixa de seleção NÃO muda)
                st.dragOffset = ImVec2(mouse.x - st.items[hitLabel].label.x,
                                       mouse.y - st.items[hitLabel].label.y);
            }
            else if (hitRect >= 0)
            {
                st.selected = hitRect;
                st.dragging = hitRect;
                st.dragMode = 1;
                st.dragOffset = ImVec2(mouse.x - st.items[hitRect].min.x,
                                       mouse.y - st.items[hitRect].min.y);
            }
            else
            {
                st.selected = -1;
                st.creating = true;
                st.createStart = mouse;
                TraceLog(LOG_INFO, "ANNOT create start %.0f,%.0f toolActive=%d",
                         mouse.x, mouse.y, (int)toolActive);
            }
        }

        if (st.dragging >= 0 && st.dragMode != 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            Annotation& a = st.items[st.dragging];
            if (st.dragMode == 1)
            {
                const ImVec2 np(mouse.x - st.dragOffset.x, mouse.y - st.dragOffset.y);
                const ImVec2 delta(np.x - a.min.x, np.y - a.min.y);
                a.min.x += delta.x;
                a.min.y += delta.y;
                a.max.x += delta.x;
                a.max.y += delta.y;
                a.label.x += delta.x;
                a.label.y += delta.y;
            }
            else if (st.dragMode == 2)
            {
                a.label.x = mouse.x - st.dragOffset.x;
                a.label.y = mouse.y - st.dragOffset.y;

                // O rótulo NUNCA cobre a área anotada: se o usuário arrastar
                // para cima dela, empurra para fora — no lado mais próximo
                // do ponto onde soltou (a informação selecionada fica sempre
                // visível).
                const float lw = std::max(100.0f, a.labelW);
                const float lh = AnnotationsLabelHeight(a);
                if (RectsOverlap(a.label, ImVec2(a.label.x + lw, a.label.y + lh),
                                 a.min, a.max))
                {
                    const float gap = 6.0f;
                    const float cand[4][2] = {
                        { a.label.x, a.max.y + gap },        // abaixo da área
                        { a.label.x, a.min.y - lh - gap },   // acima da área
                        { a.min.x - lw - gap, a.label.y },   // esquerda da área
                        { a.max.x + gap, a.label.y },        // direita da área
                    };
                    float best = FLT_MAX;
                    int bestI = 0;
                    for (int k = 0; k < 4; ++k)
                    {
                        const float ddx = cand[k][0] - a.label.x;
                        const float ddy = cand[k][1] - a.label.y;
                        const float d = ddx * ddx + ddy * ddy;
                        if (d < best) { best = d; bestI = k; }
                    }
                    a.label.x = cand[bestI][0];
                    a.label.y = cand[bestI][1];
                }

                const float boundX = std::max(0.0f, viewportSize.x - lw);
                const float boundY = std::max(0.0f, viewportSize.y - lh);
                a.label.x = std::min(std::max(a.label.x, 0.0f), boundX);
                a.label.y = std::min(std::max(a.label.y, 0.0f), boundY);
            }
            else
            {
                // Redimensionar a CAIXA DE SELEÇÃO pelas alças (3..10)
                const float minS = 24.0f;
                float l = a.min.x;
                float t = a.min.y;
                float r = a.max.x;
                float b = a.max.y;
                if (st.dragMode == 3) { l = mouse.x - st.dragOffset.x; t = mouse.y - st.dragOffset.y; }
                else if (st.dragMode == 4) { r = mouse.x - st.dragOffset.x; t = mouse.y - st.dragOffset.y; }
                else if (st.dragMode == 5) { l = mouse.x - st.dragOffset.x; b = mouse.y - st.dragOffset.y; }
                else if (st.dragMode == 6) { r = mouse.x - st.dragOffset.x; b = mouse.y - st.dragOffset.y; }
                else if (st.dragMode == 7) { l = mouse.x - st.dragOffset.x; }
                else if (st.dragMode == 8) { r = mouse.x - st.dragOffset.x; }
                else if (st.dragMode == 9) { t = mouse.y - st.dragOffset.y; }
                else if (st.dragMode == 10) { b = mouse.y - st.dragOffset.y; }
                if (r - l < minS) { if (st.dragMode == 3 || st.dragMode == 5 || st.dragMode == 7) l = r - minS; else r = l + minS; }
                if (b - t < minS) { if (st.dragMode == 3 || st.dragMode == 4 || st.dragMode == 9) t = b - minS; else b = t + minS; }
                l = std::max(0.0f, l);
                t = std::max(0.0f, t);
                r = std::min(viewportSize.x, r);
                b = std::min(viewportSize.y, b);
                a.min = ImVec2(l, t);
                a.max = ImVec2(r, b);
            }
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (st.creating)
            {
                const ImVec2 a0 = st.createStart;
                const float dx = mouse.x - a0.x;
                const float dy = mouse.y - a0.y;
                if (fabsf(dx) > 10.0f && fabsf(dy) > 10.0f)
                {
                    const ImVec2 mn(std::min(a0.x, mouse.x), std::min(a0.y, mouse.y));
                    const ImVec2 mx(std::max(a0.x, mouse.x), std::max(a0.y, mouse.y));
                    AnnotationsAdd(st, mn, mx, "");
                }
                st.creating = false;
            }
            st.dragging = -1;
            st.dragMode = 0;
        }

        // Clique direito fora das anotações = desmarca
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hovered)
        {
            bool onAny = false;
            for (const Annotation& a : st.items)
            {
                const float aw = std::max(100.0f, a.labelW);
                if (PointInRect(mouse, a.min, a.max) ||
                    PointInRect(mouse, a.label,
                                ImVec2(a.label.x + aw,
                                       a.label.y + AnnotationsLabelHeight(a))))
                {
                    onAny = true;
                    break;
                }
            }
            if (!onAny) st.selected = -1;
        }
    }

    void AnnotationsDraw(const AnnotationState& st, bool clipPopup,
                         const ImVec2& popupMin, const ImVec2& popupMax)
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImFont* font = ImGui::GetFont();

        for (const Annotation& a : st.items)
        {
            // Não desenha por cima da janela de edição aberta
            const float aw = std::max(100.0f, a.labelW);
            // A anotação SELECIONADA (a que está sendo editada) NUNCA é
            // escondida pelo popup — a caixa de seleção continua visível.
            // Só as outras anotações são puladas quando ficariam por cima
            // da caixa de digitação.
            const bool isSelected = (st.selected >= 0 && &a == &st.items[st.selected]);
            const bool overPopup = clipPopup && !isSelected &&
                (RectsOverlap(a.min, a.max, popupMin, popupMax) ||
                 RectsOverlap(a.label, ImVec2(a.label.x + aw, a.label.y + AnnotationsLabelHeight(a)),
                              popupMin, popupMax));
            if (overPopup) continue;

            // Coordenadas já são globais (janela do SeedUI)
            const ImVec2 r0 = a.min;
            const ImVec2 r1 = a.max;
            const ImU32 col = a.color;
            const ImU32 fill = (col & 0x00FFFFFF) | 0x22000000;
            const bool isSel = (st.selected >= 0 && &a == &st.items[st.selected]);

            // Área selecionada
            dl->AddRectFilled(r0, r1, fill);
            dl->AddRect(r0, r1, col, 0.0f, 0, isSel ? 2.5f : 1.5f);

            // Alças de redimensionamento da CAIXA DE SELEÇÃO (só na
            // selecionada): 4 cantos + 4 meios de borda, estilo Illustrator/
            // CorelDRAW — quadrados brancos com contorno na cor.
            if (isSel && !st.creating)
            {
                const float hs = 6.0f;
                const ImU32 hfill = IM_COL32(240, 240, 240, 255);
                const ImVec2 pts[8] = {
                    ImVec2(r0.x, r0.y), ImVec2(r1.x, r0.y),
                    ImVec2(r0.x, r1.y), ImVec2(r1.x, r1.y),
                    ImVec2(r0.x, (r0.y + r1.y) * 0.5f),
                    ImVec2(r1.x, (r0.y + r1.y) * 0.5f),
                    ImVec2((r0.x + r1.x) * 0.5f, r0.y),
                    ImVec2((r0.x + r1.x) * 0.5f, r1.y),
                };
                for (const ImVec2& c : pts)
                {
                    dl->AddRectFilled(ImVec2(c.x - hs, c.y - hs),
                                      ImVec2(c.x + hs, c.y + hs), hfill);
                    dl->AddRect(ImVec2(c.x - hs, c.y - hs),
                                ImVec2(c.x + hs, c.y + hs), col, 1.0f);
                }
            }

            // Número (badge)
            const float badge = 18.0f;
            const ImVec2 bc(r0.x + badge, r0.y + badge);
            dl->AddCircleFilled(bc, badge, col);
            char nb[16];
            snprintf(nb, sizeof nb, "%d", a.id);
            const ImVec2 nts = font->CalcTextSizeA(12.0f, FLT_MAX, 0.0f, nb);
            dl->AddText(font, 12.0f, ImVec2(bc.x - nts.x * 0.5f, bc.y - nts.y * 0.5f),
                        IM_COL32(25, 25, 25, 255), nb);

            // Rótulo flutuante (largura redimensionável pelo usuário)
            const float lw = std::max(100.0f, a.labelW);
            const float lh = AnnotationsLabelHeight(a);
            const ImVec2 l0 = a.label;
            const ImVec2 l1(l0.x + lw, l0.y + lh);

            // Seta do rótulo para a área
            const ImVec2 from((l0.x + l1.x) * 0.5f, l1.y);
            const ImVec2 to((r0.x + r1.x) * 0.5f, r0.y);
            ImVec2 dir(to.x - from.x, to.y - from.y);
            const float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
            if (len > 1.0f)
            {
                dir.x /= len;
                dir.y /= len;
            }
            dl->AddLine(from, to, col, 1.5f);
            ArrowHead(dl, to, dir, col, 6.0f);

            dl->AddRectFilled(l0, l1, IM_COL32(43, 43, 43, 245), 4.0f);
            dl->AddRect(l0, l1, col, 4.0f, 0, isSel ? 2.0f : 1.5f);

            char tb[32];
            snprintf(tb, sizeof tb, "Anotação #%d", a.id);
            dl->AddText(ImVec2(l0.x + 8.0f, l0.y + 6.0f), col, tb);

            const char* preview = a.text.empty() ? "Clique para editar..." : a.text.c_str();
            const float bodySize = BodyFontSize(a);
            ImFont* bodyFont = BodyFont();
            dl->PushClipRect(ImVec2(l0.x + 8.0f, l0.y + 24.0f),
                             ImVec2(l1.x - 8.0f, l1.y - 6.0f), true);
            if (bodyFont)
                dl->AddText(bodyFont, bodySize,
                            ImVec2(l0.x + 8.0f, l0.y + 26.0f),
                            IM_COL32(236, 236, 236, 255), preview, nullptr,
                            lw - 16.0f);
            dl->PopClipRect();
        }

        // Pré-visualização do retângulo em criação
        if (st.creating)
        {
            const ImVec2 a0 = st.createStart;
            const ImVec2 m = ImGui::GetMousePos();
            dl->AddRect(a0, m, kAnnotColors[0], 0.0f, 0, 1.5f);
            const char* hint = "Solte para criar a anotação";
            dl->AddText(font, 13.0f, ImVec2(m.x + 12.0f, m.y + 8.0f),
                        IM_COL32(241, 196, 15, 255), hint);
        }
    }

    int AnnotationsEditWindow(AnnotationState& st)
    {
        // Enquanto o usuário estiver ARRASTANDO (área, rótulo ou redimensionando),
        // a caixa de digitação não aparece — nada fica por cima da seleção.
        if (st.selected < 0 || st.selected >= (int)st.items.size() ||
            st.dragging >= 0 || st.creating)
        {
            st.editedIndex = -1;
            return AnnotationsEdit_None;
        }

        Annotation& a = st.items[st.selected];
        bool focusEditor = false;

        if (st.editedIndex != st.selected)
        {
            focusEditor = true;
            strncpy(st.editBuf, a.text.c_str(), sizeof(st.editBuf) - 1);
            st.editBuf[sizeof(st.editBuf) - 1] = 0;
            st.editedIndex = st.selected;
        }

        // Popup sempre DENTRO da janela (abaixo do rótulo; acima dele se não couber)
        const float popupH = PopupHeight(st);
        const AnnotationsPopupRect pr =
            AnnotationsPopupRectFor(st, a, ImGui::GetMainViewport()->Size);
        ImGui::SetNextWindowPos(pr.min, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(kPopupW, popupH), ImGuiCond_Always);

        char title[48];
        snprintf(title, sizeof title, "Anotação #%d — escreva a alteração", a.id);

        bool open = true;
        if (!ImGui::Begin(title, &open,
                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                          ImGuiWindowFlags_NoResize))
        {
            ImGui::End();
            return AnnotationsEdit_None;
        }

        // Fechou pelo X ou Esc = confirma e fecha a caixa
        if (!open || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            st.selected = -1;
            st.editedIndex = -1;
            ImGui::End();
            return AnnotationsEdit_Confirmed;
        }

        if (focusEditor) ImGui::SetKeyboardFocusHere();
        ImGui::InputTextMultiline("##texto", st.editBuf, sizeof(st.editBuf),
                                  ImVec2(-1.0f, 84.0f));
        a.text = st.editBuf; // salvo automaticamente: o rótulo atualiza em tempo real

        ImGui::TextColored(Theme::TextDisabled,
                           "Área: (%.0f, %.0f) %.0f x %.0f — janela do SeedUI",
                           a.min.x, a.min.y, a.max.x - a.min.x, a.max.y - a.min.y);

        // Tamanho da fonte do texto (reflete ao vivo no rótulo e no print)
        a.fontSize = ClampFontSize(a.fontSize <= 0.0f ? 13.0f : a.fontSize);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::DragFloat("##fonte_anotacao", &a.fontSize, 0.5f, 8.0f, 64.0f,
                         "%.1f");
        ImGui::SameLine();
        if (ImGui::Button("-"))
            a.fontSize = ClampFontSize(a.fontSize - 1.0f);
        ImGui::SameLine();
        if (ImGui::Button("+"))
            a.fontSize = ClampFontSize(a.fontSize + 1.0f);
        ImGui::SameLine();
        ImGui::TextColored(Theme::TextDisabled, "Fonte do texto (px)");

        const float btnW = (ImGui::GetContentRegionAvail().x - 8.0f) * 0.5f;
        if (ImGui::Button("Confirmar ✓", ImVec2(btnW, 0)))
        {
            st.selected = -1;
            st.editedIndex = -1;
            ImGui::End();
            return AnnotationsEdit_Confirmed;
        }
        ImGui::SameLine();
        if (ImGui::Button("Apagar anotação", ImVec2(btnW, 0)))
        {
            st.items.erase(st.items.begin() + st.selected);
            st.selected = -1;
            st.editedIndex = -1;
            st.dragging = -1;
            st.dragMode = 0;
            ImGui::End();
            return AnnotationsEdit_Deleted;
        }

        // Seletor de ícones opcional (SeedNotas registra o hook): desenhado
        // dentro da janela de edição, após o campo de texto e os botões.
        if (st.iconPicker)
        {
            ImGui::Separator();
            st.iconPicker(a);
        }

        ImGui::TextColored(Theme::TextDisabled,
                           "Salvamento automático · Esc fecha");
        ImGui::End();
        return AnnotationsEdit_None;
    }

    std::string AnnotationsToText(const AnnotationState& st, const std::string& globalComment)
    {
        std::string out = "SEEDUI — DIRETRIZES DE ALTERAÇÃO (modo debug)\n";
        out += "=============================================\n\n";
        out += "COMENTÁRIO GERAL DO PROGRAMA:\n";
        out += globalComment.empty() ? std::string("(vazio)\n") : globalComment + "\n";
        out += "\nANOTAÇÕES SOBRE ÁREAS DA INTERFACE (coordenadas da janela do SeedUI):\n";
        if (st.items.empty())
        {
            out += "(nenhuma)\n";
        }
        else
        {
            for (const Annotation& a : st.items)
            {
                char line[1024];
                snprintf(line, sizeof line,
                         "[%d] área (%.0f, %.0f) %.0f x %.0f: %s\n",
                         a.id, a.min.x, a.min.y, a.max.x - a.min.x, a.max.y - a.min.y,
                         a.text.empty() ? "(sem texto)" : a.text.c_str());
                out += line;
            }
        }
        return out;
    }
}
