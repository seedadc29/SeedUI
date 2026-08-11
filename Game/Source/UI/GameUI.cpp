#include "UI/GameUI.h"

#include <cmath>

namespace game
{
    namespace ui
    {
        static const Color kPanelBackground = { 22, 25, 33, 226 };
        static const Color kPanelBorder     = { 148, 158, 176, 110 };
        static const Color kTextPrimary     = { 228, 232, 240, 255 };
        static const Color kTextDim         = { 166, 172, 184, 255 };
        static const Color kButtonIdle      = { 42, 46, 58, 205 };
        static const Color kButtonHover     = { 60, 66, 82, 220 };
        static const Color kButtonActive    = { 30, 33, 42, 235 };
        static const Color kTrackOff        = { 58, 64, 78, 255 };
        static const Color kTrackOn         = { 122, 138, 158, 255 };
        static const Color kAccent          = { 203, 176, 126, 255 };

        float UiScale()
        {
            float s = (float)GetScreenWidth() / 1280.0f;
            if (s < 0.5f) s = 0.5f;
            if (s > 2.0f) s = 2.0f;
            return s;
        }

        static Font DefaultFont() { return GetFontDefault(); }

        Color PanelBackground() { return kPanelBackground; }
        Color PanelBorder() { return kPanelBorder; }
        Color TextPrimary() { return kTextPrimary; }
        Color TextDim() { return kTextDim; }
        Color ButtonBackground() { return kButtonIdle; }
        Color ButtonHover() { return kButtonHover; }
        Color ButtonActive() { return kButtonActive; }
        Color Accent() { return kAccent; }

        void DrawOverlay(float alpha)
        {
            Color c = { 8, 10, 14, (unsigned char)(alpha * 255.0f) };
            DrawRectangleV({ 0, 0 }, { (float)GetScreenWidth(), (float)GetScreenHeight() }, c);
        }

        void DrawPanel(Rectangle bounds, const char *title)
        {
            float scale = UiScale();
            DrawRectangleRounded(bounds, 0.025f, 0, kPanelBackground);
            DrawRectangleRoundedLinesEx(bounds, 0.025f, 0, 1.5f, kPanelBorder);

            if (title && title[0])
            {
                int size = (int)(28 * scale);
                Vector2 ts = MeasureTextEx(DefaultFont(), title, (float)size, 1.0f);
                Vector2 pos = { bounds.x + (bounds.width - ts.x) * 0.5f, bounds.y + 20 * scale };
                DrawTextEx(DefaultFont(), title, pos, (float)size, 1.0f, kTextPrimary);

                float lineY = pos.y + ts.y + 12 * scale;
                DrawRectangleV({ bounds.x + 28 * scale, lineY }, { bounds.width - 56 * scale, 1.0f }, kPanelBorder);
            }
        }

        void DrawText(const char *text, Vector2 position, int fontSize, Color color)
        {
            DrawTextEx(DefaultFont(), text, position, (float)fontSize, 1.0f, color);
        }

        void DrawTextCentered(const char *text, float centerX, float y, int fontSize, Color color)
        {
            float w = MeasureTextEx(DefaultFont(), text, (float)fontSize, 1.0f).x;
            DrawTextEx(DefaultFont(), text, { centerX - w * 0.5f, y }, (float)fontSize, 1.0f, color);
        }

        bool DrawButton(const char *label, Rectangle bounds)
        {
            bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
            bool active = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

            Color bg = active ? kButtonActive : (hovered ? kButtonHover : kButtonIdle);
            DrawRectangleRounded(bounds, 0.16f, 0, bg);
            DrawRectangleRoundedLinesEx(bounds, 0.16f, 0, hovered ? 1.5f : 1.0f,
                                        hovered ? kAccent : kPanelBorder);

            int size = (int)(19 * UiScale());
            Vector2 ts = MeasureTextEx(DefaultFont(), label, (float)size, 1.0f);
            Vector2 pos = { bounds.x + (bounds.width - ts.x) * 0.5f,
                            bounds.y + (bounds.height - ts.y) * 0.5f };
            DrawTextEx(DefaultFont(), label, pos, (float)size, 1.0f,
                       hovered ? kTextPrimary : kTextDim);
            return clicked;
        }

