#ifndef GAMEUI_H
#define GAMEUI_H

#include "raylib.h"

namespace game
{
    namespace ui
    {
        struct UiState
        {
            int activeControl = -1;
        };

        float UiScale();

        Color PanelBackground();
        Color PanelBorder();
        Color TextPrimary();
        Color TextDim();
        Color ButtonBackground();
        Color ButtonHover();
        Color ButtonActive();
        Color Accent();

        void DrawOverlay(float alpha = 0.72f);
        void DrawPanel(Rectangle bounds, const char *title);
        void DrawText(const char *text, Vector2 position, int fontSize, Color color);
        void DrawTextCentered(const char *text, float centerX, float y, int fontSize, Color color);

        bool DrawButton(const char *label, Rectangle bounds);
        bool DrawToggle(const char *label, Rectangle bounds, bool value);
        bool DrawSelector(int id, UiState &state, const char *label, Rectangle bounds,
                          const char *const *options, int optionCount, int *current);
        bool DrawSlider(int id, UiState &state, const char *label, Rectangle bounds,
                        float *value, float minValue, float maxValue, const char *display);
        bool DrawColorWheel(int id, UiState &state, Rectangle bounds,
                            float *hue, float *saturation, float value = 0.9f);
    }
}

#endif // GAMEUI_H
