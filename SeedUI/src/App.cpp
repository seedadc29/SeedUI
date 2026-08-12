// Não incluímos <windows.h> inteiro: nomes dele (CloseWindow, ShowCursor, DrawText...)
// colidem com funções do raylib. Declaramos apenas o que realmente usamos.
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Dwmapi.lib")
extern "C"
{
    // HINSTANCE é um ponteiro; retornamos void* (o valor não é usado)
    __declspec(dllimport) void* ShellExecuteA(void* hwnd, const char* operation,
                                              const char* file, const char* parameters,
                                              const char* directory, int showCmd);
    __declspec(dllimport) int DwmSetWindowAttribute(void* hwnd, unsigned int attribute,
                                                    const void* value, unsigned int valueSize);
}

#include "App.h"

#include "FileDialogs.h"
#include "Theme.h"
#include "Icons.h"
#include "Manual.h"
#include "Canvas.h"
#include "ColorPicker.h"
#include "ColorUtils.h"
#include "Geo.h"
#include "SmartGuides.h"
#include "AlignUtils.h"

#include "raylib.h"
#include "imgui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <direct.h>

namespace seedui
{
    namespace
    {
        constexpr int kWindowWidth = 1600;
        constexpr int kWindowHeight = 900;

        FILE* gLogFile = nullptr;

        void TraceLogToFile(int logLevel, const char* text, va_list args)
        {
            char buf[2048];
            vsnprintf(buf, sizeof buf, text, args);
            if (gLogFile)
            {
                fprintf(gLogFile, "[%d] %s\n", logLevel, buf);
                fflush(gLogFile);
            }
            printf("%s\n", buf);
        }

        constexpr float kToolbarWidth = 50.0f;
        constexpr float kActionBarHeight = 42.0f;
        constexpr float kPropertyBarHeight = 30.0f; // barra contextual (CorelDRAW)
        constexpr float kStatusBarHeight = 34.0f;
        constexpr float kColorBarHeight = 34.0f; // paleta de cores inferior (CorelDRAW)
        constexpr float kToolButtonSize = 32.0f;
        constexpr float kRightPanelMinWidth = 260.0f;
        constexpr float kRightPanelMaxWidth = 520.0f;
        constexpr float kRightRailWidth = 50.0f;

        // Cores de família da barra de ferramentas (identidade premium).
        constexpr ImU32 kFamilySelect     = IM_COL32(0x4f, 0x8c, 0xff, 255);
        constexpr ImU32 kFamilyCreate     = IM_COL32(0x2e, 0xcc, 0x71, 255);
        constexpr ImU32 kFamilyAppearance = IM_COL32(0xa7, 0x8b, 0xfa, 255);
        constexpr ImU32 kFamilyNav        = IM_COL32(0xa0, 0xa0, 0xa0, 255);
        constexpr ImU32 kFamilyReview     = IM_COL32(0xf5, 0x79, 0x00, 255);

        void EnableDarkTitleBar()
        {
            void* hwnd = GetWindowHandle();
            if (!hwnd) return;

            const int enabled = 1;
            DwmSetWindowAttribute(hwnd, 20, &enabled, sizeof enabled);
            DwmSetWindowAttribute(hwnd, 19, &enabled, sizeof enabled);

            const unsigned int dark = 0x001e1e1e;
            DwmSetWindowAttribute(hwnd, 35, &dark, sizeof dark);
            DwmSetWindowAttribute(hwnd, 36, &dark, sizeof dark);
        }

        void MenuItemSoon(const char* label, const char* milestone)
        {
            ImGui::MenuItem(label, milestone, false, false);
        }

        void PanelHint(const char* text)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDisabled);
            ImGui::TextWrapped("%s", text);
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        bool AlignmentIconButton(int operation, const char* tooltip)
        {
            ImGui::PushID(operation);
            const ImVec2 size(30.0f, 32.0f); // altura da barra de ações (centro alinhado)
            const bool clicked = ImGui::InvisibleButton("##align", size);
            const bool hovered = ImGui::IsItemHovered();
            const ImVec2 a = ImGui::GetItemRectMin();
            const ImVec2 b = ImGui::GetItemRectMax();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImU32 bg = ImGui::ColorConvertFloat4ToU32(
                hovered ? Theme::BorderLight : Theme::BackgroundChild);
            const ImU32 border = ImGui::ColorConvertFloat4ToU32(Theme::Border);
            const ImU32 guide = ImGui::ColorConvertFloat4ToU32(Theme::AccentBlue);
            const ImU32 shape = ImGui::ColorConvertFloat4ToU32(Theme::TextSecondary);
            dl->AddRectFilled(a, b, bg, 3.0f);
            dl->AddRect(a, b, border, 3.0f);

            const float cx = (a.x + b.x) * 0.5f;
            const float cy = (a.y + b.y) * 0.5f;
            if (operation <= 2)
            {
                const float gx = operation == 0 ? a.x + 7.0f
                               : operation == 1 ? cx : b.x - 7.0f;
                dl->AddLine(ImVec2(gx, a.y + 5.0f), ImVec2(gx, b.y - 5.0f), guide, 1.5f);
                const float x1 = operation == 0 ? gx + 2.0f
                               : operation == 1 ? gx - 8.0f : gx - 10.0f;
                const float x2 = operation == 0 ? gx + 10.0f
                               : operation == 1 ? gx + 8.0f : gx - 2.0f;
                dl->AddRect(ImVec2(x1, cy - 8.0f), ImVec2(x2, cy - 2.0f), shape);
                const float sx1 = operation == 0 ? gx + 2.0f
                                : operation == 1 ? gx - 5.0f : gx - 7.0f;
                const float sx2 = operation == 0 ? gx + 7.0f
                                : operation == 1 ? gx + 5.0f : gx - 2.0f;
                dl->AddRect(ImVec2(sx1, cy + 2.0f), ImVec2(sx2, cy + 8.0f), shape);
            }
            else
            {
                const int vertical = operation - 3;
                const float gy = vertical == 0 ? a.y + 6.0f
                               : vertical == 1 ? cy : b.y - 6.0f;
                dl->AddLine(ImVec2(a.x + 6.0f, gy), ImVec2(b.x - 6.0f, gy), guide, 1.5f);
                const float y1 = vertical == 0 ? gy + 2.0f
                               : vertical == 1 ? gy - 8.0f : gy - 10.0f;
                const float y2 = vertical == 0 ? gy + 10.0f
                               : vertical == 1 ? gy + 8.0f : gy - 2.0f;
                dl->AddRect(ImVec2(cx - 8.0f, y1), ImVec2(cx - 2.0f, y2), shape);
                const float sy1 = vertical == 0 ? gy + 2.0f
                                : vertical == 1 ? gy - 5.0f : gy - 7.0f;
                const float sy2 = vertical == 0 ? gy + 7.0f
                                : vertical == 1 ? gy + 5.0f : gy - 2.0f;
                dl->AddRect(ImVec2(cx + 2.0f, sy1), ImVec2(cx + 8.0f, sy2), shape);
            }
            if (hovered) ImGui::SetTooltip("%s", tooltip);
            ImGui::PopID();
            return clicked;
        }

        Tool ToolFromIcon(IconId id)
        {
            switch (id)
            {
            case IconId::Select: return Tool::Select;
            case IconId::Move:   return Tool::Move;
            case IconId::Text:   return Tool::Text;
            case IconId::ZoomIn: return Tool::Zoom;
            case IconId::Pan:    return Tool::Pan;
            case IconId::Rectangle: return Tool::Rectangle;
            case IconId::Ellipse: return Tool::Ellipse;
            case IconId::Polygon: return Tool::Polygon;
            case IconId::Slash:  return Tool::Line;
            case IconId::PenTool: return Tool::Pen;
            case IconId::Color:  return Tool::Color;
            case IconId::Grid:   return Tool::Grid;
            case IconId::Ruler:  return Tool::Measure;
            case IconId::Annotate: return Tool::Annotate;
            default:             return Tool::Select;
            }
        }

        std::string LowerAscii(std::string value)
        {
            for (char& c : value) c = (char)std::tolower((unsigned char)c);
            return value;
        }

        bool ElementMatchesSearch(const Element& element, const char* query)
        {
            if (!query || !query[0]) return true;
            const std::string needle = LowerAscii(query);
            if (LowerAscii(element.nome).find(needle) != std::string::npos ||
                LowerAscii(element.id).find(needle) != std::string::npos ||
                LowerAscii(element.tipo).find(needle) != std::string::npos)
                return true;
            for (const Element& child : element.filhos)
                if (ElementMatchesSearch(child, query)) return true;
            return false;
        }

        void CollectElementsInRect(const std::vector<Element>& elements,
                                   float left, float top, float right, float bottom,
                                   std::vector<std::string>& out, bool containOnly)
        {
            for (const Element& element : elements)
            {
                if (!element.visivel) continue;
                const float x = element.transformacao.value("x", 0.0f);
                const float y = element.transformacao.value("y", 0.0f);
                const float w = element.transformacao.value("largura", 160.0f);
                const float h = element.transformacao.value("altura", 32.0f);
                // containOnly: o corpo INTEIRO do elemento precisa estar dentro
                // da caixa; senão, qualquer toque já seleciona (modo opcional).
                const bool contained = containOnly
                    ? (x >= left && x + w <= right && y >= top && y + h <= bottom)
                    : (x < right && x + w > left && y < bottom && y + h > top);
                if (contained)
                    out.push_back(element.id);
                CollectElementsInRect(element.filhos, left, top, right, bottom,
                                      out, containOnly);
            }
        }

        template<typename Start>
        void CollectTransformStarts(Element& element, std::vector<Start>& out)
        {
            const bool exists = std::any_of(out.begin(), out.end(), [&](const auto& start)
            {
                return start.id == element.id;
            });
            // Grupos NÃO entram como alvos de transformação (são contêineres
            // sem corpo): apenas os filhos reais — assim mover/redimensionar/
            // rotacionar um grupo opera o conjunto inteiro como unidade.
            if (!exists && !element.bloqueado && element.tipo != "grupo")
            {
                Start start;
                start.id = element.id;
                start.x = element.transformacao.value("x", 0.0f);
                start.y = element.transformacao.value("y", 0.0f);
                start.w = element.transformacao.value("largura", 160.0f);
                start.h = element.transformacao.value("altura", 32.0f);
                start.rot = Geo::ElementRotation(element);
                out.push_back(start);
            }
            for (Element& child : element.filhos) CollectTransformStarts(child, out);
        }

        // Recalcula a caixa (x/y/largura/altura) de cada GRUPO do modo a partir
        // dos filhos visíveis — após mover/redimensionar/rotacionar em conjunto,
        // a caixa do grupo deve voltar a envolver os filhos.
        void RebuildGroupBounds(Modo& mode)
        {
            auto rebuild = [](Element& element, auto& self) -> void
            {
                for (Element& child : element.filhos)
                    self(child, self);
                if (element.tipo != "grupo") return;
                float left = FLT_MAX, top = FLT_MAX, right = -FLT_MAX, bottom = -FLT_MAX;
                bool any = false;
                for (const Element& child : element.filhos)
                {
                    if (!child.visivel) continue;
                    float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
                    Geo::RotatedAABB(child, x0, y0, x1, y1);
                    left = std::min(left, x0);
                    top = std::min(top, y0);
                    right = std::max(right, x1);
                    bottom = std::max(bottom, y1);
                    any = true;
                }
                if (!any) return;
                element.transformacao["x"] = left;
                element.transformacao["y"] = top;
                element.transformacao["largura"] = right - left;
                element.transformacao["altura"] = bottom - top;
            };
            for (Element& root : mode.raiz)
                rebuild(root, rebuild);
        }

        const char* CornerKeyForDragMode(int dragMode)
        {
            switch (dragMode)
            {
            case 10: return "superior_esquerda";
            case 11: return "superior_direita";
            case 12: return "inferior_direita";
            case 13: return "inferior_esquerda";
            default: return nullptr;
            }
        }

        float ElementCornerRadius(const Element& element, int dragMode)
        {
            const char* key = CornerKeyForDragMode(dragMode);
            if (!key) return 0.0f;
            float uniform = 0.0f;
            if (element.estilos.is_object())
            {
                const auto uniformIt = element.estilos.find("raio");
                if (uniformIt != element.estilos.end() && uniformIt->is_number())
                    uniform = std::max(0.0f, uniformIt->get<float>());
                const auto cornersIt = element.estilos.find("raio_quinas");
                if (cornersIt != element.estilos.end() && cornersIt->is_object())
                {
                    const auto valueIt = cornersIt->find(key);
                    if (valueIt != cornersIt->end() && valueIt->is_number())
                        return std::max(0.0f, valueIt->get<float>());
                }
            }
            return uniform;
        }

        void SetElementCornerRadius(Element& element, int dragMode, float radius)
        {
            const char* key = CornerKeyForDragMode(dragMode);
            if (!key) return;
            if (!element.estilos.is_object()) element.estilos = nlohmann::json::object();
            if (!element.estilos.contains("raio_quinas") ||
                !element.estilos["raio_quinas"].is_object())
                element.estilos["raio_quinas"] = nlohmann::json::object();
            element.estilos["raio_quinas"][key] = std::max(0.0f, radius);
        }

        void ClampElementCornerRadii(Element& element)
        {
            const float width = element.transformacao.value("largura", 160.0f);
            const float height = element.transformacao.value("altura", 32.0f);
            const float maximum = std::max(0.0f, std::min(width, height) * 0.5f);
            for (int mode = 10; mode <= 13; ++mode)
                SetElementCornerRadius(element, mode,
                    std::min(maximum, ElementCornerRadius(element, mode)));
        }

        // Aplica um estilo (ex.: cor) ao elemento e, se for grupo, a TODOS os
        // descendentes — o grupo é só um contêiner (não desenha corpo).
        void ApplyStyleToTree(Element& element, const char* key, const std::string& value)
        {
            if (element.tipo != "grupo")
                element.estilos[key] = value;
            for (Element& child : element.filhos)
                ApplyStyleToTree(child, key, value);
        }

        // Aplica um estilo a todos os elementos selecionados, descendo nos
        // grupos. Retorna quantos elementos receberam o estilo.
        int ApplyStyleToSelection(Modo& mode, const std::vector<std::string>& ids,
                                  const char* key, const std::string& value,
                                  bool& outBlocked)
        {
            int applied = 0;
            for (const std::string& id : ids)
            {
                Element* element = Project::ResolverId(mode, id);
                if (!element || element->bloqueado) { outBlocked = true; continue; }
                ApplyStyleToTree(*element, key, value);
                ++applied;
            }
            return applied;
        }

        void DrawDashedLine(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 color)
        {
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float length = sqrtf(dx * dx + dy * dy);
            if (length <= 0.0f) return;
            const float ux = dx / length;
            const float uy = dy / length;
            for (float p = 0.0f; p < length; p += 10.0f)
            {
                const float end = std::min(length, p + 6.0f);
                dl->AddLine(ImVec2(a.x + ux * p, a.y + uy * p),
                            ImVec2(a.x + ux * end, a.y + uy * end), color, 1.5f);
            }
        }

        void DrawDashedRect(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 color)
        {
            const ImVec2 mn(std::min(a.x, b.x), std::min(a.y, b.y));
            const ImVec2 mx(std::max(a.x, b.x), std::max(a.y, b.y));
            DrawDashedLine(dl, mn, ImVec2(mx.x, mn.y), color);
            DrawDashedLine(dl, ImVec2(mx.x, mn.y), mx, color);
            DrawDashedLine(dl, mx, ImVec2(mn.x, mx.y), color);
            DrawDashedLine(dl, ImVec2(mn.x, mx.y), mn, color);
        }