        bool DrawToggle(const char *label, Rectangle bounds, bool value)
        {
            float scale = UiScale();
            float switchW = 46 * scale;
            float switchH = 22 * scale;
            Rectangle sw = { bounds.x + bounds.width - switchW,
                             bounds.y + (bounds.height - switchH) * 0.5f,
                             switchW, switchH };

            int size = (int)(18 * scale);
            Vector2 ts = MeasureTextEx(DefaultFont(), label, (float)size, 1.0f);
            DrawTextEx(DefaultFont(), label,
                       { bounds.x, bounds.y + (bounds.height - ts.y) * 0.5f },
                       (float)size, 1.0f, kTextPrimary);

            bool hovered = CheckCollisionPointRec(GetMousePosition(), sw);
            DrawRectangleRounded(sw, 0.5f, 0, value ? kTrackOn : kTrackOff);

            float knobRadius = switchH * 0.5f - 3.0f * scale;
            Vector2 knob = {
                value ? sw.x + sw.width - knobRadius - 3.0f * scale
                      : sw.x + 3.0f * scale + knobRadius,
                sw.y + switchH * 0.5f
            };
            DrawCircleV(knob, knobRadius, kTextPrimary);
            if (hovered) DrawRectangleRoundedLinesEx(sw, 0.5f, 0, 1.5f, kAccent);

            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return !value;
            return value;
        }

        bool DrawSelector(int id, UiState &state, const char *label, Rectangle bounds,
                          const char *const *options, int optionCount, int *current)
        {
            float scale = UiScale();
            float controlW = 230 * scale;
            float arrowW = 34 * scale;
            Rectangle control = { bounds.x + bounds.width - controlW,
                                  bounds.y, controlW, bounds.height };
            Rectangle left = { control.x, control.y, arrowW, control.height };
            Rectangle right = { control.x + control.width - arrowW, control.y, arrowW, control.height };
            Rectangle valueRect = { control.x + arrowW, control.y,
                                    control.width - 2 * arrowW, control.height };

            int size = (int)(18 * scale);
            Vector2 ts = MeasureTextEx(DefaultFont(), label, (float)size, 1.0f);
            DrawTextEx(DefaultFont(), label,
                       { bounds.x, bounds.y + (bounds.height - ts.y) * 0.5f },
                       (float)size, 1.0f, kTextPrimary);

            bool leftHover = CheckCollisionPointRec(GetMousePosition(), left);
            bool rightHover = CheckCollisionPointRec(GetMousePosition(), right);

            DrawRectangleRounded(left, 0.25f, 0, leftHover ? kButtonHover : kButtonIdle);
            DrawRectangleRounded(right, 0.25f, 0, rightHover ? kButtonHover : kButtonIdle);

            DrawTextCentered("<", left.x + left.width * 0.5f,
                             left.y + (left.height - 18 * scale) * 0.5f,
                             (int)(18 * scale), leftHover ? kTextPrimary : kTextDim);
            DrawTextCentered(">", right.x + right.width * 0.5f,
                             right.y + (right.height - 18 * scale) * 0.5f,
                             (int)(18 * scale), rightHover ? kTextPrimary : kTextDim);

            bool changed = false;
            if (leftHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && *current > 0)
            {
                --*current;
                changed = true;
            }
            if (rightHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && *current < optionCount - 1)
            {
                ++*current;
                changed = true;
            }

            Vector2 vs = MeasureTextEx(DefaultFont(), options[*current], (float)size, 1.0f);
            DrawTextEx(DefaultFont(), options[*current],
                       { valueRect.x + (valueRect.width - vs.x) * 0.5f,
                         valueRect.y + (valueRect.height - vs.y) * 0.5f },
                       (float)size, 1.0f, kTextDim);
            return changed;
        }

