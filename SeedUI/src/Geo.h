#ifndef SEEDUI_GEO_H
#define SEEDUI_GEO_H

// Camada vetorial base compartilhada entre o Canvas (render) e a interação
// (App.cpp). Um elemento é descrito por um retângulo local (x, y, largura,
// altura) + rotação em graus em torno de um pivô (default: centro). Os
// contornos são amostrados como pontos e transformados para o espaço da
// tela, o que permite desenhar, testar hit e calcular limites mesmo com
// rotação aplicada.

#include "Project.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace seedui
{
    namespace Geo
    {
        inline constexpr float kMaxCornerSegments = 12.0f;

        // Passo ADAPTATIVO do grid com o zoom:
        // A grade adapta-se dinamicamente para manter o espaçamento entre pontos na tela
        // sempre confortável (~20px a ~50px).
        // Em zoom out (visão ampla): passos maiores (100, 50, 20).
        // Em zoom in (precisão): passos cada vez menores (10, 5, 2, 1, 0.5, 0.2, 0.1).
        inline float GetAdaptiveGridStep(float viewScale)
        {
            constexpr float kSteps[] = {
                0.05f, 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f
            };
            for (float s : kSteps)
            {
                if (s * viewScale >= 20.0f)
                    return s;
            }
            return 100.0f;
        }

        inline constexpr float kGridStep = 10.0f;
        inline constexpr float kGridMajorMult = 5.0f;

        inline float DegToRad(float degrees)
        {
            return degrees * 0.017453292519943295f;
        }

        inline float ElementRotation(const Element& e)
        {
            return e.transformacao.is_object()
                ? e.transformacao.value("rotacao", 0.0f)
                : 0.0f;
        }

        // Pivô da rotação (coordenadas do projeto). Default: centro do
        // elemento; sobrescrevível com transformacao.centro_rotacao.
        inline void ElementPivot(const Element& e, float& px, float& py)
        {
            const float x = e.transformacao.value("x", 0.0f);
            const float y = e.transformacao.value("y", 0.0f);
            const float w = e.transformacao.value("largura", 160.0f);
            const float h = e.transformacao.value("altura", 32.0f);
            float cx = x + w * 0.5f;
            float cy = y + h * 0.5f;
            if (e.transformacao.is_object() &&
                e.transformacao.contains("centro_rotacao") &&
                e.transformacao["centro_rotacao"].is_object())
            {
                cx = e.transformacao["centro_rotacao"].value("x", cx);
                cy = e.transformacao["centro_rotacao"].value("y", cy);
            }
            px = cx;
            py = cy;
        }

        inline void RotatePoint(float& x, float& y, float pivotX, float pivotY,
                                float radians)
        {
            const float c = cosf(radians);
            const float s = sinf(radians);
            const float dx = x - pivotX;
            const float dy = y - pivotY;
            x = pivotX + dx * c - dy * s;
            y = pivotY + dx * s + dy * c;
        }

        // Raios das quinas (mesmos campos de estilos usados no Canvas).
        inline void CornerRadii(const Element& e, float out[4])
        {
            const float w = e.transformacao.value("largura", 160.0f);
            const float h = e.transformacao.value("altura", 32.0f);
            const float maximum = std::max(0.0f, std::min(w, h) * 0.5f);
            const float uniform = e.estilos.is_object()
                ? std::max(0.0f, e.estilos.value("raio", 0.0f))
                : 0.0f;
            static const char* keys[4] = {
                "superior_esquerda", "superior_direita",
                "inferior_direita", "inferior_esquerda"
            };
            for (int i = 0; i < 4; ++i)
            {
                float radius = uniform;
                if (e.estilos.is_object() &&
                    e.estilos.contains("raio_quinas") &&
                    e.estilos["raio_quinas"].is_object())
                    radius = e.estilos["raio_quinas"].value(keys[i], radius);
                out[i] = std::max(0.0f, std::min(maximum, radius));
            }
        }

        // Encontra os valores mínimo e máximo de uma coordenada em uma curva cúbica
        // avaliando os extremos locais através das raízes da derivada B'(t) = 0.
        inline void CubicExtrema(float p0, float p1, float p2, float p3, float& minVal, float& maxVal)
        {
            minVal = std::min(minVal, std::min(p0, p3));
            maxVal = std::max(maxVal, std::max(p0, p3));
            const float a = 3.0f * (-p0 + 3.0f * p1 - 3.0f * p2 + p3);
            const float b = 6.0f * (p0 - 2.0f * p1 + p2);
            const float c = 3.0f * (p1 - p0);
            if (fabsf(a) < 1e-6f)
            {
                if (fabsf(b) > 1e-6f)
                {
                    const float t = -c / b;
                    if (t > 0.0f && t < 1.0f)
                    {
                        const float u = 1.0f - t;
                        const float val = u * u * u * p0 + 3.0f * u * u * t * p1 + 3.0f * u * t * t * p2 + t * t * t * p3;
                        minVal = std::min(minVal, val);
                        maxVal = std::max(maxVal, val);
                    }
                }
                return;
            }
            const float disc = b * b - 4.0f * a * c;
            if (disc >= 0.0f)
            {
                const float sqrtD = sqrtf(disc);
                const float t1 = (-b + sqrtD) / (2.0f * a);
                const float t2 = (-b - sqrtD) / (2.0f * a);
                if (t1 > 0.0f && t1 < 1.0f)
                {
                    const float u = 1.0f - t1;
                    const float val = u * u * u * p0 + 3.0f * u * u * t1 * p1 + 3.0f * u * t1 * t1 * p2 + t1 * t1 * t1 * p3;
                    minVal = std::min(minVal, val);
                    maxVal = std::max(maxVal, val);
                }
                if (t2 > 0.0f && t2 < 1.0f)
                {
                    const float u = 1.0f - t2;
                    const float val = u * u * u * p0 + 3.0f * u * u * t2 * p1 + 3.0f * u * t2 * t2 * p2 + t2 * t2 * t2 * p3;
                    minVal = std::min(minVal, val);
                    maxVal = std::max(maxVal, val);
                }
            }
        }

        // Subdivide uma curva Bézier cúbica no parâmetro t usando o algoritmo de De Casteljau.
        inline void SplitCubic(const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3,
                               float t, ImVec2 q[4], ImVec2 r[4])
        {
            const float u = 1.0f - t;
            const ImVec2 p01(u * p0.x + t * p1.x, u * p0.y + t * p1.y);
            const ImVec2 p12(u * p1.x + t * p2.x, u * p1.y + t * p2.y);
            const ImVec2 p23(u * p2.x + t * p3.x, u * p2.y + t * p3.y);
            const ImVec2 p012(u * p01.x + t * p12.x, u * p01.y + t * p12.y);
            const ImVec2 p123(u * p12.x + t * p23.x, u * p12.y + t * p23.y);
            const ImVec2 p0123(u * p012.x + t * p123.x, u * p012.y + t * p123.y);

            q[0] = p0;
            q[1] = p01;
            q[2] = p012;
            q[3] = p0123;

            r[0] = p0123;
            r[1] = p123;
            r[2] = p23;
            r[3] = p3;
        }

        // AABB exata de um caminho calculada pelas raízes reais da derivada de
        // cada segmento cúbico, cobrindo arcos e barrigas salientes perfeitamente.
        inline bool PathBounds(const Element& e, float& minX, float& minY,
                               float& maxX, float& maxY)
        {
            if (e.tipo != "caminho" || !e.transformacao.is_object() ||
                !e.transformacao.contains("pontos") ||
                !e.transformacao["pontos"].is_array())
                return false;
            const auto& arr = e.transformacao["pontos"];
            if (arr.empty()) return false;
            minX = arr[0].value("x", 0.0f);
            minY = arr[0].value("y", 0.0f);
            maxX = minX;
            maxY = minY;
            const int n = (int)arr.size();
            const bool closed = e.transformacao.value("fechado", 0.0f) > 0.5f;
            const int segments = closed ? n : (n - 1);
            for (int i = 0; i < segments; ++i)
            {
                const int j = (i + 1) % n;
                const auto& pa = arr[i];
                const auto& pb = arr[j];
                const float ax = pa.value("x", 0.0f);
                const float ay = pa.value("y", 0.0f);
                const float bx = pb.value("x", 0.0f);
                const float by = pb.value("y", 0.0f);
                const bool curved = pa.value("curva", 0.0f) > 0.5f || pb.value("curva", 0.0f) > 0.5f;
                if (curved)
                {
                    const float c2x = ax + pa.value("cx2", 0.0f);
                    const float c2y = ay + pa.value("cy2", 0.0f);
                    float c1x = 0.0f, c1y = 0.0f;
                    if (pb.value("quebrado", 0.0f) > 0.5f)
                    {
                        c1x = bx + pb.value("cx1", 0.0f);
                        c1y = by + pb.value("cy1", 0.0f);
                    }
                    else
                    {
                        c1x = bx - pb.value("cx2", 0.0f);
                        c1y = by - pb.value("cy2", 0.0f);
                    }
                    CubicExtrema(ax, c2x, c1x, bx, minX, maxX);
                    CubicExtrema(ay, c2y, c1y, by, minY, maxY);
                }
                else
                {
                    minX = std::min(minX, std::min(ax, bx));
                    maxX = std::max(maxX, std::max(ax, bx));
                    minY = std::min(minY, std::min(ay, by));
                    maxY = std::max(maxY, std::max(ay, by));
                }
            }
            return true;
        }

        // Localiza se um ponto (px, py) em coordenadas locais está sobre um segmento
        // do caminho (para inserção de nós com De Casteljau).
        inline bool FindSegmentOnPath(const Element& e, float px, float py, float maxDist,
                                      int& outSeg, float& outT, float& outProjX, float& outProjY)
        {
            if (e.tipo != "caminho" || !e.transformacao.contains("pontos") ||
                !e.transformacao["pontos"].is_array())
                return false;
            const auto& arr = e.transformacao["pontos"];
            const int n = (int)arr.size();
            if (n < 2) return false;
            const bool closed = e.transformacao.value("fechado", 0.0f) > 0.5f;
            const int segments = closed ? n : (n - 1);
            float bestDistSq = maxDist * maxDist;
            outSeg = -1;

            for (int i = 0; i < segments; ++i)
            {
                const int j = (i + 1) % n;
                const auto& pa = arr[i];
                const auto& pb = arr[j];
                const float ax = pa.value("x", 0.0f);
                const float ay = pa.value("y", 0.0f);
                const float bx = pb.value("x", 0.0f);
                const float by = pb.value("y", 0.0f);
                const bool curved = pa.value("curva", 0.0f) > 0.5f || pb.value("curva", 0.0f) > 0.5f;

                const int samples = curved ? 32 : 8;
                float prevX = ax, prevY = ay;
                for (int s = 1; s <= samples; ++s)
                {
                    const float t = (float)s / (float)samples;
                    float curX = 0.0f, curY = 0.0f;
                    if (curved)
                    {
                        const float c2x = ax + pa.value("cx2", 0.0f);
                        const float c2y = ay + pa.value("cy2", 0.0f);
                        float c1x = 0.0f, c1y = 0.0f;
                        if (pb.value("quebrado", 0.0f) > 0.5f)
                        {
                            c1x = bx + pb.value("cx1", 0.0f);
                            c1y = by + pb.value("cy1", 0.0f);
                        }
                        else
                        {
                            c1x = bx - pb.value("cx2", 0.0f);
                            c1y = by - pb.value("cy2", 0.0f);
                        }
                        const float u = 1.0f - t;
                        const float w0 = u * u * u;
                        const float w1 = 3.0f * u * u * t;
                        const float w2 = 3.0f * u * t * t;
                        const float w3 = t * t * t;
                        curX = w0 * ax + w1 * c2x + w2 * c1x + w3 * bx;
                        curY = w0 * ay + w1 * c2y + w2 * c1y + w3 * by;
                    }
                    else
                    {
                        curX = ax + (bx - ax) * t;
                        curY = ay + (by - ay) * t;
                    }

                    // Projeção no subsegmento linear [prev, cur]
                    const float segDx = curX - prevX;
                    const float segDy = curY - prevY;
                    const float segLenSq = segDx * segDx + segDy * segDy;
                    float segT = 0.0f;
                    if (segLenSq > 1e-6f)
                        segT = std::max(0.0f, std::min(1.0f, ((px - prevX) * segDx + (py - prevY) * segDy) / segLenSq));
                    const float projX = prevX + segDx * segT;
                    const float projY = prevY + segDy * segT;
                    const float ddx = px - projX;
                    const float ddy = py - projY;
                    const float dSq = ddx * ddx + ddy * ddy;
                    if (dSq < bestDistSq)
                    {
                        bestDistSq = dSq;
                        outSeg = i;
                        outT = ((float)(s - 1) + segT) / (float)samples;
                        outProjX = projX;
                        outProjY = projY;
                    }
                    prevX = curX;
                    prevY = curY;
                }
            }
            return (outSeg >= 0);
        }

        // AABB do elemento rotacionado (espaço do projeto). Útil para hit
        // aproximado, caixa de seleção e alinhamento. Rotaciona os 4 cantos
        // do retângulo local e pode ser usado por ReformatBBox abaixo.
        inline void RotatedAABB(const Element& e, float& minX, float& minY,
                                float& maxX, float& maxY)
        {
            const float x = e.transformacao.value("x", 0.0f);
            const float y = e.transformacao.value("y", 0.0f);
            const float w = e.transformacao.value("largura", 160.0f);
            const float h = e.transformacao.value("altura", 32.0f);
            const float radians = DegToRad(ElementRotation(e));
            if (fabsf(radians) < 0.0001f)
            {
                minX = x; minY = y; maxX = x + w; maxY = y + h;
                return;
            }
            float px = 0.0f, py = 0.0f;
            ElementPivot(e, px, py);
            float corners[8];
            float cs[4] = { x, y, x + w, y + h };
            for (int i = 0; i < 4; ++i)
            {
                corners[i * 2] = cs[i % 2 == 0 ? 0 : 2];
                corners[i * 2 + 1] = cs[i < 2 ? 0 : 2];
                RotatePoint(corners[i * 2], corners[i * 2 + 1], px, py, radians);
            }
            minX = corners[0]; minY = corners[1];
            maxX = corners[0]; maxY = corners[1];
            for (int i = 0; i < 4; ++i)
            {
                minX = std::min(minX, corners[i * 2]);
                minY = std::min(minY, corners[i * 2 + 1]);
                maxX = std::max(maxX, corners[i * 2]);
                maxY = std::max(maxY, corners[i * 2 + 1]);
            }
        }

        enum class ShapeKind
        {
            Rect,
            Ellipse,
            Polygon,
        };

        inline ShapeKind KindOf(const Element& e)
        {
            if (e.tipo == "elipse") return ShapeKind::Ellipse;
            if (e.tipo == "poligono") return ShapeKind::Polygon;
            return ShapeKind::Rect;
        }

        // Linha: os dois extremos da diagonal da caixa (0,0)->(w,h) no espaço
        // local, com espelhamento/rotação aplicados, em coordenadas do projeto.
        inline void LineEndpointsProject(const Element& e,
                                         float& x1, float& y1,
                                         float& x2, float& y2)
        {
            const float x = e.transformacao.value("x", 0.0f);
            const float y = e.transformacao.value("y", 0.0f);
            const float w = e.transformacao.value("largura", 160.0f);
            const float h = e.transformacao.value("altura", 32.0f);
            float p[4] = { 0.0f, 0.0f, w, h };
            if (e.transformacao.value("espelhado_h", 0.0f) > 0.5f)
            {
                p[0] = w - p[0];
                p[2] = w - p[2];
            }
            if (e.transformacao.value("espelhado_v", 0.0f) > 0.5f)
            {
                p[1] = h - p[1];
                p[3] = h - p[3];
            }
            const float radians = DegToRad(ElementRotation(e));
            float px = 0.0f, py = 0.0f;
            ElementPivot(e, px, py);
            for (int i = 0; i < 4; i += 2)
            {
                float ex = x + p[i];
                float ey = y + p[i + 1];
                if (fabsf(radians) > 0.0001f)
                    RotatePoint(ex, ey, px, py, radians);
                p[i] = ex;
                p[i + 1] = ey;
            }
            x1 = p[0]; y1 = p[1]; x2 = p[2]; y2 = p[3];
        }

        // Extremos da linha em coordenadas de tela.
        inline void LineEndpointsScreen(const Element& e, float originX, float originY,
                                        float scale, float& x1, float& y1,
                                        float& x2, float& y2)
        {
            LineEndpointsProject(e, x1, y1, x2, y2);
            x1 = originX + x1 * scale;
            y1 = originY + y1 * scale;
            x2 = originX + x2 * scale;
            y2 = originY + y2 * scale;
        }

        // Aplica os flags de espelhamento (transformacao.espelhado_h/v) aos
        // pontos locais. Chamado ao final de OutlineLocal.
        inline void ApplyMirror(const Element& e, float w, float h,
                                std::vector<ImVec2>& out)
        {
            const bool mh = e.transformacao.value("espelhado_h", 0.0f) > 0.5f;
            const bool mv = e.transformacao.value("espelhado_v", 0.0f) > 0.5f;
            if (!mh && !mv) return;
            for (ImVec2& p : out)
            {
                if (mh) p.x = w - p.x;
                if (mv) p.y = h - p.y;
            }
        }

        // Amostra o contorno do elemento na forma local (0..w, 0..h), sem
        // translação/rotação/escala. Pontos em sentido anti-horário.
        // pixelsPerUnit: escala de tela (pixels por unidade de projeto). O
        // caminho é tessellado com passo ALVO em pixels de tela (~2px), para
        // que a curva fique lisa em QUALQUER zoom — sem facetas "low-poly".
        inline void OutlineLocal(const Element& e, std::vector<ImVec2>& out,
                                 int segments = 48, float pixelsPerUnit = 1.0f)
        {
            out.clear();
            const float w = e.transformacao.value("largura", 160.0f);
            const float h = e.transformacao.value("altura", 32.0f);
            const ShapeKind kind = KindOf(e);
            if (kind == ShapeKind::Ellipse)
            {
                const float rx = w * 0.5f, ry = h * 0.5f;
                out.reserve(segments);
                for (int i = 0; i < segments; ++i)
                {
                    const float a = (float)(2.0 * 3.14159265358979323846 * i) /
                                    (float)segments;
                    out.push_back(ImVec2(rx + rx * cosf(a), ry + ry * sinf(a)));
                }
                ApplyMirror(e, w, h, out);
                return;
            }
            if (e.tipo == "caminho")
            {
                // Caminho (caneta Bezier): pontos locais com alça de saída.
                // Cada ponto: {x, y, cx2, cy2, curva}. A alça de entrada de B
                // é o espelho da alça de saída de B (modelo Illustrator).
                if (e.transformacao.contains("pontos") &&
                    e.transformacao["pontos"].is_array())
                {
                    const auto& arr = e.transformacao["pontos"];
                    const int n = (int)arr.size();
                    if (n >= 2)
                    {
                        const bool closed = e.transformacao.value("fechado", 0.0f) > 0.5f;
                        const int segments = closed ? n : n - 1;
                        for (int i = 0; i < segments; ++i)
                        {
                            const int j = (i + 1) % n;
                            const auto& pa = arr[i];
                            const auto& pb = arr[j];
                            const float ax = pa.value("x", 0.0f);
                            const float ay = pa.value("y", 0.0f);
                            const float bx = pb.value("x", 0.0f);
                            const float by = pb.value("y", 0.0f);
                            const bool curved = pa.value("curva", 0.0f) > 0.5f ||
                                                pb.value("curva", 0.0f) > 0.5f;
                            if (curved)
                            {
                                const float c2x = ax + pa.value("cx2", 0.0f);
                                const float c2y = ay + pa.value("cy2", 0.0f);
                                // Alça de ENTRADA de B: quando o ponto está
                                // "quebrado" (alças individuais, Alt+clique),
                                // usa a própria cx1/cy1; senão usa o espelho
                                // da alça de saída (modo uniforme).
                                float c1x = 0.0f, c1y = 0.0f;
                                if (pb.value("quebrado", 0.0f) > 0.5f)
                                {
                                    c1x = bx + pb.value("cx1", 0.0f);
                                    c1y = by + pb.value("cy1", 0.0f);
                                }
                                else
                                {
                                    c1x = bx - pb.value("cx2", 0.0f);
                                    c1y = by - pb.value("cy2", 0.0f);
                                }
                                // Passos ADAPTATIVOS em PIXELS DE TELA: o
                                // comprimento aproximado do polígono de controle
                                // (em unidades de projeto) multiplicado pela
                                // escala vira pixels, e o passo alvo é ~2px de
                                // tela. Em qualquer zoom o traço fica liso — sem
                                // facetas "low-poly" (que o passo fixo causava).
                                const float approx = fabsf(c2x - ax) + fabsf(c2y - ay) +
                                                     fabsf(c1x - c2x) + fabsf(c1y - c2y) +
                                                     fabsf(bx - c1x) + fabsf(by - c1y);
                                const float stepPx = std::max(0.25f, pixelsPerUnit);
                                const int steps = std::max(16, std::min(512,
                                    (int)(approx * stepPx / 2.0f + 0.5f)));
                                for (int s = 0; s <= steps; ++s)
                                {
                                    const float t = (float)s / (float)steps;
                                    const float u = 1.0f - t;
                                    const float w0 = u * u * u;
                                    const float w1 = 3.0f * u * u * t;
                                    const float w2 = 3.0f * u * t * t;
                                    const float w3 = t * t * t;
                                    const float px = w0 * ax + w1 * c2x + w2 * c1x + w3 * bx;
                                    const float py = w0 * ay + w1 * c2y + w2 * c1y + w3 * by;
                                    if (out.empty())
                                        out.push_back(ImVec2(px, py));
                                    else
                                    {
                                        const float dx = px - out.back().x;
                                        const float dy = py - out.back().y;
                                        if (dx * dx + dy * dy > 0.0001f)
                                            out.push_back(ImVec2(px, py));
                                    }
                                }
                            }
                            else
                            {
                                if (out.empty())
                                    out.push_back(ImVec2(ax, ay));
                                else
                                {
                                    const float dx = ax - out.back().x;
                                    const float dy = ay - out.back().y;
                                    if (dx * dx + dy * dy > 0.0001f)
                                        out.push_back(ImVec2(ax, ay));
                                }
                            }
                        }
                        if (closed)
                        {
                            if (out.size() >= 2)
                            {
                                const float fpx = out.front().x;
                                const float fpy = out.front().y;
                                const float ddx = out.back().x - fpx;
                                const float ddy = out.back().y - fpy;
                                if (ddx * ddx + ddy * ddy > 0.0001f)
                                    out.push_back(ImVec2(fpx, fpy));
                            }
                        }
                        else if (n >= 2)
                        {
                            const float lastX = arr[n - 1].value("x", 0.0f);
                            const float lastY = arr[n - 1].value("y", 0.0f);
                            if (out.empty())
                                out.push_back(ImVec2(lastX, lastY));
                            else
                            {
                                const float dx = lastX - out.back().x;
                                const float dy = lastY - out.back().y;
                                if (dx * dx + dy * dy > 0.0001f)
                                    out.push_back(ImVec2(lastX, lastY));
                            }
                        }
                        else
                        {
                            const auto& last = arr[n - 1];
                            const float lx = last.value("x", 0.0f);
                            const float ly = last.value("y", 0.0f);
                            if (out.empty())
                                out.push_back(ImVec2(lx, ly));
                            else
                            {
                                const float dx = lx - out.back().x;
                                const float dy = ly - out.back().y;
                                if (dx * dx + dy * dy > 0.0001f)
                                    out.push_back(ImVec2(lx, ly));
                            }
                        }
                        ApplyMirror(e, w, h, out);
                        return;
                    }
                }
                // Sem pontos: diagonal da caixa como fallback visível.
                out.push_back(ImVec2(0.0f, 0.0f));
                out.push_back(ImVec2(w, h));
                ApplyMirror(e, w, h, out);
                return;
            }

            if (kind == ShapeKind::Polygon)
            {
                // Polígono/estrela configurável: transformacao.lados,
                // transformacao.estrela e transformacao.raio_interno.
                const int sides = std::max(3, (int)std::lroundf(
                    e.transformacao.value("lados", 6.0f)));
                const bool star = e.transformacao.value("estrela", 0.0f) > 0.5f;
                const float innerRatio = std::max(0.1f, std::min(0.95f,
                    e.transformacao.value("raio_interno", 0.5f)));
                const float rx = w * 0.5f, ry = h * 0.5f;
                const int count = star ? sides * 2 : sides;
                out.reserve(count);
                const float twoPi = (float)(2.0 * 3.14159265358979323846);
                for (int i = 0; i < count; ++i)
                {
                    const float a = (float)(-0.5 * 3.14159265358979323846) +
                                    twoPi * (float)i / (float)count;
                    const float r = (star && (i % 2 == 1)) ? innerRatio : 1.0f;
                    out.push_back(ImVec2(w * 0.5f + rx * r * cosf(a),
                                         h * 0.5f + ry * r * sinf(a)));
                }
                ApplyMirror(e, w, h, out);
                return;
            }

            float radii[4];
            CornerRadii(e, radii);
            const int perCorner = std::max(1, segments / 4);
            const float twoPi = (float)(2.0 * 3.14159265358979323846);
            // Cantos: ângulos de 180° até 0°, percorrendo TL, TR, BR, BL.
            const float centers[4][2] = {
                { radii[0], radii[0] },
                { w - radii[1], radii[1] },
                { w - radii[2], h - radii[2] },
                { radii[3], h - radii[3] }
            };
            const float startAngles[4] = { 180.0f, 90.0f, 0.0f, -90.0f };
            const float endAngles[4] = { 90.0f, 0.0f, -90.0f, -180.0f };
            out.reserve(perCorner * 4);
            for (int corner = 0; corner < 4; ++corner)
            {
                const float r = radii[corner];
                if (r < 0.01f)
                {
                    out.push_back(ImVec2(centers[corner][0], centers[corner][1]));
                    continue;
                }
                for (int i = 0; i < perCorner; ++i)
                {
                    const float t = (float)i / (float)perCorner;
                    const float a = startAngles[corner] +
                                    (endAngles[corner] - startAngles[corner]) * t;
                    const float rad = DegToRad(a);
                    out.push_back(ImVec2(centers[corner][0] + r * cosf(rad),
                                         centers[corner][1] + r * sinf(rad)));
                }
            }

            // Espelhamento H/V (flip estilo CorelDRAW): inverte a geometria
            // local. O retângulo/elipse são simétricos (não muda visualmente),
            // mas polígonos/estrelas/linhas realmente espelham.
            ApplyMirror(e, w, h, out);
        }

        // Contorno em coordenadas de projeto (translação + rotação aplicada).
        inline void OutlineProject(const Element& e, std::vector<ImVec2>& out,
                                   int segments = 48, float pixelsPerUnit = 1.0f)
        {
            OutlineLocal(e, out, segments, pixelsPerUnit);
            const float x = e.transformacao.value("x", 0.0f);
            const float y = e.transformacao.value("y", 0.0f);
            const float radians = DegToRad(ElementRotation(e));
            if (fabsf(radians) < 0.0001f)
            {
                for (ImVec2& p : out) { p.x += x; p.y += y; }
                return;
            }
            float px = 0.0f, py = 0.0f;
            ElementPivot(e, px, py);
            for (ImVec2& p : out)
            {
                p.x += x; p.y += y;
                RotatePoint(p.x, p.y, px, py, radians);
            }
        }

        // Contorno em coordenadas de tela (escala + deslocamento de origem).
        // A escala é repassada como pixelsPerUnit para a tesselação do caminho
        // ficar lisa em qualquer zoom (passo alvo ~2px de tela).
        inline void OutlineScreen(const Element& e, float originX, float originY,
                                  float scale, std::vector<ImVec2>& out,
                                  int segments = 48)
        {
            OutlineProject(e, out, segments, scale);
            for (ImVec2& p : out)
            {
                p.x = originX + p.x * scale;
                p.y = originY + p.y * scale;
            }
        }

        // Limite exato do contorno rotacionado (equivalente ao da seleção).
        // Usado pelos guias inteligentes e pela caixa de seleção.
        inline void OutlineBounds(const Element& e, float& minX, float& minY,
                                  float& maxX, float& maxY, int segments = 48)
        {
            std::vector<ImVec2> points;
            OutlineProject(e, points, segments);
            if (points.empty()) { RotatedAABB(e, minX, minY, maxX, maxY); return; }
            minX = points[0].x; minY = points[0].y;
            maxX = points[0].x; maxY = points[0].y;
            for (const ImVec2& p : points)
            {
                minX = std::min(minX, p.x); minY = std::min(minY, p.y);
                maxX = std::max(maxX, p.x); maxY = std::max(maxY, p.y);
            }
        }
    }
}

#endif // SEEDUI_GEO_H