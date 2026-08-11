#include "UI/LoadingScreen.h"
#include "UI/GameUI.h"

#include "raylib.h"

#include <cstdio>

namespace game
{
    namespace
    {
        void DrawLoadingFrame(float progress, const char *stage)
        {
            const float scale = ui::UiScale();
            const int screenWidth = GetScreenWidth();
            const int screenHeight = GetScreenHeight();

            BeginDrawing();
            {
                ClearBackground({ 14, 16, 22, 255 });

                // Game title.
                const int titleSize = (int)(52 * scale);
                ui::DrawTextCentered("ELDORIA", screenWidth * 0.5f,
                                     screenHeight * 0.36f, titleSize, ui::Accent());

                // Progress bar track.
                const float barWidth = 440 * scale;
                const float barHeight = 10 * scale;
                Rectangle track = { screenWidth * 0.5f - barWidth * 0.5f,
                                    screenHeight * 0.52f, barWidth, barHeight };
                DrawRectangleRounded(track, 0.5f, 0, { 58, 64, 78, 255 });

                // Progress bar fill.
                float frac = progress;
                if (frac < 0.0f) frac = 0.0f;
                if (frac > 1.0f) frac = 1.0f;
                if (frac > 0.0f)
                {
                    Rectangle fill = { track.x, track.y, track.width * frac, track.height };
                    DrawRectangleRounded(fill, 0.5f, 0, ui::Accent());
                }

                // Current stage label.
                const int labelSize = (int)(17 * scale);
                ui::DrawTextCentered(stage, screenWidth * 0.5f,
                                     track.y + barHeight + 18 * scale, labelSize, ui::TextDim());

                // Percentage.
                char percent[16];
                snprintf(percent, sizeof(percent), "%d%%", (int)(frac * 100.0f + 0.5f));
                ui::DrawTextCentered(percent, screenWidth * 0.5f,
                                     track.y + barHeight + 44 * scale,
                                     (int)(14 * scale), ui::TextDim());
            }
            EndDrawing();
        }
    }

    void LoadingScreen::Draw(float progress, const char *stage)
    {
        DrawLoadingFrame(progress, stage);
    }

    void LoadingScreen::Finish()
    {
        DrawLoadingFrame(1.0f, "Pronto!");
    }
}
