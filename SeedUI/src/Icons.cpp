#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#include "Icons.h"

#include "Theme.h"
#include "imgui.h"
#include "raylib.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace seedui
{
    namespace
    {
        // Ordem idêntica ao enum IconId
        struct IconDef
        {
            IconId id;
            const char* file;
        };

        const IconDef kIcons[] = {
            { IconId::Select,    "cursor-click" },
            { IconId::Move,      "arrows-out" },
            { IconId::Text,      "text-t" },
            { IconId::ZoomIn,    "magnifying-glass-plus" },
            { IconId::ZoomOut,   "magnifying-glass-minus" },
            { IconId::Pan,       "hand" },
            { IconId::Color,     "palette" },
            { IconId::Grid,      "grid-four" },
            { IconId::New,       "file-plus" },
            { IconId::Open,      "folder-open" },
            { IconId::Save,      "floppy-disk" },
            { IconId::Undo,      "arrows-counter-clockwise" },
            { IconId::Redo,      "arrows-clockwise" },
            { IconId::Copy,      "copy" },
            { IconId::Paste,     "clipboard-text" },
            { IconId::Trash,     "trash" },
            { IconId::Gear,      "gear" },
            { IconId::Question,  "question" },
            { IconId::Plus,      "plus" },
            { IconId::Check,     "check" },
            { IconId::X,         "x" },
            { IconId::Eye,       "eye" },
            { IconId::Lock,      "lock" },
            { IconId::Image,     "image" },
            { IconId::Palette,   "palette" },
            { IconId::Sparkle,   "sparkle" },
            { IconId::Heart,     "heart" },
            { IconId::Warning,   "warning" },
            { IconId::CaretDown, "caret-down" },
            { IconId::CaretRight,"caret-right" },
            { IconId::Download,  "download-simple" },
            { IconId::Hierarchy, "squares-four" },
            { IconId::Inspector, "sliders" },
            { IconId::Library,   "squares-four" },
            { IconId::Directives,"text-aa" },
            { IconId::Assets,    "image" },
            { IconId::History,   "clock-counter-clockwise" },
            { IconId::Model,     "squares-four" },
            { IconId::Magnifier, "magnifying-glass" },
            { IconId::List,      "list" },
            { IconId::Annotate,  "chat-circle-dots" },
        };

        const char* kIconDirs[] = {
            "assets/icons/",
            "../../assets/icons/",
            "../assets/icons/",
        };

        std::vector<Texture2D> gTextures;

        std::string ResolveIconPath(const char* name)
        {
            for (const char* dir : kIconDirs)
            {
                std::string p = std::string(dir) + name + ".svg";
                if (FileExists(p.c_str())) return p;
            }
            return {};
        }

        std::string ReadFile(const std::string& path)
        {
            std::ifstream f(path, std::ios::binary);
            if (!f) return {};
            std::ostringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }

        std::string ReplaceAll(std::string s, const std::string& from, const std::string& to)
        {
            size_t pos = 0;
            while ((pos = s.find(from, pos)) != std::string::npos)
            {
                s.replace(pos, from.size(), to);
                pos += to.size();
            }
            return s;
        }

        // OBSERVAÇÃO SOBRE ORIENTAÇÃO:
        // O nanosvg rasteriza a imagem de cima para baixo (linha 0 = topo do
        // SVG). No backend OpenGL do ImGui, a primeira linha enviada à GPU
        // aparece no TOPO da imagem desenhada. Portanto NÃO devemos inverter
        // as linhas aqui — fazer isso deixa os ícones de cabeça para baixo.

        ImTextureID Tex(IconId id)
        {
            const int idx = (int)id;
            if (idx < 0 || idx >= (int)gTextures.size()) return 0;
            return (ImTextureID)(intptr_t)gTextures[idx].id;
        }
    }

    void Load()
    {
        for (const IconDef& def : kIcons)
        {
            Texture2D tex = { 0, 0, 0, 0, 0 };

            const std::string path = ResolveIconPath(def.file);
            std::string svg = path.empty() ? std::string() : ReadFile(path);

            if (!svg.empty())
            {
                svg = ReplaceAll(std::move(svg), "currentColor", "#ececec");
                NSVGimage* img = nsvgParse(svg.data(), "px", 96.0f);
                if (img)
                {
                    const float iw = img->width;
                    const float ih = img->height;
                    const float scale = 48.0f / iw; // viewBox 256 -> 48px
                    const int rw = (int)(iw * scale + 0.5f);
                    const int rh = (int)(ih * scale + 0.5f);

                    std::vector<unsigned char> rgba((size_t)rw * rh * 4, 0);
                    NSVGrasterizer* rast = nsvgCreateRasterizer();
                    if (rast)
                    {
                        nsvgRasterize(rast, img, 0.0f, 0.0f, scale, rgba.data(), rw, rh, rw * 4);
                        nsvgDeleteRasterizer(rast);
                    }
                    nsvgDelete(img);

                    Image im = {};
                    im.data = rgba.data();
                    im.width = rw;
                    im.height = rh;
                    im.mipmaps = 1;
                    im.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
                    tex = LoadTextureFromImage(im);

                    TraceLog(LOG_INFO, "Icone carregado: %s (%dx%d)", def.file, rw, rh);
                }
                else
                {
                    TraceLog(LOG_WARNING, "Icone invalido: %s", def.file);
                }
            }
            else
            {
                TraceLog(LOG_WARNING, "Icone ausente: %s", def.file);
            }

            gTextures.push_back(tex);
        }
    }

    void Unload()
    {
        for (Texture2D& t : gTextures)
        {
            if (t.id != 0) UnloadTexture(t);
        }
        gTextures.clear();
    }

    bool IconButton(IconId id, const char* tooltip, float buttonSize)
    {
        const ImTextureID tex = Tex(id);
        bool clicked = false;

        if (tex)
        {
            clicked = ImGui::Button("##icon", ImVec2(buttonSize, buttonSize));
            const ImVec2 p = ImGui::GetItemRectMin();
            const float icon = buttonSize * 0.55f;
            const ImVec2 ip(p.x + (buttonSize - icon) * 0.5f,
                            p.y + (buttonSize - icon) * 0.5f);
            ImGui::GetWindowDrawList()->AddImage(tex, ip, ImVec2(ip.x + icon, ip.y + icon));
        }
        else
        {
            clicked = ImGui::Button("?", ImVec2(buttonSize, buttonSize));
        }

        if (tooltip && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tooltip);

        return clicked;
    }
}
