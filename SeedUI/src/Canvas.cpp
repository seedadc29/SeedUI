#include "Canvas.h"

#include "ColorUtils.h"
#include "Geo.h"
#include "Theme.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace seedui
{
    namespace
    {
        struct CornerRadii
        {
            float topLeft = 0.0f;
            float topRight = 0.0f;
            float bottomRight = 0.0f;
            float bottomLeft = 0.0f;
        };

        float JsonNumber(const nlohmann::json& object, const char* key, float fallback = 0.0f)
        {
            if (!object.is_object()) return fallback;
            const auto it = object.find(key);
            return it != object.end() && it->is_number()
                ? std::max(0.0f, it->get<float>())
                : fallback;
        }

        CornerRadii GetCornerRadii(const Element& element, float width, float height)
        {
            const float uniform = JsonNumber(element.estilos, "raio", 0.0f);
            const nlohmann::json empty = nlohmann::json::object();
            const nlohmann::json& corners =
                element.estilos.is_object() && element.estilos.contains("raio_quinas") &&
                element.estilos["raio_quinas"].is_object()
                    ? element.estilos["raio_quinas"]
                    : empty;
            const float maximum = std::max(0.0f, std::min(width, height) * 0.5f);
            CornerRadii radii;
            radii.topLeft = std::min(maximum, JsonNumber(corners, "superior_esquerda", uniform));
            radii.topRight = std::min(maximum, JsonNumber(corners, "superior_direita", uniform));
            radii.bottomRight = std::min(maximum, JsonNumber(corners, "inferior_direita", uniform));
            radii.bottomLeft = std::min(maximum, JsonNumber(corners, "inferior_esquerda", uniform));
            return radii;
        }

        void RoundedRectPath(ImDrawList* dl, const ImVec2& a, const ImVec2& b,
                             const CornerRadii& radii)
        {
            constexpr float kappa = 0.5522847498f;
            dl->PathLineTo(ImVec2(a.x + radii.topLeft, a.y));
            dl->PathLineTo(ImVec2(b.x - radii.topRight, a.y));
            if (radii.topRight > 0.0f)
                dl->PathBezierCubicCurveTo(
                    ImVec2(b.x - radii.topRight * (1.0f - kappa), a.y),
                    ImVec2(b.x, a.y + radii.topRight * (1.0f - kappa)),
                    ImVec2(b.x, a.y + radii.topRight));
            else dl->PathLineTo(ImVec2(b.x, a.y));
            dl->PathLineTo(ImVec2(b.x, b.y - radii.bottomRight));
            if (radii.bottomRight > 0.0f)
                dl->PathBezierCubicCurveTo(
                    ImVec2(b.x, b.y - radii.bottomRight * (1.0f - kappa)),
                    ImVec2(b.x - radii.bottomRight * (1.0f - kappa), b.y),
                    ImVec2(b.x - radii.bottomRight, b.y));
            else dl->PathLineTo(b);
            dl->PathLineTo(ImVec2(a.x + radii.bottomLeft, b.y));
            if (radii.bottomLeft > 0.0f)
                dl->PathBezierCubicCurveTo(
                    ImVec2(a.x + radii.bottomLeft * (1.0f - kappa), b.y),
                    ImVec2(a.x, b.y - radii.bottomLeft * (1.0f - kappa)),
                    ImVec2(a.x, b.y - radii.bottomLeft));
            else dl->PathLineTo(ImVec2(a.x, b.y));
            dl->PathLineTo(ImVec2(a.x, a.y + radii.topLeft));
            if (radii.topLeft > 0.0f)
                dl->PathBezierCubicCurveTo(
                    ImVec2(a.x, a.y + radii.topLeft * (1.0f - kappa)),
                    ImVec2(a.x + radii.topLeft * (1.0f - kappa), a.y),
                    ImVec2(a.x + radii.topLeft, a.y));
            else dl->PathLineTo(a);
        }

        void DrawRoundedRect(ImDrawList* dl, const ImVec2& a, const ImVec2& b,
                             const CornerRadii& radii, ImU32 fill, ImU32 outline,
                             float outlineWidth = 1.0f)
        {
            const bool square = radii.topLeft <= 0.01f && radii.topRight <= 0.01f &&
                                radii.bottomRight <= 0.01f && radii.bottomLeft <= 0.01f;
            if (square)
            {
                if (outlineWidth > 0.0f) dl->AddRectFilled(a, b, outline);
                const float inset = std::min(std::max(0.0f, outlineWidth),
                    std::max(0.0f, std::min(b.x - a.x, b.y - a.y) * 0.5f));
                const ImVec2 innerA(a.x + inset, a.y + inset);
                const ImVec2 innerB(b.x - inset, b.y - inset);
                if (innerB.x > innerA.x && innerB.y > innerA.y)
                    dl->AddRectFilled(innerA, innerB, fill);
                return;
            }

            // Um PathStroke grosso produz juncoes em mitra (os "bicos" vistos
            // nas quinas). Como num editor vetorial, construimos o contorno
            // como duas formas preenchidas: silhueta externa e miolo interno.
            // Assim a espessura fica uniforme, limpa e antialiasada.
            if (outlineWidth <= 0.0f)
            {
                RoundedRectPath(dl, a, b, radii);
                dl->PathFillConvex(fill);
                return;
            }

            RoundedRectPath(dl, a, b, radii);
            dl->PathFillConvex(outline);

            const float maximumInset = std::max(0.0f,
                std::min(b.x - a.x, b.y - a.y) * 0.5f);
            const float inset = std::min(outlineWidth, maximumInset);
            const ImVec2 innerA(a.x + inset, a.y + inset);
            const ImVec2 innerB(b.x - inset, b.y - inset);
            if (innerB.x <= innerA.x || innerB.y <= innerA.y) return;

            CornerRadii inner = radii;
            inner.topLeft = std::max(0.0f, inner.topLeft - inset);
            inner.topRight = std::max(0.0f, inner.topRight - inset);
            inner.bottomRight = std::max(0.0f, inner.bottomRight - inset);
            inner.bottomLeft = std::max(0.0f, inner.bottomLeft - inset);
            RoundedRectPath(dl, innerA, innerB, inner);
            dl->PathFillConvex(fill);
        }

        bool GetDashStyle(const Element& e, float& dash, float& gap);
        void DrawDashedClosedPolyline(ImDrawList* dl, const ImVec2* pts, int count,
                                      ImU32 color, float width,
                                      float dash, float gap);

        // Sombra suave barata (compatível com PC fraco): desenha camadas do
        // contorno tessellado, cada uma levemente expandida e mais transparente
        // que a anterior, todas deslocadas por (deslocamento_x/y). 4 camadas
        // máximas, sem blur de GPU.
        void DrawShadow(const Element& e, const ImVec2& origin, float scale,
                        float opacity, ImDrawList* dl)
        {
            if (!e.estilos.is_object() || !e.estilos.contains("sombra") ||
                !e.estilos["sombra"].is_object())
                return;
            const auto& s = e.estilos["sombra"];
            const float dx = s.value("deslocamento_x", 4.0f);
            const float dy = s.value("deslocamento_y", 4.0f);
            const float blur = std::max(0.0f, s.value("desfoque", 6.0f));
            if (fabsf(dx) < 0.01f && fabsf(dy) < 0.01f && blur < 0.01f) return;

            float rgb[3] = { 0.0f, 0.0f, 0.0f };
            if (s.contains("cor") && s["cor"].is_string())
                ColorUtils::ParseHex(s["cor"].get<std::string>(), rgb);

            std::vector<ImVec2> pts;
            Geo::OutlineScreen(e, origin.x, origin.y, scale, pts, 48);
            if (pts.size() < 3) return;

            const int layers = blur > 0.5f ? 4 : 1;
            // Camadas: a interna é a mais escura e compacta; as externas
            // expandem levemente e somem (simula o desfoque).
            const float blurPx = blur * scale;
            const float cx = origin.x +
                (e.transformacao.value("x", 0.0f) +
                 e.transformacao.value("largura", 160.0f) * 0.5f) * scale;
            const float cy = origin.y +
                (e.transformacao.value("y", 0.0f) +
                 e.transformacao.value("altura", 32.0f) * 0.5f) * scale;
            const float shiftX = dx * scale, shiftY = dy * scale;
            for (int layer = 0; layer < layers; ++layer)
            {
                const float t = (float)(layer + 1) / (float)layers;
                const float expand = blurPx * t * t * 0.55f;
                const float alpha = 0.38f / (float)layers * t * opacity;
                if (alpha <= 0.004f) continue;
                const ImU32 color = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(rgb[0], rgb[1], rgb[2], alpha));
                std::vector<ImVec2> layerPts(pts.size());
                for (size_t i = 0; i < pts.size(); ++i)
                {
                    const float px = pts[i].x + shiftX;
                    const float py = pts[i].y + shiftY;
                    const float ddx = px - cx, ddy = py - cy;
                    const float dist = sqrtf(ddx * ddx + ddy * ddy);
                    const float k = dist > 0.01f ? (dist + expand) / dist : 1.0f;
                    layerPts[i] = ImVec2(cx + ddx * k, cy + ddy * k);
                }
                dl->AddConvexPolyFilled(layerPts.data(), (int)layerPts.size(), color);
            }
        }

        // Desenha um elemento e seus filhos recursivamente (versão simples do M03).
        void DrawElement(const Element& e, const ImVec2& origin, float scale, ImDrawList* dl)
        {
            if (!e.visivel) return;

            float x = 0, y = 0, w = 160, h = 32;
            if (e.transformacao.is_object())
            {
                x = e.transformacao.value("x", 0.0f);
                y = e.transformacao.value("y", 0.0f);
                w = e.transformacao.value("largura", 160.0f);
                h = e.transformacao.value("altura", 32.0f);
            }

            const ImVec2 a(origin.x + x * scale, origin.y + y * scale);
            const ImVec2 b(origin.x + (x + w) * scale, origin.y + (y + h) * scale);

            const float opacity = std::max(0.0f, std::min(1.0f,
                e.estilos.value("opacidade", 1.0f)));
            // Cor do preenchimento/contorno vinda de estilos.cor_fundo e
            // estilos.cor_borda ("#rrggbb"); fallback para o neutro atual.
            float fillRgb[3] = { 0x2b / 255.0f, 0x2b / 255.0f, 0x2b / 255.0f };
            float borderRgb[3] = { 0x5a / 255.0f, 0x5a / 255.0f, 0x5a / 255.0f };
            if (e.estilos.is_object() && e.estilos.contains("cor_fundo"))
                ColorUtils::ParseHex(e.estilos["cor_fundo"].get<std::string>(), fillRgb);
            if (e.estilos.is_object() && e.estilos.contains("cor_borda"))
                ColorUtils::ParseHex(e.estilos["cor_borda"].get<std::string>(), borderRgb);
            const ImU32 fill = ImGui::ColorConvertFloat4ToU32(
                ImVec4(fillRgb[0], fillRgb[1], fillRgb[2], opacity));
            // Elementos não selecionados usam uma borda neutra e discreta.
            // Azul/laranja ficam reservados exclusivamente para a seleção.
            const ImU32 outline = ImGui::ColorConvertFloat4ToU32(
                ImVec4(borderRgb[0], borderRgb[1], borderRgb[2], 0.82f));
            const float outlineWidth = std::max(0.0f,
                e.estilos.value("espessura_borda", 1.0f)) * scale;
            const ImU32 label = ImGui::ColorConvertFloat4ToU32(Theme::TextPrimary);
            const float rotation = Geo::ElementRotation(e);

            float dashLen = 0.0f, gapLen = 0.0f;
            const bool dashed = GetDashStyle(e, dashLen, gapLen) &&
                                outlineWidth > 0.0f;

            if (e.tipo != "grupo")
            {
                DrawShadow(e, origin, scale, opacity, dl);
                if (dashed)
                {
                    // Contorno tracejado: usa o mesmo contorno tessellado
                    // (funciona rotacionado, espelhado, com quinas).
                    std::vector<ImVec2> pts;
                    Geo::OutlineScreen(e, origin.x, origin.y, scale, pts, 64);
                    if (pts.size() >= 3)
                    {
                        dl->AddConvexPolyFilled(pts.data(), (int)pts.size(), fill);
                        DrawDashedClosedPolyline(dl, pts.data(), (int)pts.size(),
                                                 outline, outlineWidth,
                                                 dashLen, gapLen);
                    }
                }
                else if (fabsf(rotation) > 0.01f)
                {
                    // Elemento rotacionado: tessela o contorno no espaço local,
                    // aplica a rotação e desenha como polígono preenchido +
                    // contorno com junções arredondadas (sem bicos).
                    std::vector<ImVec2> pts;
                    Geo::OutlineScreen(e, origin.x, origin.y, scale, pts, 64);
                    if (pts.size() >= 3)
                    {
                        dl->AddConvexPolyFilled(pts.data(), (int)pts.size(), fill);
                        if (outlineWidth > 0.0f)
                            dl->AddPolyline(pts.data(), (int)pts.size(), outline,
                                ImDrawFlags_Closed, outlineWidth);
                    }
                }
                else if (e.tipo == "elipse")
                {
                    const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
                    const ImVec2 radius((b.x - a.x) * 0.5f, (b.y - a.y) * 0.5f);
                    if (outlineWidth > 0.0f)
                    {
                        dl->AddEllipseFilled(center, radius, outline, 0.0f, 64);
                        const ImVec2 innerRadius(
                            std::max(0.0f, radius.x - outlineWidth),
                            std::max(0.0f, radius.y - outlineWidth));
                        if (innerRadius.x > 0.0f && innerRadius.y > 0.0f)
                            dl->AddEllipseFilled(center, innerRadius, fill, 0.0f, 64);
                    }
                    else dl->AddEllipseFilled(center, radius, fill, 0.0f, 64);
                }
                else if (e.tipo == "poligono")
                {
                    const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
                    const float radius = std::min(b.x - a.x, b.y - a.y) * 0.5f;
                    if (outlineWidth > 0.0f)
                    {
                        dl->AddNgonFilled(center, radius, outline, 6);
                        const float innerRadius = std::max(0.0f, radius - outlineWidth);
                        if (innerRadius > 0.0f)
                            dl->AddNgonFilled(center, innerRadius, fill, 6);
                    }
                    else dl->AddNgonFilled(center, radius, fill, 6);
                }
                else
                {
                    CornerRadii radii = GetCornerRadii(e, w, h);
                    radii.topLeft *= scale;
                    radii.topRight *= scale;
                    radii.bottomRight *= scale;
                    radii.bottomLeft *= scale;
                    DrawRoundedRect(dl, a, b, radii, fill, outline, outlineWidth);
                }

                if (e.tipo != "retangulo" && e.tipo != "elipse" && e.tipo != "poligono")
                {
                    const char* text = e.nome.empty() ? e.id.c_str() : e.nome.c_str();
                    dl->AddText(ImVec2(a.x + 4, a.y + 4), label, text);
                }
            }

            for (const Element& f : e.filhos)
                DrawElement(f, origin, scale, dl);
        }

        // Estilo de traço do contorno: estilos.tracejado {largura_traco,
        // largura_espaco}. Devolve false quando não configurado.
        bool GetDashStyle(const Element& e, float& dash, float& gap)
        {
            if (!e.estilos.is_object() || !e.estilos.contains("tracejado") ||
                !e.estilos["tracejado"].is_object())
                return false;
            const auto& t = e.estilos["tracejado"];
            dash = std::max(0.5f, t.value("largura_traco", 6.0f));
            gap = std::max(0.5f, t.value("largura_espaco", 4.0f));
            return true;
        }

        // Contorno fechado tracejado: caminha pelos segmentos do polígono
        // acumulando o comprimento, alternando traço/gap (estilo CorelDRAW).
        void DrawDashedClosedPolyline(ImDrawList* dl, const ImVec2* pts, int count,
                                      ImU32 color, float width,
                                      float dash, float gap)
        {
            if (count < 2) return;
            bool draw = true;
            float segRemaining = dash;
            for (int i = 0; i < count; ++i)
            {
                const ImVec2& a = pts[i];
                const ImVec2& b = pts[(i + 1) % count];
                const float dx = b.x - a.x, dy = b.y - a.y;
                const float len = sqrtf(dx * dx + dy * dy);
                if (len <= 0.0f) continue;
                const float ux = dx / len, uy = dy / len;
                float pos = 0.0f;
                while (pos < len)
                {
                    const float step = std::min(len - pos, segRemaining);
                    if (draw)
                        dl->AddLine(ImVec2(a.x + ux * pos, a.y + uy * pos),
                                    ImVec2(a.x + ux * (pos + step),
                                           a.y + uy * (pos + step)),
                                    color, width);
                    pos += step;
                    segRemaining -= step;
                    if (segRemaining <= 0.001f)
                    {
                        draw = !draw;
                        segRemaining = draw ? dash : gap;
                    }
                }
            }
        }

        void DrawDashedLine(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 color)
        {
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float length = sqrtf(dx * dx + dy * dy);
            if (length <= 0.0f) return;
            const float ux = dx / length;
            const float uy = dy / length;
            for (float p = 0.0f; p < length; p += 10.0f)
            {
                const float end = std::min(length, p + 6.0f);
                dl->AddLine(ImVec2(a.x + ux * p, a.y + uy * p),
                            ImVec2(a.x + ux * end, a.y + uy * end), color, 1.5f);
            }
        }

        void DrawDashedRect(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 color)
        {
            const ImVec2 mn(std::min(a.x, b.x), std::min(a.y, b.y));
            const ImVec2 mx(std::max(a.x, b.x), std::max(a.y, b.y));
            DrawDashedLine(dl, mn, ImVec2(mx.x, mn.y), color);
            DrawDashedLine(dl, ImVec2(mx.x, mn.y), mx, color);
            DrawDashedLine(dl, mx, ImVec2(mn.x, mx.y), color);
            DrawDashedLine(dl, ImVec2(mn.x, mx.y), mn, color);
        }

        const Element* FindElement(const std::vector<Element>& elements,
                                   const std::string& id)
        {
            for (const Element& element : elements)
            {
                if (element.id == id) return &element;
                if (const Element* found = FindElement(element.filhos, id)) return found;
            }
            return nullptr;
        }
    }

    bool CanvasScreenToProject(const Project* projeto, float screenX, float screenY,
                               float& projectX, float& projectY,
                               bool limitarNaMoldura, float zoom,
                               float panX, float panY)
    {
        if (!projeto) return false;
        const ImVec2 min = ImGui::GetWindowPos();
        const ImVec2 max(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());
        const float baseW = (float)projeto->telaBaseLargura;
        const float baseH = (float)projeto->telaBaseAltura;
        if (baseW <= 0.0f || baseH <= 0.0f) return false;

        const float ruler = 24.0f;
        const float viewportPadding = 44.0f;
        const ImVec2 contentMin(min.x + ruler + viewportPadding,
                                min.y + ruler + viewportPadding);
        const ImVec2 contentMax(max.x - viewportPadding, max.y - viewportPadding);
        const float availW = contentMax.x - contentMin.x;
        const float availH = contentMax.y - contentMin.y;
        float scale = std::min(availW / baseW, availH / baseH);
        scale = std::max(0.25f, std::min(1.0f, scale));
        scale *= std::max(0.1f, std::min(32.0f, zoom));
        const ImVec2 frame(baseW * scale, baseH * scale);
        const ImVec2 origin(contentMin.x + (availW - frame.x) * 0.5f + panX,
                            contentMin.y + (availH - frame.y) * 0.5f + panY);
        // O canvas é um espaço de trabalho livre: a conversão nunca falha
        // dentro da janela e devolve coordenadas de projeto além da moldura.
        // limitarNaMoldura apenas clampeia o resultado (marquee antigo, etc.).
        projectX = (screenX - origin.x) / scale;
        projectY = (screenY - origin.y) / scale;
        if (limitarNaMoldura)
        {
            projectX = std::max(0.0f, std::min(baseW, projectX));
            projectY = std::max(0.0f, std::min(baseH, projectY));
        }
        return true;
    }

    bool CanvasProjectToScreen(const Project* projeto, float projectX, float projectY,
                               float& screenX, float& screenY, float& scale, float zoom,
                               float panX, float panY)
    {
        if (!projeto || projeto->telaBaseLargura <= 0 || projeto->telaBaseAltura <= 0)
            return false;
        const ImVec2 min = ImGui::GetWindowPos();
        const ImVec2 max(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());
        const float baseW = (float)projeto->telaBaseLargura;
        const float baseH = (float)projeto->telaBaseAltura;
        const float ruler = 24.0f;
        const float viewportPadding = 44.0f;
        const ImVec2 contentMin(min.x + ruler + viewportPadding,
                                min.y + ruler + viewportPadding);
        const ImVec2 contentMax(max.x - viewportPadding, max.y - viewportPadding);
        const float availW = contentMax.x - contentMin.x;
        const float availH = contentMax.y - contentMin.y;
        scale = std::max(0.25f, std::min(1.0f, std::min(availW / baseW, availH / baseH)));
        scale *= std::max(0.1f, std::min(32.0f, zoom));
        const ImVec2 frame(baseW * scale, baseH * scale);
        const ImVec2 origin(contentMin.x + (availW - frame.x) * 0.5f + panX,
                            contentMin.y + (availH - frame.y) * 0.5f + panY);
        screenX = origin.x + projectX * scale;
        screenY = origin.y + projectY * scale;
        return true;
    }

    void CanvasDraw(const Project* projeto, int telaAtiva, int modoAtivo,
                    const std::vector<std::string>* elementosSelecionados,
                    const char* elementoPrincipalId,
                    unsigned int quinasSelecionadas,
                    bool exibirReguas, bool reguasBloqueadas,
                    bool exibirGrade,
                    float zoom, float panX, float panY,
                    float unidadeEmPixels)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 min = ImGui::GetWindowPos();
        const ImVec2 max(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());

        const ImU32 bg = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x050505));
        // Grade discreta (menos vibrante): tons escuros de baixo contraste,
        // só pontinhos — a tela-base e as formas continuam sendo o foco.
        const ImU32 gridMin = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x3f3f3f, 0.42f));
        const ImU32 gridMaj = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x5c5c5c, 0.60f));
        const ImU32 border = ImGui::ColorConvertFloat4ToU32(Theme::Border);
        const ImU32 textSec = ImGui::ColorConvertFloat4ToU32(Theme::TextSecondary);
        const ImU32 accent = ImGui::ColorConvertFloat4ToU32(Theme::AccentBlue);

        // Fundo
        dl->AddRectFilled(min, max, bg);

        // Moldura da tela base — computada ANTES da grade para que o grid
        // compartilhe o MESMO espaço de projeto dos elementos (acompanha
        // pan/zoom e alinha exatamente com o snap de 8 unidades).
        const float baseW = projeto ? (float)projeto->telaBaseLargura : 1280.0f;
        const float baseH = projeto ? (float)projeto->telaBaseAltura : 720.0f;
        const float viewportPadding = 44.0f;
        const float ruler = 24.0f;
        const ImVec2 contentMin(min.x + ruler + viewportPadding, min.y + ruler + viewportPadding);
        const ImVec2 contentMax(max.x - viewportPadding, max.y - viewportPadding);
        const float availW = contentMax.x - contentMin.x;
        const float availH = contentMax.y - contentMin.y;
        float viewScale = 1.0f;
        if (baseW > 0.0f && baseH > 0.0f)
        {
            const float scaleX = availW / baseW;
            const float scaleY = availH / baseH;
            viewScale = scaleX < scaleY ? scaleX : scaleY;
            if (viewScale > 1.0f) viewScale = 1.0f;
            if (viewScale < 0.25f) viewScale = 0.25f;
            viewScale *= std::max(0.1f, std::min(32.0f, zoom));
        }

        const ImVec2 frame(baseW * viewScale, baseH * viewScale);
        const ImVec2 origin(contentMin.x + (availW - frame.x) * 0.5f + panX,
                            contentMin.y + (availH - frame.y) * 0.5f + panY);

        // Grade em ESPAÇO DE PROJETO: minor a cada kGridStep (8 unidades — o
        // MESMO passo do snap, fonte única em Geo::kGridStep), major a cada
        // 40 (5×). Recortada à moldura da tela base e à janela visível — fica
        // "dentro do compasso" do snap: cada ponto visível da grade é um
        // ponto EXATO de encaixe, em qualquer zoom.
        if (exibirGrade)
        {
            const float gridMinor = Geo::kGridStep;
            const float gridMajor = Geo::kGridStep * Geo::kGridMajorMult;
            const bool drawMinor = gridMinor * viewScale >= 7.0f;
            // A grade percorre o CANVAS INTEIRO (não só a moldura): o espaço de
            // trabalho é livre e os pontos continuam alinhados às unidades do
            // projeto (8/40), então o snap encaixa exatamente neles em qualquer
            // ponto do canvas.
            const float stepPx = drawMinor ? gridMinor : gridMajor;
            const float stepScreen = stepPx * viewScale;
            const int ix0 = (int)floorf((min.x - origin.x) / stepScreen);
            const int iy0 = (int)floorf((min.y - origin.y) / stepScreen);
            const int ix1 = (int)ceilf((max.x - origin.x) / stepScreen);
            const int iy1 = (int)ceilf((max.y - origin.y) / stepScreen);
            for (int ix = ix0; ix <= ix1; ++ix)
            {
                const float px = ix * stepPx;
                const bool xMajor = drawMinor ? (ix % 5 == 0) : true;
                for (int iy = iy0; iy <= iy1; ++iy)
                {
                    if (!drawMinor && !xMajor) continue;
                    const bool major = xMajor && (drawMinor ? (iy % 5 == 0) : true);
                    dl->AddCircleFilled(
                        ImVec2(origin.x + px * viewScale, origin.y + iy * stepPx * viewScale),
                        major ? 1.35f : 1.0f, major ? gridMaj : gridMin, 8);
                }
            }
        }

        // Réguas (margens da janela) — sincronizadas com o zoom/pan: os
        // números são coordenadas de PROJETO (não pixels da janela), então a
        // régua acompanha o movimento e a escala do canvas. A unidade exibida
        // é convertida (px/mm/cm/in/pt) pelo fator unidadeEmPixels.
        if (exibirReguas)
        {
            const ImU32 rulerBg = ImGui::ColorConvertFloat4ToU32(
                ImVec4(0.06f, 0.06f, 0.06f, 0.92f));
            dl->AddRectFilled(min, ImVec2(max.x, min.y + ruler), rulerBg);
            dl->AddRectFilled(min, ImVec2(min.x + ruler, max.y), rulerBg);
            dl->AddRect(min, ImVec2(min.x + ruler, min.y + ruler), border);

            // Passo "bonito" (1/2/5 ×10^n) em unidades de projeto: ticks com
            // ~40-100px de tela, qualquer que seja o zoom.
            float stepPx = 1.0f;
            {
                constexpr float kNice[] = { 1, 2, 5, 10, 20, 50, 100, 200, 500,
                                            1000, 2000, 5000, 10000, 20000, 50000 };
                for (float s : kNice)
                {
                    stepPx = s;
                    if (s * viewScale >= 40.0f) break;
                }
            }
            const bool unitPx = unidadeEmPixels <= 1.001f;
            const char* fmt = unitPx ? "%.0f" : "%.1f";

            // Régua horizontal: ticks em coordenadas de PROJETO.
            {
                const float p0 = (min.x + ruler - origin.x) / viewScale;
                const float p1 = (max.x - origin.x) / viewScale;
                const int i0 = (int)floorf(p0 / stepPx);
                const int i1 = (int)ceilf(p1 / stepPx);
                for (int i = i0; i <= i1; ++i)
                {
                    const float sx = origin.x + i * stepPx * viewScale;
                    if (sx < min.x + ruler || sx > max.x) continue;
                    const bool major = (i % 5 == 0);
                    const float h = major ? 12.0f : 6.0f;
                    dl->AddLine(ImVec2(sx, min.y), ImVec2(sx, min.y + h),
                                major ? textSec : border, 1.0f);
                    if (major)
                    {
                        char buf[32];
                        snprintf(buf, sizeof buf, fmt,
                                 (float)i * stepPx / unidadeEmPixels);
                        dl->AddText(ImVec2(sx + 3, min.y + 2), textSec, buf);
                    }
                }
            }
            // Régua vertical: idem, agora com números também.
            {
                const float p0 = (min.y + ruler - origin.y) / viewScale;
                const float p1 = (max.y - origin.y) / viewScale;
                const int j0 = (int)floorf(p0 / stepPx);
                const int j1 = (int)ceilf(p1 / stepPx);
                for (int j = j0; j <= j1; ++j)
                {
                    const float sy = origin.y + j * stepPx * viewScale;
                    if (sy < min.y + ruler || sy > max.y) continue;
                    const bool major = (j % 5 == 0);
                    const float w = major ? 12.0f : 6.0f;
                    dl->AddLine(ImVec2(min.x, sy), ImVec2(min.x + w, sy),
                                major ? textSec : border, 1.0f);
                    if (major)
                    {
                        char buf[32];
                        snprintf(buf, sizeof buf, fmt,
                                 (float)j * stepPx / unidadeEmPixels);
                        dl->AddText(ImVec2(min.x + 3, sy - 5.0f), textSec, buf);
                    }
                }
            }

            // Indicador de BLOQUEIO da régua: cadeado discreto no canto onde
            // as réguas se cruzam. A régua continua visível e funcional como
            // referência de snap — apenas a interação (criar/arrastar guias)
            // fica bloqueada. Clicar no cadeado alterna o bloqueio (o clique
            // cai na área da janela, tratado no App via hit no canto).
            if (reguasBloqueadas)
            {
                const ImVec2 corner(min.x + ruler, min.y + ruler);
                const float cx = min.x + ruler * 0.5f;
                const float cy = min.y + ruler * 0.5f;
                const ImU32 lockCol = ImGui::ColorConvertFloat4ToU32(
                    Theme::Hex(0xffb347, 1.0f));
                const ImU32 lockDim = ImGui::ColorConvertFloat4ToU32(
                    Theme::Hex(0xffb347, 0.45f));
                // Corpo do cadeado (arco) e caixa.
                dl->AddRect(ImVec2(cx - 2.5f, cy - 1.5f),
                            ImVec2(cx + 2.5f, cy + 4.0f), lockCol, 1.0f);
                dl->AddCircle(ImVec2(cx, cy - 2.5f), 2.6f, lockCol, 12, 1.2f);
                dl->AddCircleFilled(ImVec2(cx, cy + 1.0f), 1.0f, lockCol, 8);
                // Borda sutil do canto (reforça a leitura do quadrado).
                dl->AddRect(min, corner, lockDim, 2.0f);
            }
        }
        if (!projeto || projeto->telas.empty())
        {
            const char* hint = "Crie um projeto para começar (Arquivo -> Novo)";
            const ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, hint);
            const ImVec2 pos((min.x + max.x - ts.x) * 0.5f, (min.y + max.y) * 0.5f);
            dl->AddText(ImGui::GetFont(), 15.0f, pos, textSec, hint);
            return;
        }

        const ImU32 frameCol = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0xb8c7d9, 0.28f));
        dl->AddRect(origin, ImVec2(origin.x + frame.x, origin.y + frame.y), frameCol, 0.0f, 0, 1.0f);
        const ImU32 labelCol = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x8fbaf5, 0.70f));
        char frameLabel[64];
        snprintf(frameLabel, sizeof frameLabel, "%.0f x %.0f — tela base", baseW, baseH);
        dl->AddText(ImVec2(origin.x + 8, origin.y - 18), labelCol, frameLabel);

        if (!projeto || projeto->telas.empty())
        {
            const char* hint = "Crie um projeto para começar (Arquivo → Novo)";
            const ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, hint);
            const ImVec2 pos((min.x + max.x - ts.x) * 0.5f, (min.y + max.y) * 0.5f + 140.0f);
            dl->AddText(ImGui::GetFont(), 15.0f, pos, textSec, hint);
            return;
        }

        // Elementos do modo ativo
        if (telaAtiva < 0 || telaAtiva >= (int)projeto->telas.size()) return;
        const Tela& tela = projeto->telas[telaAtiva];
        if (modoAtivo < 0 || modoAtivo >= (int)tela.modos.size()) return;
        const Modo& modo = tela.modos[modoAtivo];
        for (const Element& e : modo.raiz)
            DrawElement(e, origin, viewScale, dl);

        // Área de segurança: se algum elemento selecionado estiver FORA da
        // tela base, um contorno vermelho fino (como linha-guia) contorna a
        // moldura — o usuário pode sair, mas é avisado do limite principal.
        {
            bool foraDaMoldura = false;
            if (elementosSelecionados)
            {
                for (const std::string& selectedId : *elementosSelecionados)
                {
                    const Element* sel = FindElement(modo.raiz, selectedId);
                    if (!sel || !sel->visivel) continue;
                    float bx0 = 0.0f, by0 = 0.0f, bx1 = 0.0f, by1 = 0.0f;
                    Geo::RotatedAABB(*sel, bx0, by0, bx1, by1);
                    if (bx0 < 0.0f || by0 < 0.0f ||
                        bx1 > baseW || by1 > baseH)
                    {
                        foraDaMoldura = true;
                        break;
                    }
                }
            }
            if (foraDaMoldura)
            {
                const ImU32 safeRed = ImGui::ColorConvertFloat4ToU32(
                    Theme::Hex(0xe74c3c, 0.85f));
                DrawDashedRect(dl, origin,
                               ImVec2(origin.x + frame.x, origin.y + frame.y),
                               safeRed);
            }
        }

        if (elementosSelecionados && !elementosSelecionados->empty())
        {
            const bool multiSelecao = elementosSelecionados->size() > 1;
            if (multiSelecao)
            {
                // Caixa de seleção conjunta (M04): quando mais de um elemento
                // está selecionado, a caixa que move/redimensiona/rotaciona
                // cobre TODO o corpo do conjunto — não cada elemento isolado.
                float uMinX = FLT_MAX, uMinY = FLT_MAX, uMaxX = -FLT_MAX, uMaxY = -FLT_MAX;
                bool any = false;
                for (const std::string& selectedId : *elementosSelecionados)
                {
                    const Element* selected = FindElement(modo.raiz, selectedId);
                    if (!selected || !selected->visivel) continue;
                    float bx0 = 0.0f, by0 = 0.0f, bx1 = 0.0f, by1 = 0.0f;
                    Geo::RotatedAABB(*selected, bx0, by0, bx1, by1);
                    uMinX = std::min(uMinX, bx0);
                    uMinY = std::min(uMinY, by0);
                    uMaxX = std::max(uMaxX, bx1);
                    uMaxY = std::max(uMaxY, by1);
                    const ImVec2 ia(origin.x + bx0 * viewScale, origin.y + by0 * viewScale);
                    const ImVec2 ib(origin.x + bx1 * viewScale, origin.y + by1 * viewScale);
                    dl->AddRect(ia, ib, IM_COL32(255, 255, 255, 45), 0.0f, 0, 1.0f);
                    any = true;
                }
                if (any && uMaxX > uMinX && uMaxY > uMinY)
                {
                    const ImU32 selection = ImGui::ColorConvertFloat4ToU32(Theme::AccentBlue);
                    const ImVec2 a(origin.x + uMinX * viewScale, origin.y + uMinY * viewScale);
                    const ImVec2 b(origin.x + uMaxX * viewScale, origin.y + uMaxY * viewScale);
                    dl->AddRect(a, b, selection, 0.0f, 0, 1.5f);
                    const float hs = 4.0f;
                    const ImVec2 points[] = {
                        a, ImVec2((a.x + b.x) * 0.5f, a.y), ImVec2(b.x, a.y),
                        ImVec2(a.x, (a.y + b.y) * 0.5f), ImVec2(b.x, (a.y + b.y) * 0.5f),
                        ImVec2(a.x, b.y), ImVec2((a.x + b.x) * 0.5f, b.y), b
                    };
                    for (const ImVec2& point : points)
                    {
                        dl->AddRectFilled(ImVec2(point.x - hs, point.y - hs),
                                          ImVec2(point.x + hs, point.y + hs),
                                          IM_COL32(245, 245, 245, 255));
                        dl->AddRect(ImVec2(point.x - hs, point.y - hs),
                                    ImVec2(point.x + hs, point.y + hs), selection);
                    }
                    // Alça de rotação acima do topo da caixa conjunta.
                    const float cx = (uMinX + uMaxX) * 0.5f;
                    const float topY = uMinY;
                    const float sticker = 18.0f / std::max(0.25f, viewScale);
                    const float handleScreenX = origin.x + cx * viewScale;
                    const float handleScreenY = origin.y + (topY - sticker) * viewScale;
                    const float topScreenY = origin.y + topY * viewScale;
                    dl->AddLine(ImVec2(handleScreenX, topScreenY),
                                ImVec2(handleScreenX, handleScreenY), selection, 1.5f);
                    const float hr = 5.0f;
                    dl->AddCircleFilled(ImVec2(handleScreenX, handleScreenY), hr, selection, 16);
                    dl->AddCircle(ImVec2(handleScreenX, handleScreenY), hr,
                                  IM_COL32(255, 255, 255, 210), 16, 1.5f);
                }
            }
            else
            {
            for (const std::string& selectedId : *elementosSelecionados)
            {
                const Element* selected = FindElement(modo.raiz, selectedId);
                if (!selected || !selected->visivel) continue;
                const float x = selected->transformacao.value("x", 0.0f);
                const float y = selected->transformacao.value("y", 0.0f);
                const float w = selected->transformacao.value("largura", 160.0f);
                const float h = selected->transformacao.value("altura", 32.0f);
                const bool primary = elementoPrincipalId && selectedId == elementoPrincipalId;
                const ImU32 selection = ImGui::ColorConvertFloat4ToU32(
                    primary ? Theme::AccentOrange : Theme::AccentBlue);
                const bool rotated = Geo::ElementRotation(*selected) != 0.0f;

                float boxMinX = 0.0f, boxMinY = 0.0f, boxMaxX = 0.0f, boxMaxY = 0.0f;
                Geo::RotatedAABB(*selected, boxMinX, boxMinY, boxMaxX, boxMaxY);
                const ImVec2 a(origin.x + boxMinX * viewScale, origin.y + boxMinY * viewScale);
                const ImVec2 b(origin.x + boxMaxX * viewScale, origin.y + boxMaxY * viewScale);
                if (rotated)
                {
                    // Contorno segue o elemento rotacionado (resolve a caixa
                    // AABB que ficaria "gorda" nas quinas).
                    std::vector<ImVec2> outline;
                    Geo::OutlineScreen(*selected, origin.x, origin.y, viewScale, outline, 64);
                    if (outline.size() >= 3)
                        dl->AddPolyline(outline.data(), (int)outline.size(), selection,
                            ImDrawFlags_Closed, primary ? 1.5f : 1.0f);
                }
                else
                {
                    CornerRadii selectionRadii = GetCornerRadii(*selected, w, h);
                    selectionRadii.topLeft *= viewScale;
                    selectionRadii.topRight *= viewScale;
                    selectionRadii.bottomRight *= viewScale;
                    selectionRadii.bottomLeft *= viewScale;
                    RoundedRectPath(dl, a, b, selectionRadii);
                    dl->PathStroke(selection, ImDrawFlags_Closed, primary ? 1.5f : 1.0f);
                }
                if (!primary || selected->bloqueado) continue;
                if (selected->tipo == "grupo") continue;

                const ImU32 handleFill = IM_COL32(245, 245, 245, 255);
                const float hs = 4.0f;
                const ImVec2 points[] = {
                    a, ImVec2((a.x + b.x) * 0.5f, a.y), ImVec2(b.x, a.y),
                    ImVec2(a.x, (a.y + b.y) * 0.5f), ImVec2(b.x, (a.y + b.y) * 0.5f),
                    ImVec2(a.x, b.y), ImVec2((a.x + b.x) * 0.5f, b.y), b
                };
                for (const ImVec2& point : points)
                {
                    dl->AddRectFilled(ImVec2(point.x - hs, point.y - hs),
                                      ImVec2(point.x + hs, point.y + hs), handleFill);
                    dl->AddRect(ImVec2(point.x - hs, point.y - hs),
                                ImVec2(point.x + hs, point.y + hs), selection);
                }

                if (selected->tipo == "elipse" || selected->tipo == "poligono" || rotated)
                    continue;

                // Cada circulo controla somente a quina onde aparece.
                const CornerRadii projectRadii = GetCornerRadii(*selected, w, h);
                const float minMarkerInset = 14.0f;
                const float maxMarkerInset = std::max(6.0f,
                    std::min((b.x - a.x) * 0.5f, (b.y - a.y) * 0.5f) - 6.0f);
                auto markerInset = [&](float radius)
                {
                    return std::min(maxMarkerInset,
                                    std::max(minMarkerInset, radius * viewScale));
                };
                const float tl = markerInset(projectRadii.topLeft);
                const float tr = markerInset(projectRadii.topRight);
                const float br = markerInset(projectRadii.bottomRight);
                const float bl = markerInset(projectRadii.bottomLeft);
                const ImVec2 cornerPoints[] = {
                    ImVec2(a.x + tl, a.y + tl),
                    ImVec2(b.x - tr, a.y + tr),
                    ImVec2(b.x - br, b.y - br),
                    ImVec2(a.x + bl, b.y - bl),
                };
                const ImU32 cornerFill = IM_COL32(30, 30, 30, 255);
                const ImU32 selectedCornerFill = IM_COL32(245, 158, 11, 255);
                for (int index = 0; index < 4; ++index)
                {
                    const bool cornerSelected = (quinasSelecionadas & (1u << index)) != 0;
                    dl->AddCircleFilled(cornerPoints[index], 5.0f,
                                        cornerSelected ? selectedCornerFill : cornerFill, 16);
                    dl->AddCircle(cornerPoints[index], 5.0f,
                                  cornerSelected ? IM_COL32(255, 255, 255, 255) : selection,
                                  16, cornerSelected ? 2.0f : 1.5f);
                }

                // Alça de rotação: fica acima da borda superior do elemento,
                // na direção da normal transformada pela rotação.
                {
                    float px = 0.0f, py = 0.0f;
                    Geo::ElementPivot(*selected, px, py);
                    const float eh2 = h * 0.5f;
                    const float rotRad = Geo::DegToRad(Geo::ElementRotation(*selected));
                    const float cosR = cosf(rotRad);
                    const float sinR = sinf(rotRad);
                    // Ponto do topo central no espaço local (0,-eh2) -> projeto.
                    const float topX = px + eh2 * sinR;
                    const float topY = py - eh2 * cosR;
                    const float dirX = sinR;
                    const float dirY = -cosR;
                    const float sticker = 18.0f / std::max(0.25f, viewScale);
                    const float handleScreenX = origin.x + (topX + dirX * sticker) * viewScale;
                    const float handleScreenY = origin.y + (topY + dirY * sticker) * viewScale;
                    const float topScreenX = origin.x + topX * viewScale;
                    const float topScreenY = origin.y + topY * viewScale;
                    dl->AddLine(ImVec2(topScreenX, topScreenY),
                                ImVec2(handleScreenX, handleScreenY), selection, 1.5f);
                    const float hr = 5.0f;
                    dl->AddCircleFilled(ImVec2(handleScreenX, handleScreenY), hr, selection, 16);
                    dl->AddCircle(ImVec2(handleScreenX, handleScreenY), hr,
                                  IM_COL32(255, 255, 255, 210), 16, 1.5f);
                }

                // Ponto de ORIGEM (pivô): círculo pequeno no centro da forma
                // (ou onde o usuário o arrastou — centro_rotacao). É o centro
                // do redimensionamento espelhado (Shift) e da rotação.
                {
                    float px = 0.0f, py = 0.0f;
                    Geo::ElementPivot(*selected, px, py);
                    const float pivotScreenX = origin.x + px * viewScale;
                    const float pivotScreenY = origin.y + py * viewScale;
                    const ImU32 pivotCol = ImGui::ColorConvertFloat4ToU32(
                        Theme::Hex(0xffb347, 1.0f));
                    const float pr = 3.5f;
                    // Cruz fina + círculo: "mira" do ponto de origem.
                    dl->AddLine(ImVec2(pivotScreenX - 6.0f, pivotScreenY),
                                ImVec2(pivotScreenX + 6.0f, pivotScreenY),
                                pivotCol, 1.0f);
                    dl->AddLine(ImVec2(pivotScreenX, pivotScreenY - 6.0f),
                                ImVec2(pivotScreenX, pivotScreenY + 6.0f),
                                pivotCol, 1.0f);
                    dl->AddCircleFilled(ImVec2(pivotScreenX, pivotScreenY), pr,
                                        IM_COL32(20, 20, 20, 255), 16);
                    dl->AddCircle(ImVec2(pivotScreenX, pivotScreenY), pr,
                                  pivotCol, 16, 1.5f);
                }
            }
            }
        }
    }
}
