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
                return;
            }
            if (kind == ShapeKind::Polygon)
            {
                const float rx = std::min(w, h) * 0.5f;
                out.reserve(6);
                for (int i = 0; i < 6; ++i)
                {
                    const float a = (float)(-0.5 * 3.14159265358979323846) +
                                    (float)(2.0 * 3.14159265358979323846 * i) / 6.0f;
                    out.push_back(ImVec2(w * 0.5f + rx * cosf(a),
                                         h * 0.5f + rx * sinf(a)));
                }
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