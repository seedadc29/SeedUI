#include "ColorPicker.h"

#include "ColorUtils.h"
#include "Theme.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace seedui
{
    namespace ColorPicker
    {
        namespace
        {
            constexpr ImU32 kBorder = IM_COL32(70, 70, 70, 255);
            constexpr ImU32 kBorderHi = IM_COL32(140, 140, 140, 255);
            constexpr ImU32 kHandle = IM_COL32(255, 255, 255, 255);

            // Contorno arredondado (acabamento consistente em toda a ferramenta).
            void RoundedOutline(ImDrawList* dl, const ImVec2& min, const ImVec2& max,
                                ImU32 col, float radius = 5.0f)
            {
                dl->AddRect(min, max, col, radius, 0, 1.0f);
            }

            // Barra de matiz (hue) com as cores do círculo cromático.
            void DrawHueStrip(ImDrawList* dl, const ImVec2& min, const ImVec2& max,
                              bool vertical)
            {
                constexpr int kSegments = 64;
                for (int i = 0; i < kSegments; ++i)
                {
                    const float hue = 360.0f * i / kSegments;
                    float r = 0, g = 0, b = 0;
                    ColorUtils::HsvToRgb(hue, 1.0f, 1.0f, r, g, b);
                    const ImU32 col = ImGui::ColorConvertFloat4ToU32(
                        ImVec4(r, g, b, 1.0f));
                    ImVec2 a, bb;
                    if (vertical)
                    {
                        const float y0 = min.y + (max.y - min.y) * i / kSegments;
                        const float y1 = min.y + (max.y - min.y) * (i + 1) / kSegments;
                        a = ImVec2(min.x, y0);
                        bb = ImVec2(max.x, y1);
                    }
                    else
                    {
                        const float x0 = min.x + (max.x - min.x) * i / kSegments;
                        const float x1 = min.x + (max.x - min.x) * (i + 1) / kSegments;
                        a = ImVec2(x0, min.y);
                        bb = ImVec2(x1, max.y);
                    }
                    dl->AddRectFilled(a, bb, col);
                }
                RoundedOutline(dl, min, max, kBorder);
            }

            // Interação genérica de faixa: InvisibleButton + arrastar.
            bool Strip(const char* id, const ImVec2& min, const ImVec2& max,
                       bool vertical, float& ratio)
            {
                const ImVec2 size(max.x - min.x, max.y - min.y);
                ImGui::SetCursorScreenPos(min);
                ImGui::InvisibleButton(id, size);
                bool changed = false;
                if (ImGui::IsItemActive() || ImGui::IsItemHovered())
                {
                    const ImVec2 mouse = ImGui::GetMousePos();
                    float t = vertical
                        ? (mouse.y - min.y) / std::max(1.0f, size.y)
                        : (mouse.x - min.x) / std::max(1.0f, size.x);
                    if (ImGui::IsItemActive() || ImGui::IsMouseClicked(0))
                    {
                        t = std::max(0.0f, std::min(1.0f, t));
                        if (t != ratio)
                        {
                            ratio = t;
                            changed = true;
                        }
                    }
                }
                return changed;
            }

            // Ponteiro circular com contorno (usado em todas as faixas).
            void DrawHandle(ImDrawList* dl, const ImVec2& center, float radius)
            {
                dl->AddCircleFilled(center, radius, kHandle, 24);
                dl->AddCircle(center, radius, IM_COL32(20, 20, 20, 200), 24, 1.5f);
            }

            // Triângulo HSV: topo = branco, base-esquerda = preto,
            // base-direita = matiz pura. Renderizado em quads que seguem as
            // arestas reais do triângulo — sem serrilhado nas bordas.
            bool TriangleArea(const ImVec2& min, const ImVec2& max,
                              float hue, float& sat, float& val)
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const float cx = (min.x + max.x) * 0.5f;
                const ImVec2 top(cx, min.y);
                const ImVec2 left(min.x, max.y);
                const ImVec2 right(max.x, max.y);

                float hr = 0, hg = 0, hb = 0;
                ColorUtils::HsvToRgb(hue, 1.0f, 1.0f, hr, hg, hb);
                constexpr int kRows = 48;
                constexpr int kSegs = 8;
                for (int row = 0; row < kRows; ++row)
                {
                    const float tTop = (kRows - row) / (float)kRows;
                    const float tBot = (kRows - 1 - row) / (float)kRows;
                    const float y0 = min.y + (max.y - min.y) * row / kRows;
                    const float y1 = min.y + (max.y - min.y) * (row + 1) / kRows;
                    const float xL0 = min.x + (cx - min.x) * tTop;
                    const float xR0 = max.x - (max.x - cx) * tTop;
                    const float xL1 = min.x + (cx - min.x) * tBot;
                    const float xR1 = max.x - (max.x - cx) * tBot;
                    const float tm = (tTop + tBot) * 0.5f;
                    for (int s = 0; s < kSegs; ++s)
                    {
                        const float f0 = s / (float)kSegs;
                        const float f1 = (s + 1) / (float)kSegs;
                        const float fm = (f0 + f1) * 0.5f;
                        const float ax0 = xL0 + (xR0 - xL0) * f0;
                        const float ax1 = xL0 + (xR0 - xL0) * f1;
                        const float bx1 = xL1 + (xR1 - xL1) * f1;
                        const float bx0 = xL1 + (xR1 - xL1) * f0;
                        // Cor no centro do segmento: lerp(esquerda->direita, fm).
                        const float r = tm + ((hr + (1.0f - hr) * tm) - tm) * fm;
                        const float g = tm + ((hg + (1.0f - hg) * tm) - tm) * fm;
                        const float b = tm + ((hb + (1.0f - hb) * tm) - tm) * fm;
                        dl->AddQuadFilled(ImVec2(ax0, y0), ImVec2(ax1, y0),
                                          ImVec2(bx1, y1), ImVec2(bx0, y1),
                                          ImGui::ColorConvertFloat4ToU32(
                                              ImVec4(r, g, b, 1.0f)));
                    }
                }
                dl->AddTriangle(top, left, right, kBorderHi);

                ImGui::SetCursorScreenPos(min);
                ImGui::InvisibleButton("##tri", ImVec2(max.x - min.x, max.y - min.y));
                bool changed = false;
                if (ImGui::IsItemHovered() &&
                    (ImGui::IsItemActive() || ImGui::IsMouseClicked(0)))
                {
                    const ImVec2 mouse = ImGui::GetMousePos();
                    const ImVec2 v0(right.x - top.x, right.y - top.y);
                    const ImVec2 v1(left.x - top.x, left.y - top.y);
                    const ImVec2 v2(mouse.x - top.x, mouse.y - top.y);
                    const float d00 = v0.x * v0.x + v0.y * v0.y;
                    const float d01 = v0.x * v1.x + v0.y * v1.y;
                    const float d11 = v1.x * v1.x + v1.y * v1.y;
                    const float d20 = v2.x * v0.x + v2.y * v0.y;
                    const float d21 = v2.x * v1.x + v2.y * v1.y;
                    const float denom = d00 * d11 - d01 * d01;
                    if (fabsf(denom) > 0.0001f)
                    {
                        float wRight = (d11 * d20 - d01 * d21) / denom;
                        float wLeft = (d00 * d21 - d01 * d20) / denom;
                        float wTop = 1.0f - wRight - wLeft;
                        const float minW = std::min({ wTop, wLeft, wRight });
                        if (minW < 0.0f)
                        {
                            wTop -= minW;
                            wLeft -= minW;
                            wRight -= minW;
                        }
                        const float total = wTop + wLeft + wRight;
                        if (total > 0.0001f)
                        {
                            wTop /= total;
                            wLeft /= total;
                            wRight /= total;
                        }
                        const float newVal = wTop + wRight;
                        const float newSat = newVal > 0.0001f
                            ? std::max(0.0f, std::min(1.0f, wRight / newVal))
                            : 0.0f;
                        if (fabsf(newSat - sat) > 0.0001f ||
                            fabsf(newVal - val) > 0.0001f)
                        {
                            sat = newSat;
                            val = newVal;
                            changed = true;
                        }
                    }
                }
                return changed;
            }

            // Quadrado SV: X = saturação, Y = valor (topo = 1).
            bool SquareArea(const ImVec2& min, const ImVec2& max,
                            float hue, float& sat, float& val)
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                float hr = 0, hg = 0, hb = 0;
                ColorUtils::HsvToRgb(hue, 1.0f, 1.0f, hr, hg, hb);
                const ImU32 white = IM_COL32(255, 255, 255, 255);
                const ImU32 black = IM_COL32(0, 0, 0, 255);
                const ImU32 hueCol = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(hr, hg, hb, 1.0f));
                dl->AddRectFilledMultiColor(min, max, white, hueCol, hueCol, black);
                RoundedOutline(dl, min, max, kBorder);

                ImGui::SetCursorScreenPos(min);
                ImGui::InvisibleButton("##sq", ImVec2(max.x - min.x, max.y - min.y));
                bool changed = false;
                if (ImGui::IsItemHovered() &&
                    (ImGui::IsItemActive() || ImGui::IsMouseClicked(0)))
                {
                    const ImVec2 mouse = ImGui::GetMousePos();
                    const float w = std::max(1.0f, max.x - min.x);
                    const float h = std::max(1.0f, max.y - min.y);
                    const float s = std::max(0.0f, std::min(1.0f,
                        (mouse.x - min.x) / w));
                    const float v = std::max(0.0f, std::min(1.0f,
                        1.0f - (mouse.y - min.y) / h));
                    if (fabsf(s - sat) > 0.0001f || fabsf(v - val) > 0.0001f)
                    {
                        sat = s;
                        val = v;
                        changed = true;
                    }
                }
                return changed;
            }

            // Faixa de um canal (R/G/B) com gradiente e ponteiro.
            bool ChannelStrip(const char* id, const ImVec2& min, const ImVec2& max,
                              const ImVec4& c0, const ImVec4& c1, float& ratio)
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilledMultiColor(
                    min, max,
                    ImGui::ColorConvertFloat4ToU32(c0),
                    ImGui::ColorConvertFloat4ToU32(c1),
                    ImGui::ColorConvertFloat4ToU32(c1),
                    ImGui::ColorConvertFloat4ToU32(c0));
                RoundedOutline(dl, min, max, kBorder);
                DrawHandle(dl, ImVec2(min.x + (max.x - min.x) * ratio,
                                      (min.y + max.y) * 0.5f), 7.0f);
                return Strip(id, min, max, false, ratio);
            }
        }

        bool Widget(const char* label, float rgb[3])
        {
            ImGui::PushID(label);
            const float r0 = rgb[0], g0 = rgb[1], b0 = rgb[2];
            float h = 0.0f, s = 0.0f, v = 1.0f;
            ColorUtils::RgbToHsv(r0, g0, b0, h, s, v);

            bool changed = false;

            // Abas dos 3 modelos — segmento destacado no ativo (sem alternância
            // do estilo entre push/pop: usa o estado ANTES do clique).
            static int sMode = 0;
            const char* names[] = { "Triângulo", "Quadrado", "Barras" };
            for (int i = 0; i < 3; ++i)
            {
                const bool active = sMode == i;
                if (active)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button,
                                          Theme::Hex(0x4f8cff, 0.32f));
                    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextPrimary);
                }
                if (i > 0) ImGui::SameLine(0, 4);
                if (ImGui::Button(names[i], ImVec2(78, 26))) sMode = i;
                if (active)
                {
                    ImGui::PopStyleColor(2);
                }
            }

            const float areaW = 210.0f;
            const float areaH = 132.0f;
            const float hueW = 16.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 cursor = ImGui::GetCursorScreenPos();

            float previewY = cursor.y + areaH + 30.0f;
            if (sMode == 2)
            {
                // Barras: faixa de matiz horizontal + canais R/G/B.
                const ImVec2 hueMin(cursor.x, cursor.y);
                const ImVec2 hueMax(cursor.x + areaW, cursor.y + 16.0f);
                DrawHueStrip(dl, hueMin, hueMax, false);
                float hueRatio = h / 360.0f;
                if (Strip("##hue_bars", hueMin, hueMax, false, hueRatio))
                {
                    h = hueRatio * 360.0f;
                    changed = true;
                }
                DrawHandle(dl, ImVec2(hueMin.x + (hueMax.x - hueMin.x) * hueRatio,
                                      (hueMin.y + hueMax.y) * 0.5f), 6.0f);

                float ratioR = rgb[0], ratioG = rgb[1], ratioB = rgb[2];
                ImVec2 stripMin(cursor.x, cursor.y + 22.0f);
                ImVec2 stripMax(cursor.x + areaW, cursor.y + 36.0f);
                if (ChannelStrip("##r", stripMin, stripMax,
                                 ImVec4(0, 0, 0, 1), ImVec4(1, 0, 0, 1), ratioR))
                { rgb[0] = ratioR; changed = true; }
                stripMin = ImVec2(cursor.x, cursor.y + 42.0f);
                stripMax = ImVec2(cursor.x + areaW, cursor.y + 56.0f);
                if (ChannelStrip("##g", stripMin, stripMax,
                                 ImVec4(0, 0, 0, 1), ImVec4(0, 1, 0, 1), ratioG))
                { rgb[1] = ratioG; changed = true; }
                stripMin = ImVec2(cursor.x, cursor.y + 62.0f);
                stripMax = ImVec2(cursor.x + areaW, cursor.y + 76.0f);
                if (ChannelStrip("##b", stripMin, stripMax,
                                 ImVec4(0, 0, 0, 1), ImVec4(0, 0, 1, 1), ratioB))
                { rgb[2] = ratioB; changed = true; }
                previewY = cursor.y + 84.0f;
            }
            else
            {
                // Triângulo/Quadrado + barra de matiz vertical.
                const ImVec2 svMin(cursor.x, cursor.y);
                const ImVec2 svMax(cursor.x + areaW, cursor.y + areaH);
                const ImVec2 hueMin(cursor.x + areaW + 8.0f, cursor.y);
                const ImVec2 hueMax(cursor.x + areaW + 8.0f + hueW,
                                    cursor.y + areaH);
                DrawHueStrip(dl, hueMin, hueMax, true);
                float hueRatio = h / 360.0f;
                if (Strip("##hue_v", hueMin, hueMax, true, hueRatio))
                {
                    h = hueRatio * 360.0f;
                    changed = true;
                }
                DrawHandle(dl, ImVec2((hueMin.x + hueMax.x) * 0.5f,
                                      hueMin.y + (hueMax.y - hueMin.y) * hueRatio),
                           6.0f);

                if (sMode == 0)
                    changed = TriangleArea(svMin, svMax, h, s, v) || changed;
                else
                    changed = SquareArea(svMin, svMax, h, s, v) || changed;

                ColorUtils::HsvToRgb(h, s, v, rgb[0], rgb[1], rgb[2]);
            }

            // Prévia + hexadecimal.
            ImGui::SetCursorScreenPos(ImVec2(cursor.x, previewY));
            const ImVec4 preview(rgb[0], rgb[1], rgb[2], 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, preview);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, preview);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, preview);
            ImGui::Button("##preview", ImVec2(30.0f, 22.0f));
            ImGui::PopStyleColor(3);
            ImGui::SameLine(0, 8);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", ColorUtils::ToHex(rgb[0], rgb[1], rgb[2]).c_str());
            ImGui::SameLine(0, 12);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(Theme::TextSecondary, "Preenchimento");

            ImGui::PopID();
            return changed;
        }
    }
}
