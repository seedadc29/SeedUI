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

        // Passo ÚNICO do sistema de grade: usado tanto pelo DESENHO do grid
        // (Canvas) quanto pelo SNAP (App) — uma única fonte matemática, para
        // que o encaixe do snap coincida exatamente com os pontos visíveis
        // da grade em qualquer zoom. Major = 5× o passo (40 unidades).
        inline constexpr float kGridStep = 8.0f;
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
        inline void OutlineLocal(const Element& e, std::vector<ImVec2>& out,
                                 int segments = 48)
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
                        const int steps = 12;
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
                                const float c1x = bx - pb.value("cx2", 0.0f);
                                const float c1y = by - pb.value("cy2", 0.0f);
                                for (int s = 0; s < steps; ++s)
                                {
                                    const float t = (float)s / (float)steps;
                                    const float u = 1.0f - t;
                                    const float w0 = u * u * u;
                                    const float w1 = 3.0f * u * u * t;
                                    const float w2 = 3.0f * u * t * t;
                                    const float w3 = t * t * t;
                                    out.push_back(ImVec2(
                                        w0 * ax + w1 * c2x + w2 * c1x + w3 * bx,
                                        w0 * ay + w1 * c2y + w2 * c1y + w3 * by));
                                }
                            }
                            else
                            {
                                out.push_back(ImVec2(ax, ay));
                            }
                        }
                        if (closed)
                        {
                            const auto& first = arr[0];
                            out.push_back(ImVec2(first.value("x", 0.0f),
                                                 first.value("y", 0.0f)));
                        }
                        else
                        {
                            // Aberto: o último ponto é o fim do último segmento.
                            const auto& last = arr[n - 1];
                            out.push_back(ImVec2(last.value("x", 0.0f),
                                                 last.value("y", 0.0f)));
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
                                   int segments = 48)
        {
            OutlineLocal(e, out, segments);
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
        inline void OutlineScreen(const Element& e, float originX, float originY,
                                  float scale, std::vector<ImVec2>& out,
                                  int segments = 48)
        {
            OutlineProject(e, out, segments);
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