#ifndef SEEDUI_THEME_H
#define SEEDUI_THEME_H

#include <cstdint>

#include "imgui.h"
#include "raylib.h"

namespace seedui
{
    namespace Theme
    {
        inline ImVec4 Hex(uint32_t rgb, float alpha = 1.0f)
        {
            return ImVec4(((rgb >> 16) & 0xFF) / 255.0f,
                          ((rgb >> 8) & 0xFF) / 255.0f,
                          (rgb & 0xFF) / 255.0f,
                          alpha);
        }

        // Paleta oficial (identidade Blender + Photoshop)
        inline const ImVec4 BackgroundWindow = Hex(0x212121);
        inline const ImVec4 BackgroundPanel   = Hex(0x2b2b2b);
        inline const ImVec4 BackgroundChild   = Hex(0x1f1f1f);
        inline const ImVec4 PanelHeader       = Hex(0x3a3a3a);
        inline const ImVec4 MenuBar           = Hex(0x323232);
        inline const ImVec4 Border            = Hex(0x3f3f3f);
        inline const ImVec4 BorderLight       = Hex(0x4a4a4a);
        inline const ImVec4 TextPrimary       = Hex(0xececec);
        inline const ImVec4 TextSecondary     = Hex(0xa0a0a0);
        inline const ImVec4 TextDisabled      = Hex(0x6a6a6a);
        inline const ImVec4 AccentBlue        = Hex(0x4f8cff);
        inline const ImVec4 AccentOrange      = Hex(0xf57900);
        inline const ImVec4 Success           = Hex(0x2ecc71);
        inline const ImVec4 Warning           = Hex(0xf1c40f);
        inline const ImVec4 Error             = Hex(0xe74c3c);
        inline const ImVec4 CanvasBackground  = Hex(0x1a1a1a);
        inline const ImVec4 GridMinor         = Hex(0x262626);
        inline const ImVec4 GridMajor         = Hex(0x2e2e2e);

        // Cor do ClearBackground (raylib)
        inline const Color RaylibWindowBg = { 33, 33, 33, 255 };

        void Apply();
        void LoadFonts();
    }
}

#endif // SEEDUI_THEME_H
