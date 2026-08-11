#include "Theme.h"

#include "imgui.h"
#include "raylib.h"

namespace seedui
{
    namespace Theme
    {
        void Apply()
        {
            ImGuiStyle& s = ImGui::GetStyle();
            ImVec4* c = s.Colors;

            // Forma
            s.WindowRounding = 6.0f;
            s.ChildRounding = 4.0f;
            s.FrameRounding = 4.0f;
            s.PopupRounding = 6.0f;
            s.GrabRounding = 4.0f;
            s.TabRounding = 4.0f;
            s.ScrollbarRounding = 4.0f;
            s.WindowBorderSize = 1.0f;
            s.PopupBorderSize = 1.0f;
            s.FrameBorderSize = 0.0f;
            s.WindowPadding = ImVec2(8, 8);
            s.FramePadding = ImVec2(8, 4);
            s.ItemSpacing = ImVec2(8, 6);
            s.ItemInnerSpacing = ImVec2(6, 6);
            s.ScrollbarSize = 12.0f;
            s.AntiAliasedLines = true;

            // Cores
            c[ImGuiCol_WindowBg] = BackgroundPanel;
            c[ImGuiCol_ChildBg] = BackgroundChild;
            c[ImGuiCol_PopupBg] = BackgroundPanel;
            c[ImGuiCol_Border] = Border;
            c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
            c[ImGuiCol_MenuBarBg] = MenuBar;
            c[ImGuiCol_TitleBg] = PanelHeader;
            c[ImGuiCol_TitleBgActive] = PanelHeader;
            c[ImGuiCol_TitleBgCollapsed] = PanelHeader;
            c[ImGuiCol_Text] = TextPrimary;
            c[ImGuiCol_TextDisabled] = TextDisabled;
            c[ImGuiCol_TextSelectedBg] = Hex(0x4f8cff, 0.25f);
            c[ImGuiCol_Header] = PanelHeader;
            c[ImGuiCol_HeaderHovered] = BorderLight;
            c[ImGuiCol_HeaderActive] = Hex(0x4f8cff, 0.25f);
            c[ImGuiCol_Button] = PanelHeader;
            c[ImGuiCol_ButtonHovered] = BorderLight;
            c[ImGuiCol_ButtonActive] = Hex(0x4f8cff, 0.30f);
            c[ImGuiCol_FrameBg] = BackgroundChild;
            c[ImGuiCol_FrameBgHovered] = BackgroundPanel;
            c[ImGuiCol_FrameBgActive] = MenuBar;
            c[ImGuiCol_CheckMark] = AccentBlue;
            c[ImGuiCol_SliderGrab] = AccentBlue;
            c[ImGuiCol_SliderGrabActive] = AccentBlue;
            c[ImGuiCol_Separator] = Border;
            c[ImGuiCol_SeparatorHovered] = BorderLight;
            c[ImGuiCol_SeparatorActive] = AccentBlue;
            c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
            c[ImGuiCol_ScrollbarGrab] = Border;
            c[ImGuiCol_ScrollbarGrabHovered] = BorderLight;
            c[ImGuiCol_ScrollbarGrabActive] = AccentBlue;
            c[ImGuiCol_Tab] = BackgroundPanel;
            c[ImGuiCol_TabHovered] = PanelHeader;
            c[ImGuiCol_TabSelected] = Hex(0x4f8cff, 0.30f);
            c[ImGuiCol_TabDimmed] = BackgroundChild;
            c[ImGuiCol_TabDimmedSelected] = Hex(0x4f8cff, 0.20f);
            c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.6f);
        }

        void LoadFonts()
        {
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->Clear();

            const char* candidates[] = {
                "assets/fonts/Inter.ttf",
                "../../assets/fonts/Inter.ttf",
                "../assets/fonts/Inter.ttf",
            };

            const char* found = nullptr;
            for (const char* p : candidates)
            {
                if (FileExists(p))
                {
                    found = p;
                    break;
                }
            }

            if (found)
            {
                ImFont* base = io.Fonts->AddFontFromFileTTF(found, 15.0f);
                if (base)
                {
                    io.Fonts->AddFontFromFileTTF(found, 13.0f);
                    io.Fonts->AddFontFromFileTTF(found, 20.0f);
                    io.FontDefault = base;
                    TraceLog(LOG_INFO, "Fonte Inter carregada: %s", found);
                    return;
                }
            }

            io.FontDefault = io.Fonts->AddFontDefault();
            TraceLog(LOG_WARNING, "Inter nao encontrada — usando fonte padrao do ImGui");
        }
    }
}
