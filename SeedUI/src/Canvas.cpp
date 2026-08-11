#include "Canvas.h"

#include "Theme.h"
#include "imgui.h"

#include <cstdio>

namespace seedui
{
    void CanvasDraw()
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 min = ImGui::GetWindowPos();
        const ImVec2 max(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());

        const ImU32 bg = ImGui::ColorConvertFloat4ToU32(Theme::CanvasBackground);
        const ImU32 gridMin = ImGui::ColorConvertFloat4ToU32(Theme::GridMinor);
        const ImU32 gridMaj = ImGui::ColorConvertFloat4ToU32(Theme::GridMajor);
        const ImU32 border = ImGui::ColorConvertFloat4ToU32(Theme::Border);
        const ImU32 textSec = ImGui::ColorConvertFloat4ToU32(Theme::TextSecondary);
        const ImU32 accent = ImGui::ColorConvertFloat4ToU32(Theme::AccentBlue);

        // Fundo
        dl->AddRectFilled(min, max, bg);

        // Grade (40px, linhas mais claras a cada 200px)
        const float step = 40.0f;
        const int majorEvery = 5;
        for (float x = min.x; x <= max.x; x += step)
        {
            const bool major = ((int)((x - min.x) / step + 0.5f) % majorEvery) == 0;
            dl->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), major ? gridMaj : gridMin, 1.0f);
        }
        for (float y = min.y; y <= max.y; y += step)
        {
            const bool major = ((int)((y - min.y) / step + 0.5f) % majorEvery) == 0;
            dl->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), major ? gridMaj : gridMin, 1.0f);
        }

        // Réguas
        const float ruler = 24.0f;
        const ImU32 rulerBg = ImGui::ColorConvertFloat4ToU32(ImVec4(0.13f, 0.13f, 0.13f, 0.88f));
        dl->AddRectFilled(min, ImVec2(max.x, min.y + ruler), rulerBg);
        dl->AddRectFilled(min, ImVec2(min.x + ruler, max.y), rulerBg);

        for (float x = min.x + ruler; x <= max.x; x += step)
        {
            const bool major = ((int)((x - min.x - ruler) / step + 0.5f) % majorEvery) == 0;
            const float h = major ? 12.0f : 6.0f;
            dl->AddLine(ImVec2(x, min.y), ImVec2(x, min.y + h), major ? textSec : border, 1.0f);
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
            dl->AddLine(ImVec2(min.x, y), ImVec2(min.x + w, y), major ? textSec : border, 1.0f);
        }

        // Moldura da tela base 1280x720
        const ImVec2 origin(min.x + 48, min.y + 40);
        const ImVec2 frame(1280.0f, 720.0f);
        dl->AddRect(origin, ImVec2(origin.x + frame.x, origin.y + frame.y), accent, 0.0f, 0, 1.5f);
        dl->AddText(ImVec2(origin.x + 8, origin.y - 18), accent, "1280 x 720 — tela base");

        // Dica central
        const char* hint = "Área de projeto — arraste componentes da Biblioteca para começar (M04)";
        const ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, hint);
        const ImVec2 pos((min.x + max.x - ts.x) * 0.5f, (min.y + max.y) * 0.5f + 140.0f);
        dl->AddText(ImGui::GetFont(), 15.0f, pos, textSec, hint);
    }
}
