#include "ColorPicker.h"

#include "ColorUtils.h"
#include "Theme.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace seedui
{
    namespace ColorPicker
    {
        namespace
        {
            constexpr ImU32 kBgCard = IM_COL32(20, 20, 24, 255);
            constexpr ImU32 kBorderCard = IM_COL32(52, 52, 60, 255);
            constexpr ImU32 kBorderCardHi = IM_COL32(90, 90, 105, 255);

            enum HarmonyType
            {
                Harmony_Triad = 0,
                Harmony_Complementary,
                Harmony_Analogous,
                Harmony_Monochromatic,
                Harmony_Square,
                Harmony_Split,
                Harmony_COUNT
            };

            const char* kHarmonyNames[] = {
                "Tríade", "Complementar", "Análoga", "Monocromática", "Quadrado", "Dividida"
            };

            struct HarmonyColor
            {
                float h, s, v;
                float r, g, b;
                std::string hex;
                ImU32 colU32;
            };

            void CalculateHarmonies(HarmonyType type, float baseH, float baseS, float baseV, std::vector<HarmonyColor>& out)
            {
                out.resize(5);
                struct HSV { float h, s, v; };
                HSV raw[5];

                auto normH = [](float h) { return fmodf(fmodf(h, 360.0f) + 360.0f, 360.0f); };
                auto clamp01 = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };

                switch (type)
                {
                    case Harmony_Triad:
                        raw[0] = { normH(baseH), clamp01(baseS), clamp01(baseV) };
                        raw[1] = { normH(baseH + 120.0f), clamp01(baseS * 0.9f), clamp01(baseV * 0.75f) };
                        raw[2] = { normH(baseH + 240.0f), clamp01(baseS * 0.95f), clamp01(baseV * 0.85f) };
                        raw[3] = { normH(baseH + 120.0f), clamp01(baseS * 0.65f), clamp01(baseV * 0.95f) };
                        raw[4] = { normH(baseH + 240.0f), clamp01(baseS * 0.45f), clamp01(baseV * 0.92f) };
                        break;
                    case Harmony_Complementary:
                        raw[0] = { normH(baseH), clamp01(baseS), clamp01(baseV) };
                        raw[1] = { normH(baseH + 180.0f), clamp01(baseS), clamp01(baseV) };
                        raw[2] = { normH(baseH), clamp01(baseS * 0.55f), clamp01(baseV * 1.1f) };
                        raw[3] = { normH(baseH + 180.0f), clamp01(baseS * 0.55f), clamp01(baseV * 1.1f) };
                        raw[4] = { normH(baseH + 180.0f), clamp01(baseS * 0.30f), clamp01(baseV * 0.7f) };
                        break;
                    case Harmony_Analogous:
                        raw[0] = { normH(baseH - 30.0f), clamp01(baseS), clamp01(baseV) };
                        raw[1] = { normH(baseH - 15.0f), clamp01(baseS), clamp01(baseV) };
                        raw[2] = { normH(baseH), clamp01(baseS), clamp01(baseV) };
                        raw[3] = { normH(baseH + 15.0f), clamp01(baseS), clamp01(baseV) };
                        raw[4] = { normH(baseH + 30.0f), clamp01(baseS), clamp01(baseV) };
                        break;
                    case Harmony_Monochromatic:
                        raw[0] = { normH(baseH), clamp01(baseS * 0.25f), clamp01(baseV * 1.25f) };
                        raw[1] = { normH(baseH), clamp01(baseS * 0.55f), clamp01(baseV * 1.10f) };
                        raw[2] = { normH(baseH), clamp01(baseS), clamp01(baseV) };
                        raw[3] = { normH(baseH), clamp01(baseS), clamp01(baseV * 0.70f) };
                        raw[4] = { normH(baseH), clamp01(baseS), clamp01(baseV * 0.40f) };
                        break;
                    case Harmony_Square:
                        raw[0] = { normH(baseH), clamp01(baseS), clamp01(baseV) };
                        raw[1] = { normH(baseH + 90.0f), clamp01(baseS), clamp01(baseV) };
                        raw[2] = { normH(baseH + 180.0f), clamp01(baseS), clamp01(baseV) };
                        raw[3] = { normH(baseH + 270.0f), clamp01(baseS), clamp01(baseV) };
                        raw[4] = { normH(baseH + 90.0f), clamp01(baseS * 0.5f), clamp01(baseV * 0.8f) };
                        break;
                    case Harmony_Split:
                        raw[0] = { normH(baseH), clamp01(baseS), clamp01(baseV) };
                        raw[1] = { normH(baseH + 150.0f), clamp01(baseS), clamp01(baseV) };
                        raw[2] = { normH(baseH + 210.0f), clamp01(baseS), clamp01(baseV) };
                        raw[3] = { normH(baseH + 150.0f), clamp01(baseS * 0.6f), clamp01(baseV * 0.9f) };
                        raw[4] = { normH(baseH + 210.0f), clamp01(baseS * 0.6f), clamp01(baseV * 0.9f) };
                        break;
                    default:
                        break;
                }

                for (int i = 0; i < 5; ++i)
                {
                    out[i].h = raw[i].h;
                    out[i].s = raw[i].s;
                    out[i].v = raw[i].v;
                    ColorUtils::HsvToRgb(raw[i].h, raw[i].s, raw[i].v, out[i].r, out[i].g, out[i].b);
                    out[i].hex = ColorUtils::ToHex(out[i].r, out[i].g, out[i].b);
                    out[i].colU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(out[i].r, out[i].g, out[i].b, 1.0f));
                }
            }

            void DrawReticleHandle(ImDrawList* dl, const ImVec2& center, ImU32 currentColor, float radius = 6.5f, bool isBase = true)
            {
                dl->AddCircleFilled(center, radius + 2.0f, IM_COL32(0, 0, 0, 130), 20);
                dl->AddCircle(center, radius, IM_COL32(255, 255, 255, 255), 20, isBase ? 2.0f : 1.6f);
                dl->AddCircle(center, radius - 1.8f, IM_COL32(15, 15, 18, 230), 20, 1.0f);
                dl->AddCircleFilled(center, std::max(1.5f, radius - 3.2f), currentColor, 14);
                if (isBase)
                {
                    dl->AddCircle(center, radius + 3.2f, IM_COL32(255, 255, 255, 180), 20, 1.0f);
                }
            }

            void DrawSliderHandle(ImDrawList* dl, const ImVec2& center, bool vertical, float length = 14.0f, float thickness = 5.0f)
            {
                ImVec2 a, b;
                if (vertical)
                {
                    a = ImVec2(center.x - length * 0.5f, center.y - thickness * 0.5f);
                    b = ImVec2(center.x + length * 0.5f, center.y + thickness * 0.5f);
                }
                else
                {
                    a = ImVec2(center.x - thickness * 0.5f, center.y - length * 0.5f);
                    b = ImVec2(center.x + thickness * 0.5f, center.y + length * 0.5f);
                }
                dl->AddRectFilled(ImVec2(a.x - 1, a.y - 1), ImVec2(b.x + 1, b.y + 1), IM_COL32(0, 0, 0, 90), 2.5f);
                dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 255), 2.0f);
                dl->AddRect(a, b, IM_COL32(30, 30, 35, 220), 2.0f, 0, 1.0f);
            }

            void DrawHueStrip(ImDrawList* dl, const ImVec2& min, const ImVec2& max, bool vertical)
            {
                dl->AddRectFilled(min, max, IM_COL32(18, 18, 22, 255), 4.0f);
                constexpr int kSegments = 64;
                for (int i = 0; i < kSegments; ++i)
                {
                    const float h0 = 360.0f * i / (float)kSegments;
                    const float h1 = 360.0f * (i + 1) / (float)kSegments;
                    float r0 = 0, g0 = 0, b0 = 0, r1 = 0, g1 = 0, b1 = 0;
                    ColorUtils::HsvToRgb(h0, 1.0f, 1.0f, r0, g0, b0);
                    ColorUtils::HsvToRgb(h1, 1.0f, 1.0f, r1, g1, b1);
                    const ImU32 col0 = ImGui::ColorConvertFloat4ToU32(ImVec4(r0, g0, b0, 1.0f));
                    const ImU32 col1 = ImGui::ColorConvertFloat4ToU32(ImVec4(r1, g1, b1, 1.0f));

                    if (vertical)
                    {
                        const float y0 = min.y + (max.y - min.y) * i / (float)kSegments;
                        const float y1 = min.y + (max.y - min.y) * (i + 1) / (float)kSegments;
                        dl->AddRectFilledMultiColor(ImVec2(min.x, y0), ImVec2(max.x, y1),
                                                    col0, col0, col1, col1);
                    }
                    else
                    {
                        const float x0 = min.x + (max.x - min.x) * i / (float)kSegments;
                        const float x1 = min.x + (max.x - min.x) * (i + 1) / (float)kSegments;
                        dl->AddRectFilledMultiColor(ImVec2(x0, min.y), ImVec2(x1, max.y),
                                                    col0, col1, col1, col0);
                    }
                }
                dl->AddRect(min, max, kBorderCard, 4.0f, 0, 1.0f);
            }

            bool Strip(const char* id, const ImVec2& min, const ImVec2& max, bool vertical, float& ratio)
            {
                const ImVec2 size(max.x - min.x, max.y - min.y);
                ImGui::SetCursorScreenPos(min);
                ImGui::InvisibleButton(id, size);
                bool changed = false;
                if (ImGui::IsItemActive() || (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)))
                {
                    const ImVec2 mouse = ImGui::GetMousePos();
                    float t = vertical
                        ? (mouse.y - min.y) / std::max(1.0f, size.y)
                        : (mouse.x - min.x) / std::max(1.0f, size.x);
                    t = std::max(0.0f, std::min(1.0f, t));
                    if (fabsf(t - ratio) > 0.0001f)
                    {
                        ratio = t;
                        changed = true;
                    }
                }
                return changed;
            }

            void DrawQuadMultiColor(ImDrawList* dl, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4,
                                    ImU32 col1, ImU32 col2, ImU32 col3, ImU32 col4)
            {
                const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
                dl->PrimReserve(6, 4);
                dl->PrimWriteIdx((ImDrawIdx)(dl->_VtxCurrentIdx));
                dl->PrimWriteIdx((ImDrawIdx)(dl->_VtxCurrentIdx + 1));
                dl->PrimWriteIdx((ImDrawIdx)(dl->_VtxCurrentIdx + 2));
                dl->PrimWriteIdx((ImDrawIdx)(dl->_VtxCurrentIdx));
                dl->PrimWriteIdx((ImDrawIdx)(dl->_VtxCurrentIdx + 2));
                dl->PrimWriteIdx((ImDrawIdx)(dl->_VtxCurrentIdx + 3));
                dl->PrimWriteVtx(p1, uv, col1);
                dl->PrimWriteVtx(p2, uv, col2);
                dl->PrimWriteVtx(p3, uv, col3);
                dl->PrimWriteVtx(p4, uv, col4);
            }

            // Círculo Cromático Radial
            bool ColorWheelArea(const ImVec2& min, const ImVec2& max, float& hue, float& sat, float val,
                                HarmonyType harmony, std::vector<HarmonyColor>& harmonyColors)
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();

                const float width = max.x - min.x;
                const float height = max.y - min.y;
                const float radius = std::min(width, height) * 0.5f - 2.0f;
                const ImVec2 center(min.x + width * 0.5f, min.y + height * 0.5f);

                dl->AddRectFilled(min, max, kBgCard, 6.0f);
                dl->AddRect(min, max, kBorderCard, 6.0f, 0, 1.0f);

                constexpr int kSectors = 64;
                constexpr int kRings = 6;
                for (int ring = 0; ring < kRings; ++ring)
                {
                    const float r0 = radius * (float)ring / (float)kRings;
                    const float r1 = radius * (float)(ring + 1) / (float)kRings;
                    const float s0 = (float)ring / (float)kRings;
                    const float s1 = (float)(ring + 1) / (float)kRings;

                    for (int s = 0; s < kSectors; ++s)
                    {
                        const float a0 = 6.2831853f * (float)s / (float)kSectors;
                        const float a1 = 6.2831853f * (float)(s + 1) / (float)kSectors;
                        const float h0 = 360.0f * (float)s / (float)kSectors;
                        const float h1 = 360.0f * (float)(s + 1) / (float)kSectors;

                        float red00, green00, blue00, red10, green10, blue10;
                        float red01, green01, blue01, red11, green11, blue11;

                        ColorUtils::HsvToRgb(h0, s0, val, red00, green00, blue00);
                        ColorUtils::HsvToRgb(h1, s0, val, red10, green10, blue10);
                        ColorUtils::HsvToRgb(h0, s1, val, red01, green01, blue01);
                        ColorUtils::HsvToRgb(h1, s1, val, red11, green11, blue11);

                        const ImVec2 p00(center.x + r0 * cosf(a0), center.y + r0 * sinf(a0));
                        const ImVec2 p10(center.x + r0 * cosf(a1), center.y + r0 * sinf(a1));
                        const ImVec2 p01(center.x + r1 * cosf(a0), center.y + r1 * sinf(a0));
                        const ImVec2 p11(center.x + r1 * cosf(a1), center.y + r1 * sinf(a1));

                        const ImU32 c00 = ImGui::ColorConvertFloat4ToU32(ImVec4(red00, green00, blue00, 1.0f));
                        const ImU32 c10 = ImGui::ColorConvertFloat4ToU32(ImVec4(red10, green10, blue10, 1.0f));
                        const ImU32 c01 = ImGui::ColorConvertFloat4ToU32(ImVec4(red01, green01, blue01, 1.0f));
                        const ImU32 c11 = ImGui::ColorConvertFloat4ToU32(ImVec4(red11, green11, blue11, 1.0f));

                        DrawQuadMultiColor(dl, p00, p10, p11, p01, c00, c10, c11, c01);
                    }
                }

                dl->AddCircle(center, radius, kBorderCardHi, 64, 1.2f);

                CalculateHarmonies(harmony, hue, sat, val, harmonyColors);

                std::vector<ImVec2> nodeScreenPos(5);
                for (size_t i = 0; i < harmonyColors.size(); ++i)
                {
                    const float radH = harmonyColors[i].h * (3.14159265f / 180.0f);
                    const float rDist = radius * harmonyColors[i].s;
                    nodeScreenPos[i] = ImVec2(center.x + rDist * cosf(radH),
                                              center.y + rDist * sinf(radH));
                }

                const ImU32 linkLineCol = IM_COL32(255, 255, 255, 170);
                for (size_t i = 0; i < nodeScreenPos.size(); ++i)
                {
                    dl->AddLine(center, nodeScreenPos[i], linkLineCol, 1.2f);
                    if (i > 0 && (harmony == Harmony_Triad || harmony == Harmony_Square || harmony == Harmony_Split))
                    {
                        dl->AddLine(nodeScreenPos[0], nodeScreenPos[i], IM_COL32(255, 255, 255, 90), 1.0f);
                    }
                }

                for (int i = (int)nodeScreenPos.size() - 1; i >= 0; --i)
                {
                    const bool isBase = (i == 0);
                    DrawReticleHandle(dl, nodeScreenPos[i], harmonyColors[i].colU32, isBase ? 7.5f : 5.5f, isBase);
                }

                ImGui::SetCursorScreenPos(min);
                ImGui::InvisibleButton("##wheel", ImVec2(width, height));
                bool changed = false;
                // Só reposiciona o nó (e a família harmônica) com ARRASTO
                // deliberado — um clique simples mantém a harmonia estática,
                // sem "pular" a tríade/complementar/analoga para a cor clicada.
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 4.0f))
                {
                    const ImVec2 mouse = ImGui::GetMousePos();
                    const float dx = mouse.x - center.x;
                    const float dy = mouse.y - center.y;
                    const float dist = sqrtf(dx * dx + dy * dy);

                    const float newSat = std::max(0.0f, std::min(1.0f, dist / radius));
                    float angleDeg = atan2f(dy, dx) * (180.0f / 3.14159265f);
                    if (angleDeg < 0.0f) angleDeg += 360.0f;
                    const float newHue = fmodf(angleDeg, 360.0f);

                    if (fabsf(newHue - hue) > 0.01f || fabsf(newSat - sat) > 0.001f)
                    {
                        hue = newHue;
                        sat = newSat;
                        CalculateHarmonies(harmony, hue, sat, val, harmonyColors);
                        changed = true;
                    }
                }

                return changed;
            }

            // Triângulo HSV com gradiente ultra-suave
            bool TriangleArea(const ImVec2& min, const ImVec2& max, float hue, float& sat, float& val, ImU32 curCol)
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(min, max, kBgCard, 6.0f);
                dl->AddRect(min, max, kBorderCard, 6.0f, 0, 1.0f);

                const float pad = 6.0f;
                const float triMinX = min.x + pad;
                const float triMaxX = max.x - pad;
                const float triMinY = min.y + pad;
                const float triMaxY = max.y - pad;

                const float cx = (triMinX + triMaxX) * 0.5f;
                const ImVec2 top(cx, triMinY);
                const ImVec2 left(triMinX, triMaxY);
                const ImVec2 right(triMaxX, triMaxY);

                float hr = 0, hg = 0, hb = 0;
                ColorUtils::HsvToRgb(hue, 1.0f, 1.0f, hr, hg, hb);

                auto evalTriColor = [&](float tm, float f) -> ImU32
                {
                    const float r = tm * (1.0f + f * (hr - 1.0f));
                    const float g = tm * (1.0f + f * (hg - 1.0f));
                    const float b = tm * (1.0f + f * (hb - 1.0f));
                    return ImGui::ColorConvertFloat4ToU32(ImVec4(std::max(0.0f, std::min(1.0f, r)),
                                                                std::max(0.0f, std::min(1.0f, g)),
                                                                std::max(0.0f, std::min(1.0f, b)), 1.0f));
                };

                constexpr int kRows = 32;
                constexpr int kSegs = 18;
                for (int row = 0; row < kRows; ++row)
                {
                    const float tTop = (float)(kRows - row) / (float)kRows;
                    const float tBot = (float)(kRows - 1 - row) / (float)kRows;
                    const float y0 = triMinY + (triMaxY - triMinY) * row / (float)kRows;
                    const float y1 = triMinY + (triMaxY - triMinY) * (row + 1) / (float)kRows;

                    const float xL0 = triMinX + (cx - triMinX) * tTop;
                    const float xR0 = triMaxX - (triMaxX - cx) * tTop;
                    const float xL1 = triMinX + (cx - triMinX) * tBot;
                    const float xR1 = triMaxX - (triMaxX - cx) * tBot;

                    const float tm0 = 1.0f - (float)row / (float)kRows;
                    const float tm1 = 1.0f - (float)(row + 1) / (float)kRows;

                    for (int s = 0; s < kSegs; ++s)
                    {
                        const float f0 = (float)s / (float)kSegs;
                        const float f1 = (float)(s + 1) / (float)kSegs;

                        const float ax0 = xL0 + (xR0 - xL0) * f0;
                        const float ax1 = xL0 + (xR0 - xL0) * f1;
                        const float bx1 = xL1 + (xR1 - xL1) * f1;
                        const float bx0 = xL1 + (xR1 - xL1) * f0;

                        const ImU32 c_top_l = evalTriColor(tm0, f0);
                        const ImU32 c_top_r = evalTriColor(tm0, f1);
                        const ImU32 c_bot_r = evalTriColor(tm1, f1);
                        const ImU32 c_bot_l = evalTriColor(tm1, f0);

                        DrawQuadMultiColor(dl, ImVec2(ax0, y0), ImVec2(ax1, y0),
                                           ImVec2(bx1, y1), ImVec2(bx0, y1),
                                           c_top_l, c_top_r, c_bot_r, c_bot_l);
                    }
                }

                dl->AddTriangle(top, left, right, kBorderCardHi, 1.2f);

                const float wPure = sat * val;
                const float wWhite = (1.0f - sat) * val;
                const float wBlack = 1.0f - val;
                const ImVec2 handlePos(
                    wWhite * top.x + wBlack * left.x + wPure * right.x,
                    wWhite * top.y + wBlack * left.y + wPure * right.y
                );
                DrawReticleHandle(dl, handlePos, curCol, 7.0f, true);

                ImGui::SetCursorScreenPos(min);
                ImGui::InvisibleButton("##tri", ImVec2(max.x - min.x, max.y - min.y));
                bool changed = false;
                if (ImGui::IsItemActive() || (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)))
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
                        float wR = (d11 * d20 - d01 * d21) / denom;
                        float wL = (d00 * d21 - d01 * d20) / denom;
                        float wT = 1.0f - wR - wL;

                        wT = std::max(0.0f, std::min(1.0f, wT));
                        wL = std::max(0.0f, std::min(1.0f, wL));
                        wR = std::max(0.0f, std::min(1.0f, wR));
                        const float sum = wT + wL + wR;
                        if (sum > 0.0001f)
                        {
                            wT /= sum; wL /= sum; wR /= sum;
                        }

                        const float newVal = std::max(0.0f, std::min(1.0f, wT + wR));
                        const float newSat = (newVal > 0.001f)
                            ? std::max(0.0f, std::min(1.0f, wR / newVal))
                            : 0.0f;

                        if (fabsf(newSat - sat) > 0.001f || fabsf(newVal - val) > 0.001f)
                        {
                            sat = newSat;
                            val = newVal;
                            changed = true;
                        }
                    }
                }
                return changed;
            }

            // Quadrado Sat/Val
            bool SquareArea(const ImVec2& min, const ImVec2& max, float hue, float& sat, float& val, ImU32 curCol)
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(min, max, kBgCard, 6.0f);

                float hr = 0, hg = 0, hb = 0;
                ColorUtils::HsvToRgb(hue, 1.0f, 1.0f, hr, hg, hb);

                constexpr int kStepsX = 20;
                constexpr int kStepsY = 20;
                const float stepW = (max.x - min.x) / (float)kStepsX;
                const float stepH = (max.y - min.y) / (float)kStepsY;

                for (int y = 0; y < kStepsY; ++y)
                {
                    const float v0 = 1.0f - (float)y / (float)kStepsY;
                    const float v1 = 1.0f - (float)(y + 1) / (float)kStepsY;
                    const float y0 = min.y + y * stepH;
                    const float y1 = min.y + (y + 1) * stepH;

                    for (int x = 0; x < kStepsX; ++x)
                    {
                        const float s0 = (float)x / (float)kStepsX;
                        const float s1 = (float)(x + 1) / (float)kStepsX;
                        const float x0 = min.x + x * stepW;
                        const float x1 = min.x + (x + 1) * stepW;

                        auto evalColor = [&](float s, float v) -> ImU32
                        {
                            const float r = v * (1.0f - s * (1.0f - hr));
                            const float g = v * (1.0f - s * (1.0f - hg));
                            const float b = v * (1.0f - s * (1.0f - hb));
                            return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
                        };

                        dl->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x1, y1),
                                                    evalColor(s0, v0), evalColor(s1, v0),
                                                    evalColor(s1, v1), evalColor(s0, v1));
                    }
                }

                dl->AddRect(min, max, kBorderCard, 6.0f, 0, 1.0f);

                const ImVec2 handlePos(
                    min.x + (max.x - min.x) * sat,
                    min.y + (max.y - min.y) * (1.0f - val)
                );
                DrawReticleHandle(dl, handlePos, curCol, 7.0f, true);

                ImGui::SetCursorScreenPos(min);
                ImGui::InvisibleButton("##sq", ImVec2(max.x - min.x, max.y - min.y));
                bool changed = false;
                if (ImGui::IsItemActive() || (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)))
                {
                    const ImVec2 mouse = ImGui::GetMousePos();
                    const float newSat = std::max(0.0f, std::min(1.0f, (mouse.x - min.x) / (max.x - min.x)));
                    const float newVal = std::max(0.0f, std::min(1.0f, 1.0f - (mouse.y - min.y) / (max.y - min.y)));
                    if (fabsf(newSat - sat) > 0.001f || fabsf(newVal - val) > 0.001f)
                    {
                        sat = newSat;
                        val = newVal;
                        changed = true;
                    }
                }
                return changed;
            }

            bool ChannelSlider(const char* label, const char* id, const ImVec2& min, const ImVec2& max,
                               const ImVec4& c0, const ImVec4& c1, float& val)
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilledMultiColor(min, max,
                                            ImGui::ColorConvertFloat4ToU32(c0),
                                            ImGui::ColorConvertFloat4ToU32(c1),
                                            ImGui::ColorConvertFloat4ToU32(c1),
                                            ImGui::ColorConvertFloat4ToU32(c0));
                dl->AddRect(min, max, kBorderCard, 3.0f, 0, 1.0f);

                const ImVec2 handlePos(min.x + (max.x - min.x) * val, (min.y + max.y) * 0.5f);
                DrawSliderHandle(dl, handlePos, false, 14.0f, 5.0f);

                return Strip(id, min, max, false, val);
            }
        }

        bool Widget(const char* label, float rgb[3])
        {
            ImGui::PushID(label);
            float h = 0.0f, s = 0.0f, v = 1.0f;
            ColorUtils::RgbToHsv(rgb[0], rgb[1], rgb[2], h, s, v);

            bool changed = false;

            // 1. SELETOR DE MODOS (Uniforme e compacto)
            static int sMode = 0; // 0 = Círculo Cromático, 1 = Triângulo, 2 = Quadrado, 3 = Sliders
            const char* modeNames[] = { "Círculo", "Triângulo", "Quadrado", "Sliders" };

            const float totalW = ImGui::GetContentRegionAvail().x;
            const float segW = (totalW - 6.0f) / 4.0f;
            const float segH = 22.0f;

            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 barPos = ImGui::GetCursorScreenPos();
            dl->AddRectFilled(barPos, ImVec2(barPos.x + totalW, barPos.y + segH),
                              IM_COL32(24, 24, 28, 255), 5.0f);
            dl->AddRect(barPos, ImVec2(barPos.x + totalW, barPos.y + segH),
                        IM_COL32(46, 46, 54, 255), 5.0f, 0, 1.0f);

            for (int i = 0; i < 4; ++i)
            {
                ImGui::PushID(i);
                const ImVec2 p0(barPos.x + 3.0f + i * segW, barPos.y + 2.0f);
                const ImVec2 p1(p0.x + segW - 3.0f, barPos.y + segH - 2.0f);

                const bool isSel = (sMode == i);
                if (isSel)
                {
                    dl->AddRectFilled(p0, p1, ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x4f8cff, 0.40f)), 3.0f);
                    dl->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x4f8cff, 0.90f)), 3.0f, 0, 1.0f);
                }

                ImGui::SetCursorScreenPos(p0);
                if (ImGui::InvisibleButton(modeNames[i], ImVec2(p1.x - p0.x, p1.y - p0.y)))
                    sMode = i;

                const ImVec2 textSize = ImGui::CalcTextSize(modeNames[i]);
                const ImVec2 textPos(
                    p0.x + ((p1.x - p0.x) - textSize.x) * 0.5f,
                    p0.y + ((p1.y - p0.y) - textSize.y) * 0.5f
                );
                dl->AddText(textPos, isSel ? IM_COL32(255, 255, 255, 255) : IM_COL32(165, 165, 175, 255),
                            modeNames[i]);
                ImGui::PopID();
            }

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + segH + 4.0f);

            const ImU32 currentColU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(rgb[0], rgb[1], rgb[2], 1.0f));

            static int sHarmony = (int)Harmony_Triad;
            static std::vector<HarmonyColor> sHarmonyColors;

            // Altura original ampla e confortável para manipulação (~140px)
            constexpr float kVisualAreaH = 140.0f;

            // 2. CONTEÚDO DA ABA SELECIONADA
            if (sMode == 0)
            {
                // CÍRCULO CROMÁTICO + HARMONIAS
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(Theme::TextSecondary, "Harmonia:");
                ImGui::SameLine(0, 6);
                ImGui::SetNextItemWidth(totalW - (ImGui::GetCursorPosX() - ImGui::GetWindowPos().x) - 4.0f);
                if (ImGui::Combo("##harmony_type", &sHarmony, kHarmonyNames, (int)Harmony_COUNT))
                {
                    CalculateHarmonies((HarmonyType)sHarmony, h, s, v, sHarmonyColors);
                }

                const float brightBarW = 14.0f;
                const float wheelAreaH = 130.0f;
                const float wheelW = totalW - brightBarW - 6.0f;

                const float startWheelY = ImGui::GetCursorPosY();
                const ImVec2 cpos = ImGui::GetCursorScreenPos();
                const ImVec2 wheelMin = cpos;
                const ImVec2 wheelMax(cpos.x + wheelW, cpos.y + wheelAreaH);

                const ImVec2 brightMin(cpos.x + wheelW + 6.0f, cpos.y);
                const ImVec2 brightMax(cpos.x + totalW, cpos.y + wheelAreaH);

                float hr = 0, hg = 0, hb = 0;
                ColorUtils::HsvToRgb(h, s, 1.0f, hr, hg, hb);
                dl->AddRectFilledMultiColor(brightMin, brightMax,
                                            ImGui::ColorConvertFloat4ToU32(ImVec4(hr, hg, hb, 1.0f)),
                                            ImGui::ColorConvertFloat4ToU32(ImVec4(hr, hg, hb, 1.0f)),
                                            IM_COL32(0, 0, 0, 255),
                                            IM_COL32(0, 0, 0, 255));
                dl->AddRect(brightMin, brightMax, kBorderCard, 3.0f, 0, 1.0f);

                float vRatio = 1.0f - v;
                if (Strip("##wheel_val", brightMin, brightMax, true, vRatio))
                {
                    v = 1.0f - vRatio;
                    ColorUtils::HsvToRgb(h, s, v, rgb[0], rgb[1], rgb[2]);
                    CalculateHarmonies((HarmonyType)sHarmony, h, s, v, sHarmonyColors);
                    changed = true;
                }
                DrawSliderHandle(dl, ImVec2((brightMin.x + brightMax.x) * 0.5f, brightMin.y + (brightMax.y - brightMin.y) * vRatio), true, 14.0f, 5.0f);

                if (ColorWheelArea(wheelMin, wheelMax, h, s, v, (HarmonyType)sHarmony, sHarmonyColors))
                {
                    ColorUtils::HsvToRgb(h, s, v, rgb[0], rgb[1], rgb[2]);
                    changed = true;
                }

                // Posiciona os 5 cards de harmonia IMEDIATAMENTE abaixo da roda (sem espaço [1])
                ImGui::SetCursorPosY(startWheelY + wheelAreaH + 4.0f);

                const float cardGap = 3.0f;
                const float cardW = (totalW - cardGap * 4.0f) / 5.0f;
                const float cardH = 22.0f;
                const ImVec2 paletteStart = ImGui::GetCursorScreenPos();

                for (size_t i = 0; i < sHarmonyColors.size(); ++i)
                {
                    const ImVec2 c0(paletteStart.x + i * (cardW + cardGap), paletteStart.y);
                    const ImVec2 c1(c0.x + cardW, c0.y + cardH);

                    dl->AddRectFilled(c0, c1, sHarmonyColors[i].colU32, 3.0f);
                    dl->AddRect(c0, c1, (i == 0) ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 100), 3.0f, 0, (i == 0) ? 1.5f : 1.0f);

                    ImGui::PushID((int)i + 200);
                    ImGui::SetCursorScreenPos(c0);
                    if (ImGui::InvisibleButton("##card_btn", ImVec2(cardW, cardH)))
                    {
                        rgb[0] = sHarmonyColors[i].r;
                        rgb[1] = sHarmonyColors[i].g;
                        rgb[2] = sHarmonyColors[i].b;
                        ColorUtils::RgbToHsv(rgb[0], rgb[1], rgb[2], h, s, v);
                        CalculateHarmonies((HarmonyType)sHarmony, h, s, v, sHarmonyColors);
                        changed = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s (Clique para aplicar)", sHarmonyColors[i].hex.c_str());
                    ImGui::PopID();
                }

                // Posiciona a barra de preview/hex IMEDIATAMENTE abaixo dos cards (sem espaço [2])
                ImGui::SetCursorPosY(startWheelY + wheelAreaH + 4.0f + cardH + 5.0f);
            }
            else if (sMode == 3)
            {
                // MODO SLIDERS
                const float sliderW = totalW - 64.0f;
                const float sliderH = 14.0f;

                auto rowSlider = [&](const char* tag, const char* name, const ImVec4& c0, const ImVec4& c1, float& val, int maxVal)
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextColored(Theme::TextSecondary, "%s", name);
                    ImGui::SameLine(18.0f);

                    const ImVec2 cpos = ImGui::GetCursorScreenPos();
                    const ImVec2 smin(cpos.x, cpos.y + 4.0f);
                    const ImVec2 smax(cpos.x + sliderW, cpos.y + 4.0f + sliderH);

                    if (ChannelSlider(name, tag, smin, smax, c0, c1, val))
                        changed = true;

                    ImGui::SameLine(0, 6);
                    int intVal = (int)roundf(val * maxVal);
                    ImGui::SetNextItemWidth(38.0f);
                    if (ImGui::DragInt((std::string("##num_") + tag).c_str(), &intVal, 1.0f, 0, maxVal))
                    {
                        val = std::max(0.0f, std::min(1.0f, (float)intVal / maxVal));
                        changed = true;
                    }
                };

                // H Slider
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(Theme::TextSecondary, "H");
                ImGui::SameLine(18.0f);
                const ImVec2 hpos = ImGui::GetCursorScreenPos();
                const ImVec2 hmin(hpos.x, hpos.y + 4.0f);
                const ImVec2 hmax(hpos.x + sliderW, hpos.y + 4.0f + sliderH);
                DrawHueStrip(dl, hmin, hmax, false);
                float hRatio = h / 360.0f;
                if (Strip("##h_strip", hmin, hmax, false, hRatio))
                {
                    h = hRatio * 360.0f;
                    ColorUtils::HsvToRgb(h, s, v, rgb[0], rgb[1], rgb[2]);
                    changed = true;
                }
                DrawSliderHandle(dl, ImVec2(hmin.x + (hmax.x - hmin.x) * hRatio, (hmin.y + hmax.y) * 0.5f), false, 14.0f, 5.0f);
                ImGui::SameLine(0, 6);
                int hInt = (int)roundf(h);
                ImGui::SetNextItemWidth(38.0f);
                if (ImGui::DragInt("##num_h", &hInt, 1.0f, 0, 360))
                {
                    h = (float)std::max(0, std::min(360, hInt));
                    ColorUtils::HsvToRgb(h, s, v, rgb[0], rgb[1], rgb[2]);
                    changed = true;
                }

                // R, G, B Sliders
                float rRatio = rgb[0], gRatio = rgb[1], bRatio = rgb[2];
                rowSlider("##r", "R", ImVec4(0, rgb[1], rgb[2], 1), ImVec4(1, rgb[1], rgb[2], 1), rRatio, 255);
                rowSlider("##g", "G", ImVec4(rgb[0], 0, rgb[2], 1), ImVec4(rgb[0], 1, rgb[2], 1), gRatio, 255);
                rowSlider("##b", "B", ImVec4(rgb[0], rgb[1], 0, 1), ImVec4(rgb[0], rgb[1], 1, 1), bRatio, 255);

                if (changed)
                {
                    rgb[0] = rRatio; rgb[1] = gRatio; rgb[2] = bRatio;
                    ColorUtils::RgbToHsv(rgb[0], rgb[1], rgb[2], h, s, v);
                }

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
            }
            else
            {
                // TRIÂNGULO (1) OU QUADRADO (2)
                const float hueBarW = 14.0f;
                const float areaW = totalW - hueBarW - 6.0f;
                const float areaH = 140.0f;

                const float startAreaY = ImGui::GetCursorPosY();
                const ImVec2 startPos = ImGui::GetCursorScreenPos();
                const ImVec2 mainMin = startPos;
                const ImVec2 mainMax(startPos.x + areaW, startPos.y + areaH);

                const ImVec2 hueMin(startPos.x + areaW + 6.0f, startPos.y);
                const ImVec2 hueMax(startPos.x + totalW, startPos.y + areaH);

                DrawHueStrip(dl, hueMin, hueMax, true);
                float hRatio = h / 360.0f;
                if (Strip("##hue_v", hueMin, hueMax, true, hRatio))
                {
                    h = hRatio * 360.0f;
                    ColorUtils::HsvToRgb(h, s, v, rgb[0], rgb[1], rgb[2]);
                    changed = true;
                }
                DrawSliderHandle(dl, ImVec2((hueMin.x + hueMax.x) * 0.5f, hueMin.y + (hueMax.y - hueMin.y) * hRatio), true, 14.0f, 5.0f);

                if (sMode == 1)
                    changed = TriangleArea(mainMin, mainMax, h, s, v, currentColU32) || changed;
                else
                    changed = SquareArea(mainMin, mainMax, h, s, v, currentColU32) || changed;

                if (changed)
                    ColorUtils::HsvToRgb(h, s, v, rgb[0], rgb[1], rgb[2]);

                // Posiciona a barra de preview/hex IMEDIATAMENTE abaixo da área (sem espaço excedente)
                ImGui::SetCursorPosY(startAreaY + areaH + 5.0f);
            }

            // 3. BARRA DE PREVIEW, HEXADECIMAL E CÓPIA
            const ImVec2 previewPos = ImGui::GetCursorScreenPos();
            const float previewBoxW = 34.0f;
            const float previewBoxH = 22.0f;

            dl->AddRectFilled(previewPos, ImVec2(previewPos.x + previewBoxW, previewPos.y + previewBoxH),
                              currentColU32, 4.0f);
            dl->AddRect(previewPos, ImVec2(previewPos.x + previewBoxW, previewPos.y + previewBoxH),
                        kBorderCardHi, 4.0f, 0, 1.0f);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + previewBoxW + 6.0f);

            const std::string hexStr = ColorUtils::ToHex(rgb[0], rgb[1], rgb[2]);
            char hexBuf[16];
            snprintf(hexBuf, sizeof(hexBuf), "%s", hexStr.c_str());
            ImGui::SetNextItemWidth(74.0f);
            if (ImGui::InputText("##hex_input", hexBuf, sizeof(hexBuf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
            {
                float newRgb[3];
                if (ColorUtils::ParseHex(hexBuf, newRgb))
                {
                    rgb[0] = newRgb[0]; rgb[1] = newRgb[1]; rgb[2] = newRgb[2];
                    ColorUtils::RgbToHsv(rgb[0], rgb[1], rgb[2], h, s, v);
                    changed = true;
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Hexadecimal (Enter para aplicar)");

            ImGui::SameLine(0, 4);
            if (ImGui::SmallButton("Copiar"))
            {
                ImGui::SetClipboardText(hexStr.c_str());
            }

            // 4. COLEÇÃO DE CORES DO USUÁRIO
            ImGui::Separator();

            static std::vector<ImU32> sUserPalette = {
                0x3b82f6, 0x10b981, 0xf59e0b, 0xef4444, 0x8b5cf6, 0xec4899,
                0x14b8a6, 0x6366f1, 0x84cc16, 0x06b6d4, 0xe11d48, 0x64748b
            };

            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(Theme::TextSecondary, "Minha Coleção:");
            ImGui::SameLine(0, 4);

            ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x3b82f6, 0.25f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::Hex(0x3b82f6, 0.45f));
            if (ImGui::SmallButton("+"))
            {
                const int rInt = (int)roundf(rgb[0] * 255.0f);
                const int gInt = (int)roundf(rgb[1] * 255.0f);
                const int bInt = (int)roundf(rgb[2] * 255.0f);
                const ImU32 packed = (rInt << 16) | (gInt << 8) | bInt;

                if (sUserPalette.empty() || sUserPalette.back() != packed)
                    sUserPalette.push_back(packed);
            }
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Adicionar cor atual à coleção");

            ImGui::SameLine(0, 3);

            ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0xef4444, 0.20f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::Hex(0xef4444, 0.40f));
            if (ImGui::SmallButton("-"))
            {
                if (!sUserPalette.empty())
                    sUserPalette.pop_back();
            }
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Remover última cor (ou botão direito no swatch)");

            if (sUserPalette.empty())
            {
                ImGui::TextColored(Theme::Hex(0x808080, 0.6f), "Clique em [+] para salvar.");
            }
            else
            {
                int toRemoveIdx = -1;
                for (size_t i = 0; i < sUserPalette.size(); ++i)
                {
                    const ImU32 qcol = sUserPalette[i];
                    const ImVec4 qvec = Theme::Hex(qcol);
                    ImGui::PushID((int)i + 5000);
                    if (ImGui::ColorButton("##user_sw", qvec,
                                           ImGuiColorEditFlags_NoTooltip |
                                           ImGuiColorEditFlags_NoPicker |
                                           ImGuiColorEditFlags_NoBorder,
                                           ImVec2(18.0f, 18.0f)))
                    {
                        rgb[0] = qvec.x; rgb[1] = qvec.y; rgb[2] = qvec.z;
                        ColorUtils::RgbToHsv(rgb[0], rgb[1], rgb[2], h, s, v);
                        CalculateHarmonies((HarmonyType)sHarmony, h, s, v, sHarmonyColors);
                        changed = true;
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s (Clique para aplicar · Botão direito para remover)",
                                          ColorUtils::ToHex(qvec.x, qvec.y, qvec.z).c_str());
                    }
                    if (ImGui::IsItemClicked(1))
                    {
                        toRemoveIdx = (int)i;
                    }
                    if ((i + 1) % 11 != 0 && (i + 1) < sUserPalette.size())
                    {
                        ImGui::SameLine(0, 3);
                    }
                    ImGui::PopID();
                }
                if (toRemoveIdx >= 0 && toRemoveIdx < (int)sUserPalette.size())
                {
                    sUserPalette.erase(sUserPalette.begin() + toRemoveIdx);
                }
            }

            ImGui::PopID();
            return changed;
        }
    }
}
