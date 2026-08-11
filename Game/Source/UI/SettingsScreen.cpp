#include "UI/SettingsScreen.h"

#include "raylib.h"

#include <cstdio>

namespace game
{
    namespace
    {
        struct Resolution
        {
            int width;
            int height;
        };

        const Resolution kResolutions[] = {
            { 1280, 720 },
            { 1600, 900 },
            { 1920, 1080 },
            { 2560, 1440 },
        };
        const int kResolutionCount = sizeof(kResolutions) / sizeof(kResolutions[0]);

        const char *kShadowOptions[] = { "Baixa", "Média", "Alta" };
        const int kShadowOptionCount = 3;
        const char *kCharacterOptions[] = { "Masculino", "Feminino" };
        const int kCharacterOptionCount = 2;

        int CurrentResolutionIndex(const Settings &settings)
        {
            for (int i = 0; i < kResolutionCount; ++i)
            {
                if (kResolutions[i].width == settings.windowWidth &&
                    kResolutions[i].height == settings.windowHeight)
                {
                    return i;
                }
            }
            return 0;
        }
    }

    bool SettingsScreen::Draw(Settings &settings)
    {
        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();
        float scale = ui::UiScale();
        if (scale * 710.0f > sh) scale = sh / 710.0f;

        ui::DrawOverlay();

        float panelW = 540 * scale;
        float panelH = 700 * scale;
        Rectangle panel = { (sw - panelW) * 0.5f, (sh - panelH) * 0.5f, panelW, panelH };
        ui::DrawPanel(panel, "Configurações");

        float x = panel.x + 40 * scale;
        float width = panel.width - 80 * scale;
        float y = panel.y + 82 * scale;
        const float rowH = 34 * scale;
        const float gap = 8 * scale;

        auto SectionHeader = [&](const char *text)
        {
            int fs = (int)(16 * scale);
            Vector2 ts = MeasureTextEx(GetFontDefault(), text, (float)fs, 1.0f);
            ui::DrawText(text, { x, y }, fs, ui::Accent());
            float lineY = y + ts.y + 4 * scale;
            DrawRectangleV({ x, lineY }, { 120 * scale, 1.0f }, ui::Accent());
            y += 28 * scale;
        };

        SectionHeader("Gráficos");

        static char resBuffer[kResolutionCount][32];
        for (int i = 0; i < kResolutionCount; ++i)
            snprintf(resBuffer[i], sizeof resBuffer[i], "%dx%d",
                     kResolutions[i].width, kResolutions[i].height);
        const char *resOptions[kResolutionCount];
        for (int i = 0; i < kResolutionCount; ++i)
            resOptions[i] = resBuffer[i];

        int resIndex = CurrentResolutionIndex(settings);
        if (ui::DrawSelector(1, mUiState, "Resolução", { x, y, width, rowH },
                             resOptions, kResolutionCount, &resIndex))
        {
            settings.windowWidth = kResolutions[resIndex].width;
            settings.windowHeight = kResolutions[resIndex].height;
        }
        y += rowH + gap;

        settings.fullscreen = ui::DrawToggle("Tela cheia", { x, y, width, rowH }, settings.fullscreen);
        y += rowH + gap;

        settings.vsync = ui::DrawToggle("V-Sync", { x, y, width, rowH }, settings.vsync);
        y += rowH + gap;

        int shadowIndex = settings.shadowQuality;
        if (ui::DrawSelector(2, mUiState, "Qualidade das sombras", { x, y, width, rowH },
                             kShadowOptions, kShadowOptionCount, &shadowIndex))
        {
            settings.shadowQuality = shadowIndex;
        }
        y += rowH + gap;

        SectionHeader("Jogo");

        float sensPercent = settings.mouseSensitivity / 0.01f * 100.0f;
        if (sensPercent < 0.0f) sensPercent = 0.0f;
        if (sensPercent > 100.0f) sensPercent = 100.0f;
        char sensText[16];
        snprintf(sensText, sizeof sensText, "%.0f%%", sensPercent);
        if (ui::DrawSlider(3, mUiState, "Sensibilidade do mouse", { x, y, width, rowH },
                           &sensPercent, 0.0f, 100.0f, sensText))
        {
            settings.mouseSensitivity = sensPercent / 100.0f * 0.01f;
        }
        y += rowH + gap;

        int characterIndex = static_cast<int>(settings.characterGender);
        if (ui::DrawSelector(5, mUiState, "Personagem", { x, y, width, rowH },
                             kCharacterOptions, kCharacterOptionCount, &characterIndex))
        {
            settings.characterGender = static_cast<CharacterGender>(characterIndex);
        }
        y += rowH + gap;

        ui::DrawText("Cor da pele", { x, y }, (int)(18 * scale), ui::TextPrimary());
        ui::DrawColorWheel(6, mUiState, { x, y + 22 * scale, width, 90 * scale },
                           &settings.skinHue, &settings.skinSaturation);
        Color skinPreview = ColorFromHSV(settings.skinHue * 360.0f,
                                         settings.skinSaturation, 0.90f);
        DrawCircleV({ x + width - 14 * scale, y + 10 * scale }, 9 * scale, skinPreview);
        DrawCircleLines((int)(x + width - 14 * scale), (int)(y + 10 * scale),
                        9 * scale, ui::TextPrimary());
        y += 120 * scale;

        float lightPercent = (settings.characterLightIntensity - 0.35f) / 1.30f * 100.0f;
        char lightText[16];
        snprintf(lightText, sizeof lightText, "%.0f%%", lightPercent);
        if (ui::DrawSlider(7, mUiState, "Incidencia de luz", { x, y, width, rowH },
                           &lightPercent, 0.0f, 100.0f, lightText))
        {
            settings.characterLightIntensity = 0.35f + lightPercent / 100.0f * 1.30f;
        }
        y += rowH + gap;

        SectionHeader("Áudio");

        float volPercent = settings.masterVolume * 100.0f;
        if (volPercent < 0.0f) volPercent = 0.0f;
        if (volPercent > 100.0f) volPercent = 100.0f;
        char volText[16];
        snprintf(volText, sizeof volText, "%.0f%%", volPercent);
        if (ui::DrawSlider(4, mUiState, "Volume geral", { x, y, width, rowH },
                           &volPercent, 0.0f, 100.0f, volText))
        {
            settings.masterVolume = volPercent / 100.0f;
        }
        y += rowH + gap;

        float btnW = 220 * scale;
        Rectangle back = { panel.x + (panel.width - btnW) * 0.5f,
                           panel.y + panel.height - 54 * scale, btnW, 42 * scale };
        return ui::DrawButton("Voltar", back);
    }
}
