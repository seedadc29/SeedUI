#include "Canvas.h"

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
                             const CornerRadii& radii, ImU32 fill, ImU32 outline)
        {
            RoundedRectPath(dl, a, b, radii);
            dl->PathFillConvex(fill);
            RoundedRectPath(dl, a, b, radii);
            dl->PathStroke(outline, ImDrawFlags_Closed, 1.0f);
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
            const ImU32 fill = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x2b2b2b, opacity));
            // Elementos não selecionados usam uma borda neutra e discreta.
            // Azul/laranja ficam reservados exclusivamente para a seleção.
            const ImU32 outline = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x5a5a5a, 0.82f));
            const ImU32 label = ImGui::ColorConvertFloat4ToU32(Theme::TextPrimary);

            if (e.tipo != "grupo")
            {
                if (e.tipo == "elipse")
                {
                    const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
                    const ImVec2 radius((b.x - a.x) * 0.5f, (b.y - a.y) * 0.5f);
                    dl->AddEllipseFilled(center, radius, fill, 0.0f, 48);
                    dl->AddEllipse(center, radius, outline, 0.0f, 48, 1.0f);
                }
                else if (e.tipo == "poligono")
                {
                    const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
                    const float radius = std::min(b.x - a.x, b.y - a.y) * 0.5f;
                    dl->AddNgonFilled(center, radius, fill, 6);
                    dl->AddNgon(center, radius, outline, 6, 1.0f);
                }
                else
                {
                    CornerRadii radii = GetCornerRadii(e, w, h);
                    radii.topLeft *= scale;
                    radii.topRight *= scale;
                    radii.bottomRight *= scale;
                    radii.bottomLeft *= scale;
                    DrawRoundedRect(dl, a, b, radii, fill, outline);
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
                               bool limitarNaMoldura, float zoom)
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
        scale *= std::max(0.25f, std::min(4.0f, zoom));
        const ImVec2 frame(baseW * scale, baseH * scale);
        const ImVec2 origin(contentMin.x + (availW - frame.x) * 0.5f,
                            contentMin.y + (availH - frame.y) * 0.5f);
        const bool inside = screenX >= origin.x && screenX <= origin.x + frame.x &&
                            screenY >= origin.y && screenY <= origin.y + frame.y;
        if (!inside && !limitarNaMoldura) return false;

        projectX = std::max(0.0f, std::min(baseW, (screenX - origin.x) / scale));
        projectY = std::max(0.0f, std::min(baseH, (screenY - origin.y) / scale));
        return true;
    }

    bool CanvasProjectToScreen(const Project* projeto, float projectX, float projectY,
                               float& screenX, float& screenY, float& scale, float zoom)
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
        scale *= std::max(0.25f, std::min(4.0f, zoom));
        const ImVec2 frame(baseW * scale, baseH * scale);
        const ImVec2 origin(contentMin.x + (availW - frame.x) * 0.5f,
                            contentMin.y + (availH - frame.y) * 0.5f);
        screenX = origin.x + projectX * scale;
        screenY = origin.y + projectY * scale;
        return true;
    }

    void CanvasDraw(const Project* projeto, int telaAtiva, int modoAtivo,
                    const std::vector<std::string>* elementosSelecionados,
                    const char* elementoPrincipalId,
                    unsigned int quinasSelecionadas,
                    bool exibirReguas, float zoom)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 min = ImGui::GetWindowPos();
        const ImVec2 max(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());

        const ImU32 bg = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x050505));
        const ImU32 gridMin = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x8c8c8c, 0.55f));
        const ImU32 gridMaj = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0xd8d8d8, 0.78f));
        const ImU32 border = ImGui::ColorConvertFloat4ToU32(Theme::Border);
        const ImU32 textSec = ImGui::ColorConvertFloat4ToU32(Theme::TextSecondary);
        const ImU32 accent = ImGui::ColorConvertFloat4ToU32(Theme::AccentBlue);

        // Fundo
        dl->AddRectFilled(min, max, bg);

        // Grade em pontos: escura, discreta e legível em telas pequenas.
        const float step = 24.0f;
        const int majorEvery = 5;
        for (float x = min.x + 24.0f; x <= max.x; x += step)
        {
            const bool major = ((int)((x - min.x) / step + 0.5f) % majorEvery) == 0;
            for (float y = min.y + 24.0f; y <= max.y; y += step)
            {
                const bool majorY = ((int)((y - min.y) / step + 0.5f) % majorEvery) == 0;
                const bool majorDot = major && majorY;
                dl->AddCircleFilled(ImVec2(x, y), majorDot ? 1.45f : 1.05f,
                                    majorDot ? gridMaj : gridMin, 8);
            }
        }

        // Réguas
        const float ruler = 24.0f;
        if (exibirReguas)
        {
            const ImU32 rulerBg = ImGui::ColorConvertFloat4ToU32(
                ImVec4(0.06f, 0.06f, 0.06f, 0.92f));
            dl->AddRectFilled(min, ImVec2(max.x, min.y + ruler), rulerBg);
            dl->AddRectFilled(min, ImVec2(min.x + ruler, max.y), rulerBg);

            for (float x = min.x + ruler; x <= max.x; x += step)
            {
                const bool major = ((int)((x - min.x - ruler) / step + 0.5f) % majorEvery) == 0;
                const float h = major ? 12.0f : 6.0f;
                dl->AddLine(ImVec2(x, min.y), ImVec2(x, min.y + h),
                            major ? textSec : border, 1.0f);
                if (major)
                {
                    char buf[32];
                    snprintf(buf, sizeof buf, "%.0f", x - min.x - ruler);
                    dl->AddText(ImVec2(x + 3, min.y + 2), textSec, buf);
                }
            }
            for (float y = min.y + ruler; y <= max.y; y += step)
            {
                const bool major = ((int)((y - min.y - ruler) / step + 0.5f) % majorEvery) == 0;
                const float w = major ? 12.0f : 6.0f;
                dl->AddLine(ImVec2(min.x, y), ImVec2(min.x + w, y),
                            major ? textSec : border, 1.0f);
            }
        }

        // Moldura da tela base
        const float baseW = projeto ? (float)projeto->telaBaseLargura : 1280.0f;
        const float baseH = projeto ? (float)projeto->telaBaseAltura : 720.0f;
        const float viewportPadding = 44.0f;
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
            viewScale *= std::max(0.25f, std::min(4.0f, zoom));
        }

        const ImVec2 frame(baseW * viewScale, baseH * viewScale);
        const ImVec2 origin(contentMin.x + (availW - frame.x) * 0.5f,
                            contentMin.y + (availH - frame.y) * 0.5f);
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

        if (elementosSelecionados)
        {
            for (const std::string& selectedId : *elementosSelecionados)
            {
                const Element* selected = FindElement(modo.raiz, selectedId);
                if (!selected || !selected->visivel) continue;
                const float x = selected->transformacao.value("x", 0.0f);
                const float y = selected->transformacao.value("y", 0.0f);
                const float w = selected->transformacao.value("largura", 160.0f);
                const float h = selected->transformacao.value("altura", 32.0f);
                const ImVec2 a(origin.x + x * viewScale, origin.y + y * viewScale);
                const ImVec2 b(origin.x + (x + w) * viewScale,
                               origin.y + (y + h) * viewScale);
                const bool primary = elementoPrincipalId && selectedId == elementoPrincipalId;
                const ImU32 selection = ImGui::ColorConvertFloat4ToU32(
                    primary ? Theme::AccentOrange : Theme::AccentBlue);
                CornerRadii selectionRadii = GetCornerRadii(*selected, w, h);
                selectionRadii.topLeft *= viewScale;
                selectionRadii.topRight *= viewScale;
                selectionRadii.bottomRight *= viewScale;
                selectionRadii.bottomLeft *= viewScale;
                RoundedRectPath(dl, a, b, selectionRadii);
                dl->PathStroke(selection, ImDrawFlags_Closed, primary ? 1.5f : 1.0f);
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

                if (selected->tipo == "elipse" || selected->tipo == "poligono")
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
            }
        }
    }
}
