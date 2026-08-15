#include "Manual.h"

#include "EmbeddedAssets.h"
#include "Theme.h"
#include "imgui.h"

#include <fstream>
#include <sstream>
#include <string>

namespace seedui
{
    namespace
    {
        std::string gText;
        std::string gSource;
        bool gLoaded = false;

        void LoadManual()
        {
            gLoaded = true;
            const char* candidates[] = {
                "assets/manual/SEEDUI_MANUAL.md",
                "../../assets/manual/SEEDUI_MANUAL.md",
                "../assets/manual/SEEDUI_MANUAL.md",
                "docs/SEEDUI_MANUAL.md",
                "../../docs/SEEDUI_MANUAL.md",
                "../docs/SEEDUI_MANUAL.md",
            };

            for (const char* p : candidates)
            {
                std::ifstream f(p);
                if (f)
                {
                    std::ostringstream ss;
                    ss << f.rdbuf();
                    gText = ss.str();
                    gSource = p;
                    break;
                }
            }

            if (gText.empty())
            {
                std::string embedded;
                if (LoadEmbeddedText("SEEDUI_MANUAL.md", embedded))
                {
                    gText = std::move(embedded);
                    gSource = "recurso embutido no executavel";
                }
            }
        }
    }

    void ManualDraw(bool* pOpen)
    {
        if (!gLoaded) LoadManual();

        ImGui::SetNextWindowSize(ImVec2(880, 640), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(140, 90), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Manual do SeedUI", pOpen, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        if (!gText.empty())
        {
            ImGui::TextColored(Theme::TextSecondary, "Fonte: %s", gSource.c_str());
            ImGui::Separator();
            ImGui::BeginChild("##manual_text", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(gText.c_str());
            ImGui::EndChild();
        }
        else
        {
            ImGui::TextWrapped(
                "Manual não encontrado. Execute o SeedUI a partir da pasta SeedUI/ ou "
                "abra manualmente o arquivo docs/SEEDUI_MANUAL.md.");
        }

        ImGui::End();
    }
}