        bool DrawSlider(int id, UiState &state, const char *label, Rectangle bounds,
                        float *value, float minValue, float maxValue, const char *display)
        {
            float scale = UiScale();
            int size = (int)(18 * scale);

            Vector2 ts = MeasureTextEx(DefaultFont(), label, (float)size, 1.0f);
            DrawTextEx(DefaultFont(), label, { bounds.x, bounds.y },
                       (float)size, 1.0f, kTextPrimary);

            Vector2 ds = MeasureTextEx(DefaultFont(), display, (float)size, 1.0f);
            DrawTextEx(DefaultFont(), display,
                       { bounds.x + bounds.width - ds.x, bounds.y },
                       (float)size, 1.0f, kTextDim);

            float trackY = bounds.y + bounds.height - 8 * scale;
            float trackH = 6 * scale;
            Rectangle track = { bounds.x, trackY, bounds.width, trackH };
            DrawRectangleRounded(track, 0.5f, 0, kTrackOff);

            float frac = (maxValue > minValue) ? (*value - minValue) / (maxValue - minValue) : 0.0f;
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;

            float usable = bounds.width - 14 * scale;
            Rectangle fill = { bounds.x, trackY, 7 * scale + usable * frac, trackH };
            DrawRectangleRounded(fill, 0.5f, 0, kAccent);

            Vector2 mouse = GetMousePosition();
            Rectangle hit = { bounds.x - 6 * scale, trackY - 12 * scale,
                              bounds.width + 12 * scale, trackH + 24 * scale };
            bool hovering = CheckCollisionPointRec(mouse, hit);

            bool changed = false;
            if (hovering && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                state.activeControl = id;

            if (state.activeControl == id)
            {
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) state.activeControl = -1;
                float t = (mouse.x - bounds.x) / bounds.width;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                float newValue = minValue + t * (maxValue - minValue);
                if (newValue != *value)
                {
                    *value = newValue;
                    changed = true;
                }
            }

            float knobX = bounds.x + 7 * scale + usable * frac;
            float knobY = trackY + trackH * 0.5f;
            float knobR = 8 * scale;
            DrawCircleV({ knobX, knobY }, knobR, state.activeControl == id ? kAccent : kTextPrimary);

            return changed;
        }

        bool DrawColorWheel(int id, UiState &state, Rectangle bounds,
                            float *hue, float *saturation, float value)
        {
            Vector2 center = { bounds.x + bounds.width * 0.5f,
                               bounds.y + bounds.height * 0.5f };
            float radius = (bounds.width < bounds.height ? bounds.width : bounds.height) * 0.5f;
            const int angleSteps = 48;
            const int radialSteps = 12;

            for (int radial = radialSteps; radial > 0; --radial)
            {
                float outer = radius * radial / radialSteps;
                float inner = radius * (radial - 1) / radialSteps;
                float sat = (radial - 0.5f) / radialSteps;
                for (int angle = 0; angle < angleSteps; ++angle)
                {
                    float start = 360.0f * angle / angleSteps;
                    float end = 360.0f * (angle + 1) / angleSteps;
                    DrawRing(center, inner, outer, start, end, 2,
                             ColorFromHSV((start + end) * 0.5f, sat, value));
                }
            }

            Vector2 mouse = GetMousePosition();
            float dx = mouse.x - center.x;
            float dy = mouse.y - center.y;
            float distance = sqrtf(dx * dx + dy * dy);
            bool hovered = distance <= radius;
            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                state.activeControl = id;

            bool changed = false;
            if (state.activeControl == id)
            {
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                    state.activeControl = -1;
                else
                {
                    float newHue = atan2f(dy, dx) / (2.0f * PI);
                    if (newHue < 0.0f) newHue += 1.0f;
                    float newSaturation = distance / radius;
                    if (newSaturation > 1.0f) newSaturation = 1.0f;
                    if (newHue != *hue || newSaturation != *saturation)
                    {
                        *hue = newHue;
                        *saturation = newSaturation;
                        changed = true;
                    }
                }
            }

            float markerAngle = *hue * 2.0f * PI;
            Vector2 marker = { center.x + cosf(markerAngle) * radius * *saturation,
                               center.y + sinf(markerAngle) * radius * *saturation };
            DrawCircleV(marker, 7.0f * UiScale(), BLACK);
            DrawCircleV(marker, 4.5f * UiScale(), WHITE);
            DrawCircleLines((int)center.x, (int)center.y, radius, kPanelBorder);
            return changed;
        }
    }
}
