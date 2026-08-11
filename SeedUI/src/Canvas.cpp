#include "Canvas.h"

#include "Theme.h"
#include "imgui.h"

#include <cstdio>

namespace seedui
{
    namespace
    {
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

            const ImU32 fill = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x2b2b2b));
            const ImU32 outline = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x4f8cff, 0.7f));
            const ImU32 label = ImGui::ColorConvertFloat4ToU32(Theme::TextPrimary);

            dl->AddRectFilled(a, b, fill);
            dl->AddRect(a, b, outline, 2.0f, 0, 1.5f);

            const char* text = e.nome.empty() ? e.id.c_str() : e.nome.c_str();
            dl->AddText(ImVec2(a.x + 4, a.y + 4), label, text);

            for (const Element& f : e.filhos)
                DrawElement(f, origin, scale, dl);
        }
    }

    void CanvasDraw(const Project* projeto, int telaAtiva, int modoAtivo)
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
        const ImU32 rulerBg = ImGui::ColorConvertFloat4ToU32(ImVec4(0.06f, 0.06f, 0.06f, 0.92f));
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
        const Tela& tela = projeto->telas[telaAtiva];
        if (!tela.modos.empty())
        {
            const Modo& modo = tela.modos[modoAtivo];
            for (const Element& e : modo.raiz)
                DrawElement(e, origin, viewScale, dl);
        }
    }
}