        // Aplica um deslocamento de posição no elemento E em todos os
        // descendentes (coordenadas absolutas): necessário ao mover grupos
        // por alinhamento/distribuição, senão só a caixa do grupo anda.
        void ApplyPositionDelta(Element& element, float dx, float dy)
        {
            element.transformacao["x"] =
                element.transformacao.value("x", 0.0f) + dx;
            element.transformacao["y"] =
                element.transformacao.value("y", 0.0f) + dy;
            for (Element& child : element.filhos)
                ApplyPositionDelta(child, dx, dy);
        }

    }    void App::Run(bool captureAfterBoot)
    {
        mCaptureAfterBoot = captureAfterBoot;
        if (captureAfterBoot) setvbuf(stdout, nullptr, _IONBF, 0);

        // Log em arquivo (diagnóstico quando aberto pelo Explorer, sem console)
        gLogFile = fopen("seedui.log", "w");
        SetTraceLogCallback(TraceLogToFile);
        TraceLog(LOG_INFO, "SeedUI iniciando...");

        // Janela (raylib) — nunca maior que o monitor atual (evita janela preta
        // em telas pequenas/placas Intel)
        SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
        // GetMonitorWidth() do raylib retorna 0 antes do InitWindow — consultamos
        // o monitor diretamente via GLFW (glfwInit é idempotente).
        glfwInit();
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        const int monitorW = mode ? mode->width : 1280;
        const int monitorH = mode ? mode->height : 720;
        // Janela um pouco menor que o monitor: deixa a barra de tarefas visível
        // (não "prende" o usuário) e permite maximizar/restaurar normalmente.
        const int capW = monitorW * 9 / 10;
        const int capH = monitorH * 9 / 10;
        const int winW = (kWindowWidth > capW) ? capW : kWindowWidth;
        const int winH = (kWindowHeight > capH) ? capH : kWindowHeight;
        TraceLog(LOG_INFO, "JANELA: monitor %dx%d, janela %dx%d", monitorW, monitorH, winW, winH);
        InitWindow(winW, winH, "SeedUI - Editor Visual de Interfaces");
        EnableDarkTitleBar();
        // A janela NUNCA pode ultrapassar a área de trabalho do monitor: se o
        // usuário redimensionar (ou MAXIMIZAR) além dela, a parte inferior
        // (barra de status + paleta de cores) fica escondida atrás da barra de
        // tarefas e "sai da janela principal". O limite vale tanto no resize
        // quanto no maximize (no Windows, o WM_GETMINMAXINFO usa esses limites
        // como ptMaxTrackSize).
        int workX = 0, workY = 0, workW = monitorW, workH = monitorH;
        glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &workX, &workY, &workW, &workH);
        // Largura e altura até a ÁREA DE TRABALHO completa (o GLFW reporta o
        // monitor inteiro — 768 no monitor 1366x768): maximizar preenche a
        // tela toda, e a janela nunca pode ser redimensionada maior que o
        // monitor (o que antes empurrava o rodapé para fora da tela — as
        // anotações antigas em y=1018 num print de 745 eram uma janela
        // esticada além do monitor). O ptMaxTrackSize do Windows usa esses
        // limites para o resize; o maximize respeita o tamanho máximo.
        SetWindowMinSize(std::min(1024, workW), std::min(600, workH));
        SetWindowMaxSize(workW, workH);
        TraceLog(LOG_INFO, "JANELA: área de trabalho %dx%d, limite máx. %dx%d",
                 workW, workH, workW, workH);
        SetExitKey(0);
        SetTargetFPS(60);
        SetTextureFilter(GetFontDefault().texture, TEXTURE_FILTER_BILINEAR);

        // ImGui (mesmos backends da engine: GLFW + OpenGL 3.3)
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        Theme::Apply();
        Theme::LoadFonts();
        ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
        ImGui_ImplOpenGL3_Init("#version 330");

        Load();
        if (mCaptureAfterBoot) TraceLog(LOG_INFO, "CAPTURE: entrando no loop");

        while (mRunning && !WindowShouldClose())
        {
            if (mCaptureAfterBoot && mFrameCount == 0) TraceLog(LOG_INFO, "CAPTURE: frame 0 - NewFrame");
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            if (mCaptureAfterBoot && mFrameCount == 0) TraceLog(LOG_INFO, "CAPTURE: NewFrame ok");

            DrawWorkspace();
            if (mHasProject && !ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
                !ImGui::IsAnyItemActive())
                CapturarHistorico();
            if (mCaptureAfterBoot && mFrameCount == 0) TraceLog(LOG_INFO, "CAPTURE: DrawWorkspace ok");

            ImGui::Render();
            BeginDrawing();
            ClearBackground(Theme::RaylibWindowBg);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if (mPendingExport)
            {
                mPendingExport = false;
                DoExportDirectives();
            }
            if (mCaptureAfterBoot && mFrameCount == 0) TraceLog(LOG_INFO, "CAPTURE: primeiro frame renderizado");
            if (mCaptureAfterBoot)
            {
                ++mFrameCount;
                if (mFrameCount == 30)
                {
                    TraceLog(LOG_INFO, "CAPTURE: capturando tela inicial");
                    TakeScreenshot("seedui_capture_start.png");
                    mProject.CriarNovo("Captura automatica", 1280, 720);
                    mHasProject = true; // segunda captura mostra um workspace valido
                    ResetarHistorico();
                    AdicionarComponente("painel", "Painel de validacao", 360.0f, 220.0f);
                    if (Element* panel = Project::ResolverId(mProject, mSelectedElementId))
                    {
                        panel->estilos["raio_quinas"] = {
                            { "superior_esquerda", 48.0f },
                            { "superior_direita", 12.0f },
                            { "inferior_direita", 72.0f },
                            { "inferior_esquerda", 28.0f }
                        };
                        mCornerSelectionElementId = panel->id;
                        mSelectedCornerMask = 0x0Fu;
                    }
                }
                else if (mFrameCount == 60)
                {
                    TraceLog(LOG_INFO, "CAPTURE: capturando workspace");
                    TakeScreenshot("seedui_capture_workspace.png");
                    mRunning = false;
                }
            }
            EndDrawing();
            if (mCaptureAfterBoot && mFrameCount == 1) TraceLog(LOG_INFO, "CAPTURE: swap concluido");
        }
        if (mCaptureAfterBoot) TraceLog(LOG_INFO, "CAPTURE: loop encerrado");

        Unload();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        if (IsWindowReady()) CloseWindow();

        if (gLogFile)
        {
            fclose(gLogFile);
            gLogFile = nullptr;
        }
    }

    void App::ExportDirectives()
    {
        mPendingExport = true;
    }

    void App::DoExportDirectives()
    {
        // Pasta "diretrizes" ao lado do executável
        std::string dir = GetApplicationDirectory();
        dir += "diretrizes";
        _mkdir(dir.c_str()); // falha silenciosamente se já existir

        const time_t t = time(nullptr);
        struct tm tmv;
        localtime_s(&tmv, &t);
        char ts[32];
        strftime(ts, sizeof ts, "%Y%m%d_%H%M%S", &tmv);

        char pngName[64];
        snprintf(pngName, sizeof pngName, "print_%s.png", ts);
        const std::string txtPath = dir + "/diretrizes_" + ts + ".txt";
        const std::string pngPath = dir + "/" + pngName;

        char head[512];
        snprintf(head, sizeof head,
                 "=============================================\n"
                 "DIRETRIZES GERADAS EM %02d/%02d/%04d %02d:%02d:%02d\n"
                 "Captura de tela: %s (na pasta diretrizes/)\n"
                 "Coordenadas = posição na janela do SeedUI (x → direita, y → baixo).\n"
                 "=============================================\n\n",
                 tmv.tm_mday, tmv.tm_mon + 1, tmv.tm_year + 1900,
                 tmv.tm_hour, tmv.tm_min, tmv.tm_sec, pngName);

        const std::string content = head + AnnotationsToText(mAnnot, mGlobalDirectives);

        FILE* f = fopen(txtPath.c_str(), "w");
        if (f)
        {
            fputs(content.c_str(), f);
            fclose(f);
            TraceLog(LOG_INFO, "Diretrizes exportadas: %s", txtPath.c_str());
        }
        else
        {
            TraceLog(LOG_WARNING, "Não foi possível gravar %s", txtPath.c_str());
        }

        TakeScreenshot(pngPath.c_str());

        // Mensagem curta: a pasta já abre sozinha; o caminho completo fica no log
        mStatusMsg = "Diretrizes exportadas em Build/Debug/diretrizes";
        mStatusMsgUntil = GetTime() + 8.0;

        // Abre a pasta para o usuário ver o .txt e o print (SW_SHOWDEFAULT = 10)
        ShellExecuteA(nullptr, "open", dir.c_str(), nullptr, nullptr, 10);
    }

    void App::NovoProjeto()
    {
        mShowNovoProjeto = true;
    }

    void App::AbrirProjeto()
    {
        const std::string path = AbrirDialogoProjeto();
        if (path.empty()) return; // cancelado

        std::string texto;
        FILE* f = fopen(path.c_str(), "rb");
        if (!f)
        {
            mStatusMsg = "Não foi possível abrir o arquivo.";
            mStatusMsgUntil = GetTime() + 6.0;
            return;
        }
        char buf[65536];
        size_t n;
        while ((n = fread(buf, 1, sizeof buf, f)) > 0) texto.append(buf, n);
        fclose(f);

        Project novo;
        const std::string erro = Project::Desserializar(novo, texto);
        if (!erro.empty())
        {
            mStatusMsg = "Erro ao abrir: " + erro;
            mStatusMsgUntil = GetTime() + 8.0;
            TraceLog(LOG_ERROR, "%s", erro.c_str());
            return;
        }

        mProject = std::move(novo);
        mProject.caminhoArquivo = path;
        mUltimoCaminho = path;
        mHasProject = true;
        ResetarHistorico();
        mProjectDirty = false;
        mTelaAtiva = 0;
        mModoAtivo = 0;
        mSelectedElementId.clear();
        mStatusMsg = "Projeto aberto: " + mProject.nome;
        mStatusMsgUntil = GetTime() + 6.0;
        TraceLog(LOG_INFO, "Projeto aberto: %s (telas=%d)", path.c_str(),
                 (int)mProject.telas.size());
    }

    void App::SalvarProjeto(bool salvarComo)
    {
        if (!mHasProject) return;

        std::string path = mUltimoCaminho;
        if (salvarComo || path.empty())
        {
            std::string sugerido = mProject.nome;
            if (sugerido.empty()) sugerido = "projeto";
            path = SalvarDialogoProjeto(sugerido);
            if (path.empty()) return; // cancelado
        }

        // Garante a extensão .ui.json
        if (path.size() < 8 ||
            path.compare(path.size() - 8, 8, ".ui.json") != 0)
        {
            path += ".ui.json";
        }

        mProject.modificadoEm = Project::StampAtual();
        const std::string js = Project::Serializar(mProject);

        FILE* f = fopen(path.c_str(), "wb");
        if (!f)
        {
            mStatusMsg = "Não foi possível salvar: " + path;
            mStatusMsgUntil = GetTime() + 8.0;
            TraceLog(LOG_ERROR, "Falha ao gravar %s", path.c_str());
            return;
        }
        fputs(js.c_str(), f);
        fclose(f);

        mUltimoCaminho = path;
        mProject.caminhoArquivo = path;
        mProjectDirty = false;
        mStatusMsg = "Salvo: " + path;
        mStatusMsgUntil = GetTime() + 6.0;
        TraceLog(LOG_INFO, "Projeto salvo: %s", path.c_str());
    }

    void App::FecharProjeto()
    {
        if (!mHasProject) return;
        mHasProject = false;
        mProjectDirty = false;
        mProject = Project(); // volta para o estado vazio (tela inicial)
        mUltimoCaminho.clear();
        mTelaAtiva = 0;
        mModoAtivo = 0;
        mSelectedElementId.clear();
        mStatusMsg = "Projeto fechado.";
        mStatusMsgUntil = GetTime() + 5.0;
    }

    void App::MarcarTudoSalvo()
    {
        mProjectDirty = false;
    }

    bool App::PossuiModoAtivo() const
    {
        if (!mHasProject || mTelaAtiva < 0 ||
            mTelaAtiva >= (int)mProject.telas.size())
            return false;
        const Tela& tela = mProject.telas[mTelaAtiva];
        return mModoAtivo >= 0 && mModoAtivo < (int)tela.modos.size();
    }

    bool App::AdicionarComponente(const char* tipo, const char* nome,
                                  float projectX, float projectY)
    {
        if (!mHasProject || mProject.telas.empty())
        {
            TraceLog(LOG_WARNING, "M04: componente recusado: nenhum projeto aberto");
            return false;
        }
        if (mTelaAtiva < 0 || mTelaAtiva >= (int)mProject.telas.size())
        {
            TraceLog(LOG_WARNING, "M04: componente recusado: tela ativa invalida");
            return false;
        }

        Tela& tela = mProject.telas[mTelaAtiva];
        if (tela.modos.empty())
        {
            TraceLog(LOG_WARNING, "M04: componente recusado: tela sem modos");
            return false;
        }
        if (mModoAtivo < 0 || mModoAtivo >= (int)tela.modos.size()) mModoAtivo = 0;

        Modo& modo = tela.modos[mModoAtivo];
        const std::string base = tipo ? tipo : "elemento";
        std::string id;
        for (int i = 1; i < 10000; ++i)
        {
            id = base + "_" + std::to_string(i);
            if (!Project::ResolverId(mProject, id)) break;
        }

        float width = 180.0f;
        float height = 42.0f;
        if (base == "janela" || base == "painel")
        {
            width = 320.0f;
            height = base == "janela" ? 220.0f : 160.0f;
        }
        else if (base == "texto")
        {
            width = 220.0f;
            height = 32.0f;
        }

        const int index = (int)modo.raiz.size();
        const float requestedX = projectX >= 0.0f ? projectX : (float)(80 + (index % 5) * 28);
        const float requestedY = projectY >= 0.0f ? projectY : (float)(80 + (index % 7) * 24);
        const float maxX = std::max(0.0f, (float)mProject.telaBaseLargura - width);
        const float maxY = std::max(0.0f, (float)mProject.telaBaseAltura - height);
        const float initialX = std::max(0.0f, std::min(maxX, requestedX));
        const float initialY = std::max(0.0f, std::min(maxY, requestedY));

        Element e;
        e.id = id;
        e.tipo = base;
        e.nome = nome ? nome : base.c_str();
        e.transformacao = {
            { "x", initialX }, { "y", initialY },
            { "largura", width }, { "altura", height }
        };

        if (base == "texto")
            e.propriedades["texto"] = "Texto";
        else if (base == "botao")
            e.propriedades["texto"] = "Botao";
        else if (base == "campo_numerico")
            e.propriedades["valor"] = 0;
        else if (base == "slider")
        {
            e.propriedades["valor"] = 50;
            e.propriedades["min"] = 0;
            e.propriedades["max"] = 100;
        }
        else if (base == "linha")
        {
            // Linha: sem preenchimento, traço visível por padrão.
            e.estilos["cor_fundo"] = "#00000000";
            e.estilos["cor_borda"] = "#cfcfcf";
            e.estilos["espessura_borda"] = 3.0f;
        }

        modo.raiz.push_back(std::move(e));
        mSelectedElementId = id;
        mSelectedElementIds.clear();
        mSelectedElementIds.push_back(id);
        mProjectDirty = true;
        mStatusMsg = "Componente fixado no canvas: " + id;
        mStatusMsgUntil = GetTime() + 5.0;
        TraceLog(LOG_INFO, "M04: %s fixado em %.1f, %.1f (%0.fx%.0f)",
                 id.c_str(), initialX, initialY, width, height);
        return true;
    }

    void App::AlternarModoAnotacao()
    {
        if (mCurrentTool == Tool::Annotate)
        {
            mCurrentTool = Tool::Select;
            mAnnot.creating = false;
            mAnnot.dragging = -1;
            mAnnot.dragMode = 0;
            mStatusMsg = "Modo de anotacoes desativado";
        }
        else
        {
            mCurrentTool = Tool::Annotate;
            mStatusMsg = "Modo de anotacoes ativado";
        }
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::CopiarElementosSelecionados()
    {
        mElementClipboard.clear();
        mElementPasteGeneration = 0;
        if (!PossuiModoAtivo()) return;

        std::vector<std::string> ids = mSelectedElementIds;
        if (!mSelectedElementId.empty() &&
            std::find(ids.begin(), ids.end(), mSelectedElementId) == ids.end())
            ids.push_back(mSelectedElementId);

        const Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        mElementClipboard = Project::CopiarElementos(mode, ids);
        if (mElementClipboard.empty())
        {
            mStatusMsg = "Selecione um elemento para copiar";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }

        mStatusMsg = std::to_string(mElementClipboard.size()) +
                     " elemento(s) copiado(s)";
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::ColarElementosCopiados()
    {
        if (!PossuiModoAtivo() || mElementClipboard.empty())
        {
            mStatusMsg = "Nada copiado para colar";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }

        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        ++mElementPasteGeneration;
        const float delta = 16.0f * (float)mElementPasteGeneration;
        const std::vector<std::string> pastedRootIds = Project::ColarElementos(
            mProject, mode, mElementClipboard, delta);

        mSelectedElementIds = pastedRootIds;
        mSelectedElementId = pastedRootIds.empty() ? std::string() : pastedRootIds.back();
        mProjectDirty = true;
        mStatusMsg = std::to_string(pastedRootIds.size()) +
                     " elemento(s) colado(s)";
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::DuplicarSelecao()
    {
        if (!PossuiModoAtivo())
        {
            mStatusMsg = "Nenhum modo ativo para duplicar";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }

        std::vector<std::string> ids = mSelectedElementIds;
        if (!mSelectedElementId.empty() &&
            std::find(ids.begin(), ids.end(), mSelectedElementId) == ids.end())
            ids.push_back(mSelectedElementId);
        if (ids.empty())
        {
            mStatusMsg = "Selecione um elemento para duplicar";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }

        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        Element* first = Project::ResolverId(mode, ids.front());
        if (!first) return;
        const float srcX = first->transformacao.value("x", 0.0f);
        const float srcY = first->transformacao.value("y", 0.0f);

        const std::vector<Element> copies = Project::CopiarElementos(mode, ids);
        if (copies.empty()) return;

        ++mElementPasteGeneration;
        // Ctrl+D repete o ÚLTIMO deslocamento aplicado (estilo CorelDRAW):
        // a primeira duplicação usa 16px, as seguintes usam o mesmo passo.
        const std::vector<std::string> pastedRootIds = Project::ColarElementosOffset(
            mProject, mode, copies, mDuplicateDX, mDuplicateDY);

        Element* pasted = pastedRootIds.empty()
            ? nullptr : Project::ResolverId(mode, pastedRootIds.front());
        if (pasted)
        {
            mDuplicateDX = pasted->transformacao.value("x", 0.0f) - srcX;
            mDuplicateDY = pasted->transformacao.value("y", 0.0f) - srcY;
        }

        mSelectedElementIds = pastedRootIds;
        mSelectedElementId = pastedRootIds.empty() ? std::string() : pastedRootIds.back();
        mProjectDirty = true;
        CapturarHistorico();
        mStatusMsg = std::to_string(pastedRootIds.size()) +
                     " elemento(s) duplicado(s)";
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::EspelharSelecao(bool horizontal)
    {
        if (!PossuiModoAtivo())
        {
            mStatusMsg = "Nenhum modo ativo para espelhar";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }
        std::vector<std::string> ids = mSelectedElementIds;
        if (!mSelectedElementId.empty() &&
            std::find(ids.begin(), ids.end(), mSelectedElementId) == ids.end())
            ids.push_back(mSelectedElementId);
        if (ids.empty())
        {
            mStatusMsg = "Selecione um elemento para espelhar";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }
        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        Project::EspelharElementos(mode, ids, horizontal);
        mProjectDirty = true;
        CapturarHistorico();
        mStatusMsg = horizontal ? "Espelhado horizontalmente"
                                : "Espelhado verticalmente";
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::HandleMeasureTool(bool canvasHovered)
    {
        const ImVec2 mouse = ImGui::GetMousePos();
        float projectX = 0.0f, projectY = 0.0f;
        const bool inside = CanvasScreenToProject(&mProject, mouse.x, mouse.y,
            projectX, projectY, false, mCanvasZoom, mCanvasPanX, mCanvasPanY);
        if (canvasHovered && inside && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            mMeasureX1 = mMeasureX2 = projectX;
            mMeasureY1 = mMeasureY2 = projectY;
            mMeasureDragging = true;
        }
        if (mMeasureDragging)
        {
            mMeasureX2 = projectX;
            mMeasureY2 = projectY;
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                mMeasureDragging = false;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            mMeasureDragging = false;
            mMeasureX1 = mMeasureY1 = mMeasureX2 = mMeasureY2 = 0.0f;
        }
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    void App::DesenharMedicao()
    {
        if (mCurrentTool != Tool::Measure || !mHasProject) return;
        if (mMeasureX1 == 0.0f && mMeasureY1 == 0.0f &&
            mMeasureX2 == 0.0f && mMeasureY2 == 0.0f)
            return;
        if (mMeasureX1 == mMeasureX2 && mMeasureY1 == mMeasureY2) return;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 color = ImGui::ColorConvertFloat4ToU32(Theme::AccentOrange);
        float sx1 = 0.0f, sy1 = 0.0f, sx2 = 0.0f, sy2 = 0.0f, scale = 1.0f;
        CanvasProjectToScreen(&mProject, mMeasureX1, mMeasureY1, sx1, sy1, scale,
                              mCanvasZoom, mCanvasPanX, mCanvasPanY);
        CanvasProjectToScreen(&mProject, mMeasureX2, mMeasureY2, sx2, sy2, scale,
                              mCanvasZoom, mCanvasPanX, mCanvasPanY);

        const float dx = mMeasureX2 - mMeasureX1;
        const float dy = mMeasureY2 - mMeasureY1;
        const float lengthPx = sqrtf(dx * dx + dy * dy);
        const float unit = UnitToPixels();
        const float length = lengthPx / unit;
        const float angleDeg = atan2f(dy, dx) * (180.0f / 3.14159265f);

        dl->AddLine(ImVec2(sx1, sy1), ImVec2(sx2, sy2), color, 1.5f);
        // Travinhas perpendiculares nas pontas.
        const float nx = -dy, ny = dx; // normal
        const float len = sqrtf(nx * nx + ny * ny);
        if (len > 0.01f)
        {
            const float ux = nx / len * 7.0f, uy = ny / len * 7.0f;
            dl->AddLine(ImVec2(sx1 - ux, sy1 - uy), ImVec2(sx1 + ux, sy1 + uy),
                        color, 1.5f);
            dl->AddLine(ImVec2(sx2 - ux, sy2 - uy), ImVec2(sx2 + ux, sy2 + uy),
                        color, 1.5f);
        }

        // Rótulo: distância (na unidade atual) + ângulo.
        const char* unitName = mUnit == 1 ? "mm" : mUnit == 2 ? "cm"
            : mUnit == 3 ? "in" : mUnit == 4 ? "pt" : "px";
        char text[64];
        snprintf(text, sizeof(text), "%.2f %s · %.1f°", length, unitName, angleDeg);
        const ImVec2 textSize = ImGui::CalcTextSize(text);
        const ImVec2 mid((sx1 + sx2) * 0.5f, (sy1 + sy2) * 0.5f);
        const ImVec2 boxMin(mid.x - textSize.x * 0.5f - 5.0f,
                            mid.y - textSize.y * 0.5f - 3.0f);
        const ImVec2 boxMax(mid.x + textSize.x * 0.5f + 5.0f,
                            mid.y + textSize.y * 0.5f + 3.0f);
        dl->AddRectFilled(boxMin, boxMax, IM_COL32(32, 32, 38, 235));
        dl->AddRect(boxMin, boxMax, color, 3.0f);
        dl->AddText(ImVec2(mid.x - textSize.x * 0.5f, mid.y - textSize.y * 0.5f),
                    IM_COL32(255, 255, 255, 255), text);
    }

    void App::AddPenPoint(Element& path, float projectX, float projectY,
                          float handleDX, float handleDY, bool curved)
    {
        const float bx = path.transformacao.value("x", 0.0f);
        const float by = path.transformacao.value("y", 0.0f);
        const float lx = projectX - bx;
        const float ly = projectY - by;
        const float w = path.transformacao.value("largura", 1.0f);
        const float h = path.transformacao.value("altura", 1.0f);
        path.transformacao["pontos"].push_back(nlohmann::json{
            { "x", lx }, { "y", ly },
            { "cx2", handleDX }, { "cy2", handleDY },
            { "curva", curved ? 1.0f : 0.0f }
        });
        path.transformacao["largura"] = std::max(w, lx + 2.0f);
        path.transformacao["altura"] = std::max(h, ly + 2.0f);
    }

    void App::HandlePenTool(bool canvasHovered)
    {
        const ImVec2 mouse = ImGui::GetMousePos();
        float px = 0.0f, py = 0.0f;
        const bool inside = CanvasScreenToProject(&mProject, mouse.x, mouse.y,
            px, py, false, mCanvasZoom, mCanvasPanX, mCanvasPanY);

        if (!mPenDrawing)
        {
            // Primeiro clique: cria o caminho com o primeiro ponto.
            if (canvasHovered && inside &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                std::string id;
                for (int i = 1; i < 10000; ++i)
                {
                    id = "caminho_" + std::to_string(i);
                    if (!Project::ResolverId(mProject, id)) break;
                }
                Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
                Element e;
                e.id = id;
                e.tipo = "caminho";
                e.nome = "Caminho";
                e.transformacao = {
                    { "x", px }, { "y", py },
                    { "largura", 1.0f }, { "altura", 1.0f },
                    { "fechado", 0.0f },
                    { "pontos", nlohmann::json::array({ nlohmann::json{
                          { "x", 0.0f }, { "y", 0.0f },
                          { "cx2", 0.0f }, { "cy2", 0.0f }, { "curva", 0.0f } } }) }
                };
                mode.raiz.push_back(std::move(e));
                mSelectedElementId = id;
                mSelectedElementIds = { id };
                mPenDrawing = true;
                mProjectDirty = true;
                mStatusMsg = "Caneta: clique adiciona ponto · arraste cria curva";
                mStatusMsgUntil = GetTime() + 6.0;
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            return;
        }

        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        Element* path = Project::ResolverId(mode, mSelectedElementId);
        if (!path || path->tipo != "caminho")
        {
            mPenDrawing = false;
            return;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            // Cancela: remove o caminho em construção.
            auto it = std::find_if(mode.raiz.begin(), mode.raiz.end(),
                [&](const Element& el) { return el.id == path->id; });
            if (it != mode.raiz.end()) mode.raiz.erase(it);
            mPenDrawing = false;
            mSelectedElementId.clear();
            mSelectedElementIds.clear();
            mProjectDirty = true;
            mStatusMsg = "Caminho cancelado";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            mPenDrawing = false;
            mProjectDirty = true;
            mStatusMsg = "Caminho finalizado (aberto)";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }

        auto& pts = path->transformacao["pontos"];
        const float bx = path->transformacao.value("x", 0.0f);
        const float by = path->transformacao.value("y", 0.0f);

        // Fechar: clique perto do primeiro ponto (com 3+ pontos).
        if (canvasHovered && inside &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) && pts.size() >= 3)
        {
            const float fpx = bx + pts[0].value("x", 0.0f);
            const float fpy = by + pts[0].value("y", 0.0f);
            const float ddx = px - fpx, ddy = py - fpy;
            if (ddx * ddx + ddy * ddy <= 144.0f)
            {
                path->transformacao["fechado"] = 1.0f;
                mPenDrawing = false;
                mProjectDirty = true;
                mStatusMsg = "Caminho fechado";
                mStatusMsgUntil = GetTime() + 4.0;
                return;
            }
        }

        // Clique / arrasto adiciona ponto.
        if (canvasHovered && inside &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            mPenDragging = true;
            mPenDragStartX = mPenDragX = px;
            mPenDragStartY = mPenDragY = py;
        }
        if (mPenDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            mPenDragX = px;
            mPenDragY = py;
        }
        if (mPenDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            const float ddx = mPenDragX - mPenDragStartX;
            const float ddy = mPenDragY - mPenDragStartY;
            if (ddx * ddx + ddy * ddy > 16.0f)
            {
                AddPenPoint(*path, mPenDragX, mPenDragY,
                            ddx * 0.4f, ddy * 0.4f, true);
            }
            else
            {
                AddPenPoint(*path, mPenDragX, mPenDragY, 0.0f, 0.0f, false);
            }
            mPenDragging = false;
            mProjectDirty = true;
        }
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    void App::ApagarElementosSelecionados()
    {
        if (!PossuiModoAtivo()) return;
        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        std::vector<std::string> ids = mSelectedElementIds;
        if (ids.empty() && !mSelectedElementId.empty()) ids.push_back(mSelectedElementId);
        int removed = 0;
        for (const std::string& id : ids)
            if (Project::ExcluirElemento(mode, id)) ++removed;
        if (removed == 0) return;
        mSelectedElementId.clear();
        mSelectedElementIds.clear();
        mSelectedCornerMask = 0;
        mProjectDirty = true;
        CapturarHistorico();
        mStatusMsg = std::to_string(removed) + " elemento(s) apagado(s)";
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::ZoomCanvas(float factor)
    {
        if (!mHasProject) return;
        const float oldZoom = mCanvasZoom;
        const float newZoom = factor > 1.0f
            ? std::min(32.0f, oldZoom * factor)
            : std::max(0.1f, oldZoom * factor);
        if (newZoom == oldZoom) return;
        if (!mZoomToMouse)
        {
            mCanvasZoom = newZoom;
            return;
        }
        // Encaminhamento completo: mantém o ponto do projeto sob o cursor
        // fixo na tela (vale para roda, botões e ferramenta Z).
        const ImVec2 mouse = ImGui::GetMousePos();
        float px = 0.0f, py = 0.0f, scale = 1.0f;
        float sx0 = 0.0f, sy0 = 0.0f, sx1 = 0.0f, sy1 = 0.0f;
        if (CanvasScreenToProject(&mProject, mouse.x, mouse.y, px, py, false,
                                  oldZoom, mCanvasPanX, mCanvasPanY))
        {
            CanvasProjectToScreen(&mProject, px, py, sx0, sy0, scale,
                                  oldZoom, mCanvasPanX, mCanvasPanY);
            mCanvasZoom = newZoom;
            CanvasProjectToScreen(&mProject, px, py, sx1, sy1, scale,
                                  newZoom, mCanvasPanX, mCanvasPanY);
            mCanvasPanX += sx0 - sx1;
            mCanvasPanY += sy0 - sy1;
        }
        else
        {
            mCanvasZoom = newZoom;
        }
    }

    void App::CriarRetanguloTelaBase()
    {
        // Duplo clique na ferramenta de criar retângulo: cria automaticamente
        // um retângulo com o TAMANHO DA TELA BASE (ex.: 1280x720), centrado
        // na tela-base — estilo CorelDRAW, sem precisar arrastar.
        if (!PossuiModoAtivo()) return;
        const float w = (float)mProject.telaBaseLargura;
        const float h = (float)mProject.telaBaseAltura;
        const float x = 0.0f;
        const float y = 0.0f;
        if (AdicionarComponente("retangulo", "Retangulo", x, y))
        {
            Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
            if (Element* created = Project::ResolverId(mode, mSelectedElementId))
            {
                created->transformacao["largura"] = w;
                created->transformacao["altura"] = h;
                created->estilos["opacidade"] = 1.0f;
            }
            mCurrentTool = Tool::Select; // volta à seleção após criar
            mStatusMsg = "Retangulo criado no tamanho da tela base (" +
                         std::to_string((int)w) + "x" + std::to_string((int)h) + ")";
            mStatusMsgUntil = GetTime() + 4.0;
        }
    }

    void App::MoverCamadaSelecionada(int delta)
    {
        if (!PossuiModoAtivo() || mSelectedElementId.empty()) return;
        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        // Move o elemento primário (e repetidamente até o topo/fundo quando
        // o delta é grande, ex.: +1000 = trazer ao topo).
        bool changed = false;
        int remaining = std::abs(delta);
        const int direction = delta > 0 ? 1 : -1;
        const int maxSteps = 100000;
        while (remaining-- > 0 && maxSteps > 0)
        {
            if (!Project::MoverCamada(mode, mSelectedElementId, direction))
                break;
            changed = true;
        }
        if (changed)
        {
            mProjectDirty = true;
            const char* label = delta > 0 ? "frente" : "trás";
            mStatusMsg = std::string("Camada: elemento movido para ") + label;
            mStatusMsgUntil = GetTime() + 4.0;
        }
    }

    void App::AlinharElementosSelecionados(int operation)
    {
        if (!PossuiModoAtivo() || operation < 0 || operation > 5) return;
        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        std::vector<Element*> selected;
        for (const std::string& id : mSelectedElementIds)
            if (Element* element = Project::ResolverId(mode, id))
                if (element->visivel) selected.push_back(element);
        if (selected.empty() && !mSelectedElementId.empty())
            if (Element* element = Project::ResolverId(mode, mSelectedElementId))
                if (element->visivel) selected.push_back(element);

        const bool singleGroupToScreen = selected.size() == 1 &&
                                         selected.front()->tipo == "grupo";
        const int alignTarget = singleGroupToScreen ? 2 : mAlignTarget;

        if (selected.empty() ||
            (alignTarget == 0 && selected.size() < 2) ||
            (alignTarget == 1 && selected.size() < 2))
        {
            mStatusMsg = alignTarget == 2
                ? "Selecione um elemento para alinhar a tela"
                : "Selecione pelo menos dois elementos para alinhar";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }

        float left = FLT_MAX, top = FLT_MAX, right = -FLT_MAX, bottom = -FLT_MAX;
        for (const Element* element : selected)
        {
            const float x = element->transformacao.value("x", 0.0f);
            const float y = element->transformacao.value("y", 0.0f);
            const float w = element->transformacao.value("largura", 160.0f);
            const float h = element->transformacao.value("altura", 32.0f);
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x + w);
            bottom = std::max(bottom, y + h);
        }

        Element* primary = Project::ResolverId(mode, mSelectedElementId);
        if (alignTarget == 1 && (!primary || !primary->visivel))
        {
            mStatusMsg = "O elemento principal da selecao nao esta disponivel";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }

        float target = 0.0f;
        if (alignTarget == 3)
        {
            // CONJUNTO: a referência é o bounding box dos elementos visíveis
            // FORA da seleção (os vizinhos). Centralizar H+V coloca o elemento
            // exatamente no centro do conjunto, com distâncias uniformes nos
            // 4 lados — ex.: a forma vermelha no centro dos quadros cinza.
            std::vector<AlignUtils::Rect> neighbors;
            auto collectNeighbors = [&](const Element& e, auto& self) -> void
            {
                if (!e.visivel) return;
                const bool isSel = std::find(mSelectedElementIds.begin(),
                                             mSelectedElementIds.end(), e.id) !=
                                   mSelectedElementIds.end() ||
                                   e.id == mSelectedElementId;
                if (!isSel)
                {
                    AlignUtils::Rect r;
                    r.x = e.transformacao.value("x", 0.0f);
                    r.y = e.transformacao.value("y", 0.0f);
                    r.w = e.transformacao.value("largura", 160.0f);
                    r.h = e.transformacao.value("altura", 32.0f);
                    neighbors.push_back(r);
                }
                for (const Element& c : e.filhos) self(c, self);
            };
            for (const Element& e : mode.raiz) collectNeighbors(e, collectNeighbors);
            if (neighbors.empty())
            {
                mStatusMsg = "Nao ha vizinhos para alinhar no conjunto";
                mStatusMsgUntil = GetTime() + 4.0;
                return;
            }
            float l = 0.0f, t = 0.0f, r = 0.0f, b = 0.0f;
            AlignUtils::SetBounds(neighbors, l, t, r, b);
            // Alvo = posição que alinharia o PRIMEIRO selecionado; o loop
            // abaixo aplica o mesmo deslocamento a todos (conjunto uniforme).
            const Element& first = *selected.front();
            const float fw = first.transformacao.value("largura", 160.0f);
            const float fh = first.transformacao.value("altura", 32.0f);
            const float fx = first.transformacao.value("x", 0.0f);
            const float fy = first.transformacao.value("y", 0.0f);
            const float tx = AlignUtils::AlignedX(fw, l, r, operation);
            const float ty = AlignUtils::AlignedY(fh, t, b, operation);
            // Aplica o MESMO delta a todos os selecionados (preserva o layout
            // relativo do conjunto selecionado enquanto centraliza no vizinho).
            bool changed = false;
            const float dx = (operation <= 2 ? tx - fx : 0.0f);
            const float dy = (operation >= 3 ? ty - fy : 0.0f);
            for (Element* element : selected)
            {
                if (element->bloqueado) continue;
                if (operation <= 2)
                {
                    const float oldX = element->transformacao.value("x", 0.0f);
                    ApplyPositionDelta(*element, dx, 0.0f);
                    if (element->transformacao.value("x", 0.0f) != oldX) changed = true;
                }
                else
                {
                    const float oldY = element->transformacao.value("y", 0.0f);
                    ApplyPositionDelta(*element, 0.0f, dy);
                    if (element->transformacao.value("y", 0.0f) != oldY) changed = true;
                }
            }
            if (changed)
            {
                mProjectDirty = true;
                mStatusMsg = "Alinhado ao conjunto (" + std::to_string(selected.size()) +
                             " elemento(s)) — distancias uniformes";
            }
            else
            {
                mStatusMsg = "Os elementos ja estao alinhados ao conjunto";
            }
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }
        else if (alignTarget == 2)
        {
            if (operation == 0 || operation == 3) target = 0.0f;
            else if (operation == 1) target = mProject.telaBaseLargura * 0.5f;
            else if (operation == 2) target = (float)mProject.telaBaseLargura;
            else if (operation == 4) target = mProject.telaBaseAltura * 0.5f;
            else target = (float)mProject.telaBaseAltura;
        }
        else if (alignTarget == 1)
        {
            const float x = primary->transformacao.value("x", 0.0f);
            const float y = primary->transformacao.value("y", 0.0f);
            const float w = primary->transformacao.value("largura", 160.0f);
            const float h = primary->transformacao.value("altura", 32.0f);
            if (operation == 0) target = x;
            else if (operation == 1) target = x + w * 0.5f;
            else if (operation == 2) target = x + w;
            else if (operation == 3) target = y;
            else if (operation == 4) target = y + h * 0.5f;
            else target = y + h;
        }
        else
        {
            if (operation == 0) target = left;
            else if (operation == 1) target = (left + right) * 0.5f;
            else if (operation == 2) target = right;
            else if (operation == 3) target = top;
            else if (operation == 4) target = (top + bottom) * 0.5f;
            else target = bottom;
        }

        bool changed = false;
        for (Element* element : selected)
        {
            if (element->bloqueado || (alignTarget == 1 && element == primary)) continue;
            const float w = element->transformacao.value("largura", 160.0f);
            const float h = element->transformacao.value("altura", 32.0f);
            if (operation <= 2)
            {
                const float oldX = element->transformacao.value("x", 0.0f);
                float x = operation == 0 ? target
                        : operation == 1 ? target - w * 0.5f : target - w;
                x = std::max(0.0f, std::min((float)mProject.telaBaseLargura - w, x));
                if (oldX != x)
                {
                    // Grupo: aplica o delta também nos filhos (senão só a
                    // caixa anda e os elementos ficam parados).
                    ApplyPositionDelta(*element, x - oldX, 0.0f);
                    changed = true;
                }
            }
            else
            {
                const float oldY = element->transformacao.value("y", 0.0f);
                float y = operation == 3 ? target
                        : operation == 4 ? target - h * 0.5f : target - h;
                y = std::max(0.0f, std::min((float)mProject.telaBaseAltura - h, y));
                if (oldY != y)
                {
                    ApplyPositionDelta(*element, 0.0f, y - oldY);
                    changed = true;
                }
            }
        }

        if (changed)
        {
            mProjectDirty = true;
            mStatusMsg = "Alinhamento aplicado a " + std::to_string(selected.size()) +
                         " elemento(s)";
        }
        else
        {
            mStatusMsg = "Os elementos ja estao alinhados";
        }
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::DistribuirElementosSelecionados(bool horizontal)
    {
        if (!PossuiModoAtivo()) return;
        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        std::vector<Element*> elements;
        for (const std::string& id : mSelectedElementIds)
            if (Element* element = Project::ResolverId(mode, id))
                if (element->visivel && !element->bloqueado) elements.push_back(element);
        if (elements.size() < 2)
        {
            mStatusMsg = "Selecione pelo menos dois elementos para espacamento";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }

        std::sort(elements.begin(), elements.end(), [&](const Element* a, const Element* b)
        {
            const char* key = horizontal ? "x" : "y";
            return a->transformacao.value(key, 0.0f) < b->transformacao.value(key, 0.0f);
        });
        const float spacing = std::max(0.0f,
            horizontal ? mHorizontalSpacing : mVerticalSpacing);
        float cursor = elements.front()->transformacao.value(horizontal ? "x" : "y", 0.0f);
        for (Element* element : elements)
        {
            const float size = element->transformacao.value(
                horizontal ? "largura" : "altura", horizontal ? 160.0f : 32.0f);
            const float old = element->transformacao.value(
                horizontal ? "x" : "y", 0.0f);
            if (old != cursor)
                ApplyPositionDelta(*element,
                                   horizontal ? cursor - old : 0.0f,
                                   horizontal ? 0.0f : cursor - old);
            cursor += size + spacing;
        }
        mProjectDirty = true;
        mStatusMsg = horizontal ? "Espacamento horizontal aplicado"
                                : "Espacamento vertical aplicado";
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::AgruparElementosSelecionados()
    {
        if (!PossuiModoAtivo() || mSelectedElementIds.size() < 2)
        {
            mStatusMsg = "Selecione pelo menos dois elementos de nivel principal";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }
        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        const std::string groupId = Project::AgruparElementos(mode, mSelectedElementIds);
        if (groupId.empty())
        {
            mStatusMsg = "Agrupamento requer dois elementos desbloqueados no mesmo nivel";
            mStatusMsgUntil = GetTime() + 5.0;
            return;
        }
        mSelectedElementIds = { groupId };
        mSelectedElementId = groupId;
        mSelectedCornerMask = 0;
        mProjectDirty = true;
        mStatusMsg = "Elementos agrupados (Ctrl+G)";
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::DesagruparElementosSelecionados()
    {
        if (!PossuiModoAtivo() || mSelectedElementIds.size() != 1)
        {
            mStatusMsg = "Selecione um grupo para desagrupar";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }
        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        const std::string groupId = mSelectedElementIds.front();
        Element* group = Project::ResolverId(mode, groupId);
        if (!group || group->tipo != "grupo")
        {
            mStatusMsg = "O elemento selecionado não é um grupo";
            mStatusMsgUntil = GetTime() + 4.0;
            return;
        }
        if (!Project::DesagruparElementos(mode, groupId))
        {
            mStatusMsg = "Não foi possível desagrupar (grupo bloqueado?)";
            mStatusMsgUntil = GetTime() + 5.0;
            return;
        }
        mSelectedElementIds.clear();
        mSelectedElementId.clear();
        mSelectedCornerMask = 0;
        mProjectDirty = true;
        mStatusMsg = "Grupo desagrupado (Ctrl+Shift+G) — posições preservadas";
        mStatusMsgUntil = GetTime() + 4.0;
    }

    void App::ResetarHistorico()
    {
        mHistory.clear();
        mHistoryIndex = -1;
        if (!mHasProject) return;
        mHistory.push_back(Project::Serializar(mProject));
        mHistoryIndex = 0;
    }

    void App::CapturarHistorico()
    {
        if (!mHasProject) return;
        const std::string snapshot = Project::Serializar(mProject);
        if (mHistoryIndex < 0 || mHistory.empty())
        {
            mHistory = { snapshot };
            mHistoryIndex = 0;
            return;
        }
        if (mHistory[mHistoryIndex] == snapshot) return;
        mHistory.erase(mHistory.begin() + mHistoryIndex + 1, mHistory.end());
        mHistory.push_back(snapshot);
        if (mHistory.size() > 100) mHistory.erase(mHistory.begin());
        mHistoryIndex = (int)mHistory.size() - 1;
    }

    void App::Desfazer()
    {
        CapturarHistorico();
        if (mHistoryIndex <= 0)
        {
            mStatusMsg = "Nada para desfazer";
            mStatusMsgUntil = GetTime() + 3.0;
            return;
        }
        const std::string path = mProject.caminhoArquivo;
        Project restored;
        if (!Project::Desserializar(restored, mHistory[--mHistoryIndex]).empty()) return;
        restored.caminhoArquivo = path;
        mProject = std::move(restored);
        mSelectedElementId.clear();
        mSelectedElementIds.clear();
        mProjectDirty = true;
        mStatusMsg = "Desfazer";
        mStatusMsgUntil = GetTime() + 3.0;
    }

    void App::Refazer()
    {
        if (mHistoryIndex < 0 || mHistoryIndex + 1 >= (int)mHistory.size())
        {
            mStatusMsg = "Nada para refazer";
            mStatusMsgUntil = GetTime() + 3.0;
            return;
        }
        const std::string path = mProject.caminhoArquivo;
        Project restored;
        if (!Project::Desserializar(restored, mHistory[++mHistoryIndex]).empty()) return;
        restored.caminhoArquivo = path;
        mProject = std::move(restored);
        mSelectedElementId.clear();
        mSelectedElementIds.clear();
        mProjectDirty = true;
        mStatusMsg = "Refazer";
        mStatusMsgUntil = GetTime() + 3.0;
    }

    void App::HandleCanvasInteraction(bool canvasHovered)
    {
        // Guia sendo arrastada/posicionada: a interação normal do canvas
        // (selecionar, mover, marquee) fica suspensa até soltar.
        if (mGuideDragKind != 0) return;

        // Ferramenta Medir: medição transitória de distância/ângulo.
        if (mCurrentTool == Tool::Measure && mHasProject)
        {
            HandleMeasureTool(canvasHovered);
            return;
        }

        // Caneta: desenha caminhos Bézier ponto a ponto.
        if (mCurrentTool == Tool::Pen && mHasProject &&
            !ImGui::GetIO().KeyCtrl)
        {
            HandlePenTool(canvasHovered);
            return;
        }

        // Ctrl pressionado = ferramenta de SELEÇÃO temporária: mesmo estando
        // em outra ferramenta (criar retângulo, zoom etc.), Ctrl + arrastar
        // seleciona/marquee — sem trocar a ferramenta ativa.
        const bool ctrlTemporary = ImGui::GetIO().KeyCtrl;
        const bool shapeTool = mCurrentTool == Tool::Rectangle ||
                               mCurrentTool == Tool::Ellipse ||
                               mCurrentTool == Tool::Polygon ||
                               mCurrentTool == Tool::Line;
        if (PossuiModoAtivo() && shapeTool && !ctrlTemporary)
        {
            const ImVec2 mouse = ImGui::GetMousePos();
            float projectX = 0.0f, projectY = 0.0f;
            const bool inside = CanvasScreenToProject(&mProject, mouse.x, mouse.y,
                projectX, projectY, false, mCanvasZoom, mCanvasPanX, mCanvasPanY);
            if (canvasHovered && inside && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                mShapeCreating = true;
                mShapeStartX = mShapeEndX = projectX;
                mShapeStartY = mShapeEndY = projectY;
            }
            if (mShapeCreating && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                CanvasScreenToProject(&mProject, mouse.x, mouse.y,
                    mShapeEndX, mShapeEndY, false, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            }
            if (mShapeCreating && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                const float x = std::min(mShapeStartX, mShapeEndX);
                const float y = std::min(mShapeStartY, mShapeEndY);
                const float w = std::max(8.0f, fabsf(mShapeEndX - mShapeStartX));
                const float h = std::max(8.0f, fabsf(mShapeEndY - mShapeStartY));
                const char* type = mCurrentTool == Tool::Rectangle ? "retangulo" :
                                   mCurrentTool == Tool::Ellipse ? "elipse" :
                                   mCurrentTool == Tool::Line ? "linha" : "poligono";
                const char* name = mCurrentTool == Tool::Rectangle ? "Retangulo" :
                                   mCurrentTool == Tool::Ellipse ? "Elipse" :
                                   mCurrentTool == Tool::Line ? "Linha" : "Poligono";
                if (AdicionarComponente(type, name, x, y))
                {
                    Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
                    if (Element* created = Project::ResolverId(mode, mSelectedElementId))
                    {
                        created->transformacao["largura"] = w;
                        created->transformacao["altura"] = h;
                        created->estilos["opacidade"] = 1.0f;
                    }
                    mStatusMsg = std::string(name) + " criado";
                    mStatusMsgUntil = GetTime() + 4.0;
                }
                mShapeCreating = false;
            }
            return;
        }

        const bool editTool = mCurrentTool == Tool::Select || mCurrentTool == Tool::Move;
        if (!PossuiModoAtivo() || (!editTool && !ctrlTemporary))
        {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                mCanvasDragMode = 0;
                mCanvasMarquee = false;
            }
            return;
        }

        Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
        auto isSelected = [&](const std::string& id)
        {
            return std::find(mSelectedElementIds.begin(), mSelectedElementIds.end(), id) !=
                   mSelectedElementIds.end();
        };
        if (mSelectedElementId.empty())
        {
            if (!mCanvasMarquee) mSelectedElementIds.clear();
        }
        else if (!isSelected(mSelectedElementId))
        {
            mSelectedElementIds.clear();
            mSelectedElementIds.push_back(mSelectedElementId);
        }
        if (mCornerSelectionElementId != mSelectedElementId)
        {
            mSelectedCornerMask = 0;
            mCanvasCornerDragMask = 0;
            mCornerSelectionElementId = mSelectedElementId;
        }

        const ImVec2 mouse = ImGui::GetMousePos();
        float mouseX = 0.0f, mouseY = 0.0f;
        const bool mouseOnFrame = CanvasScreenToProject(&mProject, mouse.x, mouse.y,
                                                         mouseX, mouseY, false, mCanvasZoom,
                                                         mCanvasPanX, mCanvasPanY);
        float unusedX = 0.0f, unusedY = 0.0f, viewScale = 1.0f;
        CanvasProjectToScreen(&mProject, 0.0f, 0.0f, unusedX, unusedY, viewScale,
                              mCanvasZoom, mCanvasPanX, mCanvasPanY);
        const float tolerance = 8.0f / std::max(0.25f, viewScale);

        auto dragModeAt = [&](const Element& element, float x, float y)
        {
            const float ex = element.transformacao.value("x", 0.0f);
            const float ey = element.transformacao.value("y", 0.0f);
            const float ew = element.transformacao.value("largura", 160.0f);
            const float eh = element.transformacao.value("altura", 32.0f);

            // Ponto de ORIGEM (pivô): clicar e arrastar reposiciona o centro
            // do resize espelhado (Shift) e da rotação. Prioridade sobre o
            // mover — o pivô é a "mira" laranja no centro da forma.
            if (element.tipo != "grupo")
            {
                float px = 0.0f, py = 0.0f;
                Geo::ElementPivot(element, px, py);
                const float pivotTol = 9.0f / std::max(0.25f, viewScale);
                const float pdx = x - px;
                const float pdy = y - py;
                if (pdx * pdx + pdy * pdy <= pivotTol * pivotTol) return 15;
            }

            // Alça de rotação: fora da caixa, no topo rotacionado (vale
            // também para grupos — o conjunto inteiro rotaciona junto).
            {
                float px = 0.0f, py = 0.0f;
                Geo::ElementPivot(element, px, py);
                const float rotRad = Geo::DegToRad(Geo::ElementRotation(element));
                const float sinR = sinf(rotRad), cosR = cosf(rotRad);
                const float halfH = eh * 0.5f;
                const float topX = px + halfH * sinR;
                const float topY = py - halfH * cosR;
                const float sticker = 18.0f / std::max(0.25f, viewScale);
                const float rotTol = 10.0f / std::max(0.25f, viewScale);
                const float hx = topX + sinR * sticker;
                const float hy = topY - cosR * sticker;
                const float rdx = x - hx;
                const float rdy = y - hy;
                if (rdx * rdx + rdy * rdy <= rotTol * rotTol) return 14;
            }

            if (element.tipo == "caminho" &&
                element.transformacao.contains("pontos") &&
                element.transformacao["pontos"].is_array())
            {
                // Nós e alças de curva editáveis (modo 16 = nó, 17 = alça).
                // Sem alças de tamanho: o caminho se edita pelos nós.
                const auto& pts = element.transformacao["pontos"];
                const float nodeTol = 7.0f / std::max(0.25f, viewScale);
                const float nodeTolSq = nodeTol * nodeTol;
                for (int i = 0; i < (int)pts.size(); ++i)
                {
                    const float nx = ex + pts[i].value("x", 0.0f);
                    const float ny = ey + pts[i].value("y", 0.0f);
                    const float hx = nx + pts[i].value("cx2", 0.0f);
                    const float hy = ny + pts[i].value("cy2", 0.0f);
                    const float hdx = x - hx, hdy = y - hy;
                    if (hdx * hdx + hdy * hdy <= nodeTolSq)
                    {
                        mPathEditIndex = i;
                        return 17;
                    }
                    const float ndx = x - nx, ndy = y - ny;
                    if (ndx * ndx + ndy * ndy <= nodeTolSq)
                    {
                        mPathEditIndex = i;
                        return 16;
                    }
                }
                if (x >= ex && x <= ex + ew && y >= ey && y <= ey + eh)
                    return 1;
                return 0;
            }

            if (x < ex - tolerance || x > ex + ew + tolerance ||
                y < ey - tolerance || y > ey + eh + tolerance)
                return 0;
            if (element.tipo == "grupo")
            {
                // Grupo: alças de tamanho (2-9) como um objeto único — o
                // resize redimensiona TODOS os filhos em conjunto.
                const bool left = fabsf(x - ex) <= tolerance;
                const bool right = fabsf(x - ex - ew) <= tolerance;
                const bool top = fabsf(y - ey) <= tolerance;
                const bool bottom = fabsf(y - ey - eh) <= tolerance;
                if (left && top) return 6;
                if (right && top) return 7;
                if (left && bottom) return 8;
                if (right && bottom) return 9;
                if (left) return 2;
                if (right) return 3;
                if (top) return 4;
                if (bottom) return 5;
                return 1;
            }

            const bool supportsCorners = element.tipo != "elipse" &&
                                         element.tipo != "poligono";
            const float minMarkerInset = 14.0f / std::max(0.25f, viewScale);
            const float maxMarkerInset = std::max(6.0f / std::max(0.25f, viewScale),
                std::min(ew, eh) * 0.5f - 6.0f / std::max(0.25f, viewScale));
            auto markerInset = [&](int mode)
            {
                return std::min(maxMarkerInset,
                                std::max(minMarkerInset, ElementCornerRadius(element, mode)));
            };
            const float cornerTolerance = 9.0f / std::max(0.25f, viewScale);
            const float cornerToleranceSq = cornerTolerance * cornerTolerance;
            const float insets[] = {
                markerInset(10), markerInset(11), markerInset(12), markerInset(13)
            };
            const float cornerX[] = {
                ex + insets[0], ex + ew - insets[1],
                ex + ew - insets[2], ex + insets[3]
            };
            const float cornerY[] = {
                ey + insets[0], ey + insets[1],
                ey + eh - insets[2], ey + eh - insets[3]
            };
            for (int index = 0; supportsCorners && index < 4; ++index)
            {
                const float dx = x - cornerX[index];
                const float dy = y - cornerY[index];
                if (dx * dx + dy * dy <= cornerToleranceSq) return 10 + index;
            }

            const bool left = fabsf(x - ex) <= tolerance;
            const bool right = fabsf(x - ex - ew) <= tolerance;
            const bool top = fabsf(y - ey) <= tolerance;
            const bool bottom = fabsf(y - ey - eh) <= tolerance;
            if (left && top) return 6;
            if (right && top) return 7;
            if (left && bottom) return 8;
            if (right && bottom) return 9;
            if (left) return 2;
            if (right) return 3;
            if (top) return 4;
            if (bottom) return 5;
            return x >= ex && x <= ex + ew && y >= ey && y <= ey + eh ? 1 : 0;
        };

        // Hit-testing de alças sobre um retângulo genérico (caixa conjunta da
        // multi-seleção): rotação (14), quinas (6-9), laterais (2-5) e dentro (1).
        auto dragModeAtRect = [&](float rx, float ry, float rw, float rh,
                                  float rotationDeg, float pivotX, float pivotY,
                                  float x, float y) -> int
        {
            const float rotRad = Geo::DegToRad(rotationDeg);
            const float sinR = sinf(rotRad), cosR = cosf(rotRad);
            const float halfH = rh * 0.5f;
            const float topX = pivotX + halfH * sinR;
            const float topY = pivotY - halfH * cosR;
            const float sticker = 18.0f / std::max(0.25f, viewScale);
            const float rotTol = 10.0f / std::max(0.25f, viewScale);
            const float hx = topX + sinR * sticker;
            const float hy = topY - cosR * sticker;
            const float rdx = x - hx;
            const float rdy = y - hy;
            if (rdx * rdx + rdy * rdy <= rotTol * rotTol) return 14;

            if (x < rx - tolerance || x > rx + rw + tolerance ||
                y < ry - tolerance || y > ry + rh + tolerance)
                return 0;
            const bool left = fabsf(x - rx) <= tolerance;
            const bool right = fabsf(x - rx - rw) <= tolerance;
            const bool top = fabsf(y - ry) <= tolerance;
            const bool bottom = fabsf(y - ry - rh) <= tolerance;
            if (left && top) return 6;
            if (right && top) return 7;
            if (left && bottom) return 8;
            if (right && bottom) return 9;
            if (left) return 2;
            if (right) return 3;
            if (top) return 4;
            if (bottom) return 5;
            return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh ? 1 : 0;
        };

        auto applyCursor = [](int dragMode)
        {
            if (dragMode == 2 || dragMode == 3) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            else if (dragMode == 4 || dragMode == 5) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            else if (dragMode == 6 || dragMode == 9) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            else if (dragMode == 7 || dragMode == 8) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
            else if (dragMode == 10 || dragMode == 12) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            else if (dragMode == 11 || dragMode == 13) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
            else if (dragMode == 14) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            else if (dragMode == 15) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            else if (dragMode == 16 || dragMode == 17)
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            else if (dragMode == 1) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        };

        if (canvasHovered && mouseOnFrame && mCanvasDragMode == 0 && !mCanvasMarquee)
        {
            if (Element* selected = Project::ResolverId(mode, mSelectedElementId))
                if (!selected->bloqueado) applyCursor(dragModeAt(*selected, mouseX, mouseY));
        }

        if (canvasHovered && mouseOnFrame &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            Element* current = Project::ResolverId(mode, mSelectedElementId);
            const bool multiSel = current && mSelectedElementIds.size() > 1;
            float cRectX = 0.0f, cRectY = 0.0f, cRectW = 0.0f, cRectH = 0.0f;
            int currentHandle = 0;
            if (multiSel)
            {
                // Caixa conjunta: alças de tamanho e rotação operam o conjunto.
                float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
                for (const std::string& id : mSelectedElementIds)
                {
                    Element* el = Project::ResolverId(mode, id);
                    if (!el) continue;
                    float bx0 = 0.0f, by0 = 0.0f, bx1 = 0.0f, by1 = 0.0f;
                    Geo::RotatedAABB(*el, bx0, by0, bx1, by1);
                    minX = std::min(minX, bx0);
                    minY = std::min(minY, by0);
                    maxX = std::max(maxX, bx1);
                    maxY = std::max(maxY, by1);
                }
                if (maxX > minX && maxY > minY)
                {
                    cRectX = minX;
                    cRectY = minY;
                    cRectW = maxX - minX;
                    cRectH = maxY - minY;
                    currentHandle = dragModeAtRect(cRectX, cRectY, cRectW, cRectH,
                                                   0.0f, minX + cRectW * 0.5f,
                                                   minY + cRectH * 0.5f,
                                                   mouseX, mouseY);
                }
            }
            else if (current)
            {
                currentHandle = dragModeAt(*current, mouseX, mouseY);
            }
            Element* hit = currentHandle >= 2
                ? current
                : Project::ElementoNoPonto(mode, mouseX, mouseY);
            // Clique em espaço vazio DENTRO da caixa conjunta move o conjunto
            // (o usuário não precisa acertar o corpo de um elemento).
            if (multiSel && !hit && currentHandle == 0 &&
                mouseX >= cRectX - tolerance &&
                mouseX <= cRectX + cRectW + tolerance &&
                mouseY >= cRectY - tolerance &&
                mouseY <= cRectY + cRectH + tolerance)
            {
                hit = current;
                currentHandle = 1;
            }
            const bool additive = ImGui::GetIO().KeyShift;
            bool keepHitForDrag = hit != nullptr;

            if (currentHandle >= 10 && currentHandle <= 13 && current)
            {
                const unsigned int clickedBit = 1u << (currentHandle - 10);
                mCornerSelectionElementId = current->id;
                if (ImGui::GetIO().KeyCtrl)
                {
                    mSelectedCornerMask = 0x0Fu;
                }
                else if (additive)
                {
                    mSelectedCornerMask ^= clickedBit;
                    if ((mSelectedCornerMask & clickedBit) == 0) keepHitForDrag = false;
                }
                else if ((mSelectedCornerMask & clickedBit) == 0)
                {
                    mSelectedCornerMask = clickedBit;
                }
                mCanvasCornerDragMask = mSelectedCornerMask;
                mSelectedElementId = current->id;
                if (!isSelected(current->id))
                {
                    mSelectedElementIds.clear();
                    mSelectedElementIds.push_back(current->id);
                }
            }
            else if (hit && additive)
            {
                mSelectedCornerMask = 0;
                mCanvasCornerDragMask = 0;
                auto it = std::find(mSelectedElementIds.begin(), mSelectedElementIds.end(), hit->id);
                if (it == mSelectedElementIds.end())
                {
                    mSelectedElementIds.push_back(hit->id);
                    mSelectedElementId = hit->id;
                }
                else
                {
                    mSelectedElementIds.erase(it);
                    keepHitForDrag = false;
                    mSelectedElementId = mSelectedElementIds.empty()
                        ? std::string()
                        : mSelectedElementIds.back();
                }
            }
            else if (hit)
            {
                mSelectedCornerMask = 0;
                mCanvasCornerDragMask = 0;
                if (!isSelected(hit->id))
                {
                    mSelectedElementIds.clear();
                    mSelectedElementIds.push_back(hit->id);
                }
                mSelectedElementId = hit->id;
            }
            else
            {
                mSelectedCornerMask = 0;
                mCanvasCornerDragMask = 0;
                if (!additive)
                {
                    mSelectedElementIds.clear();
                    mSelectedElementId.clear();
                }
                mCanvasMarquee = true;
                mCanvasMarqueeAdditive = additive;
                mCanvasMarqueeStartX = mCanvasMarqueeEndX = mouseX;
                mCanvasMarqueeStartY = mCanvasMarqueeEndY = mouseY;
            }

            mCanvasDragMode = 0;
            mCanvasDragChanged = false;
            mCanvasGroupStarts.clear();
            mGuideSnapX = -1.0f;
            mGuideSnapY = -1.0f;
            mGuideFixedSnapX = -1.0f;
            mGuideFixedSnapY = -1.0f;
            mGuideSpacingX1 = mGuideSpacingX2 = -1.0f;
            mGuideSpacingY1 = mGuideSpacingY2 = -1.0f;
            mShiftGuides.clear();
            mShiftGuidesLabels.clear();
            mCloneForked = false;
            mCanvasDragPastThreshold = false;
            if (hit && keepHitForDrag && !hit->bloqueado)
            {
                if (multiSel && currentHandle >= 2)
                    mCanvasDragMode = currentHandle; // alça da caixa conjunta
                else if (multiSel && currentHandle == 1)
                    mCanvasDragMode = 1; // mover o conjunto por espaço vazio
                else
                    mCanvasDragMode = dragModeAt(*hit, mouseX, mouseY);
                mCanvasDragMouseX = mouseX;
                mCanvasDragMouseY = mouseY;
                // Delta do frame anterior zera no início do arrasto (evita
                // "cruzamento fantasma" na previsão de espaçamento).
                mCanvasPrevDX = 0.0f;
                mCanvasPrevDY = 0.0f;
                if (multiSel && currentHandle >= 2)
                {
                    mCanvasDragX = cRectX;
                    mCanvasDragY = cRectY;
                    mCanvasDragW = cRectW;
                    mCanvasDragH = cRectH;
                    mCanvasDragRotation = 0.0f;
                    mCanvasDragPivotX = cRectX + cRectW * 0.5f;
                    mCanvasDragPivotY = cRectY + cRectH * 0.5f;
                }
                else
                {
                    mCanvasDragX = hit->transformacao.value("x", 0.0f);
                    mCanvasDragY = hit->transformacao.value("y", 0.0f);
                    mCanvasDragW = hit->transformacao.value("largura", 160.0f);
                    mCanvasDragH = hit->transformacao.value("altura", 32.0f);
                    mCanvasDragRotation = Geo::ElementRotation(*hit);
                    Geo::ElementPivot(*hit, mCanvasDragPivotX, mCanvasDragPivotY);
                }
                for (int corner = 0; corner < 4; ++corner)
                    mCanvasCornerRadiusStarts[corner] =
                        ElementCornerRadius(*hit, 10 + corner);
                const bool groupResize = (mCanvasDragMode >= 2 && mCanvasDragMode <= 9) &&
                                         hit && hit->tipo == "grupo";
                if (mCanvasDragMode == 1 || mCanvasDragMode == 14 ||
                    (mCanvasDragMode >= 2 && mCanvasDragMode <= 9 &&
                     (mSelectedElementIds.size() > 1 || groupResize)))
                {
                    for (const std::string& id : mSelectedElementIds)
                    {
                        Element* element = Project::ResolverId(mode, id);
                        if (!element || element->bloqueado) continue;
                        CollectTransformStarts(*element, mCanvasGroupStarts);
                    }
                    // Redimensionar um GRUPO opera todos os filhos como unidade
                    // (o grupo em si não entra nos starts — é contêiner).
                    if (groupResize && mCanvasGroupStarts.empty())
                        CollectTransformStarts(*hit, mCanvasGroupStarts);
                }
                applyCursor(mCanvasDragMode);
            }
        }

        // Clone com o botão direito (estilo CorelDRAW): pressionar sobre um
        // elemento e arrastar cria uma cópia e move a cópia; sem arrastar,
        // nada acontece (clique simples). Durante um arrasto com o botão
        // esquerdo em andamento, o clique direito é o fork de clone (tratado
        // no bloco de arrasto abaixo) — por isso o armamento só vale quando
        // não há arrasto ativo.
        if (canvasHovered && mouseOnFrame && mCurrentTool != Tool::Zoom &&
            mCanvasDragMode == 0 && !mRightDragArmed &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            Element* hit = Project::ElementoNoPonto(mode, mouseX, mouseY);
            if (hit && !hit->bloqueado)
            {
                mRightDragArmed = true;
                mRightDragElementId = hit->id;
                mRightDragStartX = mouseX;
                mRightDragStartY = mouseY;
            }
        }
        else if (mRightDragArmed && !mCloneDragging &&
                 ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            const float dx = mouseX - mRightDragStartX;
            const float dy = mouseY - mRightDragStartY;
            const float threshold = 4.0f / mCanvasZoom;
            if (dx * dx + dy * dy > threshold * threshold)
            {
                const std::string cloneId =
                    Project::ClonarElemento(mode, mProject, mRightDragElementId);
                if (!cloneId.empty())
                {
                    mCloneDragging = true;
                    mSelectedElementIds = { cloneId };
                    mSelectedElementId = cloneId;
                    mSelectedCornerMask = 0;
                    if (Element* clone = Project::ResolverId(mode, cloneId))
                    {
                        mCanvasDragMode = 1;
                        mCanvasDragMouseX = mouseX;
                        mCanvasDragMouseY = mouseY;
                        mCanvasPrevDX = 0.0f;
                        mCanvasPrevDY = 0.0f;
                        mCanvasDragX = clone->transformacao.value("x", 0.0f);
                        mCanvasDragY = clone->transformacao.value("y", 0.0f);
                        mCanvasDragW = clone->transformacao.value("largura", 160.0f);
                        mCanvasDragH = clone->transformacao.value("altura", 32.0f);
                        mCanvasDragRotation = Geo::ElementRotation(*clone);
                        Geo::ElementPivot(*clone, mCanvasDragPivotX, mCanvasDragPivotY);
                        mCanvasGroupStarts.clear();
                        CollectTransformStarts(*clone, mCanvasGroupStarts);
                        mProjectDirty = true;
                        mStatusMsg = "Clone criado — arraste para posicionar";
                        mStatusMsgUntil = GetTime() + 4.0;
                    }
                }
            }
        }
        if (mRightDragArmed && !mCloneDragging &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            mRightDragArmed = false; // clique simples — sem clone
        }

        if (mCanvasMarquee && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            // O marquee percorre o canvas inteiro (espaço livre, sem moldura).
            CanvasScreenToProject(&mProject, mouse.x, mouse.y, mouseX, mouseY, false,
                                  mCanvasZoom, mCanvasPanX, mCanvasPanY);
            mCanvasMarqueeEndX = mouseX;
            mCanvasMarqueeEndY = mouseY;
        }

        if (mCanvasMarquee && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            const float left = std::min(mCanvasMarqueeStartX, mCanvasMarqueeEndX);
            const float top = std::min(mCanvasMarqueeStartY, mCanvasMarqueeEndY);
            const float right = std::max(mCanvasMarqueeStartX, mCanvasMarqueeEndX);
            const float bottom = std::max(mCanvasMarqueeStartY, mCanvasMarqueeEndY);
            if (right - left > tolerance && bottom - top > tolerance)
            {
                std::vector<std::string> found;
                CollectElementsInRect(mode.raiz, left, top, right, bottom, found,
                                      mMarqueeContainOnly);
                for (const std::string& id : found)
                    if (!isSelected(id)) mSelectedElementIds.push_back(id);
                mSelectedElementId = mSelectedElementIds.empty()
                    ? std::string()
                    : mSelectedElementIds.back();
                mStatusMsg = std::to_string(mSelectedElementIds.size()) +
                             " elemento(s) selecionado(s)";
                mStatusMsgUntil = GetTime() + 4.0;
            }
            mCanvasMarquee = false;
            return;
        }

        if (mCanvasDragMode != 0 &&
            (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
             (mCloneDragging && ImGui::IsMouseDown(ImGuiMouseButton_Right))))
        {
            // Espaço de trabalho livre: o arrasto acompanha o mouse além da
            // moldura (o usuário pode sair da tela base por conta própria).
            CanvasScreenToProject(&mProject, mouse.x, mouse.y, mouseX, mouseY, false,
                                  mCanvasZoom, mCanvasPanX, mCanvasPanY);
            Element* selected = Project::ResolverId(mode, mSelectedElementId);
            if (!selected || selected->bloqueado)
            {
                mCanvasDragMode = 0;
                mGuideSnapX = -1.0f;
                mGuideSnapY = -1.0f;
                mGuideSpacingX1 = mGuideSpacingX2 = -1.0f;
                mGuideSpacingY1 = mGuideSpacingY2 = -1.0f;
                mShiftGuides.clear();
                mShiftGuidesLabels.clear();
                mCloneForked = false;
                mCanvasDragPastThreshold = false;
                return;
            }

            // CLONE DURANTE O ARRASTO (fork): o elemento é movido só com o
            // botão esquerdo; a qualquer momento, apertar o botão direito faz
            // o ORIGINAL voltar ao ponto de partida e o CLONE assumir o
            // arrasto — o clone segue o cursor até o usuário soltar.
            if (mCanvasDragMode == 1 && !mCanvasGroupStarts.empty() &&
                ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                const std::string cloneId =
                    Project::ClonarElemento(mode, mProject, mSelectedElementId);
                if (!cloneId.empty())
                {
                    // Original(ais) voltam ao ponto de partida do arrasto.
                    for (const CanvasTransformStart& start : mCanvasGroupStarts)
                    {
                        if (Element* el = Project::ResolverId(mode, start.id))
                        {
                            el->transformacao["x"] = start.x;
                            el->transformacao["y"] = start.y;
                        }
                    }
                    // O clone assume o arrasto a partir da posição atual
                    // (o delta volta a zero neste quadro — sem salto).
                    if (Element* clone = Project::ResolverId(mode, cloneId))
                    {
                        mSelectedElementId = cloneId;
                        mSelectedElementIds = { cloneId };
                        mSelectedCornerMask = 0;
                        mCanvasDragMouseX = mouseX;
                        mCanvasDragMouseY = mouseY;
                        mCanvasPrevDX = 0.0f;
                        mCanvasPrevDY = 0.0f;
                        mCanvasDragX = clone->transformacao.value("x", 0.0f);
                        mCanvasDragY = clone->transformacao.value("y", 0.0f);
                        mCanvasDragW = clone->transformacao.value("largura", 160.0f);
                        mCanvasDragH = clone->transformacao.value("altura", 32.0f);
                        mCanvasDragRotation = Geo::ElementRotation(*clone);
                        Geo::ElementPivot(*clone, mCanvasDragPivotX, mCanvasDragPivotY);
                        mCanvasGroupStarts.clear();
                        CollectTransformStarts(*clone, mCanvasGroupStarts);
                        mRightDragArmed = false;
                        mCloneDragging = false;
                        mCloneForked = true;
                        mProjectDirty = true;
                        mStatusMsg = "Clonando — original voltou ao início; solte para posicionar";
                        mStatusMsgUntil = GetTime() + 4.0;
                    }
                }
            }

            float dx = mouseX - mCanvasDragMouseX;
            float dy = mouseY - mCanvasDragMouseY;
            // Limiar de clique: mover (modo 1) só engaja depois de ~4px de
            // tela — abaixo disso o objeto fica FIXO no lugar. Depois de
            // cruzado, a trava permanece até o fim do arrasto (não volta
            // atrás se o mouse voltar perto do ponto de clique).
            if (mCanvasDragMode == 1 && !mCanvasDragPastThreshold)
            {
                const float movePxX = dx * viewScale;
                const float movePxY = dy * viewScale;
                if (movePxX * movePxX + movePxY * movePxY >= 16.0f)
                    mCanvasDragPastThreshold = true;
            }
            const bool dragEngaged = mCanvasDragMode != 1 || mCanvasDragPastThreshold;
            if (!dragEngaged)
            {
                dx = 0.0f;
                dy = 0.0f;
            }
            if (dragEngaged && mSnapEnabled &&
                !(mCanvasDragMode >= 10 && mCanvasDragMode <= 13))
            {
                constexpr float snapStep = Geo::kGridStep;
                if (mCanvasDragMode == 1)
                {
                    // Snap assertivo estilo CorelDRAW: as guias inteligentes
                    // encaixam bordas/centros PRIMEIRO (no movimento bruto),
                    // depois a grade de 8px e uma re-puxada — o encaixe vence
                    // a grade e o elemento não "escapa" da trava.
                    std::vector<SmartGuides::Rect> guideRects;
                    guideRects.reserve(mCanvasGroupStarts.size());
                    for (const CanvasTransformStart& s : mCanvasGroupStarts)
                        guideRects.push_back({ s.x, s.y, s.w, s.h });
                    const float guideTol = SnapTol(10.0f);
                    const float spacingTol = SnapTol(12.0f);
                    SmartGuides::Apply(mode, guideRects, mSelectedElementIds,
                                       dx, dy, (float)mProject.telaBaseLargura,
                                       (float)mProject.telaBaseAltura,
                                       guideTol, mGuideSnapX, mGuideSnapY);
                    // Previsão de espaçamento (M04, assertiva): QUALQUER
                    // espaço existente entre vizinhos é um alvo — o ímã
                    // mais forte. A grade de 8px NÃO é aplicada no eixo em
                    // que a previsão engatou (senão a quantização desfaz o
                    // encaixe exato, pousando em 31/33 em vez de 32).
                    SmartGuides::ApplySpacing(mode, guideRects, mSelectedElementIds,
                                              dx, dy, spacingTol,
                                              mCanvasPrevDX, mCanvasPrevDY,
                                              mGuideSpacingX1, mGuideSpacingX2,
                                              mGuideSpacingY1, mGuideSpacingY2);
                    // A grade de 8px NÃO é aplicada quando a MOLDURA (ou uma
                    // guia inteligente) já engatou no eixo: a quantização da
                    // grade pode desfazer o snap exato da tela-base (ex.
                    // objeto com x não múltiplo de 8 desgruda da borda 1280
                    // e pousa em 1284). O delimitador principal SEMPRE vence.
                    if (mGuideSpacingX1 < 0.0f && mGuideSnapX < 0.0f)
                        dx = roundf((mCanvasDragX + dx) / snapStep) * snapStep -
                             mCanvasDragX;
                    if (mGuideSpacingY1 < 0.0f && mGuideSnapY < 0.0f)
                        dy = roundf((mCanvasDragY + dy) / snapStep) * snapStep -
                             mCanvasDragY;
                    SmartGuides::Apply(mode, guideRects, mSelectedElementIds,
                                       dx, dy, (float)mProject.telaBaseLargura,
                                       (float)mProject.telaBaseAltura,
                                       guideTol, mGuideSnapX, mGuideSnapY);
                    // Re-afirma a previsão por último: corrige qualquer
                    // desvio da re-puxada e garante o espaçamento EXATO.
                    SmartGuides::ApplySpacing(mode, guideRects, mSelectedElementIds,
                                              dx, dy, spacingTol,
                                              mCanvasPrevDX, mCanvasPrevDY,
                                              mGuideSpacingX1, mGuideSpacingX2,
                                              mGuideSpacingY1, mGuideSpacingY2);
                    // Snap às guias fixas (arrastadas das réguas) por último:
                    // referência intencional do usuário, tem prioridade.
                    SnapGuias(dx, dy, guideRects);
                    mCanvasPrevDX = dx;
                    mCanvasPrevDY = dy;
                }
                else
                {
                    const float snappedMouseX = roundf(mouseX / snapStep) * snapStep;
                    const float snappedMouseY = roundf(mouseY / snapStep) * snapStep;
                    dx = snappedMouseX - mCanvasDragMouseX;
                    dy = snappedMouseY - mCanvasDragMouseY;
                }
            }
            const float minSize = 24.0f;
            float left = mCanvasDragX;
            float top = mCanvasDragY;
            float right = mCanvasDragX + mCanvasDragW;
            float bottom = mCanvasDragY + mCanvasDragH;

            if (mCanvasDragMode == 1)
            {
                // Espaço livre: sem clamp à moldura (o usuário pode sair da
                // tela base por conta própria). O snap já encaixou em dx/dy.
                if (!mSnapEnabled)
                {
                    mGuideSnapX = -1.0f;
                    mGuideSnapY = -1.0f;
                    mGuideSpacingX1 = mGuideSpacingX2 = -1.0f;
                    mGuideSpacingY1 = mGuideSpacingY2 = -1.0f;
                }

                for (const CanvasTransformStart& start : mCanvasGroupStarts)
                {
                    if (Element* element = Project::ResolverId(mode, start.id))
                    {
                        element->transformacao["x"] = start.x + dx;
                        element->transformacao["y"] = start.y + dy;
                        // O ponto de origem acompanha o movimento.
                        if (element->transformacao.is_object() &&
                            element->transformacao.contains("centro_rotacao") &&
                            element->transformacao["centro_rotacao"].is_object())
                        {
                            element->transformacao["centro_rotacao"]["x"] =
                                element->transformacao["centro_rotacao"].value("x", 0.0f) + dx;
                            element->transformacao["centro_rotacao"]["y"] =
                                element->transformacao["centro_rotacao"].value("y", 0.0f) + dy;
                        }
                    }
                }
            }
            else if (mCanvasDragMode >= 10 && mCanvasDragMode <= 13)
            {
                float cornerDelta = 0.0f;
                if (mCanvasDragMode == 10) cornerDelta = (dx + dy) * 0.5f;
                if (mCanvasDragMode == 11) cornerDelta = (-dx + dy) * 0.5f;
                if (mCanvasDragMode == 12) cornerDelta = (-dx - dy) * 0.5f;
                if (mCanvasDragMode == 13) cornerDelta = (dx - dy) * 0.5f;
                const float maximum = std::max(0.0f,
                    std::min(mCanvasDragW, mCanvasDragH) * 0.5f);
                unsigned int activeMask = ImGui::GetIO().KeyCtrl
                    ? 0x0Fu : mCanvasCornerDragMask;
                if (activeMask == 0)
                    activeMask = 1u << (mCanvasDragMode - 10);
                if (ImGui::GetIO().KeyCtrl) mSelectedCornerMask = 0x0Fu;
                for (int corner = 0; corner < 4; ++corner)
                {
                    if ((activeMask & (1u << corner)) == 0) continue;
                    SetElementCornerRadius(*selected, 10 + corner,
                        std::max(0.0f, std::min(maximum,
                            mCanvasCornerRadiusStarts[corner] + cornerDelta)));
                }
            }
            else if (mCanvasDragMode == 14)
            {
                // ROTAÇÃO EM CONJUNTO (corpo rígido): o conjunto inteiro gira
                // como um todo ao redor do pivô — cada elemento ORBITA o
                // centro (posição x/y rotacionada) E ganha o mesmo ângulo na
                // própria rotação. Assim 4 retângulos juntos giram juntos,
                // como no CorelDRAW (Shift trava em 15°).
                const float startAngle = atan2f(mCanvasDragMouseY - mCanvasDragPivotY,
                                                mCanvasDragMouseX - mCanvasDragPivotX);
                const float angle = atan2f(mouseY - mCanvasDragPivotY,
                                           mouseX - mCanvasDragPivotX);
                float deltaDeg = (angle - startAngle) * (180.0f / 3.14159265f);
                if (ImGui::GetIO().KeyShift)
                    deltaDeg = roundf(deltaDeg / 15.0f) * 15.0f;
                const float deltaRad = deltaDeg * 0.017453292519943295f;
                const float cosA = cosf(deltaRad);
                const float sinA = sinf(deltaRad);
                if (mCanvasGroupStarts.empty())
                {
                    selected->transformacao["rotacao"] =
                        fmodf(mCanvasDragRotation + deltaDeg, 360.0f);
                }
                else
                {
                    for (const CanvasTransformStart& start : mCanvasGroupStarts)
                    {
                        Element* element = Project::ResolverId(mode, start.id);
                        if (!element || element->bloqueado) continue;
                        // Orbita o centro do elemento inicial ao redor do pivô.
                        const float dx = start.x + start.w * 0.5f - mCanvasDragPivotX;
                        const float dy = start.y + start.h * 0.5f - mCanvasDragPivotY;
                        const float nx = mCanvasDragPivotX + dx * cosA - dy * sinA;
                        const float ny = mCanvasDragPivotY + dx * sinA + dy * cosA;
                        element->transformacao["x"] = nx - start.w * 0.5f;
                        element->transformacao["y"] = ny - start.h * 0.5f;
                        element->transformacao["rotacao"] =
                            fmodf(start.rot + deltaDeg, 360.0f);
                    }
                }
            }
            else if (mCanvasDragMode == 15)
            {
                // ARRASTAR O PONTO DE ORIGEM (pivô): o pivô segue o mouse com
                // SNAP nos pontos-chave da própria forma (cantos, meios de
                // aresta e centro) + grade de 8px. Ele é o centro do resize
                // espelhado (Shift) e da rotação — reposicionar muda onde a
                // forma "cresce" e em torno do que gira.
                Element* element = Project::ResolverId(mode, mSelectedElementId);
                if (element && !element->bloqueado)
                {
                    const float x = mCanvasDragX, y = mCanvasDragY;
                    const float w = mCanvasDragW, h = mCanvasDragH;
                    const float pivotTol = 9.0f / std::max(0.5f, mCanvasZoom);
                    // Candidatos de snap: centro, 4 cantos, 4 meios de aresta.
                    const float cands[9][2] = {
                        { x + w * 0.5f, y + h * 0.5f },
                        { x, y }, { x + w, y }, { x, y + h }, { x + w, y + h },
                        { x + w * 0.5f, y }, { x + w * 0.5f, y + h },
                        { x, y + h * 0.5f }, { x + w, y + h * 0.5f }
                    };
                    float px = mouseX, py = mouseY;
                    float bestD = pivotTol;
                    for (const auto& c : cands)
                    {
                        const float d = fabsf(c[0] - px) + fabsf(c[1] - py);
                        if (d < bestD) { bestD = d; px = c[0]; py = c[1]; }
                    }
                    // Grade de 8px quando nenhum ponto da forma pegou.
                    if (bestD >= pivotTol && mSnapEnabled)
                    {
                        const float snapStep = Geo::kGridStep;
                        px = roundf(px / snapStep) * snapStep;
                        py = roundf(py / snapStep) * snapStep;
                    }
                    // Centro da forma = estado padrão (remove o override).
                    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
                    if (fabsf(px - cx) < 0.01f && fabsf(py - cy) < 0.01f)
                        element->transformacao.erase("centro_rotacao");
                    else
                        element->transformacao["centro_rotacao"] =
                            { { "x", px }, { "y", py } };
                    mProjectDirty = true;
                }
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            }
            else if (mCanvasDragMode == 16 || mCanvasDragMode == 17)
            {
                // Edição de NÓS do caminho (caneta): 16 move o ponto, 17
                // arrasta a alça de saída (cria curva). Coordenadas em
                // unidades do projeto, convertidas para o espaço local.
                Element* element = Project::ResolverId(mode, mSelectedElementId);
                if (element && !element->bloqueado &&
                    element->transformacao.contains("pontos") &&
                    element->transformacao["pontos"].is_array())
                {
                    auto& pts = element->transformacao["pontos"];
                    const int index = mPathEditIndex;
                    if (index >= 0 && index < (int)pts.size())
                    {
                        const float bx = element->transformacao.value("x", 0.0f);
                        const float by = element->transformacao.value("y", 0.0f);
                        if (mCanvasDragMode == 16)
                        {
                            pts[index]["x"] = mouseX - bx;
                            pts[index]["y"] = mouseY - by;
                        }
                        else
                        {
                            const float nx = bx + pts[index].value("x", 0.0f);
                            const float ny = by + pts[index].value("y", 0.0f);
                            pts[index]["cx2"] = mouseX - nx;
                            pts[index]["cy2"] = mouseY - ny;
                            pts[index]["curva"] = 1.0f;
                        }
                        mProjectDirty = true;
                    }
                }
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            }
            else
            {
                const bool resizeLeft = mCanvasDragMode == 2 || mCanvasDragMode == 6 ||
                                        mCanvasDragMode == 8;
                const bool resizeRight = mCanvasDragMode == 3 || mCanvasDragMode == 7 ||
                                         mCanvasDragMode == 9;
                const bool resizeTop = mCanvasDragMode == 4 || mCanvasDragMode == 6 ||
                                       mCanvasDragMode == 7;
                const bool resizeBottom = mCanvasDragMode == 5 || mCanvasDragMode == 8 ||
                                          mCanvasDragMode == 9;

                // Shift+Alt + alça de CANTO = redimensionamento PROPORCIONAL
                // (uniforme): largura e altura escalam JUNTAS, preservando a
                // proporção original (largura/altura = constante — ex.
                // 200x100 -> 400x200). O canto OPOSTO à alça arrastada fica
                // fixo como âncora, então o objeto não desloca durante a
                // transformação.
                // Shift isolado + qualquer alça = redimensionamento ESPELHADO
                // a partir do PONTO DE ORIGEM (pivô, no centro por padrão):
                // puxar uma aresta faz a OPOSTA espelhar o movimento para o
                // lado contrário — a forma estica proporcionalmente para os
                // dois lados (lateral e superior), estilo CorelDRAW.
                // Sem Shift, o comportamento livre (deformar) permanece.
                // Ao redimensionar, o ponto de origem acompanha a forma
                // mantendo a posição relativa (o pivô "viaja" com o objeto).
                auto RescalePivot = [](Element* el, float ol, float ot,
                                       float ow, float oh,
                                       float nl, float nt, float nw, float nh)
                {
                    if (!el || !el->transformacao.is_object() ||
                        !el->transformacao.contains("centro_rotacao") ||
                        !el->transformacao["centro_rotacao"].is_object())
                        return;
                    const float px = el->transformacao["centro_rotacao"].value("x", 0.0f);
                    const float py = el->transformacao["centro_rotacao"].value("y", 0.0f);
                    const float sx = (ow > 0.01f) ? (nw / ow) : 1.0f;
                    const float sy = (oh > 0.01f) ? (nh / oh) : 1.0f;
                    el->transformacao["centro_rotacao"] = {
                        { "x", nl + (px - ol) * sx },
                        { "y", nt + (py - ot) * sy }
                    };
                };
                const bool proportional = ImGui::GetIO().KeyShift &&
                                          ImGui::GetIO().KeyAlt &&
                                          mCanvasDragMode >= 6 &&
                                          mCanvasDragMode <= 9;
                const bool mirrored = ImGui::GetIO().KeyShift &&
                                      !ImGui::GetIO().KeyAlt &&
                                      mCanvasDragMode >= 2 &&
                                      mCanvasDragMode <= 9;
                float propScale = 1.0f;
                float anchorX = 0.0f, anchorY = 0.0f;
                if (proportional)
                {
                    // Canto arrastado (posição do mouse) e âncora (oposto).
                    const float x0 = mCanvasDragX, y0 = mCanvasDragY;
                    const float x1 = mCanvasDragX + mCanvasDragW;
                    const float y1 = mCanvasDragY + mCanvasDragH;
                    float hx, hy, ax, ay;
                    switch (mCanvasDragMode)
                    {
                        case 6: hx = x0 + dx; hy = y0 + dy; ax = x1; ay = y1; break; // TL
                        case 7: hx = x1 + dx; hy = y0 + dy; ax = x0; ay = y1; break; // TR
                        case 8: hx = x0 + dx; hy = y1 + dy; ax = x1; ay = y0; break; // BL
                        default: hx = x1 + dx; hy = y1 + dy; ax = x0; ay = y0; break; // BR
                    }
                    const float origW = std::max(0.01f, mCanvasDragW);
                    const float origH = std::max(0.01f, mCanvasDragH);
                    propScale = std::max(fabsf(hx - ax) / origW,
                                         fabsf(hy - ay) / origH);
                    propScale = std::max(propScale,
                                         minSize / std::max(origW, origH));
                    anchorX = ax;
                    anchorY = ay;
                }

                // Resize livre (sem Shift): sem clamps à moldura.
                if (resizeLeft)
                    left = std::min(right - minSize, mCanvasDragX + dx);
                if (resizeRight)
                    right = std::max(left + minSize, mCanvasDragX + mCanvasDragW + dx);
                if (resizeTop)
                    top = std::min(bottom - minSize, mCanvasDragY + dy);
                if (resizeBottom)
                    bottom = std::max(top + minSize, mCanvasDragY + mCanvasDragH + dy);

                // Resize ESPELHADO (Shift isolado): a aresta oposta à alça
                // espelha o movimento em torno do ponto de origem (pivô).
                if (mirrored)
                {
                    const float px = mCanvasDragPivotX;
                    const float py = mCanvasDragPivotY;
                    const float origLeft = mCanvasDragX;
                    const float origRight = mCanvasDragX + mCanvasDragW;
                    const float origTop = mCanvasDragY;
                    const float origBottom = mCanvasDragY + mCanvasDragH;
                    if (resizeRight)
                    {
                        right = origRight + dx;
                        left = 2.0f * px - origRight - dx;
                    }
                    else if (resizeLeft)
                    {
                        left = origLeft + dx;
                        right = 2.0f * px - origLeft - dx;
                    }
                    if (resizeBottom)
                    {
                        bottom = origBottom + dy;
                        top = 2.0f * py - origBottom - dy;
                    }
                    else if (resizeTop)
                    {
                        top = origTop + dy;
                        bottom = 2.0f * py - origTop - dy;
                    }
                    if (right - left < minSize)
                    {
                        const float c = (right + left) * 0.5f;
                        left = c - minSize * 0.5f;
                        right = c + minSize * 0.5f;
                    }
                    if (bottom - top < minSize)
                    {
                        const float c = (bottom + top) * 0.5f;
                        top = c - minSize * 0.5f;
                        bottom = c + minSize * 0.5f;
                    }
                }

                // Snap das ARESTAS às guias fixas das réguas durante o
                // RESIZE (a régua é referência de encaixe também ao
                // redimensionar, não só ao mover): a aresta arrastada
                // encaixa na guia mais próxima e a guia engatada acende em
                // laranja (feedback igual ao do mover).
                // Depois, a MOLDURA da tela-base (delimitador principal)
                // magnetiza com força 8x e VENCE as guias em empate — é o
                // que faz a aresta "travar" claramente nas laterais do
                // canvas ao redimensionar.
                if (mSnapEnabled)
                {
                    mGuideFixedSnapX = -1.0f;
                    mGuideFixedSnapY = -1.0f;
                    mGuideSnapX = -1.0f;
                    mGuideSnapY = -1.0f;
                    SmartGuides::SnapResizeToGuides(
                        left, right, top, bottom,
                        resizeLeft, resizeRight, resizeTop, resizeBottom,
                        mGuidesV, mGuidesH,
                        SnapTol(12.0f), mirrored,
                        mCanvasDragPivotX, mCanvasDragPivotY,
                        mGuideFixedSnapX, mGuideFixedSnapY);
                    // Moldura primeiro de tudo no ajuste (roda por último
                    // aqui para ter a palavra final no eixo).
                    SmartGuides::SnapResizeToFrame(
                        left, right, top, bottom,
                        resizeLeft, resizeRight, resizeTop, resizeBottom,
                        (float)mProject.telaBaseLargura,
                        (float)mProject.telaBaseAltura,
                        8.0f * SnapTol(12.0f), mirrored,
                        mCanvasDragPivotX, mCanvasDragPivotY,
                        mGuideSnapX, mGuideSnapY);
                }

                if (mCanvasGroupStarts.size() > 1)
                {
                    // Resize em grupo: preserva o layout relativo dentro da
                    // caixa conjunta, escalando cada elemento proporcionalmente.
                    float groupLeft = mCanvasDragX;
                    float groupTop = mCanvasDragY;
                    float groupRight = mCanvasDragX + mCanvasDragW;
                    float groupBottom = mCanvasDragY + mCanvasDragH;
                    for (const CanvasTransformStart& start : mCanvasGroupStarts)
                    {
                        groupLeft = std::min(groupLeft, start.x);
                        groupTop = std::min(groupTop, start.y);
                        groupRight = std::max(groupRight, start.x + start.w);
                        groupBottom = std::max(groupBottom, start.y + start.h);
                    }
                    const float groupW = std::max(0.01f, groupRight - groupLeft);
                    const float groupH = std::max(0.01f, groupBottom - groupTop);
                    float newLeft = groupLeft;
                    float newRight = groupRight;
                    float newTop = groupTop;
                    float newBottom = groupBottom;
                    float scaleX = 1.0f, scaleY = 1.0f;
                    if (proportional)
                    {
                        // Caixa do grupo proporcional: âncora = canto oposto
                        // da caixa CONJUNTA; escala uniforme nos dois eixos.
                        const float gx0 = groupLeft, gy0 = groupTop;
                        const float gx1 = groupRight, gy1 = groupBottom;
                        float hx, hy, ax, ay;
                        switch (mCanvasDragMode)
                        {
                            case 6: hx = gx0 + dx; hy = gy0 + dy; ax = gx1; ay = gy1; break;
                            case 7: hx = gx1 + dx; hy = gy0 + dy; ax = gx0; ay = gy1; break;
                            case 8: hx = gx0 + dx; hy = gy1 + dy; ax = gx1; ay = gy0; break;
                            default: hx = gx1 + dx; hy = gy1 + dy; ax = gx0; ay = gy0; break;
                        }
                        const float s = std::max(fabsf(hx - ax) / groupW,
                                                 fabsf(hy - ay) / groupH);
                        const float nw = groupW * s, nh = groupH * s;
                        switch (mCanvasDragMode)
                        {
                            case 6: newLeft = ax - nw; newRight = ax;
                                    newTop = ay - nh; newBottom = ay; break;
                            case 7: newLeft = ax; newRight = ax + nw;
                                    newTop = ay - nh; newBottom = ay; break;
                            case 8: newLeft = ax - nw; newRight = ax;
                                    newTop = ay; newBottom = ay + nh; break;
                            default: newLeft = ax; newRight = ax + nw;
                                     newTop = ay; newBottom = ay + nh; break;
                        }
                        scaleX = scaleY = s;
                    }
                    else if (mirrored)
                    {
                        // Conjunto espelhado em torno do CENTRO da caixa
                        // conjunta (Shift isolado): a aresta oposta à alça
                        // espelha o movimento — cresce para os dois lados.
                        const float px = groupLeft + groupW * 0.5f;
                        const float py = groupTop + groupH * 0.5f;
                        if (resizeRight)
                        {
                            newRight = groupRight + dx;
                            newLeft = 2.0f * px - groupRight - dx;
                        }
                        else if (resizeLeft)
                        {
                            newLeft = groupLeft + dx;
                            newRight = 2.0f * px - groupLeft - dx;
                        }
                        if (resizeBottom)
                        {
                            newBottom = groupBottom + dy;
                            newTop = 2.0f * py - groupBottom - dy;
                        }
                        else if (resizeTop)
                        {
                            newTop = groupTop + dy;
                            newBottom = 2.0f * py - groupTop - dy;
                        }
                        if (newRight - newLeft < minSize)
                        {
                            const float c = (newRight + newLeft) * 0.5f;
                            newLeft = c - minSize * 0.5f;
                            newRight = c + minSize * 0.5f;
                        }
                        if (newBottom - newTop < minSize)
                        {
                            const float c = (newBottom + newTop) * 0.5f;
                            newTop = c - minSize * 0.5f;
                            newBottom = c + minSize * 0.5f;
                        }
                        scaleX = (newRight - newLeft) / groupW;
                        scaleY = (newBottom - newTop) / groupH;
                    }
                    else
                    {
                        // Resize do conjunto: livre da moldura, escalando todos.
                        if (resizeLeft)
                            newLeft = std::min(newRight - minSize, groupLeft + dx);
                        if (resizeRight)
                            newRight = std::max(newLeft + minSize, groupRight + dx);
                        if (resizeTop)
                            newTop = std::min(newBottom - minSize, groupTop + dy);
                        if (resizeBottom)
                            newBottom = std::max(newTop + minSize, groupBottom + dy);
                        scaleX = (newRight - newLeft) / groupW;
                        scaleY = (newBottom - newTop) / groupH;
                    }
                    // Snap das arestas da caixa CONJUNTA às guias fixas e à
                    // MOLDURA (delimitador principal, força 8x — vence).
                    if (mSnapEnabled)
                    {
                        mGuideFixedSnapX = -1.0f;
                        mGuideFixedSnapY = -1.0f;
                        mGuideSnapX = -1.0f;
                        mGuideSnapY = -1.0f;
                        SmartGuides::SnapResizeToGuides(
                            newLeft, newRight, newTop, newBottom,
                            resizeLeft, resizeRight, resizeTop, resizeBottom,
                            mGuidesV, mGuidesH,
                            SnapTol(12.0f), mirrored,
                            groupLeft + groupW * 0.5f,
                            groupTop + groupH * 0.5f,
                            mGuideFixedSnapX, mGuideFixedSnapY);
                        SmartGuides::SnapResizeToFrame(
                            newLeft, newRight, newTop, newBottom,
                            resizeLeft, resizeRight, resizeTop, resizeBottom,
                            (float)mProject.telaBaseLargura,
                            (float)mProject.telaBaseAltura,
                            8.0f * SnapTol(12.0f),
                            mirrored,
                            groupLeft + groupW * 0.5f,
                            groupTop + groupH * 0.5f,
                            mGuideSnapX, mGuideSnapY);
                        scaleX = (newRight - newLeft) / groupW;
                        scaleY = (newBottom - newTop) / groupH;
                    }
                    for (const CanvasTransformStart& start : mCanvasGroupStarts)
                    {
                        Element* element = Project::ResolverId(mode, start.id);
                        if (!element) continue;
                        element->transformacao["x"] =
                            newLeft + (start.x - groupLeft) * scaleX;
                        element->transformacao["y"] =
                            newTop + (start.y - groupTop) * scaleY;
                        element->transformacao["largura"] =
                            std::max(1.0f, start.w * scaleX);
                        element->transformacao["altura"] =
                            std::max(1.0f, start.h * scaleY);
                        ClampElementCornerRadii(*element);
                    }
                }
                else if (proportional)
                {
                    // Elemento único proporcional: caixa derivada da âncora.
                    const float newW = mCanvasDragW * propScale;
                    const float newH = mCanvasDragH * propScale;
                    switch (mCanvasDragMode)
                    {
                        case 6: left = anchorX - newW; right = anchorX;
                                top = anchorY - newH; bottom = anchorY; break;
                        case 7: left = anchorX; right = anchorX + newW;
                                top = anchorY - newH; bottom = anchorY; break;
                        case 8: left = anchorX - newW; right = anchorX;
                                top = anchorY; bottom = anchorY + newH; break;
                        default: left = anchorX; right = anchorX + newW;
                                 top = anchorY; bottom = anchorY + newH; break;
                    }
                    // Snap das arestas às guias fixas (elemento proporcional).
                    if (mSnapEnabled)
                    {
                        mGuideFixedSnapX = -1.0f;
                        mGuideFixedSnapY = -1.0f;
                        SmartGuides::SnapResizeToGuides(
                            left, right, top, bottom,
                            resizeLeft, resizeRight, resizeTop, resizeBottom,
                            mGuidesV, mGuidesH,
                            SnapTol(12.0f), false,
                            0.0f, 0.0f,
                            mGuideFixedSnapX, mGuideFixedSnapY);
                    }
                    selected->transformacao["x"] = left;
                    selected->transformacao["y"] = top;
                    selected->transformacao["largura"] = right - left;
                    selected->transformacao["altura"] = bottom - top;
                    ClampElementCornerRadii(*selected);
                    RescalePivot(selected, mCanvasDragX, mCanvasDragY,
                                 mCanvasDragW, mCanvasDragH,
                                 left, top, right - left, bottom - top);
                }
                else if (mirrored)
                {
                    // Elemento único espelhado (Shift isolado): left/right e
                    // top/bottom já foram espelhados em torno do pivô acima.
                    selected->transformacao["x"] = left;
                    selected->transformacao["y"] = top;
                    selected->transformacao["largura"] = right - left;
                    selected->transformacao["altura"] = bottom - top;
                    ClampElementCornerRadii(*selected);
                    RescalePivot(selected, mCanvasDragX, mCanvasDragY,
                                 mCanvasDragW, mCanvasDragH,
                                 left, top, right - left, bottom - top);
                }
                else
                {
                    selected->transformacao["x"] = left;
                    selected->transformacao["y"] = top;
                    selected->transformacao["largura"] = right - left;
                    selected->transformacao["altura"] = bottom - top;
                    ClampElementCornerRadii(*selected);
                    RescalePivot(selected, mCanvasDragX, mCanvasDragY,
                                 mCanvasDragW, mCanvasDragH,
                                 left, top, right - left, bottom - top);
                }
            }

            mCanvasDragChanged = dragEngaged;
            if (dragEngaged) mProjectDirty = true;
            applyCursor(mCanvasDragMode);

            // Preview de espaçamento (estilo CorelDRAW): pequenos traços nos
            // cantos das laterais de cada objeto da fileira + valor de cada
            // distância. Aparece com Shift (exploração) OU quando a previsão
            // de espaçamento ENGATA — o rótulo do espaço previsto fica
            // destacado: ali é o ponto onde o usuário possivelmente quer
            // estar (trava forte, sem prender).
            mShiftGuides.clear();
            mShiftGuidesLabels.clear();
            const bool spacingEngaged = mGuideSpacingX1 >= 0.0f ||
                                        mGuideSpacingY1 >= 0.0f;
            if (mCanvasDragMode == 1 &&
                (ImGui::GetIO().KeyShift || spacingEngaged))
            {
                const float previewTol = 5.0f / std::max(0.25f, mCanvasZoom);
                const float markSize = 7.0f / std::max(0.25f, mCanvasZoom);
                const float engagedValue = mGuideSpacingX1 >= 0.0f
                    ? mGuideSpacingX2 - mGuideSpacingX1
                    : (mGuideSpacingY1 >= 0.0f
                        ? mGuideSpacingY2 - mGuideSpacingY1 : -1.0f);
                SmartGuides::ComputeSpacingPreview(mode, mSelectedElementIds,
                                                   previewTol, markSize,
                                                   engagedValue,
                                                   mShiftGuides,
                                                   mShiftGuidesLabels);
            }
        }

        if (mCanvasDragMode != 0 &&
            (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
             (mCloneDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Right))))
        {
            if (mCloneDragging)
            {
                mStatusMsg = "Clone criado (botão direito)";
                mStatusMsgUntil = GetTime() + 4.0;
            }
            else if (mCanvasDragChanged)
            {
                if (mCanvasDragMode == 1)
                {
                    if (mCloneForked)
                    {
                        mStatusMsg = "Clone posicionado — original voltou à posição inicial";
                        mStatusMsgUntil = GetTime() + 4.0;
                    }
                    else
                    {
                        mStatusMsg = std::to_string(mCanvasGroupStarts.size()) +
                                     " elemento(s) movido(s)";
                        mStatusMsgUntil = GetTime() + 4.0;
                    }
                }
                else if (mCanvasDragMode == 14)
                    mStatusMsg = "Rotação ajustada";
                else if (mCanvasDragMode >= 10 && mCanvasDragMode <= 13)
                    mStatusMsg = "Arredondamento da quina alterado";
                else
                    mStatusMsg = "Elemento redimensionado";
                mStatusMsgUntil = GetTime() + 4.0;
                TraceLog(LOG_INFO, "M05 selecao: transformacao alterada (%s)",
                         mSelectedElementId.c_str());
            }
            // Após transformações, a caixa dos GRUPOS deve voltar a envolver
            // os filhos (mover/redimensionar/rotacionar um grupo altera os
            // filhos — o contêiner acompanha).
            if (PossuiModoAtivo())
                RebuildGroupBounds(mProject.telas[mTelaAtiva].modos[mModoAtivo]);

            mCanvasDragMode = 0;
            mCanvasDragChanged = false;
            mCanvasGroupStarts.clear();
            mGuideSnapX = -1.0f;
            mGuideSnapY = -1.0f;
            mGuideFixedSnapX = -1.0f;
            mGuideFixedSnapY = -1.0f;
            mGuideSpacingX1 = mGuideSpacingX2 = -1.0f;
            mGuideSpacingY1 = mGuideSpacingY2 = -1.0f;
            mShiftGuides.clear();
            mShiftGuidesLabels.clear();
            mRightDragArmed = false;
            mCloneDragging = false;
            mCloneForked = false;
            mCanvasDragPastThreshold = false;
        }
    }
    void App::DrawElementTree(Element& element)
    {
        if (!ElementMatchesSearch(element, mHierarchySearch)) return;

        if (mRenamingElementId == element.id && element.bloqueado)
            mRenamingElementId.clear();

        if (mRenamingElementId == element.id)
        {
            ImGui::PushID(element.id.c_str());
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("##rename", mRenameBuffer, sizeof(mRenameBuffer),
                                 ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (mRenameBuffer[0])
                {
                    element.nome = mRenameBuffer;
                    mProjectDirty = true;
                }
                mRenamingElementId.clear();
            }
            if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
                mRenamingElementId.clear();
            ImGui::PopID();
            return;
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;
        if (element.filhos.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
        if (element.id == mSelectedElementId) flags |= ImGuiTreeNodeFlags_Selected;
        if (mHierarchySearch[0]) flags |= ImGuiTreeNodeFlags_DefaultOpen;

        std::string visibleName = element.nome.empty() ? element.id : element.nome;
        if (!element.visivel) visibleName += "  (oculto)";
        if (element.bloqueado) visibleName += "  (bloqueado)";
        const std::string label = visibleName + "##" + element.id;

        if (!element.visivel) ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDisabled);
        const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
        if (!element.visivel) ImGui::PopStyleColor();
        if (ImGui::IsItemClicked()) mSelectedElementId = element.id;

        if (!element.bloqueado &&
            ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::SetDragDropPayload("SEEDUI_ELEMENT", element.id.c_str(), element.id.size() + 1);
            ImGui::TextUnformatted(visibleName.c_str());
            ImGui::TextColored(Theme::TextDisabled, "Mover na hierarquia");
            ImGui::EndDragDropSource();
        }
        if (!element.bloqueado && ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SEEDUI_ELEMENT"))
            {
                mPendingReparentElementId = (const char*)payload->Data;
                mPendingReparentTargetId = element.id;
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Renomear", "F2", false, !element.bloqueado))
            {
                mRenamingElementId = element.id;
                const std::string current = element.nome.empty() ? element.id : element.nome;
                strncpy(mRenameBuffer, current.c_str(), sizeof(mRenameBuffer) - 1);
                mRenameBuffer[sizeof(mRenameBuffer) - 1] = 0;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Mover para cima", nullptr, false, !element.bloqueado))
            {
                mPendingMoveElementId = element.id;
                mPendingMoveDelta = -1;
            }
            if (ImGui::MenuItem("Mover para baixo", nullptr, false, !element.bloqueado))
            {
                mPendingMoveElementId = element.id;
                mPendingMoveDelta = 1;
            }
            ImGui::Separator();
            if (ImGui::MenuItem(element.visivel ? "Ocultar" : "Mostrar"))
            {
                element.visivel = !element.visivel;
                mProjectDirty = true;
            }
            if (ImGui::MenuItem(element.bloqueado ? "Desbloquear" : "Bloquear"))
            {
                element.bloqueado = !element.bloqueado;
                mProjectDirty = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Excluir", "Del", false, !element.bloqueado))
                mPendingDeleteElementId = element.id;
            ImGui::EndPopup();
        }

        if (open)
        {
            ImGui::TextColored(Theme::TextDisabled, "id: %s · tipo: %s",
                               element.id.c_str(), element.tipo.c_str());
            for (Element& child : element.filhos) DrawElementTree(child);
            ImGui::TreePop();
        }
    }
    void App::DrawNovoProjetoDialog()
    {
        if (!mShowNovoProjeto) return;

        ImGui::SetNextWindowSize(ImVec2(460, 0), ImGuiCond_Always);
        if (!ImGui::Begin("Novo projeto", &mShowNovoProjeto,
                          ImGuiWindowFlags_NoResize))
        {
            ImGui::End();
            return;
        }

        ImGui::TextWrapped("Nome do projeto (vira o arquivo .ui.json):");
        ImGui::InputText("##nome", mNovoNome, sizeof mNovoNome);

        ImGui::Spacing();
        ImGui::TextWrapped("Descrição (opcional, guardada nos metadados):");
        ImGui::InputTextMultiline("##desc", mNovoDescricao, sizeof mNovoDescricao,
                                  ImVec2(-1.0f, 60.0f));

        ImGui::Spacing();
        ImGui::Text("Tela de referência (base do canvas):");
        ImGui::SetNextItemWidth(90.0f);
        ImGui::InputInt("Largura", &mNovoLargura, 0, 0);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        ImGui::InputInt("Altura", &mNovoAltura, 0, 0);

        ImGui::Spacing();
        const char* hint =
            "Cria telas & modos: uma tela \"Tela principal\" com o modo \"Padrão\". "
            "Adicione mais em HIERARQUIA depois.";
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDisabled);
        ImGui::TextWrapped("%s", hint);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        const bool nomeOk = mNovoNome[0] != 0;
        if (!nomeOk) ImGui::BeginDisabled();
        if (ImGui::Button("Criar e começar a desenhar", ImVec2(-1.0f, 0.0f)))
        {
            mProject.CriarNovo(nomeOk ? mNovoNome : "Novo projeto",
                               mNovoLargura, mNovoAltura);
            mHasProject = true;
            ResetarHistorico();
            mProjectDirty = false;
            mTelaAtiva = 0;
            mModoAtivo = 0;
            mSelectedElementId.clear();
            mUltimoCaminho.clear();
            mShowNovoProjeto = false;
            mStatusMsg = "Projeto criado: " + mProject.nome + " (salve com Ctrl+S)";
            mStatusMsgUntil = GetTime() + 8.0;
        }
        if (!nomeOk) ImGui::EndDisabled();

        ImGui::End();
    }

    void App::DrawWorkspace()
    {
        // Atalhos
        if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) mShowManual = !mShowManual;
        if (mShowManual && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) mShowManual = false;
        if (ImGui::IsKeyPressed(ImGuiKey_F12, false)) TakeScreenshot("seedui_capture.png");
        if (IsKeyPressed(KEY_F11))
        {
            if (IsWindowMaximized()) RestoreWindow();
            else MaximizeWindow();
        }
        if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
            ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z, false))
            Refazer();
        else if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
                 ImGui::IsKeyPressed(ImGuiKey_Z, false))
            Desfazer();
        if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
            !ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_G, false))
            AgruparElementosSelecionados();
        if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
            ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_G, false))
            DesagruparElementosSelecionados();
        if (!ImGui::GetIO().WantTextInput && !ImGui::GetIO().KeyCtrl &&
            !ImGui::GetIO().KeyAlt)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) AlinharElementosSelecionados(0);
            if (ImGui::IsKeyPressed(ImGuiKey_W, false)) AlinharElementosSelecionados(1);
            if (ImGui::IsKeyPressed(ImGuiKey_E, false)) AlinharElementosSelecionados(2);
            if (ImGui::IsKeyPressed(ImGuiKey_2, false)) AlinharElementosSelecionados(3);
            if (ImGui::IsKeyPressed(ImGuiKey_D, false)) AlinharElementosSelecionados(4);
            if (ImGui::IsKeyPressed(ImGuiKey_S, false)) AlinharElementosSelecionados(5);
            if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) mCurrentTool = Tool::Zoom;
            if (ImGui::IsKeyPressed(ImGuiKey_X, false)) ApagarElementosSelecionados();
            // Setas movem o elemento selecionado (1px com Shift, 8px sem) —
            // sem modificador, para não conflitar com Ctrl+seta (camadas).
            if (PossuiModoAtivo() && !mSelectedElementId.empty())
            {
                const float step = ImGui::GetIO().KeyShift ? 1.0f : 8.0f;
                float mx = 0.0f, my = 0.0f;
                if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false)) my = -step;
                if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false)) my = step;
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) mx = -step;
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) mx = step;
                if (mx != 0.0f || my != 0.0f)
                {
                    Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
                    bool changed = false;
                    for (const std::string& id : mSelectedElementIds)
                    {
                        Element* el = Project::ResolverId(mode, id);
                        if (!el || el->bloqueado) continue;
                        const float oldX = el->transformacao.value("x", 0.0f);
                        const float oldY = el->transformacao.value("y", 0.0f);
                        ApplyPositionDelta(*el, mx, my);
                        if (el->transformacao.value("x", 0.0f) != oldX ||
                            el->transformacao.value("y", 0.0f) != oldY)
                            changed = true;
                    }
                    if (changed)
                    {
                        mProjectDirty = true;
                        if (PossuiModoAtivo())
                            RebuildGroupBounds(mProject.telas[mTelaAtiva].modos[mModoAtivo]);
                    }
                }
            }
        }
        // Camadas (estilo CorelDRAW): Ctrl+seta cima/baixo sobe/desce o
        // elemento na ordem de renderização (frente/trás).
        if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
            !ImGui::GetIO().KeyShift && !ImGui::GetIO().KeyAlt &&
            PossuiModoAtivo() && !mSelectedElementId.empty())
        {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false))
                MoverCamadaSelecionada(+1);
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false))
                MoverCamadaSelecionada(-1);
        }
        if (!ImGui::GetIO().WantTextInput && !ImGui::GetIO().KeyCtrl &&
            !ImGui::GetIO().KeyAlt && ImGui::IsKeyPressed(ImGuiKey_A, false))
            AlternarModoAnotacao();
        if (!ImGui::GetIO().WantTextInput && !ImGui::GetIO().KeyCtrl &&
            !ImGui::GetIO().KeyAlt && ImGui::IsKeyPressed(ImGuiKey_V, false))
            mCurrentTool = Tool::Select;
        if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift &&
            ImGui::IsKeyPressed(ImGuiKey_C, false))
        {
            ImGui::SetClipboardText(AnnotationsToText(mAnnot, mGlobalDirectives).c_str());
        }
        if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
            !ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_C, false))
        {
            CopiarElementosSelecionados();
        }
        if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
            !ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_V, false))
        {
            ColarElementosCopiados();
        }
        if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
            !ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_D, false))
        {
            DuplicarSelecao();
        }
        if (ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift &&
            ImGui::IsKeyPressed(ImGuiKey_S, false))
        {
            if (mHasProject) SalvarProjeto(false);
        }
        if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift &&
            ImGui::IsKeyPressed(ImGuiKey_S, false))
        {
            if (mHasProject) SalvarProjeto(true);
        }
        if (ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift &&
            ImGui::IsKeyPressed(ImGuiKey_O, false))
        {
            AbrirProjeto();
        }

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowBgAlpha(1.0f);

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin("##workspace", nullptr, flags);
        {
            DrawMenuBar();

            if (mHasProject)
            {
                // 42 px = 5 px de margem + botão de 32 px + 5 px de margem.
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 5.0f));
                ImGui::BeginChild("##actionbar", ImVec2(0, kActionBarHeight), false,
                                  ImGuiWindowFlags_NoScrollbar);
                DrawActionBar();
                ImGui::EndChild();
                ImGui::PopStyleVar();
                // ItemSize acrescenta ItemSpacing.y após o child. Removemos esse
                // espaço para a barra terminar exatamente onde começa a próxima.
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);

                // Barra de propriedades contextual (estilo CorelDRAW): abaixo da
                // toolbar principal, mostra X/Y/Largura/Altura/Rotação da seleção
                // com entrada numérica direta, unidade, precisão e zoom. Compacta
                // para não competir com o canvas.
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 3.0f));
                ImGui::BeginChild("##propertybar", ImVec2(0, kPropertyBarHeight), false,
                                  ImGuiWindowFlags_NoScrollbar);
                DrawPropertyBar();
                ImGui::EndChild();
                ImGui::PopStyleVar();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);

                const float availY = ImGui::GetContentRegionAvail().y -
                                     kStatusBarHeight - kColorBarHeight;

                // Toolbar com rolagem fina (estilo Blender): se as ferramentas
                // não couberem na altura disponível, uma barra de rolagem fina
                // aparece na borda — nenhum ícone fica cortado/invisível. O
                // scroll da RODA não rola a toolbar (pertence ao zoom do canvas);
                // a rolagem é feita arrastando a barra ou com o trackpad.
                ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 0));
                ImGui::BeginChild("##toolbar", ImVec2(kToolbarWidth, availY), false,
                                  ImGuiWindowFlags_NoScrollWithMouse);
                DrawToolbar();
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();

                ImGui::SameLine();

                const float rightWidth = mRightPanelCollapsed ? kRightRailWidth : mRightPanelWidth;
                ImGui::BeginChild("##canvas", ImVec2(-(rightWidth + 6.0f), availY), true,
                                  ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoScrollWithMouse);
                const ImVec2 canvasSurface = ImGui::GetContentRegionAvail();
                ImGui::InvisibleButton("##canvas_surface", canvasSurface);
                const ImVec2 canvasDropMin = ImGui::GetItemRectMin();
                const ImVec2 canvasDropMax = ImGui::GetItemRectMax();
                const bool canvasHovered = ImGui::IsItemHovered(
                    ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                if (canvasHovered)
                {
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
                    {
                        const ImVec2 delta = ImGui::GetIO().MouseDelta;
                        mCanvasPanX += delta.x;
                        mCanvasPanY += delta.y;
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    }
                    const float wheel = ImGui::GetIO().MouseWheel;
                    if (wheel > 0.0f) ZoomCanvas(1.1f);
                    if (wheel < 0.0f) ZoomCanvas(1.0f / 1.1f);
                    if (mCurrentTool == Tool::Zoom)
                    {
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            ZoomCanvas(1.25f);
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                            ZoomCanvas(1.0f / 1.25f);
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    }
                }
                // Guias das réguas ANTES da interação normal: ao arrastar uma
                // guia, a seleção/movimento do canvas é suprimida (o guard em
                // HandleCanvasInteraction devolve cedo quando mGuideDragKind).
                HandleGuidesInteraction();
                HandleCanvasInteraction(canvasHovered);
                CanvasDraw(mHasProject ? &mProject : nullptr, mTelaAtiva, mModoAtivo,
                           &mSelectedElementIds, mSelectedElementId.c_str(),
                           mSelectedCornerMask, mRulersVisible, mRulersLocked,
                           mGridVisible,
                           mCanvasZoom, mCanvasPanX, mCanvasPanY, UnitToPixels());
                DesenharGuias();
                DesenharMedicao();
                // Guias inteligentes: linha magenta na posição encaixada
                // (a janela child recorta o desenho à área do canvas).
                // Guias FIXAS engatadas (forma->guia): laranja, mesma lógica
                // — feedback do encaixe na régua.
                if (mGuideSnapX >= 0.0f || mGuideSnapY >= 0.0f ||
                    mGuideFixedSnapX >= 0.0f || mGuideFixedSnapY >= 0.0f)
                {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    const ImU32 guideColor = ImGui::ColorConvertFloat4ToU32(Theme::SmartGuide);
                    const ImU32 fixedColor = ImGui::ColorConvertFloat4ToU32(
                        Theme::Hex(0xffb347, 1.0f));
                    float sx = 0.0f, sy = 0.0f, scale = 1.0f;
                    if (mGuideSnapX >= 0.0f)
                    {
                        CanvasProjectToScreen(&mProject, mGuideSnapX, 0.0f, sx, sy,
                                              scale, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                        const float topY = sy;
                        CanvasProjectToScreen(&mProject, mGuideSnapX,
                                              (float)mProject.telaBaseAltura, sx, sy,
                                              scale, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                        dl->AddLine(ImVec2(sx, topY), ImVec2(sx, sy), guideColor, 1.0f);
                    }
                if (mGuideSnapY >= 0.0f)
                {
                    CanvasProjectToScreen(&mProject, 0.0f, mGuideSnapY, sx, sy,
                                          scale, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                    const float leftX = sx;
                    CanvasProjectToScreen(&mProject, (float)mProject.telaBaseLargura,
                                          mGuideSnapY, sx, sy, scale,
                                          mCanvasZoom, mCanvasPanX, mCanvasPanY);
                    dl->AddLine(ImVec2(leftX, sy), ImVec2(sx, sy), guideColor, 1.0f);
                }
                // Guias fixas engatadas: linha laranja na posição exata da
                // guia da régua (feedback do snap forma->guia).
                const float frameTop = 0.0f;
                const float frameLeft = 0.0f;
                const float frameRight = (float)mProject.telaBaseLargura;
                const float frameBottom = (float)mProject.telaBaseAltura;
                if (mGuideFixedSnapX >= 0.0f)
                {
                    CanvasProjectToScreen(&mProject, mGuideFixedSnapX, 0.0f, sx, sy,
                                          scale, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                    const float ax = sx;
                    CanvasProjectToScreen(&mProject, mGuideFixedSnapX,
                                          frameBottom, sx, sy,
                                          scale, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                    dl->AddLine(ImVec2(ax, sy), ImVec2(sx, sy), fixedColor, 1.5f);
                }
                if (mGuideFixedSnapY >= 0.0f)
                {
                    CanvasProjectToScreen(&mProject, 0.0f, mGuideFixedSnapY, sx, sy,
                                          scale, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                    const float ay = sy;
                    CanvasProjectToScreen(&mProject, frameRight, mGuideFixedSnapY,
                                          sx, sy, scale,
                                          mCanvasZoom, mCanvasPanX, mCanvasPanY);
                    dl->AddLine(ImVec2(sx, ay), ImVec2(sx, sy), fixedColor, 1.5f);
                }
            }
            // Preview de espaçamento com Shift (estilo CorelDRAW): traços nos
            // cantos das laterais de cada objeto (ciano) + valor de cada
            // distância — o rótulo PREVISTO (engatado) ganha fundo destacado.
            if (!mShiftGuides.empty())
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const ImU32 shiftColor = ImGui::ColorConvertFloat4ToU32(
                    Theme::Hex(0x22d3ee, 0.9f));
                float sx = 0.0f, sy = 0.0f, scale = 1.0f;
                for (const SmartGuides::GuideLine& g : mShiftGuides)
                {
                    CanvasProjectToScreen(&mProject, g.x0, g.y0, sx, sy,
                                          scale, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                    const float ax = sx, ay = sy;
                    CanvasProjectToScreen(&mProject, g.x1, g.y1, sx, sy,
                                          scale, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                    DrawDashedLine(dl, ImVec2(ax, ay), ImVec2(sx, sy), shiftColor);
                }
                for (const SmartGuides::GuideLabel& lb : mShiftGuidesLabels)
                {
                    CanvasProjectToScreen(&mProject, lb.x, lb.y, sx, sy,
                                          scale, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                    char buf[32];
                    snprintf(buf, sizeof buf, "%.1f", lb.value);
                    if (lb.highlight)
                    {
                        const ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(
                            13.0f, FLT_MAX, 0.0f, buf);
                        const ImVec2 p(sx - ts.x * 0.5f - 4.0f, sy - 1.0f);
                        dl->AddRectFilled(p,
                                          ImVec2(p.x + ts.x + 8.0f, p.y + ts.y + 2.0f),
                                          IM_COL32(16, 28, 40, 235), 3.0f);
                        dl->AddText(ImGui::GetFont(), 13.0f,
                                    ImVec2(sx - ts.x * 0.5f, sy),
                                    ImGui::ColorConvertFloat4ToU32(
                                        Theme::Hex(0x7dd3fc, 1.0f)),
                                    buf);
                    }
                    else
                    {
                        dl->AddText(ImGui::GetFont(), 12.0f, ImVec2(sx, sy),
                                    shiftColor, buf);
                    }
                }
            }
                if (mShapeCreating)
                {
                    float sx0 = 0.0f, sy0 = 0.0f, sx1 = 0.0f, sy1 = 0.0f, scale = 1.0f;
                    CanvasProjectToScreen(&mProject, mShapeStartX, mShapeStartY,
                                          sx0, sy0, scale, mCanvasZoom,
                                          mCanvasPanX, mCanvasPanY);
                    CanvasProjectToScreen(&mProject, mShapeEndX, mShapeEndY,
                                          sx1, sy1, scale, mCanvasZoom,
                                          mCanvasPanX, mCanvasPanY);
                    ImGui::GetWindowDrawList()->AddRect(
                        ImVec2(std::min(sx0, sx1), std::min(sy0, sy1)),
                        ImVec2(std::max(sx0, sx1), std::max(sy0, sy1)),
                        ImGui::ColorConvertFloat4ToU32(Theme::AccentBlue), 0.0f, 0, 1.5f);
                }
                if (mCanvasMarquee)
                {
                    float sx0 = 0.0f, sy0 = 0.0f, sx1 = 0.0f, sy1 = 0.0f, scale = 1.0f;
                    CanvasProjectToScreen(&mProject, mCanvasMarqueeStartX, mCanvasMarqueeStartY,
                                          sx0, sy0, scale, mCanvasZoom,
                                          mCanvasPanX, mCanvasPanY);
                    CanvasProjectToScreen(&mProject, mCanvasMarqueeEndX, mCanvasMarqueeEndY,
                                          sx1, sy1, scale, mCanvasZoom,
                                          mCanvasPanX, mCanvasPanY);
                    const ImVec2 a(sx0, sy0), b(sx1, sy1);
                    const ImU32 marqueeColor = ImGui::ColorConvertFloat4ToU32(Theme::AccentOrange);
                    const ImU32 marqueeFill = ImGui::ColorConvertFloat4ToU32(
                        Theme::Hex(0xf59e0b, 0.08f));
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(std::min(a.x, b.x), std::min(a.y, b.y)),
                        ImVec2(std::max(a.x, b.x), std::max(a.y, b.y)), marqueeFill);
                    DrawDashedRect(ImGui::GetWindowDrawList(), a, b, marqueeColor);
                }
                if (PossuiModoAtivo() && ImGui::BeginDragDropTarget())
                {
                    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                        "SEEDUI_COMPONENT", ImGuiDragDropFlags_AcceptBeforeDelivery);
                    if (payload && payload->Data)
                    {
                        const char* tipo = (const char*)payload->Data;
                        if (strcmp(tipo, "painel") == 0)
                        {
                            const ImU32 dropColor = ImGui::ColorConvertFloat4ToU32(Theme::AccentBlue);
                            ImGui::GetWindowDrawList()->AddRect(canvasDropMin, canvasDropMax,
                                                                dropColor, 0.0f, 0, 3.0f);
                            mStatusMsg = "Painel reconhecido · solte para fixar no canvas";
                            mStatusMsgUntil = GetTime() + 0.5;

                            if (payload->IsDelivery())
                            {
                                float projectX = 0.0f, projectY = 0.0f;
                                const ImVec2 mouse = ImGui::GetMousePos();
                                const bool mapped = CanvasScreenToProject(
                                    &mProject, mouse.x, mouse.y, projectX, projectY, true,
                                    mCanvasZoom, mCanvasPanX, mCanvasPanY);
                                TraceLog(LOG_INFO,
                                         "M04 DROP Painel: delivery=%d mapped=%d mouse=%.1f,%.1f project=%.1f,%.1f",
                                         payload->IsDelivery() ? 1 : 0, mapped ? 1 : 0,
                                         mouse.x, mouse.y, projectX, projectY);
                                if (!mapped || !AdicionarComponente("painel", "Painel",
                                                                    projectX, projectY))
                                {
                                    mStatusMsg = "Falha ao fixar Painel · consulte seedui.log";
                                    mStatusMsgUntil = GetTime() + 8.0;
                                    TraceLog(LOG_WARNING, "M04 DROP Painel: falha na criacao");
                                }
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, Theme::Border);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::BorderLight);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::AccentBlue);
                ImGui::Button("##right_splitter", ImVec2(6.0f, availY));
                if (!mRightPanelCollapsed && ImGui::IsItemActive())
                {
                    mRightPanelWidth -= ImGui::GetIO().MouseDelta.x;
                    if (mRightPanelWidth < kRightPanelMinWidth) mRightPanelWidth = kRightPanelMinWidth;
                    if (mRightPanelWidth > kRightPanelMaxWidth) mRightPanelWidth = kRightPanelMaxWidth;
                }
                ImGui::PopStyleColor(3);

                ImGui::SameLine();

                ImGui::BeginChild("##panels", ImVec2(rightWidth, availY), false);
                if (mRightPanelCollapsed) DrawRightRail();
                else DrawRightPanels();
                ImGui::EndChild();

                // Paleta de cores ACIMA da barra de status (sem sobrepor nada;
                // cada faixa tem altura fixa e o rodapé não vaza da janela).
                DrawColorBar();
                DrawStatusBar();

                // Entrada das anotações: cobre a janela INTEIRA (menus, ícones,
                // painéis, canvas). O popup de edição fica acima da camada.
                AnnotationsUpdate(mAnnot,
                    mCurrentTool == Tool::Annotate && !mSkipAnnotationInputThisFrame,
                    vp->Size);
                mSkipAnnotationInputThisFrame = false;
            }
            else
            {
                DrawStartScreen();
            }
        }
        ImGui::End();

        // Anotações por cima de TODA a interface (draw list de primeiro plano).
        // Quando o popup de edição está aberto, não desenha por cima dele.
        if (!mShowManual && !mShowAbout)
        {
            bool clipPopup = false;
            ImVec2 popupMin(0, 0), popupMax(0, 0);
            if (mHasProject && mAnnot.selected >= 0 &&
                mAnnot.selected < (int)mAnnot.items.size())
            {
                const AnnotationsPopupRect pr =
                    AnnotationsPopupRectFor(mAnnot.items[mAnnot.selected], vp->Size);
                popupMin = pr.min;
                popupMax = pr.max;
                clipPopup = true;
            }
            AnnotationsDraw(mAnnot, clipPopup, popupMin, popupMax);
        }

        // Popup de edição da anotação selecionada
        if (mHasProject)
        {
            AnnotationsEditWindow(mAnnot);
        }

        if (mShowManual) DrawManualWindow();
        if (mShowAbout) DrawAboutWindow();
        DrawNovoProjetoDialog();
        DrawColorPickerWindow();
    }

    void App::DrawMenuBar()
    {
        if (!ImGui::BeginMenuBar()) return;

        // Ponto de sangria do topo-esquerda na mesma régua da coluna de
        // ferramentas (o primeiro menu começa no mesmo eixo X dos ícones).
        ImGui::SetCursorPosX(9.0f);

        if (ImGui::BeginMenu("Arquivo"))
        {
            if (mHasProject && ImGui::MenuItem("Fechar projeto (voltar à tela inicial)"))
                FecharProjeto();
            if (ImGui::MenuItem("Novo projeto")) NovoProjeto();
            MenuItemSoon("Novo a partir de modelo", "M09");
            if (ImGui::MenuItem("Abrir projeto", "Ctrl+O")) AbrirProjeto();
            ImGui::Separator();
            const bool salvarHabilitado = mHasProject;
            if (!salvarHabilitado) ImGui::BeginDisabled();
            if (ImGui::MenuItem("Salvar", "Ctrl+S")) SalvarProjeto(false);
            if (ImGui::MenuItem("Salvar como", "Ctrl+Shift+S")) SalvarProjeto(true);
            if (!salvarHabilitado) ImGui::EndDisabled();
            MenuItemSoon("Exportar pacote", "M13");
            ImGui::Separator();
            if (ImGui::MenuItem("Sair", "Alt+F4")) mRunning = false;
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Editar"))
        {
            if (ImGui::MenuItem("Desfazer", "Ctrl+Z", false, mHistoryIndex > 0)) Desfazer();
            const bool canRedo = mHistoryIndex >= 0 && mHistoryIndex + 1 < (int)mHistory.size();
            if (ImGui::MenuItem("Refazer", "Ctrl+Shift+Z", false, canRedo)) Refazer();
            ImGui::Separator();
            const bool canCopy = PossuiModoAtivo() && !mSelectedElementId.empty();
            if (!canCopy) ImGui::BeginDisabled();
            if (ImGui::MenuItem("Copiar", "Ctrl+C")) CopiarElementosSelecionados();
            if (!canCopy) ImGui::EndDisabled();
            const bool canPaste = PossuiModoAtivo() && !mElementClipboard.empty();
            if (!canPaste) ImGui::BeginDisabled();
            if (ImGui::MenuItem("Colar", "Ctrl+V")) ColarElementosCopiados();
            if (!canPaste) ImGui::EndDisabled();
            const bool hasSel = !mSelectedElementId.empty() ||
                                !mSelectedElementIds.empty();
            if (!PossuiModoAtivo() || !hasSel) ImGui::BeginDisabled();
            if (ImGui::MenuItem("Duplicar", "Ctrl+D")) DuplicarSelecao();
            if (!PossuiModoAtivo() || !hasSel) ImGui::EndDisabled();
            MenuItemSoon("Apagar", "M05");
            ImGui::Separator();
            if (ImGui::MenuItem("Copiar anotações para a IA", "Ctrl+Shift+C"))
            {
                ImGui::SetClipboardText(AnnotationsToText(mAnnot, mGlobalDirectives).c_str());
            }
            if (ImGui::MenuItem("Exportar diretrizes (.txt + print)"))
                ExportDirectives();
            if (ImGui::MenuItem("Limpar anotações"))
            {
                mAnnot.items.clear();
                mAnnot.selected = -1;
                mAnnot.editedIndex = -1;
                mAnnot.creating = false;
                mAnnot.nextId = 1;
            }
            ImGui::Separator();
            MenuItemSoon("Preferências", "M14");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Exibir"))
        {
            if (ImGui::MenuItem(IsWindowMaximized() ? "Restaurar janela" : "Maximizar janela", "F11"))
            {
                if (IsWindowMaximized()) RestoreWindow();
                else MaximizeWindow();
            }
            ImGui::Separator();
            MenuItemSoon("Zoom 100%", "M05");
            if (ImGui::MenuItem("Réguas", nullptr, mRulersVisible))
                mRulersVisible = !mRulersVisible;
            if (ImGui::MenuItem("Bloquear réguas", nullptr, mRulersLocked))
                mRulersLocked = !mRulersLocked;
            if (ImGui::MenuItem("Grade", nullptr, mGridVisible))
                mGridVisible = !mGridVisible;
            if (ImGui::MenuItem("Snap de 8 unidades", nullptr, mSnapEnabled))
                mSnapEnabled = !mSnapEnabled;
            if (ImGui::BeginMenu("Força do snap"))
            {
                ImGui::TextUnformatted("Ímã de encaixe");
                float value = mSnapStrength;
                if (ImGui::SliderFloat("##snap_str_menu", &value, 0.0f, 3.0f, "%.2fx"))
                    mSnapStrength = value;
                ImGui::TextUnformatted(SnapStrengthLabel());
                if (ImGui::MenuItem("Restaurar padrão (1x)"))
                    mSnapStrength = 1.0f;
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Zoom no cursor", nullptr, mZoomToMouse))
                mZoomToMouse = !mZoomToMouse;
            ImGui::Separator();
            if (ImGui::MenuItem("Seleção exige cobertura total", nullptr,
                                mMarqueeContainOnly))
                mMarqueeContainOnly = !mMarqueeContainOnly;
            MenuItemSoon("Grade", "M05");
            ImGui::Separator();
            MenuItemSoon("Workspace", "M10");
            MenuItemSoon("Tema claro/escuro", "M14");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Inserir"))
        {
            const bool canInsert = PossuiModoAtivo();
            if (!canInsert) ImGui::BeginDisabled();
            if (ImGui::MenuItem("Painel")) AdicionarComponente("painel", "Painel");
            if (!canInsert) ImGui::EndDisabled();
            ImGui::BeginDisabled();
            ImGui::MenuItem("Janela");
            ImGui::MenuItem("Texto");
            ImGui::MenuItem("Botao");
            ImGui::MenuItem("Campo numerico");
            ImGui::MenuItem("Slider");
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Objeto"))
        {
            if (ImGui::BeginMenu("Alinhar"))
            {
                if (ImGui::MenuItem("Esquerda")) AlinharElementosSelecionados(0);
                if (ImGui::MenuItem("Centro horizontal")) AlinharElementosSelecionados(1);
                if (ImGui::MenuItem("Direita")) AlinharElementosSelecionados(2);
                ImGui::Separator();
                if (ImGui::MenuItem("Topo")) AlinharElementosSelecionados(3);
                if (ImGui::MenuItem("Centro vertical")) AlinharElementosSelecionados(4);
                if (ImGui::MenuItem("Base")) AlinharElementosSelecionados(5);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Distribuir"))
            {
                if (ImGui::MenuItem("Espacamento horizontal"))
                    DistribuirElementosSelecionados(true);
                if (ImGui::MenuItem("Espacamento vertical"))
                    DistribuirElementosSelecionados(false);
                ImGui::EndMenu();
            }
            const bool canGroup = mSelectedElementIds.size() >= 2;
            if (ImGui::MenuItem("Agrupar", "Ctrl+G", false, canGroup))
                AgruparElementosSelecionados();
            bool canUngroup = false;
            if (PossuiModoAtivo() && mSelectedElementIds.size() == 1)
            {
                if (Element* group = Project::ResolverId(
                        mProject.telas[mTelaAtiva].modos[mModoAtivo],
                        mSelectedElementIds.front()))
                    canUngroup = group->tipo == "grupo";
            }
            if (ImGui::MenuItem("Desagrupar", "Ctrl+Shift+G", false, canUngroup))
                DesagruparElementosSelecionados();
            ImGui::Separator();
            const bool canMirror = PossuiModoAtivo() &&
                                   (!mSelectedElementId.empty() ||
                                    !mSelectedElementIds.empty());
            if (ImGui::BeginMenu("Espelhar", canMirror))
            {
                if (ImGui::MenuItem("Horizontalmente"))
                    EspelharSelecao(true);
                if (ImGui::MenuItem("Verticalmente"))
                    EspelharSelecao(false);
                ImGui::EndMenu();
            }
            ImGui::Separator();
            const bool canResetPivot = !mSelectedElementId.empty() &&
                                       PossuiModoAtivo();
            if (ImGui::MenuItem("Redefinir ponto de origem", nullptr, false,
                                canResetPivot))
            {
                if (Element* el = Project::ResolverId(
                        mProject.telas[mTelaAtiva].modos[mModoAtivo],
                        mSelectedElementId))
                {
                    el->transformacao.erase("centro_rotacao");
                    mProjectDirty = true;
                    mStatusMsg = "Ponto de origem voltou ao centro";
                    mStatusMsgUntil = GetTime() + 4.0;
                }
            }
            ImGui::Separator();
            const bool canLayer = !mSelectedElementId.empty();
            if (ImGui::BeginMenu("Camada (CorelDRAW)", canLayer))
            {
                if (ImGui::MenuItem("Trazer para frente", "Ctrl+Up"))
                    MoverCamadaSelecionada(+1);
                if (ImGui::MenuItem("Enviar para trás", "Ctrl+Down"))
                    MoverCamadaSelecionada(-1);
                ImGui::Separator();
                if (ImGui::MenuItem("Trazer ao topo"))
                    MoverCamadaSelecionada(+1000);
                if (ImGui::MenuItem("Enviar ao fundo"))
                    MoverCamadaSelecionada(-1000);
                ImGui::EndMenu();
            }
            MenuItemSoon("Bloquear", "M04");
            MenuItemSoon("Ocultar", "M04");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Modelos"))
        {
            MenuItemSoon("Galeria de modelos", "M09");
            MenuItemSoon("Salvar como modelo", "M09");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Ajuda"))
        {
            if (ImGui::MenuItem("Manual", "F1")) mShowManual = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Sobre o SeedUI")) mShowAbout = true;
            ImGui::EndMenu();
        }

        // Bloco de DEBUG no TOPO (menu bar): as funções de revisão/diretrizes
        // (modo Anotar) ficam aqui, junto aos menus — nada sufoca o rodapé.
        // Bloco de DEBUG no TOPO (menu bar): as funções de revisão/diretrizes
        // (modo Anotar) ficam aqui, junto aos menus — nada sufoca o rodapé.
        // Posicionado à direita, ANTES do seletor de tela/modo, sem sobrepor.
        if (mCurrentTool == Tool::Annotate && mHasProject)
        {
            const float debugRight = ImGui::GetWindowContentRegionMax().x - 340.0f;
            ImGui::SameLine(debugRight - 250.0f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(Theme::AccentOrange,
                               "● Debug · %d", (int)mAnnot.items.size());
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Anotações de revisão ativas");
            ImGui::SameLine(0, 10);
            if (ImGui::Button("Exportar##menu_export"))
            {
                mAnnot.selected = -1;
                mAnnot.editedIndex = -1;
                ExportDirectives();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Gera o arquivo de diretrizes (.txt) + print para a IA");
        }

        // Seletor de tela/modo (estilo Blender) — funcional a partir do M03
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 310.0f);

        if (mHasProject && !mProject.telas.empty())
        {
            if (mTelaAtiva >= (int)mProject.telas.size()) mTelaAtiva = 0;
            Tela& tela = mProject.telas[mTelaAtiva];

            // Combo de telas
            auto telasPreview = [&]() {
                return std::string("Tela: ") + tela.nome;
            };
            ImGui::SetNextItemWidth(150.0f);
            if (ImGui::BeginCombo("##tela", telasPreview().c_str()))
            {
                for (int i = 0; i < (int)mProject.telas.size(); ++i)
                {
                    const bool sel = (i == mTelaAtiva);
                    if (ImGui::Selectable(mProject.telas[i].nome.c_str(), sel))
                    {
                        mTelaAtiva = i;
                        mModoAtivo = 0;
                    }
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();

            // Combo de modos da tela ativa
            if (mModoAtivo >= (int)tela.modos.size()) mModoAtivo = 0;
            std::string modoNome = tela.modos.empty()
                                       ? "Modo: —"
                                       : "Modo: " + tela.modos[mModoAtivo].nome;
            ImGui::SetNextItemWidth(150.0f);
            if (ImGui::BeginCombo("##modo", modoNome.c_str()))
            {
                for (int i = 0; i < (int)tela.modos.size(); ++i)
                {
                    const bool sel = (i == mModoAtivo);
                    if (ImGui::Selectable(tela.modos[i].nome.c_str(), sel))
                        mModoAtivo = i;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            ImGui::SetNextItemWidth(150.0f);
            ImGui::BeginDisabled();
            ImGui::Combo("##tela", &mTelaAtiva, "Tela: —\0\0");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150.0f);
            ImGui::Combo("##modo", &mModoAtivo, "Modo: —\0\0");
            ImGui::EndDisabled();
        }

        ImGui::EndMenuBar();
    }

    void App::DrawActionBar()
    {
        constexpr float button = 32.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 0));
        // Centraliza cada item na barra (42px) pelo eixo CENTRAL — itens com
        // alturas diferentes não ficam mais "colados" no topo da barra.
        auto centerRow = [&](float itemHeight)
        {
            ImGui::SetCursorPosY((kActionBarHeight - itemHeight) * 0.5f);
        };
        centerRow(button);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::BorderLight);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::Hex(0x4f8cff, 0.28f));

        auto separator = [&]()
        {
            ImGui::SameLine(0, 10);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + 5.0f),
                                                ImVec2(p.x, p.y + button - 5.0f),
                                                ImGui::ColorConvertFloat4ToU32(Theme::BorderLight));
            ImGui::Dummy(ImVec2(1.0f, button));
            ImGui::SameLine(0, 10);
        };

        // Ponto de sangria alinhado com a régua da coluna de ferramentas
        // (o primeiro ícone da action bar começa no mesmo eixo X dos botões
        // da toolbar vertical: (50-32)/2 = 9px).
        ImGui::SetCursorPosX(9.0f);
        if (IconButton(IconId::New, "Arquivo · Novo projeto", button)) NovoProjeto();
        ImGui::SameLine();
        if (IconButton(IconId::Open, "Arquivo · Abrir projeto (Ctrl+O)", button)) AbrirProjeto();
        ImGui::SameLine();
        if (!mHasProject) ImGui::BeginDisabled();
        if (IconButton(IconId::Save, "Arquivo · Salvar (Ctrl+S)", button)) SalvarProjeto(false);
        if (!mHasProject) ImGui::EndDisabled();

        separator();
        if (mHistoryIndex <= 0) ImGui::BeginDisabled();
        if (IconButton(IconId::Undo, "Histórico · Desfazer (Ctrl+Z)", button)) Desfazer();
        if (mHistoryIndex <= 0) ImGui::EndDisabled();
        ImGui::SameLine();
        const bool canRedo = mHistoryIndex >= 0 && mHistoryIndex + 1 < (int)mHistory.size();
        if (!canRedo) ImGui::BeginDisabled();
        if (IconButton(IconId::Redo, "Histórico · Refazer (Ctrl+Shift+Z)", button)) Refazer();
        if (!canRedo) ImGui::EndDisabled();

        separator();
        const bool canCopy = PossuiModoAtivo() && !mSelectedElementId.empty();
        if (!canCopy) ImGui::BeginDisabled();
        if (IconButton(IconId::Copy, "Edição · Copiar seleção (Ctrl+C)", button))
            CopiarElementosSelecionados();
        if (!canCopy) ImGui::EndDisabled();
        ImGui::SameLine();
        const bool canPaste = PossuiModoAtivo() && !mElementClipboard.empty();
        if (!canPaste) ImGui::BeginDisabled();
        if (IconButton(IconId::Paste, "Edição · Colar (Ctrl+V)", button))
            ColarElementosCopiados();
        if (!canPaste) ImGui::EndDisabled();
        ImGui::SameLine();
        const bool canDup = PossuiModoAtivo() &&
                            (!mSelectedElementId.empty() || !mSelectedElementIds.empty());
        if (!canDup) ImGui::BeginDisabled();
        if (IconButton(IconId::Duplicate, "Edição · Duplicar (Ctrl+D)", button))
            DuplicarSelecao();
        if (!canDup) ImGui::EndDisabled();
        ImGui::SameLine();
        const bool canDelete = !mSelectedElementId.empty() || !mSelectedElementIds.empty();
        if (!canDelete) ImGui::BeginDisabled();
        if (IconButton(IconId::Trash, "Edição · Apagar (X)", button))
            ApagarElementosSelecionados();
        if (!canDelete) ImGui::EndDisabled();

        separator();
        bool isolatedGroup = false;
        if (PossuiModoAtivo() && mSelectedElementIds.size() == 1)
            if (Element* element = Project::ResolverId(
                    mProject.telas[mTelaAtiva].modos[mModoAtivo], mSelectedElementIds.front()))
                isolatedGroup = element->tipo == "grupo";
        const bool canAlign = isolatedGroup || (mAlignTarget == 2
            ? !mSelectedElementId.empty()
            : mAlignTarget == 3 ? !mSelectedElementId.empty()
                                : mSelectedElementIds.size() >= 2);
        if (!canAlign) ImGui::BeginDisabled();
        const char* alignTips[] = {
            "Alinhar à esquerda (Q)", "Centralizar horizontalmente (W)", "Alinhar à direita (E)",
            "Alinhar ao topo (2)", "Centralizar verticalmente (D)", "Alinhar à base (S)"
        };
        for (int operation = 0; operation < 6; ++operation)
        {
            if (operation > 0) ImGui::SameLine();
            if (AlignmentIconButton(operation, alignTips[operation]))
                AlinharElementosSelecionados(operation);
        }
        if (!canAlign) ImGui::EndDisabled();

        separator();
        const bool canMirror = canDelete;
        if (!canMirror) ImGui::BeginDisabled();
        if (IconButton(IconId::FlipH, "Objeto · Espelhar horizontalmente", button))
            EspelharSelecao(true);
        ImGui::SameLine();
        if (IconButton(IconId::FlipV, "Objeto · Espelhar verticalmente", button))
            EspelharSelecao(false);
        if (!canMirror) ImGui::EndDisabled();
        ImGui::SameLine(0.0f, 8.0f);
        centerRow(ImGui::GetFrameHeight());
        ImGui::SetNextItemWidth(54.0f);
        ImGui::InputFloat("##space_h", &mHorizontalSpacing, 0.0f, 0.0f, "H %.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Espaçamento horizontal");
        ImGui::SameLine();
        centerRow(button);
        if (ImGui::Button("↔##distribute_h", ImVec2(28.0f, button)))
            DistribuirElementosSelecionados(true);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Aplicar espaçamento horizontal");
        ImGui::SameLine();
        centerRow(ImGui::GetFrameHeight());
        ImGui::SetNextItemWidth(54.0f);
        ImGui::InputFloat("##space_v", &mVerticalSpacing, 0.0f, 0.0f, "V %.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Espaçamento vertical");
        ImGui::SameLine();
        centerRow(button);
        if (ImGui::Button("↕##distribute_v", ImVec2(28.0f, button)))
            DistribuirElementosSelecionados(false);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Aplicar espaçamento vertical");

        separator();
        const bool canGroup = mSelectedElementIds.size() >= 2;
        if (!canGroup) ImGui::BeginDisabled();
        if (IconButton(IconId::Hierarchy, "Agrupar seleção (Ctrl+G)", button))
            AgruparElementosSelecionados();
        if (!canGroup) ImGui::EndDisabled();
        ImGui::SameLine();
        bool canUngroup = false;
        if (PossuiModoAtivo() && mSelectedElementIds.size() == 1)
        {
            if (Element* group = Project::ResolverId(
                    mProject.telas[mTelaAtiva].modos[mModoAtivo],
                    mSelectedElementIds.front()))
                canUngroup = group->tipo == "grupo";
        }
        if (!canUngroup) ImGui::BeginDisabled();
        if (IconButton(IconId::CornersOut, "Desagrupar seleção (Ctrl+Shift+G)", button))
            DesagruparElementosSelecionados();
        if (!canUngroup) ImGui::EndDisabled();
        ImGui::SameLine();
        // Snap: o clique alterna o estado — capturamos o estado ANTES do botão
        // para que o Push/Pop fiquem simétricos (senão o clique vaza 1 estilo
        // e o assert Missing PopStyleColor trava o build Debug no End()).
        const bool snapWasOn = mSnapEnabled;
        if (snapWasOn) ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x4f8cff, 0.30f));
        if (IconButton(IconId::Grid, mSnapEnabled ? "Snap ativo · clique para desligar"
                                                  : "Snap inativo · clique para ligar", button))
            mSnapEnabled = !mSnapEnabled;
        if (snapWasOn) ImGui::PopStyleColor();
        ImGui::SameLine();
        // Força do snap (ímã): abre o popup de controle.
        const std::string snapTip = mSnapStrength <= 0.01f
            ? std::string("Snap: DESLIGADO · clique para ajustar")
            : std::string("Força do snap: ") + SnapStrengthLabel() +
              " · clique para ajustar";
        if (IconButton(IconId::Magnet, snapTip.c_str(), button))
            ImGui::OpenPopup("##snap_strength");
        DrawSnapStrengthPopup();
        ImGui::SameLine();
        const bool rulersWereOn = mRulersVisible;
        if (rulersWereOn) ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x4f8cff, 0.30f));
        if (IconButton(IconId::List, mRulersVisible ? "Réguas visíveis · clique para ocultar"
                                                    : "Réguas ocultas · clique para mostrar", button))
            mRulersVisible = !mRulersVisible;
        if (rulersWereOn) ImGui::PopStyleColor();
        ImGui::SameLine();
        const bool rulersWereLocked = mRulersLocked;
        if (rulersWereLocked) ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0xffb347, 0.35f));
        if (IconButton(IconId::Lock, mRulersLocked
                                         ? "Réguas bloqueadas · clique para desbloquear"
                                         : "Réguas desbloqueadas · clique para bloquear",
                       button))
            mRulersLocked = !mRulersLocked;
        if (rulersWereLocked) ImGui::PopStyleColor();
        ImGui::SameLine();
        // Grade visível/oculta: ocultar NÃO desliga o snap (continua
        // encaixando nos pontos exatos da grade, mesmo sem vê-la). Usa o
        // ícone de OLHO — o ID do IconButton é o do ícone (PushID(ícone)),
        // então um segundo botão com IconId::Grid colidiria com o do snap.
        const bool gridWasOn = mGridVisible;
        if (gridWasOn) ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x4f8cff, 0.30f));
        if (IconButton(IconId::Eye, mGridVisible
                                        ? "Grade visível · clique para ocultar (snap continua ativo)"
                                        : "Grade oculta · clique para mostrar",
                       button))
            mGridVisible = !mGridVisible;
        if (gridWasOn) ImGui::PopStyleColor();

        separator();
        IconButton(IconId::Model, "Projeto · Galeria de modelos (M09)", button);
        ImGui::SameLine();
        IconButton(IconId::Download, "Projeto · Exportar pacote (M13)", button);

        const float rightX = ImGui::GetWindowContentRegionMax().x - button * 2.0f - 5.0f;
        if (ImGui::GetCursorPosX() + 12.0f < rightX)
            ImGui::SameLine(rightX);
        else
            ImGui::SameLine();
        if (IconButton(IconId::Question, "Ajuda · Manual (F1)", button)) mShowManual = true;
        ImGui::SameLine();
        IconButton(IconId::Gear, "Sistema · Preferências (M14)", button);
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }

    float App::UnitToPixels() const
    {
        switch (mUnit)
        {
            case 1: return 3.779528f;  // milímetros (96 dpi)
            case 2: return 37.79528f;  // centímetros
            case 3: return 96.0f;      // polegadas
            case 4: return 1.333333f;  // pontos
            default: return 1.0f;      // pixels
        }
    }

    float App::PixelsToUnit(float px) const
    {
        const float f = UnitToPixels();
        return (f > 0.0f) ? px / f : px;
    }

    void App::DrawPropertyBar()
    {
        // Barra de propriedades contextual (estilo CorelDRAW): apresenta os
        // valores do elemento selecionado com entrada numérica direta.
        // Seleção única = edita o elemento; multi-seleção = caixa conjunta
        // (somente leitura). Unidade + precisão afetam os campos.
        constexpr float fieldW = 74.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 0));
        ImGui::AlignTextToFramePadding();

        // Preset/perfil (indicador visual; galeria de modelos é M09).
        ImGui::TextDisabled("Personalizado");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Predefinições de ferramenta (M09)");

        // Referência: elemento primário ou caixa conjunta da seleção.
        Element* primary = nullptr;
        Modo* mode = nullptr;
        bool single = false;
        float bx = 0.0f, by = 0.0f, bw = 0.0f, bh = 0.0f, rot = 0.0f;
        if (PossuiModoAtivo() && !mSelectedElementIds.empty())
        {
            mode = &mProject.telas[mTelaAtiva].modos[mModoAtivo];
            if (mSelectedElementIds.size() == 1)
            {
                primary = Project::ResolverId(*mode, mSelectedElementIds.front());
                if (primary)
                {
                    single = true;
                    bx = primary->transformacao.value("x", 0.0f);
                    by = primary->transformacao.value("y", 0.0f);
                    bw = primary->transformacao.value("largura", 160.0f);
                    bh = primary->transformacao.value("altura", 32.0f);
                    rot = Geo::ElementRotation(*primary);
                }
            }
            else
            {
                float minX = FLT_MAX, minY = FLT_MAX;
                float maxX = -FLT_MAX, maxY = -FLT_MAX;
                for (const std::string& id : mSelectedElementIds)
                {
                    if (Element* el = Project::ResolverId(*mode, id))
                    {
                        const float ex = el->transformacao.value("x", 0.0f);
                        const float ey = el->transformacao.value("y", 0.0f);
                        const float ew = el->transformacao.value("largura", 160.0f);
                        const float eh = el->transformacao.value("altura", 32.0f);
                        minX = std::min(minX, ex);
                        minY = std::min(minY, ey);
                        maxX = std::max(maxX, ex + ew);
                        maxY = std::max(maxY, ey + eh);
                    }
                }
                if (minX <= maxX && minY <= maxY)
                {
                    bx = minX; by = minY;
                    bw = maxX - minX; bh = maxY - minY;
                }
            }
        }
        const bool hasSel = (primary != nullptr) || (mode && mSelectedElementIds.size() > 1);

        auto field = [&](const char* label, float* value, bool enabled)
        {
            if (!enabled) ImGui::BeginDisabled();
            ImGui::SetNextItemWidth(fieldW);
            const bool edited = ImGui::InputFloat(label, value, 0.0f, 0.0f, "%.2f");
            if (!enabled) ImGui::EndDisabled();
            return edited;
        };

        bool propertyEdited = false;
        ImGui::SameLine(0, 14);
        ImGui::TextDisabled("X:");
        ImGui::SameLine();
        float v = hasSel ? PixelsToUnit(bx) : 0.0f;
        if (field("##px", &v, single) && primary)
        {
            primary->transformacao["x"] = v * UnitToPixels();
            propertyEdited = true;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Y:");
        ImGui::SameLine();
        v = hasSel ? PixelsToUnit(by) : 0.0f;
        if (field("##py", &v, single) && primary)
        {
            primary->transformacao["y"] = v * UnitToPixels();
            propertyEdited = true;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("L:");
        ImGui::SameLine();
        v = hasSel ? PixelsToUnit(bw) : 0.0f;
        if (field("##pw", &v, single) && primary)
        {
            primary->transformacao["largura"] = std::max(1.0f, v * UnitToPixels());
            propertyEdited = true;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("A:");
        ImGui::SameLine();
        v = hasSel ? PixelsToUnit(bh) : 0.0f;
        if (field("##ph", &v, single) && primary)
        {
            primary->transformacao["altura"] = std::max(1.0f, v * UnitToPixels());
            propertyEdited = true;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Rot:");
        ImGui::SameLine();
        v = hasSel ? rot : 0.0f;
        if (field("##prot", &v, single) && primary)
        {
            primary->transformacao["rotacao"] = fmodf(v, 360.0f);
            propertyEdited = true;
        }

        if (propertyEdited) mProjectDirty = true;

        // Separador
        ImGui::SameLine(0, 12);
        const ImVec2 sep = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(sep.x, sep.y + 2.0f), ImVec2(sep.x, sep.y + kPropertyBarHeight - 8.0f),
            ImGui::ColorConvertFloat4ToU32(Theme::BorderLight));
        ImGui::Dummy(ImVec2(1.0f, 1.0f));

        // Unidade
        ImGui::SameLine(0, 10);
        ImGui::TextDisabled("Unid:");
        ImGui::SameLine();
        static const char* kUnits[] = { "px", "mm", "cm", "in", "pt" };
        ImGui::SetNextItemWidth(56.0f);
        if (ImGui::BeginCombo("##unit", kUnits[mUnit]))
        {
            for (int i = 0; i < 5; ++i)
            {
                if (ImGui::Selectable(kUnits[i], mUnit == i))
                {
                    mUnit = i;
                    // Precisão padrão por unidade (estilo CorelDRAW).
                    const float kPreset[] = { 1.0f, 0.1f, 0.01f, 0.01f, 0.1f };
                    mPrecision = kPreset[i];
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Unidade dos campos numéricos e réguas");

        // Precisão (incremento)
        ImGui::SameLine();
        ImGui::TextDisabled("Prec:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(52.0f);
        ImGui::InputFloat("##prec", &mPrecision, 0.0f, 0.0f, "%.2f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Incremento/precisão dos ajustes");

        // Zoom (à direita): campo editável + ajustar página.
        const float zoomRight = ImGui::GetWindowContentRegionMax().x - 132.0f;
        if (ImGui::GetCursorPosX() + 40.0f < zoomRight)
            ImGui::SameLine(zoomRight);
        else
            ImGui::SameLine(0, 14);
        ImGui::TextDisabled("Zoom:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(58.0f);
        float zoomPct = mCanvasZoom * 100.0f;
        if (ImGui::InputFloat("##zoom_pct", &zoomPct, 0.0f, 0.0f, "%.0f%%"))
            mCanvasZoom = std::max(0.1f, std::min(32.0f, zoomPct / 100.0f));
        ImGui::SameLine();
        if (ImGui::SmallButton("Ajustar##fit"))
        {
            mCanvasZoom = 1.0f;
            mCanvasPanX = 0.0f;
            mCanvasPanY = 0.0f;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Ajustar página ao canvas (zoom 100%)");

        ImGui::PopStyleVar();
    }

    void App::DesenharGuias()
    {
        // Guias fixas (arrastadas das réguas): linha azul fina sobre o canvas.
        if (!mHasProject || (mGuidesH.empty() && mGuidesV.empty())) return;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cmin = ImGui::GetWindowPos();
        const ImVec2 cmax(cmin.x + ImGui::GetWindowWidth(),
                          cmin.y + ImGui::GetWindowHeight());
        const ImU32 color = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0x4f8cff, 0.85f));
        const ImU32 engaged = ImGui::ColorConvertFloat4ToU32(Theme::Hex(0xffb347, 1.0f));
        float sx = 0.0f, sy = 0.0f, scale = 1.0f;
        for (int i = 0; i < (int)mGuidesH.size(); ++i)
        {
            const bool isEngaged = mGuideSnapEngaged && mGuideDragKind == 3 &&
                                   i == mGuideDragIndex;
            CanvasProjectToScreen(&mProject, 0.0f, mGuidesH[i], sx, sy, scale,
                                  mCanvasZoom, mCanvasPanX, mCanvasPanY);
            dl->AddLine(ImVec2(cmin.x, sy), ImVec2(cmax.x, sy),
                        isEngaged ? engaged : color, isEngaged ? 2.0f : 1.0f);
        }
        for (int i = 0; i < (int)mGuidesV.size(); ++i)
        {
            const bool isEngaged = mGuideSnapEngaged && mGuideDragKind == 4 &&
                                   i == mGuideDragIndex;
            CanvasProjectToScreen(&mProject, mGuidesV[i], 0.0f, sx, sy, scale,
                                  mCanvasZoom, mCanvasPanX, mCanvasPanY);
            dl->AddLine(ImVec2(sx, cmin.y), ImVec2(sx, cmax.y),
                        isEngaged ? engaged : color, isEngaged ? 2.0f : 1.0f);
        }
    }

    void App::HandleGuidesInteraction()
    {
        // Guias arrastáveis das réguas (estilo CorelDRAW): clique na régua
        // cria; clique na linha arrasta; soltar na régua/fora remove.
        // Régua BLOQUEADA: sem interação — as guias existentes continuam
        // funcionando como referência de snap (bloquear ≠ desativar).
        if (!mHasProject || !mRulersVisible || mRulersLocked) return;
        if (mCurrentTool != Tool::Select && mCurrentTool != Tool::Move) return;
        const ImVec2 wpos = ImGui::GetWindowPos();
        const ImVec2 wsize = ImGui::GetWindowSize();
        const float ruler = 24.0f;
        const ImVec2 mouse = ImGui::GetMousePos();
        const bool overTop = mouse.y >= wpos.y && mouse.y < wpos.y + ruler &&
                             mouse.x >= wpos.x + ruler && mouse.x <= wpos.x + wsize.x;
        const bool overLeft = mouse.x >= wpos.x && mouse.x < wpos.x + ruler &&
                              mouse.y >= wpos.y + ruler && mouse.y <= wpos.y + wsize.y;

        if (mGuideDragKind == 0)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                float px = 0.0f, py = 0.0f;
                CanvasScreenToProject(&mProject, mouse.x, mouse.y, px, py, false,
                                      mCanvasZoom, mCanvasPanX, mCanvasPanY);
                if (overTop)
                {
                    Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
                    bool snapped = false;
                    mGuidesH.push_back(SmartGuides::SnapGuideToShapes(
                        py, true, mode, (float)mProject.telaBaseLargura,
                        (float)mProject.telaBaseAltura,
                        SnapTol(10.0f), snapped));
                    mGuideSnapEngaged = snapped;
                    mGuideDragKind = 3;
                    mGuideDragIndex = (int)mGuidesH.size() - 1;
                }
                else if (overLeft)
                {
                    Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
                    bool snapped = false;
                    mGuidesV.push_back(SmartGuides::SnapGuideToShapes(
                        px, false, mode, (float)mProject.telaBaseLargura,
                        (float)mProject.telaBaseAltura,
                        SnapTol(10.0f), snapped));
                    mGuideSnapEngaged = snapped;
                    mGuideDragKind = 4;
                    mGuideDragIndex = (int)mGuidesV.size() - 1;
                }
                else
                {
                    // Clicar numa guia existente (dentro da área de edição).
                    for (int i = 0; i < (int)mGuidesH.size(); ++i)
                    {
                        float sx, sy, sc;
                        CanvasProjectToScreen(&mProject, 0.0f, mGuidesH[i], sx, sy, sc,
                                              mCanvasZoom, mCanvasPanX, mCanvasPanY);
                        if (fabsf(mouse.y - sy) <= 6.0f)
                        {
                            mGuideDragKind = 3;
                            mGuideDragIndex = i;
                            break;
                        }
                    }
                    if (mGuideDragKind == 0)
                    {
                        for (int i = 0; i < (int)mGuidesV.size(); ++i)
                        {
                            float sx, sy, sc;
                            CanvasProjectToScreen(&mProject, mGuidesV[i], 0.0f, sx, sy, sc,
                                                  mCanvasZoom, mCanvasPanX, mCanvasPanY);
                            if (fabsf(mouse.x - sx) <= 6.0f)
                            {
                                mGuideDragKind = 4;
                                mGuideDragIndex = i;
                                break;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                float px = 0.0f, py = 0.0f;
                CanvasScreenToProject(&mProject, mouse.x, mouse.y, px, py, false,
                                      mCanvasZoom, mCanvasPanX, mCanvasPanY);
                if (mGuideDragKind == 3 && mGuideDragIndex >= 0 &&
                    mGuideDragIndex < (int)mGuidesH.size())
                {
                    Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
                    bool snapped = false;
                    mGuidesH[mGuideDragIndex] = SmartGuides::SnapGuideToShapes(
                        py, true, mode, (float)mProject.telaBaseLargura,
                        (float)mProject.telaBaseAltura,
                        SnapTol(10.0f), snapped);
                    mGuideSnapEngaged = snapped;
                }
                else if (mGuideDragKind == 4 && mGuideDragIndex >= 0 &&
                         mGuideDragIndex < (int)mGuidesV.size())
                {
                    Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
                    bool snapped = false;
                    mGuidesV[mGuideDragIndex] = SmartGuides::SnapGuideToShapes(
                        px, false, mode, (float)mProject.telaBaseLargura,
                        (float)mProject.telaBaseAltura,
                        SnapTol(10.0f), snapped);
                    mGuideSnapEngaged = snapped;
                }
                ImGui::SetMouseCursor(mGuideDragKind == 3
                                          ? ImGuiMouseCursor_ResizeNS
                                          : ImGuiMouseCursor_ResizeEW);
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                // Soltar na régua de origem ou fora do canvas remove a guia.
                const bool onRuler = (mGuideDragKind == 3 && overTop) ||
                                     (mGuideDragKind == 4 && overLeft);
                const bool outside = mouse.x < wpos.x || mouse.x > wpos.x + wsize.x ||
                                     mouse.y < wpos.y || mouse.y > wpos.y + wsize.y;
                if (onRuler || outside)
                {
                    if (mGuideDragKind == 3 && mGuideDragIndex >= 0 &&
                        mGuideDragIndex < (int)mGuidesH.size())
                        mGuidesH.erase(mGuidesH.begin() + mGuideDragIndex);
                    else if (mGuideDragKind == 4 && mGuideDragIndex >= 0 &&
                             mGuideDragIndex < (int)mGuidesV.size())
                        mGuidesV.erase(mGuidesV.begin() + mGuideDragIndex);
                }
                mGuideDragKind = 0;
                mGuideDragIndex = -1;
                mGuideSnapEngaged = false;
            }
        }
        // Cursor de hover sobre guias existentes (sem arrasto).
        if (mGuideDragKind == 0)
        {
            for (float gy : mGuidesH)
            {
                float sx, sy, sc;
                CanvasProjectToScreen(&mProject, 0.0f, gy, sx, sy, sc,
                                      mCanvasZoom, mCanvasPanX, mCanvasPanY);
                if (fabsf(mouse.y - sy) <= 6.0f)
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                    break;
                }
            }
            for (float gx : mGuidesV)
            {
                float sx, sy, sc;
                CanvasProjectToScreen(&mProject, gx, 0.0f, sx, sy, sc,
                                      mCanvasZoom, mCanvasPanX, mCanvasPanY);
                if (fabsf(mouse.x - sx) <= 6.0f)
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                    break;
                }
            }
        }
    }

    float App::SnapTol(float basePx) const
    {
        // Tolerância de snap em unidades do projeto: `basePx` (px de tela no
        // zoom 1) × a FORÇA configurada (mSnapStrength), normalizada pelo
        // zoom — o encaixe ocupa sempre a mesma área de tela, qualquer que
        // seja o zoom. Força 1.0 = comportamento padrão; <1 = mais difícil
        // de engatar (preciso); >1 = ímã mais forte (gruda de longe);
        // 0.0 = snap completamente desligado (todas as tolerâncias zeram).
        return basePx * mSnapStrength / std::max(0.5f, mCanvasZoom);
    }

    const char* App::SnapStrengthLabel() const
    {
        if (mSnapStrength <= 0.01f)
            return "Desligado · sem encaixe";
        if (mSnapStrength < 0.75f)
            return "Fino · só gruda bem pertinho";
        if (mSnapStrength > 1.5f)
            return "Forte · gruda de longe";
        return "Médio · equilíbrio padrão";
    }

    void App::DrawSnapStrengthPopup()
    {
        // Popup de controle da FORÇA do snap: um ímã mais forte faz os
        // objetos grudarem de mais longe; mais fraco exige precisão.
        if (ImGui::BeginPopup("##snap_strength"))
        {
            ImGui::TextUnformatted("Força do snap (ímã)");
            ImGui::SetNextItemWidth(220.0f);
            float value = mSnapStrength;
            if (ImGui::SliderFloat("##snap_str", &value, 0.0f, 3.0f, "%.2fx"))
                mSnapStrength = value;
            ImGui::TextUnformatted(SnapStrengthLabel());
            ImGui::Separator();
            if (ImGui::MenuItem("Restaurar padrão (1x)"))
                mSnapStrength = 1.0f;
            ImGui::EndPopup();
        }
    }

    void App::SnapGuias(float& dx, float& dy,
                        const std::vector<SmartGuides::Rect>& starts)
    {
        // Snap às guias fixas (por último: referência intencional do usuário
        // vence os demais snaps). Tolera 12px de tela.
        if (starts.empty()) return;
        float selLeft = starts[0].x + dx, selTop = starts[0].y + dy;
        float selRight = starts[0].x + starts[0].w + dx;
        float selBottom = starts[0].y + starts[0].h + dy;
        for (const SmartGuides::Rect& s : starts)
        {
            selLeft = std::min(selLeft, s.x + dx);
            selTop = std::min(selTop, s.y + dy);
            selRight = std::max(selRight, s.x + s.w + dx);
            selBottom = std::max(selBottom, s.y + s.h + dy);
        }
        const float tol = SnapTol(12.0f);

        mGuideFixedSnapX = -1.0f;
        mGuideFixedSnapY = -1.0f;
        float bestDx = 0.0f, bestDist = tol;
        float engagedGx = -1.0f;
        for (float gx : mGuidesV)
        {
            const float dl = gx - selLeft;
            const float dc = gx - (selLeft + selRight) * 0.5f;
            const float dr = gx - selRight;
            if (fabsf(dl) < bestDist) { bestDist = fabsf(dl); bestDx = dl; engagedGx = gx; }
            if (fabsf(dc) < bestDist) { bestDist = fabsf(dc); bestDx = dc; engagedGx = gx; }
            if (fabsf(dr) < bestDist) { bestDist = fabsf(dr); bestDx = dr; engagedGx = gx; }
        }
        if (engagedGx >= 0.0f) { dx += bestDx; mGuideFixedSnapX = engagedGx; }

        float bestDy = 0.0f;
        bestDist = tol;
        float engagedGy = -1.0f;
        for (float gy : mGuidesH)
        {
            const float dt = gy - selTop;
            const float dc = gy - (selTop + selBottom) * 0.5f;
            const float db = gy - selBottom;
            if (fabsf(dt) < bestDist) { bestDist = fabsf(dt); bestDy = dt; engagedGy = gy; }
            if (fabsf(dc) < bestDist) { bestDist = fabsf(dc); bestDy = dc; engagedGy = gy; }
            if (fabsf(db) < bestDist) { bestDist = fabsf(db); bestDy = db; engagedGy = gy; }
        }
        if (engagedGy >= 0.0f) { dy += bestDy; mGuideFixedSnapY = engagedGy; }
    }

    void App::DrawToolbar()
    {
        // Somente ferramentas que atuam diretamente no canvas.
        // Agrupadas por família (identidade premium): cada grupo tem um
        // separador fino na cor da família; o ícone ativo usa essa cor.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
        ImGui::Spacing();

        ToolButton(IconId::Select, "Seleção · Selecionar (V)", kFamilySelect);
        ToolButton(IconId::Move, "Transformação · Mover (M)", kFamilySelect);
        DrawToolFamilySeparator(kFamilySelect);

        ToolButton(IconId::Text, "Criação · Texto (T)", kFamilyCreate);
        ToolButton(IconId::Rectangle,
                   "Criar retângulo · arraste no canvas · clique duplo = retângulo do tamanho da tela",
                   kFamilyCreate);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            CriarRetanguloTelaBase();
        ToolButton(IconId::Ellipse, "Criar elipse · arraste no canvas", kFamilyCreate);
        ToolButton(IconId::Polygon, "Criar polígono · arraste no canvas", kFamilyCreate);
        ToolButton(IconId::Slash, "Criar linha · arraste para definir o traço", kFamilyCreate);
        ToolButton(IconId::PenTool,
                   "Caneta · clique adiciona ponto · arraste cria curva · duplo clique/Enter fecha",
                   kFamilyCreate);
        DrawToolFamilySeparator(kFamilyCreate);

        ToolButton(IconId::Color, "Aparência · Conta-gotas (I)", kFamilyAppearance);
        ImGui::SetCursorPosX((kToolbarWidth - kToolButtonSize) * 0.5f);
        if (IconButton(IconId::Contour, "Espessura do contorno selecionado",
                       kToolButtonSize, kFamilyAppearance))
            ImGui::OpenPopup("##contour_tool");
        if (ImGui::BeginPopup("##contour_tool"))
        {
            Element* selected = nullptr;
            if (PossuiModoAtivo() && !mSelectedElementId.empty())
                selected = Project::ResolverId(
                    mProject.telas[mTelaAtiva].modos[mModoAtivo], mSelectedElementId);
            if (!selected)
                ImGui::TextDisabled("Selecione um elemento");
            else
            {
                float width = selected->estilos.value("espessura_borda", 1.0f);
                ImGui::SetNextItemWidth(180.0f);
                if (ImGui::SliderFloat("Contorno", &width, 0.0f, 16.0f, "%.1f px"))
                {
                    selected->estilos["espessura_borda"] = width;
                    mProjectDirty = true;
                }
            }
            ImGui::EndPopup();
        }
        ImGui::SetCursorPosX((kToolbarWidth - kToolButtonSize) * 0.5f);
        if (IconButton(IconId::Transparency, "Transparência do elemento selecionado",
                       kToolButtonSize, kFamilyAppearance))
            ImGui::OpenPopup("##transparency_tool");
        if (ImGui::BeginPopup("##transparency_tool"))
        {
            Element* selected = nullptr;
            if (PossuiModoAtivo() && !mSelectedElementId.empty())
                selected = Project::ResolverId(
                    mProject.telas[mTelaAtiva].modos[mModoAtivo], mSelectedElementId);
            if (!selected)
                ImGui::TextDisabled("Selecione um elemento");
            else
            {
                float transparency = 100.0f * (1.0f -
                    selected->estilos.value("opacidade", 1.0f));
                ImGui::SetNextItemWidth(180.0f);
                if (ImGui::SliderFloat("Transparência", &transparency,
                                       0.0f, 100.0f, "%.0f%%"))
                {
                    selected->estilos["opacidade"] = 1.0f - transparency / 100.0f;
                    mProjectDirty = true;
                }
            }
            ImGui::EndPopup();
        }
        DrawToolFamilySeparator(kFamilyAppearance);

        ImGui::SetCursorPosX((kToolbarWidth - kToolButtonSize) * 0.5f);
        // Zoom: o clique também pode LIGAR a ferramenta — o push/pop usa o
        // estado anterior ao clique para ficar simétrico (mesma regra do snap).
        const bool wasZoom = mCurrentTool == Tool::Zoom;
        if (wasZoom)
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x4f8cff, 0.30f));
        if (IconButton(IconId::ZoomIn, "Zoom In - clique ou use Z no canvas",
                       kToolButtonSize, kFamilyNav))
        {
            mCurrentTool = Tool::Zoom;
            ZoomCanvas(1.25f);
        }
        if (wasZoom) ImGui::PopStyleColor();
        ImGui::SetCursorPosX((kToolbarWidth - kToolButtonSize) * 0.5f);
        if (IconButton(IconId::ZoomOut, "Zoom Out - botao direito com Z",
                       kToolButtonSize, kFamilyNav))
        {
            mCurrentTool = Tool::Zoom;
            ZoomCanvas(1.0f / 1.25f);
        }
        ToolButton(IconId::Pan, "Navegação · Mão (H)", kFamilyNav);
        ToolButton(IconId::Grid, "Visualização · Grade (G)", kFamilyNav);
        ToolButton(IconId::Ruler, "Navegação · Medir distância e ângulo", kFamilyNav);
        DrawToolFamilySeparator(kFamilyNav);

        ToolButton(IconId::Annotate,
                   "Revisão · Anotar para a IA (A)", kFamilyReview);

        ImGui::PopStyleVar();
    }
    void App::DrawToolFamilySeparator(ImU32 familyColor)
    {
        ImGui::Separator();
        ImGui::Spacing();
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float lineW = 22.0f;
        dl->AddRectFilled(ImVec2(p.x + (kToolbarWidth - lineW) * 0.5f, p.y + 2.0f),
                          ImVec2(p.x + (kToolbarWidth - lineW) * 0.5f + lineW, p.y + 3.0f),
                          familyColor);
        ImGui::Dummy(ImVec2(1, 3));
    }
    void App::ToolButton(IconId id, const char* tip, ImU32 familyColor)
    {
        // Centro geométrico da coluna: (50 - 32) / 2 = 9 px.
        ImGui::SetCursorPosX((kToolbarWidth - kToolButtonSize) * 0.5f);
        const bool active = (mCurrentTool == ToolFromIcon(id));
        const ImU32 tint = active ? familyColor : kIconTintDefault;
        if (active)
        {
            const ImVec4 fc = ImGui::ColorConvertU32ToFloat4(familyColor);
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(fc.x, fc.y, fc.z, 0.28f));
        }
        if (IconButton(id, tip, kToolButtonSize, tint))
        {
            const Tool requested = ToolFromIcon(id);
            if (requested == Tool::Annotate)
            {
                const bool wasActive = mCurrentTool == Tool::Annotate;
                AlternarModoAnotacao();
                if (!wasActive) mSkipAnnotationInputThisFrame = true;
            }
            else
            {
                mCurrentTool = requested;
            }
        }
        if (active) ImGui::PopStyleColor();
    }

    void App::DrawRightRail()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));
        if (IconButton(IconId::CaretRight, "Expandir painel direito", 32.0f))
            mRightPanelCollapsed = false;
        ImGui::Separator();
        IconButton(IconId::List, "Hierarquia", 32.0f);
        IconButton(IconId::Inspector, "Inspetor", 32.0f);
        IconButton(IconId::Plus, "Biblioteca", 32.0f);
        IconButton(IconId::Directives, "Diretrizes", 32.0f);
        IconButton(IconId::Assets, "Recursos", 32.0f);
        IconButton(IconId::History, "Historico", 32.0f);
        ImGui::PopStyleVar();
    }

    void App::DrawRightPanels()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 5));
        if (ImGui::Button("<", ImVec2(28.0f, 24.0f)))
            mRightPanelCollapsed = true;
        ImGui::SameLine();
        ImGui::TextColored(Theme::TextSecondary, "Painel direito");
        ImGui::Separator();

        // Abas organizadas (estilo Blender/Photoshop): uma seção por vez —
        // sem pilhas confusas de cabeçalhos abertos ao mesmo tempo.
        if (!ImGui::BeginTabBar("##right_tabs"))
        {
            ImGui::PopStyleVar();
            return;
        }

        if (ImGui::BeginTabItem("Hierarquia"))
        {
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputTextWithHint("##hierarchy_search", "Pesquisar nome, ID ou tipo...",
                                     mHierarchySearch, sizeof(mHierarchySearch));

            Modo* activeMode = nullptr;
            if (mHasProject && mTelaAtiva >= 0 && mTelaAtiva < (int)mProject.telas.size())
            {
                Tela& activeScreen = mProject.telas[mTelaAtiva];
                if (mModoAtivo >= 0 && mModoAtivo < (int)activeScreen.modos.size())
                    activeMode = &activeScreen.modos[mModoAtivo];
            }

            if (!mHasProject)
            {
                PanelHint("Nenhum projeto aberto. Crie um projeto (Arquivo → Novo).");
            }
            else if (mProject.telas.empty())
            {
                PanelHint("Projeto sem telas.");
            }
            else
            {
                for (int ti = 0; ti < (int)mProject.telas.size(); ++ti)
                {
                    Tela& t = mProject.telas[ti];
                    ImGui::PushID(ti);
                    const bool isAtiva = (ti == mTelaAtiva);
                    if (isAtiva) ImGui::PushStyleColor(ImGuiCol_Header,
                                                       Theme::Hex(0x4f8cff, 0.22f));
                    if (ImGui::TreeNodeEx(t.nome.c_str(), ImGuiTreeNodeFlags_DefaultOpen |
                                                          ImGuiTreeNodeFlags_OpenOnArrow))
                    {
                        if (isAtiva)
                            ImGui::TextColored(Theme::TextDisabled, "tela · id: %s", t.id.c_str());

                        for (int mi = 0; mi < (int)t.modos.size(); ++mi)
                        {
                            Modo& mo = t.modos[mi];
                            const bool modoAtivo = (ti == mTelaAtiva && mi == mModoAtivo);
                            if (modoAtivo) ImGui::PushStyleColor(ImGuiCol_Text, Theme::AccentOrange);
                            const std::string modeLabel = "  " + mo.nome + "##mode_" + std::to_string(mi);
                            if (ImGui::Selectable(modeLabel.c_str(), modoAtivo))
                            {
                                mTelaAtiva = ti;
                                mModoAtivo = mi;
                                mSelectedElementId.clear();
                            }
                            if (modoAtivo) ImGui::PopStyleColor();

                            if (modoAtivo)
                            {
                                if (ImGui::BeginDragDropTarget())
                                {
                                    if (const ImGuiPayload* payload =
                                            ImGui::AcceptDragDropPayload("SEEDUI_ELEMENT"))
                                    {
                                        mPendingReparentElementId = (const char*)payload->Data;
                                        mPendingReparentTargetId.clear();
                                    }
                                    ImGui::EndDragDropTarget();
                                }

                                ImGui::Indent(14.0f);
                                if (mo.raiz.empty())
                                    ImGui::TextColored(Theme::TextDisabled, "Nenhum elemento neste modo.");
                                else
                                    for (Element& e : mo.raiz) DrawElementTree(e);
                                ImGui::Unindent(14.0f);
                            }
                        }
                        ImGui::TreePop();
                    }
                    if (isAtiva) ImGui::PopStyleColor();
                    ImGui::PopID();
                }
            }

            if (activeMode)
            {
                if (!mSelectedElementId.empty() && !ImGui::GetIO().WantTextInput &&
                    ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
                {
                    if (ImGui::IsKeyPressed(ImGuiKey_F2))
                    {
                        if (Element* selected = Project::ResolverId(*activeMode, mSelectedElementId))
                        {
                            if (!selected->bloqueado)
                            {
                                mRenamingElementId = selected->id;
                                const std::string current = selected->nome.empty() ? selected->id : selected->nome;
                                strncpy(mRenameBuffer, current.c_str(), sizeof(mRenameBuffer) - 1);
                                mRenameBuffer[sizeof(mRenameBuffer) - 1] = 0;
                            }
                        }
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
                    {
                        Element* selected = Project::ResolverId(*activeMode, mSelectedElementId);
                        if (selected && !selected->bloqueado)
                            mPendingDeleteElementId = mSelectedElementId;
                    }
                }

                bool changed = false;
                if (!mPendingDeleteElementId.empty())
                {
                    changed = Project::ExcluirElemento(*activeMode, mPendingDeleteElementId) || changed;
                    if (changed && mSelectedElementId == mPendingDeleteElementId)
                        mSelectedElementId.clear();
                    mPendingDeleteElementId.clear();
                }
                if (!mPendingMoveElementId.empty())
                {
                    changed = Project::MoverElemento(*activeMode, mPendingMoveElementId,
                                            mPendingMoveDelta) || changed;
                    mPendingMoveElementId.clear();
                    mPendingMoveDelta = 0;
                }
                if (!mPendingReparentElementId.empty())
                {
                    changed = Project::ReparentearElemento(*activeMode, mPendingReparentElementId,
                                                mPendingReparentTargetId) || changed;
                    mPendingReparentElementId.clear();
                    mPendingReparentTargetId.clear();
                }
                if (changed)
                {
                    mProjectDirty = true;
                    mStatusMsg = "Hierarquia atualizada";
                    mStatusMsgUntil = GetTime() + 4.0;
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Inspetor"))
        {
            Element* selected = mSelectedElementId.empty()
                ? nullptr
                : Project::ResolverId(mProject, mSelectedElementId);
            if (selected)
            {
                if (mInspectorBufferedElementId != selected->id)
                {
                    mInspectorBufferedElementId = selected->id;
                    strncpy(mInspectorNameBuffer, selected->nome.c_str(),
                            sizeof(mInspectorNameBuffer) - 1);
                    mInspectorNameBuffer[sizeof(mInspectorNameBuffer) - 1] = 0;
                    strncpy(mInspectorIdBuffer, selected->id.c_str(),
                            sizeof(mInspectorIdBuffer) - 1);
                    mInspectorIdBuffer[sizeof(mInspectorIdBuffer) - 1] = 0;
                }

                ImGui::TextColored(Theme::TextDisabled, "Identidade do elemento");
                if (selected->bloqueado)
                    ImGui::TextColored(Theme::AccentOrange,
                                       "Bloqueado: desbloqueie para editar identidade");
                if (selected->bloqueado) ImGui::BeginDisabled();
                ImGui::TextUnformatted("Nome");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputText("##element_name", mInspectorNameBuffer,
                                     sizeof(mInspectorNameBuffer)))
                {
                    selected->nome = mInspectorNameBuffer;
                    mProjectDirty = true;
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Nome visual");

                ImGui::TextUnformatted("ID");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputText("##element_id", mInspectorIdBuffer,
                                     sizeof(mInspectorIdBuffer),
                                     ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    const std::string requested = mInspectorIdBuffer;
                    Element* conflict = Project::ResolverId(mProject, requested);
                    if (!requested.empty() && (!conflict || conflict == selected))
                    {
                        const std::string oldId = selected->id;
                        selected->id = requested;
                        mSelectedElementId = requested;
                        mInspectorBufferedElementId = requested;
                        mProjectDirty = true;
                        mStatusMsg = "ID alterado: " + oldId + " → " + requested;
                        mStatusMsgUntil = GetTime() + 5.0;
                    }
                    else
                    {
                        strncpy(mInspectorIdBuffer, selected->id.c_str(),
                                sizeof(mInspectorIdBuffer) - 1);
                        mInspectorIdBuffer[sizeof(mInspectorIdBuffer) - 1] = 0;
                        mStatusMsg = requested.empty() ? "O ID não pode ficar vazio"
                                                       : "Esse ID já existe no projeto";
                        mStatusMsgUntil = GetTime() + 5.0;
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("ID estável e único · Enter para confirmar");
                ImGui::TextColored(Theme::TextDisabled, "tipo: %s", selected->tipo.c_str());
                if (selected->bloqueado) ImGui::EndDisabled();

                const bool beforeVisible = selected->visivel;
                const bool beforeLocked = selected->bloqueado;
                ImGui::Checkbox("Visivel", &selected->visivel);
                ImGui::Checkbox("Bloqueado", &selected->bloqueado);
                if (beforeVisible != selected->visivel || beforeLocked != selected->bloqueado)
                    mProjectDirty = true;
                float transparency = 100.0f * (1.0f -
                    selected->estilos.value("opacidade", 1.0f));
                float outlineWidth = selected->estilos.value("espessura_borda", 1.0f);
                if (ImGui::SliderFloat("Contorno", &outlineWidth,
                                       0.0f, 16.0f, "%.1f px"))
                {
                    selected->estilos["espessura_borda"] = outlineWidth;
                    mProjectDirty = true;
                }
                if (ImGui::SliderFloat("Transparencia", &transparency,
                                       0.0f, 100.0f, "%.0f%%"))
                {
                    selected->estilos["opacidade"] = 1.0f - transparency / 100.0f;
                    mProjectDirty = true;
                }

                // Sombra (estilo CorelDRAW): cor, deslocamento e desfoque.
                ImGui::Spacing();
                ImGui::TextColored(Theme::TextSecondary, "Sombra");
                bool shadowOn = selected->estilos.contains("sombra");
                if (ImGui::Checkbox("Ativar sombra", &shadowOn))
                {
                    if (shadowOn)
                        selected->estilos["sombra"] = nlohmann::json{
                            { "cor", "#000000" },
                            { "deslocamento_x", 4.0 },
                            { "deslocamento_y", 4.0 },
                            { "desfoque", 6.0 }
                        };
                    else
                        selected->estilos.erase("sombra");
                    mProjectDirty = true;
                }
                if (shadowOn)
                {
                    auto& sombra = selected->estilos["sombra"];
                    float sx = sombra.value("deslocamento_x", 4.0f);
                    float sy = sombra.value("deslocamento_y", 4.0f);
                    float blur = sombra.value("desfoque", 6.0f);
                    if (ImGui::SliderFloat("Desloc. X", &sx, -40.0f, 40.0f))
                    {
                        sombra["deslocamento_x"] = sx;
                        mProjectDirty = true;
                    }
                    if (ImGui::SliderFloat("Desloc. Y", &sy, -40.0f, 40.0f))
                    {
                        sombra["deslocamento_y"] = sy;
                        mProjectDirty = true;
                    }
                    if (ImGui::SliderFloat("Desfoque", &blur, 0.0f, 24.0f))
                    {
                        sombra["desfoque"] = blur;
                        mProjectDirty = true;
                    }
                }

                // Contorno tracejado (estilo CorelDRAW): traço + espaço.
                ImGui::Spacing();
                ImGui::TextColored(Theme::TextSecondary, "Contorno tracejado");
                bool dashedOn = selected->estilos.contains("tracejado");
                if (ImGui::Checkbox("Ativar tracejado", &dashedOn))
                {
                    if (dashedOn)
                        selected->estilos["tracejado"] = nlohmann::json{
                            { "largura_traco", 6.0 }, { "largura_espaco", 4.0 }
                        };
                    else
                        selected->estilos.erase("tracejado");
                    mProjectDirty = true;
                }
                if (dashedOn)
                {
                    auto& tracejado = selected->estilos["tracejado"];
                    float dashLen = tracejado.value("largura_traco", 6.0f);
                    float gapLen = tracejado.value("largura_espaco", 4.0f);
                    if (ImGui::SliderFloat("Traço", &dashLen, 0.5f, 40.0f))
                    {
                        tracejado["largura_traco"] = dashLen;
                        mProjectDirty = true;
                    }
                    if (ImGui::SliderFloat("Espaço", &gapLen, 0.5f, 40.0f))
                    {
                        tracejado["largura_espaco"] = gapLen;
                        mProjectDirty = true;
                    }
                }

                // Forma do polígono: lados, estrela e raio interno.
                if (selected->tipo == "poligono")
                {
                    ImGui::Spacing();
                    ImGui::TextColored(Theme::TextSecondary, "Forma (polígono)");
                    int sides = std::max(3, (int)std::lroundf(
                        selected->transformacao.value("lados", 6.0f)));
                    if (ImGui::SliderInt("Lados", &sides, 3, 24))
                    {
                        selected->transformacao["lados"] = (float)sides;
                        mProjectDirty = true;
                    }
                    bool starOn = selected->transformacao.value("estrela", 0.0f) > 0.5f;
                    if (ImGui::Checkbox("Estrela", &starOn))
                    {
                        selected->transformacao["estrela"] = starOn ? 1.0f : 0.0f;
                        mProjectDirty = true;
                    }
                    if (starOn)
                    {
                        float innerRatio = std::max(0.1f, std::min(0.95f,
                            selected->transformacao.value("raio_interno", 0.5f)));
                        if (ImGui::SliderFloat("Raio interno", &innerRatio,
                                               0.1f, 0.95f))
                        {
                            selected->transformacao["raio_interno"] = innerRatio;
                            mProjectDirty = true;
                        }
                    }
                    ImGui::Separator();
                }

                ImGui::TextUnformatted("Alinhar");
                const char* alignTips[] = {
                    "Alinhar bordas esquerdas", "Alinhar centros horizontais",
                    "Alinhar bordas direitas", "Alinhar bordas superiores",
                    "Alinhar centros verticais", "Alinhar bordas inferiores"
                };
                const bool alignEnabled = selected->tipo == "grupo" ||
                    (mAlignTarget == 2 ? !mSelectedElementId.empty()
                     : mAlignTarget == 3 ? !mSelectedElementId.empty()
                                         : mSelectedElementIds.size() >= 2);
                if (!alignEnabled) ImGui::BeginDisabled();
                for (int operation = 0; operation < 6; ++operation)
                {
                    if (operation > 0) ImGui::SameLine(0.0f, 5.0f);
                    if (AlignmentIconButton(operation, alignTips[operation]))
                        AlinharElementosSelecionados(operation);
                }
                if (!alignEnabled) ImGui::EndDisabled();

                ImGui::Spacing();
                ImGui::TextColored(Theme::TextSecondary, "Alinhar a:");
                if (ImGui::RadioButton("Selecao", mAlignTarget == 0)) mAlignTarget = 0;
                ImGui::SameLine();
                if (ImGui::RadioButton("Principal", mAlignTarget == 1)) mAlignTarget = 1;
                ImGui::SameLine();
                if (ImGui::RadioButton("Tela", mAlignTarget == 2)) mAlignTarget = 2;
                ImGui::SameLine();
                if (ImGui::RadioButton("Conjunto", mAlignTarget == 3)) mAlignTarget = 3;
                ImGui::TextColored(Theme::TextDisabled,
                    mAlignTarget == 0 ? "Usa os limites da selecao"
                  : mAlignTarget == 1 ? "Mantem o elemento principal parado"
                  : mAlignTarget == 2 ? "Usa os limites da tela base"
                                      : "Centraliza no conjunto dos vizinhos (distancias uniformes)");
                ImGui::Separator();
            }

            if (!selected)
                PanelHint("Selecione um elemento na Hierarquia ou no canvas.");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Biblioteca"))
        {
            struct ComponentButton { const char* tipo; const char* nome; IconId icon; };
            static const ComponentButton kBasicComponents[] = {
                { "painel",         "Painel",          IconId::Library },
                { "janela",         "Janela",          IconId::Model },
                { "texto",          "Texto",           IconId::Text },
                { "botao",          "Botão",           IconId::Check },
                { "campo_numerico", "Campo numérico",  IconId::Plus },
                { "slider",         "Slider",          IconId::Inspector },
            };

            auto componentRow = [&](const char* id, const char* name, IconId icon, bool enabled)
            {
                ImGui::PushID(id);
                if (!enabled) ImGui::BeginDisabled();
                const float rowW = ImGui::GetContentRegionAvail().x;
                const bool clicked = ImGui::Selectable("##component", false,
                    ImGuiSelectableFlags_None, ImVec2(rowW, 32.0f));
                const ImVec2 rowMin = ImGui::GetItemRectMin();
                const ImU32 textColor = ImGui::ColorConvertFloat4ToU32(
                    enabled ? Theme::TextPrimary : Theme::TextDisabled);
                DrawIconAt(icon, rowMin.x + 9.0f, rowMin.y + 7.0f, 18.0f);
                ImGui::GetWindowDrawList()->AddText(ImVec2(rowMin.x + 38.0f,
                                                           rowMin.y + 8.0f),
                                                    textColor, name);
                if (enabled && ImGui::IsItemHovered())
                    ImGui::SetTooltip("Clique para inserir ou arraste para o canvas");
                if (enabled && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    ImGui::SetDragDropPayload("SEEDUI_COMPONENT", id, strlen(id) + 1);
                    DrawIcon(icon, 18.0f);
                    ImGui::SameLine();
                    ImGui::Text("Criar %s no canvas", name);
                    ImGui::EndDragDropSource();
                }
                if (!enabled) ImGui::EndDisabled();
                ImGui::PopID();
                return clicked && enabled;
            };

            if (PossuiModoAtivo())
            {
                ImGui::TextColored(Theme::AccentOrange, "Validação atual · Painel");
                for (const ComponentButton& c : kBasicComponents)
                {
                    const bool validatingNow = strcmp(c.tipo, "painel") == 0;
                    if (componentRow(c.tipo, c.nome, c.icon, validatingNow))
                        AdicionarComponente(c.tipo, c.nome);
                }
                ImGui::TextColored(Theme::TextDisabled,
                                   "Clique para inserir ou arraste para o canvas.");
                ImGui::Separator();
            }
            else
            {
                PanelHint("Nenhuma tela/modo ativo. Crie um projeto em Arquivo → Novo para inserir componentes.");
            }

            if (ImGui::CollapsingHeader("Próximos componentes · indisponíveis"))
            {
                PanelHint("Estes componentes pertencem a marcos futuros e ainda não são interativos.");
                struct PlannedComponent { const char* nome; IconId icon; };
                static const PlannedComponent kPlannedComponents[] = {
                    { "Caixa", IconId::Library }, { "Grupo", IconId::Hierarchy },
                    { "Título", IconId::Text }, { "Botão com ícone", IconId::Check },
                    { "Alternância", IconId::Redo }, { "Caixa de seleção", IconId::Check },
                    { "Campo de texto", IconId::Text }, { "Campo de senha", IconId::Lock },
                    { "Área de texto", IconId::Directives }, { "Lista", IconId::List },
                    { "Lista suspensa", IconId::CaretDown }, { "Slider com valor", IconId::Inspector },
                    { "Barra de progresso", IconId::History }, { "Indicador circular", IconId::Redo },
                    { "Seletor de cor", IconId::Palette }, { "Separador", IconId::List },
                    { "Barra de rolagem", IconId::Inspector }, { "Tooltip", IconId::Question },
                    { "Menu de contexto", IconId::List }, { "Janela modal", IconId::Model },
                    { "Diálogo", IconId::Directives }, { "Notificação", IconId::Warning },
                    { "Inspetor", IconId::Inspector }, { "Viewport", IconId::Image },
                };
                int plannedIndex = 0;
                for (const PlannedComponent& c : kPlannedComponents)
                {
                    const std::string id = std::string("planned_") + std::to_string(plannedIndex++);
                    componentRow(id.c_str(), c.nome, c.icon, false);
                }
            }
            ImGui::Spacing();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Diretrizes"))
        {
            ImGui::TextWrapped("Comentário geral do programa (o que você quer comunicar à IA sobre o SeedUI):");
            if (!mGlobalBufLoaded)
            {
                strncpy(mGlobalBuf, mGlobalDirectives.c_str(), sizeof(mGlobalBuf) - 1);
                mGlobalBuf[sizeof(mGlobalBuf) - 1] = 0;
                mGlobalBufLoaded = true;
            }
            ImGui::InputTextMultiline("##global", mGlobalBuf, sizeof(mGlobalBuf),
                                      ImVec2(-1.0f, 110.0f));
            mGlobalDirectives = mGlobalBuf;
            if (ImGui::Button("Copiar tudo para a IA (anotações + comentário)", ImVec2(-1.0f, 0.0f)))
            {
                ImGui::SetClipboardText(AnnotationsToText(mAnnot, mGlobalDirectives).c_str());
            }

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDisabled);
            ImGui::TextWrapped("Use a ferramenta \"Anotar\" (A) para marcar QUALQUER parte do SeedUI — menus, ícones, painéis ou canvas. Depois exporte as diretrizes: você só precisa dizer aqui no chat \"veja as novas alterações\".");
            ImGui::PopStyleColor();
            ImGui::Spacing();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Recursos"))
        {
            PanelHint("Importe SVGs, imagens e fontes. Gerenciador completo no M08.");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Histórico"))
        {
            PanelHint("Desfazer/refazer com histórico chega no M05.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::PopStyleVar();
    }

    void App::DrawStatusBar()
    {
        ImGui::Separator();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
        ImGui::BeginChild("##status", ImVec2(0, kStatusBarHeight), false,
                          ImGuiWindowFlags_NoScrollbar);

        if (mCurrentTool == Tool::Annotate)
        {
            // O bloco de exportar/diretrizes agora fica no TOPO (menu bar).
            // Aqui no rodapé só resta o indicador compacto do modo.
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(Theme::AccentOrange,
                               "● Modo Debug · Anotações: %d", (int)mAnnot.items.size());
            ImGui::SameLine(0, 14);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(Theme::TextSecondary,
                               "Exportar no menu superior (Exportar .txt + print)");
        }
        else
        {
            ImGui::AlignTextToFramePadding();

            // Detalhes do objeto (estilo CorelDRAW): quando há seleção, mostra
            // tipo · dimensões · posição; senão, a mensagem de status.
            Element* detailEl = nullptr;
            if (PossuiModoAtivo() && !mSelectedElementIds.empty())
                detailEl = Project::ResolverId(
                    mProject.telas[mTelaAtiva].modos[mModoAtivo],
                    mSelectedElementIds.front());
            if (detailEl)
            {
                const float ex = detailEl->transformacao.value("x", 0.0f);
                const float ey = detailEl->transformacao.value("y", 0.0f);
                const float ew = detailEl->transformacao.value("largura", 160.0f);
                const float eh = detailEl->transformacao.value("altura", 32.0f);
                ImGui::TextColored(Theme::TextPrimary,
                                   "Detalhes: %s · %.0f×%.0f · (%.0f, %.0f)",
                                   detailEl->tipo.c_str(), ew, eh, ex, ey);
            }
            else if (!mStatusMsg.empty() && GetTime() < mStatusMsgUntil)
            {
                ImGui::TextColored(Theme::Success, "%.*s", 52, mStatusMsg.c_str());
            }
            else
            {
                ImGui::TextColored(Theme::TextSecondary, "Nenhum objeto ativo");
            }

            ImGui::SameLine(0, 16);
            ImGui::TextColored(Theme::TextSecondary, "● Modo Normal");
            ImGui::SameLine(0, 18);
            ImGui::TextColored(Theme::TextSecondary, "Zoom: %.0f%%", mCanvasZoom * 100.0f);
            ImGui::SameLine(0, 18);
            ImGui::TextColored(Theme::TextSecondary, "%.0f×%.0f",
                               (float)mProject.telaBaseLargura,
                               (float)mProject.telaBaseAltura);

            const ImGuiIO& io = ImGui::GetIO();
            ImGui::SameLine(0, 18);
            if (fabsf(io.MousePos.x) > 1.0e30f || fabsf(io.MousePos.y) > 1.0e30f)
                ImGui::TextColored(Theme::TextSecondary, "Mouse: (—, —)");
            else
                ImGui::TextColored(Theme::TextSecondary, "Mouse: (%.0f, %.0f)",
                                   io.MousePos.x, io.MousePos.y);

            // Cor do preenchimento em CMYK + espessura do contorno (direita).
            if (detailEl && detailEl->estilos.is_object())
            {
                const std::string hex = detailEl->estilos.value("cor_fundo", "ffffff");
                unsigned int value = 0;
                std::string h = hex;
                if (!h.empty() && h[0] == '#') h = h.substr(1);
                bool parsed = h.size() >= 6;
                if (parsed)
                {
                    try { value = (unsigned int)std::stoul(h.substr(0, 6), nullptr, 16); }
                    catch (...) { parsed = false; }
                }
                if (parsed)
                {
                    const float r = ((value >> 16) & 0xFF) / 255.0f;
                    const float g = ((value >> 8) & 0xFF) / 255.0f;
                    const float b = (value & 0xFF) / 255.0f;
                    const float k = 1.0f - std::max(r, std::max(g, b));
                    const float denom = (k < 0.999f) ? (1.0f - k) : 1.0f;
                    const float c = (1.0f - r - k) / denom;
                    const float m = (1.0f - g - k) / denom;
                    const float y = (1.0f - b - k) / denom;
                    const float borda = detailEl->estilos.value("espessura_borda", 0.0f);
                    const std::string cmyk = "C:" + std::to_string((int)roundf(c * 100.0f)) +
                                             " M:" + std::to_string((int)roundf(m * 100.0f)) +
                                             " Y:" + std::to_string((int)roundf(y * 100.0f)) +
                                             " K:" + std::to_string((int)roundf(k * 100.0f));
                    const float needC = ImGui::CalcTextSize(cmyk.c_str()).x +
                                        ImGui::CalcTextSize(" · 1.0px").x + 20.0f;
                    const float rightBound = ImGui::GetWindowContentRegionMax().x - 170.0f;
                    if (ImGui::GetCursorPosX() + needC < rightBound)
                    {
                        ImGui::SameLine(0, 20);
                        ImGui::TextColored(Theme::TextSecondary, "%s", cmyk.c_str());
                        ImGui::SameLine(0, 8);
                        ImGui::TextColored(Theme::TextSecondary, "· %.1fpx", borda);
                    }
                }
            }
        }

        const char* savedLabel = mProjectDirty ? "● Alterações não salvas"
                                               : "Sem alterações não salvas";
        const float needW = ImGui::CalcTextSize(savedLabel).x;
        const float rightX = ImGui::GetWindowContentRegionMax().x - needW;
        if (ImGui::GetCursorPosX() + 20.0f < rightX)
        {
            ImGui::SameLine(rightX);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(mProjectDirty ? Theme::Warning : Theme::TextSecondary,
                               "%s", savedLabel);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    void App::DrawColorBar()
    {
        // Paleta de cores fixa na parte inferior (estilo CorelDRAW): clique
        // esquerdo = preenchimento, clique direito = contorno do selecionado.
        ImGui::Separator();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 5.0f));
        ImGui::BeginChild("##colorbar", ImVec2(0, kColorBarHeight), false,
                          ImGuiWindowFlags_NoScrollbar);
        // Centralização vertical da linha na faixa: o cursor Y dentro do child
        // já desconta o padding (8,5); (kColorBarHeight - altura) / 2 - 5
        // deixa texto e swatches exatamente no MEIO da faixa.
        const float swatchSize = 18.0f;
        ImGui::SetCursorPosY((kColorBarHeight - swatchSize) * 0.5f - 5.0f);
        ImGui::TextColored(Theme::TextSecondary, "Paleta:");
        ImGui::SameLine(0, 8);

        constexpr ImU32 kPalette[] = {
            0xffffff, 0x000000, 0x808080, 0xc0c0c0, 0x404040,
            0xff0000, 0xff8000, 0xffff00, 0x80ff00, 0x00ff00, 0x00ff80,
            0x00ffff, 0x0080ff, 0x0000ff, 0x8000ff, 0xff00ff, 0xff0080,
            0x8b0000, 0x8b4500, 0x8b8b00, 0x008b00, 0x008b8b, 0x00008b, 0x8b008b,
            0x4f8cff, 0xf57900, 0x2ecc71, 0xa78bfa, 0xf1c40f, 0xe74c3c,
        };
        for (ImU32 color : kPalette)
        {
            const ImVec4 col = Theme::Hex(color);
            ImGui::PushID((int)color);
            ImGui::ColorButton("##sw", col,
                               ImGuiColorEditFlags_NoTooltip |
                               ImGuiColorEditFlags_NoPicker |
                               ImGuiColorEditFlags_NoBorder,
                               ImVec2(18.0f, 18.0f));
            const bool hovered = ImGui::IsItemHovered();
            const bool leftClick = hovered && ImGui::IsMouseClicked(0);
            const bool rightClick = hovered && ImGui::IsMouseClicked(1);
            if (hovered)
                ImGui::SetTooltip("%s  (esq. preenche · dir. contorno)",
                                  ColorUtils::ToHex(col.x, col.y, col.z).c_str());
            if (leftClick || rightClick)
            {
                if (PossuiModoAtivo() && !mSelectedElementIds.empty())
                {
                    // Multi-seleção: a cor é aplicada a TODOS os selecionados
                    // (e a todos os filhos quando o selecionado é um GRUPO).
                    Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
                    const bool border = rightClick;
                    const std::string hex = ColorUtils::ToHex(col.x, col.y, col.z);
                    int applied = 0;
                    bool blocked = false;
                    applied = ApplyStyleToSelection(mode, mSelectedElementIds,
                        border ? "cor_borda" : "cor_fundo", hex, blocked);
                    if (applied)
                    {
                        mProjectDirty = true;
                        mStatusMsg = std::string(border ? "Contorno: "
                                                        : "Preenchimento: ") +
                                     hex + (applied > 1
                                         ? " (" + std::to_string(applied) + " elementos)"
                                         : "");
                        mStatusMsgUntil = GetTime() + 4.0;
                    }
                    else if (blocked)
                    {
                        mStatusMsg = "Elemento bloqueado — desbloqueie para colorir";
                        mStatusMsgUntil = GetTime() + 4.0;
                    }
                }
                else
                {
                    mStatusMsg = "Selecione um elemento para aplicar a cor";
                    mStatusMsgUntil = GetTime() + 4.0;
                }
            }
            ImGui::SameLine(0, 3);
            ImGui::PopID();
        }

        ImGui::SameLine(0, 10);
        ImGui::SetCursorPosY((kColorBarHeight - 22.0f) * 0.5f - 5.0f);
        if (ImGui::Button("Seletor de cor...##open_picker", ImVec2(118, 22)))
            mColorPickerOpen = true;
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    void App::DrawColorPickerWindow()
    {
        if (!mColorPickerOpen) return;
        ImGui::SetNextWindowSize(ImVec2(284, 0), ImGuiCond_Always);
        if (!ImGui::Begin("Seletor de cor", &mColorPickerOpen,
                          ImGuiWindowFlags_NoResize))
        {
            ImGui::End();
            return;
        }
        if (!PossuiModoAtivo() || mSelectedElementId.empty())
        {
            ImGui::TextColored(Theme::TextSecondary,
                               "Selecione um elemento para colorir.");
            ImGui::End();
            return;
        }
        Element* element = Project::ResolverId(
            mProject.telas[mTelaAtiva].modos[mModoAtivo], mSelectedElementId);
        if (!element || element->bloqueado)
        {
            ImGui::TextColored(Theme::TextSecondary,
                               "Elemento bloqueado — desbloqueie para colorir.");
            ImGui::End();
            return;
        }
        float rgb[3] = { 0x2b / 255.0f, 0x2b / 255.0f, 0x2b / 255.0f };
        ColorUtils::ParseHex(element->estilos.value("cor_fundo", "#2b2b2b"), rgb);
        if (ColorPicker::Widget("##picker", rgb))
        {
            // Multi-seleção: aplica a todos os selecionados (grupos incluem
            // todos os filhos recursivamente).
            const std::string hex = ColorUtils::ToHex(rgb[0], rgb[1], rgb[2]);
            Modo& mode = mProject.telas[mTelaAtiva].modos[mModoAtivo];
            bool blocked = false;
            const int applied = ApplyStyleToSelection(mode, mSelectedElementIds,
                                                      "cor_fundo", hex, blocked);
            if (applied) mProjectDirty = true;
        }
        ImGui::End();
    }

    void App::DrawStartScreen()
    {
        ImGui::BeginChild("##start", ImVec2(0, 0));
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 c = ImGui::GetWindowPos();
        const ImVec2 sz = ImGui::GetWindowSize();
        const float cx = c.x + sz.x * 0.5f;

        ImFont* font = ImGui::GetFont();
        const ImU32 text = ImGui::ColorConvertFloat4ToU32(Theme::TextPrimary);
        const ImU32 sub = ImGui::ColorConvertFloat4ToU32(Theme::TextSecondary);
        const ImU32 accent = ImGui::ColorConvertFloat4ToU32(Theme::AccentBlue);

        // Título
        const char* title = "SeedUI";
        const ImVec2 ts = font->CalcTextSizeA(52.0f, FLT_MAX, 0.0f, title);
        dl->AddText(font, 52.0f, ImVec2(cx - ts.x * 0.5f, c.y + 120.0f), text, title);

        const char* sub1 = "Editor Visual de Interfaces";
        const ImVec2 ss1 = font->CalcTextSizeA(19.0f, FLT_MAX, 0.0f, sub1);
        dl->AddText(font, 19.0f, ImVec2(cx - ss1.x * 0.5f, c.y + 120.0f + 64.0f), sub, sub1);

        const char* sub2 = "WYSIWYG  ·  Telas & Modos  ·  Diretrizes para IA  ·  100% offline";
        const ImVec2 ss2 = font->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, sub2);
        dl->AddText(font, 14.0f, ImVec2(cx - ss2.x * 0.5f, c.y + 120.0f + 96.0f),
                    ImGui::ColorConvertFloat4ToU32(Theme::TextDisabled), sub2);

        // Botões
        const float bw = 220.0f;
        const float gap = 12.0f;
        const float rowY = c.y + 330.0f;
        const float rowX = cx - (bw * 3.0f + gap * 2.0f) * 0.5f;

        ImGui::SetCursorPosX(rowX - c.x);
        ImGui::SetCursorPosY(rowY - c.y);
        if (ImGui::Button("Novo projeto", ImVec2(bw, 42))) NovoProjeto();
        ImGui::SameLine(0, gap);
        if (ImGui::Button("Abrir projeto", ImVec2(bw, 42))) AbrirProjeto();
        ImGui::SameLine(0, gap);
        ImGui::BeginDisabled();
        if (ImGui::Button("Modelos", ImVec2(bw, 42))) {}
        ImGui::EndDisabled();

        ImGui::SetCursorPosX(cx - c.x - bw * 0.5f);
        ImGui::SetCursorPosY(rowY - c.y + 42.0f + 14.0f);
        if (ImGui::Button("Abrir Manual (F1)", ImVec2(bw, 36))) mShowManual = true;

        ImGui::SetCursorPosY(rowY - c.y + 42.0f + 14.0f + 36.0f + 10.0f);
        if (ImGui::Button("Explorar o workspace (demo)", ImVec2(bw, 36)))
        {
            mProject.CriarNovo("Projeto de demonstração", 1280, 720);
            mHasProject = true;
            ResetarHistorico();
            mProjectDirty = false;
            mTelaAtiva = 0;
            mModoAtivo = 0;
            mSelectedElementId.clear();
            mStatusMsg = "Workspace de demonstração criado com Tela principal · modo Padrão";
            mStatusMsgUntil = GetTime() + 8.0;
        }

        // Linha de acento
        dl->AddLine(ImVec2(cx - 60.0f, c.y + 180.0f), ImVec2(cx + 60.0f, c.y + 180.0f), accent, 2.0f);

        // Rodapé
        const char* foot = "Milestone 03 — projetos reais (.ui.json) · telas, modos e salvamento (ctrl+S)";
        const ImVec2 fs = font->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, foot);
        dl->AddText(font, 13.0f, ImVec2(cx - fs.x * 0.5f, c.y + sz.y - 40.0f),
                    ImGui::ColorConvertFloat4ToU32(Theme::TextDisabled), foot);

        ImGui::EndChild();
    }

    void App::DrawManualWindow()
    {
        ManualDraw(&mShowManual);
    }

    void App::DrawAboutWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Always);
        if (!ImGui::Begin("Sobre o SeedUI", &mShowAbout, ImGuiWindowFlags_NoResize))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("SeedUI — Editor Visual de Interfaces");
        ImGui::TextColored(Theme::TextSecondary, "Versão 0.2 — Milestone 03 (projetos .ui.json)");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Ferramenta independente para projetar interfaces de jogo/aplicação, "
            "com WYSIWYG, telas & modos, e diretrizes em texto para a IA.");
        ImGui::Separator();
        ImGui::TextColored(Theme::TextSecondary,
                           "Tecnologias: C++17 · raylib · Dear ImGui · nanosvg");
        ImGui::TextColored(Theme::TextSecondary,
                           "Ícones: Phosphor (MIT) · Fonte: Inter (OFL)");
        ImGui::Spacing();
        ImGui::End();
    }
}
