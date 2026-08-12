#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#include "Icons.h"

#include "Theme.h"
#include "imgui.h"
#include "raylib.h"

#include <algorithm>
#include <cmath>
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
            { IconId::Ruler,     "ruler" },
            { IconId::Pan,       "hand" },
            { IconId::Color,     "eyedropper" },
            { IconId::Grid,      "grid-four" },
            { IconId::New,       "file-plus" },
            { IconId::Open,      "folder-open" },
            { IconId::Save,      "floppy-disk" },
            { IconId::Undo,      "arrows-counter-clockwise" },
            { IconId::Redo,      "arrows-clockwise" },
            { IconId::Copy,      "copy" },
            { IconId::Paste,     "clipboard-text" },
            { IconId::Duplicate, "copy-plus" },
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
            { IconId::Magnet,    "magnet" },
            { IconId::List,      "list" },
            { IconId::Annotate,  "chat-circle-dots" },
            { IconId::Rectangle, "rectangle" },
            { IconId::Ellipse,   "ellipse" },
            { IconId::Polygon,   "polygon" },
            { IconId::Slash,     "slash" },
            { IconId::PenTool,   "pen-tool" },
            { IconId::Contour,   "contour" },
            { IconId::FlipH,     "flip-h" },
            { IconId::FlipV,     "flip-v" },
            { IconId::Transparency, "transparency" },
            { IconId::CornersOut, "corners-out" },
        };

        const char* kIconDirs[] = {
            "assets/icons/",
            "../../assets/icons/",
            "../assets/icons/",
        };

        std::vector<Texture2D> gTextures;

        std::string ResolveIconPath(const char* name)
        {
            // O diretório de trabalho muda conforme o app é aberto. O build
            // copia assets ao lado do executável, que é a âncora confiável.
            const std::string appPath = std::string(GetApplicationDirectory()) +
                                        "assets/icons/" + name + ".svg";
            if (FileExists(appPath.c_str())) return appPath;

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

        void StrengthenAlpha(std::vector<unsigned char>& rgba, int width, int height)
        {
            // Os SVGs thin perdem definição quando reduzidos para 18–23 px.
            // Uma expansão de 1 px no bitmap de 96 px preserva o desenho e
            // produz um traço final nítido, sem transformar o estilo em bold.
            std::vector<unsigned char> source = rgba;
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    unsigned char alpha = 0;
                    for (int oy = -1; oy <= 1; ++oy)
                    {
                        const int sy = y + oy;
                        if (sy < 0 || sy >= height) continue;
                        for (int ox = -1; ox <= 1; ++ox)
                        {
                            const int sx = x + ox;
                            if (sx < 0 || sx >= width) continue;
                            const size_t si = ((size_t)sy * width + sx) * 4;
                            alpha = std::max(alpha, source[si + 3]);
                        }
                    }
                    const size_t di = ((size_t)y * width + x) * 4;
                    if (alpha > rgba[di + 3])
                    {
                        rgba[di + 0] = 255;
                        rgba[di + 1] = 255;
                        rgba[di + 2] = 255;
                        rgba[di + 3] = alpha;
                    }
                }
            }
        }

        ImTextureID Tex(IconId id)
        {
            const int idx = (int)id;
            if (idx < 0 || idx >= (int)gTextures.size()) return 0;
            return (ImTextureID)(intptr_t)gTextures[idx].id;
        }
    }

    void Load()
    {
        // Recarregar não pode duplicar a tabela nem deslocar IconId.
        Unload();
        gTextures.reserve((size_t)IconId::Count);

        for (const IconDef& def : kIcons)
        {
            Texture2D tex = { 0, 0, 0, 0, 0 };

            const std::string path = ResolveIconPath(def.file);
            std::string svg = path.empty() ? std::string() : ReadFile(path);

            if (!svg.empty())
            {
                svg = ReplaceAll(std::move(svg), "currentColor", "#ffffff");
                NSVGimage* img = nsvgParse(svg.data(), "px", 96.0f);
                if (img)
                {
                    const float iw = img->width;
                    const float ih = img->height;
                    const float scale = 96.0f / std::max(iw, ih); // alta resolução para redução nítida
                    const int rw = (int)(iw * scale + 0.5f);
                    const int rh = (int)(ih * scale + 0.5f);

                    std::vector<unsigned char> rgba((size_t)rw * rh * 4, 0);
                    NSVGrasterizer* rast = nsvgCreateRasterizer();
                    if (rast)
                    {
                        nsvgRasterize(rast, img, 0.0f, 0.0f, scale, rgba.data(), rw, rh, rw * 4);
                        StrengthenAlpha(rgba, rw, rh);
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
                    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);

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

    void DrawIcon(IconId id, float size, ImU32 tint)
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        DrawIconAt(id, p.x, p.y, size, tint);
        ImGui::Dummy(ImVec2(size, size));
    }

    void DrawIconAt(IconId id, float x, float y, float size, ImU32 tint)
    {
        const ImTextureID tex = Tex(id);
        if (!tex)
        {
            ImGui::GetWindowDrawList()->AddText(ImVec2(x, y), IM_COL32(236, 236, 236, 255), "?");
            return;
        }

        ImGui::GetWindowDrawList()->AddImage(tex, ImVec2(x, y),
                                             ImVec2(x + size, y + size),
                                             ImVec2(0, 0), ImVec2(1, 1), tint);
    }

    bool IconButton(IconId id, const char* tooltip, float buttonSize, ImU32 tint)
    {
        const ImTextureID tex = Tex(id);
        bool clicked = false;

        ImGui::PushID((int)id);
        if (tex)
        {
            clicked = ImGui::Button("##icon", ImVec2(buttonSize, buttonSize));
            const ImVec2 p = ImGui::GetItemRectMin();
            const float icon = std::round(buttonSize * 0.70f);
            const ImVec2 ip(std::round(p.x + (buttonSize - icon) * 0.5f),
                            std::round(p.y + (buttonSize - icon) * 0.5f));
            ImGui::GetWindowDrawList()->AddImage(tex, ip, ImVec2(ip.x + icon, ip.y + icon),
                                                 ImVec2(0, 0), ImVec2(1, 1), tint);
        }
        else
        {
            clicked = ImGui::Button("?", ImVec2(buttonSize, buttonSize));
        }

        if (tooltip && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tooltip);

        ImGui::PopID();
        return clicked;
    }
}
