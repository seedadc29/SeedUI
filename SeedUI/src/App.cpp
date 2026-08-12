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
        constexpr float kStatusBarHeight = 34.0f;
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

        // Cabeçalho de painel no estilo Blender (faixa de título)
        bool PanelBegin(IconId icon, const char* title)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, Theme::PanelHeader);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Theme::BorderLight);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, Theme::PanelHeader);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 7));
            const std::string label = std::string("        ") + title;
            const bool open = ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            const ImVec2 headerMin = ImGui::GetItemRectMin();
            DrawIconAt(icon, headerMin.x + 33.0f, headerMin.y + 6.0f, 18.0f);
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            return open;
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
            const ImVec2 size(30.0f, 28.0f);
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
            case IconId::Color:  return Tool::Color;
            case IconId::Grid:   return Tool::Grid;
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
                                   std::vector<std::string>& out)
        {
            for (const Element& element : elements)
            {
                if (!element.visivel) continue;
                const float x = element.transformacao.value("x", 0.0f);
                const float y = element.transformacao.value("y", 0.0f);
                const float w = element.transformacao.value("largura", 160.0f);
                const float h = element.transformacao.value("altura", 32.0f);
                if (x < right && x + w > left && y < bottom && y + h > top)
                    out.push_back(element.id);
                CollectElementsInRect(element.filhos, left, top, right, bottom, out);
            }
        }

        template<typename Start>
        void CollectTransformStarts(Element& element, std::vector<Start>& out)
        {
            const bool exists = std::any_of(out.begin(), out.end(), [&](const auto& start)
            {
                return start.id == element.id;
            });
            if (!exists && !element.bloqueado)
            {
                Start start;
                start.id = element.id;
                start.x = element.transformacao.value("x", 0.0f);
                start.y = element.transformacao.value("y", 0.0f);
                start.w = element.transformacao.value("largura", 160.0f);
                start.h = element.transformacao.value("altura", 32.0f);
                out.push_back(start);
            }
            if (element.tipo == "grupo")
                for (Element& child : element.filhos) CollectTransformStarts(child, out);
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
        if (alignTarget == 2)
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
                float x = operation == 0 ? target
                        : operation == 1 ? target - w * 0.5f : target - w;
                x = std::max(0.0f, std::min((float)mProject.telaBaseLargura - w, x));
                if (element->transformacao.value("x", 0.0f) != x)
                {
                    element->transformacao["x"] = x;
                    changed = true;
                }
            }
            else
            {
                float y = operation == 3 ? target
                        : operation == 4 ? target - h * 0.5f : target - h;
                y = std::max(0.0f, std::min((float)mProject.telaBaseAltura - h, y));
                if (element->transformacao.value("y", 0.0f) != y)
                {
                    element->transformacao["y"] = y;
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
            element->transformacao[horizontal ? "x" : "y"] = cursor;
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
        const bool shapeTool = mCurrentTool == Tool::Rectangle ||
                               mCurrentTool == Tool::Ellipse ||
                               mCurrentTool == Tool::Polygon;
        if (PossuiModoAtivo() && shapeTool)
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
                    mShapeEndX, mShapeEndY, true, mCanvasZoom, mCanvasPanX, mCanvasPanY);
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            }
            if (mShapeCreating && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                const float x = std::min(mShapeStartX, mShapeEndX);
                const float y = std::min(mShapeStartY, mShapeEndY);
                const float w = std::max(8.0f, fabsf(mShapeEndX - mShapeStartX));
                const float h = std::max(8.0f, fabsf(mShapeEndY - mShapeStartY));
                const char* type = mCurrentTool == Tool::Rectangle ? "retangulo" :
                                   mCurrentTool == Tool::Ellipse ? "elipse" : "poligono";
                const char* name = mCurrentTool == Tool::Rectangle ? "Retangulo" :
                                   mCurrentTool == Tool::Ellipse ? "Elipse" : "Poligono";
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
        if (!PossuiModoAtivo() || !editTool)
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
            if (x < ex - tolerance || x > ex + ew + tolerance ||
                y < ey - tolerance || y > ey + eh + tolerance)
                return 0;
            if (element.tipo == "grupo")
                return x >= ex && x <= ex + ew && y >= ey && y <= ey + eh ? 1 : 0;

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

        auto applyCursor = [](int dragMode)
        {
            if (dragMode == 2 || dragMode == 3) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            else if (dragMode == 4 || dragMode == 5) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            else if (dragMode == 6 || dragMode == 9) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            else if (dragMode == 7 || dragMode == 8) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
            else if (dragMode == 10 || dragMode == 12) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            else if (dragMode == 11 || dragMode == 13) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
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
            const int currentHandle = current ? dragModeAt(*current, mouseX, mouseY) : 0;
            Element* hit = currentHandle >= 2
                ? current
                : Project::ElementoNoPonto(mode, mouseX, mouseY);
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
            if (hit && keepHitForDrag && !hit->bloqueado)
            {
                mCanvasDragMode = dragModeAt(*hit, mouseX, mouseY);
                mCanvasDragMouseX = mouseX;
                mCanvasDragMouseY = mouseY;
                mCanvasDragX = hit->transformacao.value("x", 0.0f);
                mCanvasDragY = hit->transformacao.value("y", 0.0f);
                mCanvasDragW = hit->transformacao.value("largura", 160.0f);
                mCanvasDragH = hit->transformacao.value("altura", 32.0f);
                for (int corner = 0; corner < 4; ++corner)
                    mCanvasCornerRadiusStarts[corner] =
                        ElementCornerRadius(*hit, 10 + corner);
                if (mCanvasDragMode == 1)
                {
                    for (const std::string& id : mSelectedElementIds)
                    {
                        Element* element = Project::ResolverId(mode, id);
                        if (!element || element->bloqueado) continue;
                        CollectTransformStarts(*element, mCanvasGroupStarts);
                    }
                }
                applyCursor(mCanvasDragMode);
            }
        }

        if (mCanvasMarquee && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            CanvasScreenToProject(&mProject, mouse.x, mouse.y, mouseX, mouseY, true,
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
                CollectElementsInRect(mode.raiz, left, top, right, bottom, found);
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

        if (mCanvasDragMode != 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            CanvasScreenToProject(&mProject, mouse.x, mouse.y, mouseX, mouseY, true,
                                  mCanvasZoom, mCanvasPanX, mCanvasPanY);
            Element* selected = Project::ResolverId(mode, mSelectedElementId);
            if (!selected || selected->bloqueado)
            {
                mCanvasDragMode = 0;
                return;
            }

            float dx = mouseX - mCanvasDragMouseX;
            float dy = mouseY - mCanvasDragMouseY;
            if (mSnapEnabled && !(mCanvasDragMode >= 10 && mCanvasDragMode <= 13))
            {
                constexpr float snapStep = 8.0f;
                if (mCanvasDragMode == 1)
                {
                    dx = roundf((mCanvasDragX + dx) / snapStep) * snapStep - mCanvasDragX;
                    dy = roundf((mCanvasDragY + dy) / snapStep) * snapStep - mCanvasDragY;
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
                float minDx = -FLT_MAX, maxDx = FLT_MAX;
                float minDy = -FLT_MAX, maxDy = FLT_MAX;
                for (const CanvasTransformStart& start : mCanvasGroupStarts)
                {
                    minDx = std::max(minDx, -start.x);
                    maxDx = std::min(maxDx, (float)mProject.telaBaseLargura - start.x - start.w);
                    minDy = std::max(minDy, -start.y);
                    maxDy = std::min(maxDy, (float)mProject.telaBaseAltura - start.y - start.h);
                }
                dx = std::max(minDx, std::min(maxDx, dx));
                dy = std::max(minDy, std::min(maxDy, dy));
                for (const CanvasTransformStart& start : mCanvasGroupStarts)
                {
                    if (Element* element = Project::ResolverId(mode, start.id))
                    {
                        element->transformacao["x"] = start.x + dx;
                        element->transformacao["y"] = start.y + dy;
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
                if (resizeLeft)
                    left = std::max(0.0f, std::min(right - minSize, mCanvasDragX + dx));
                if (resizeRight)
                    right = std::min((float)mProject.telaBaseLargura,
                                     std::max(left + minSize, mCanvasDragX + mCanvasDragW + dx));
                if (resizeTop)
                    top = std::max(0.0f, std::min(bottom - minSize, mCanvasDragY + dy));
                if (resizeBottom)
                    bottom = std::min((float)mProject.telaBaseAltura,
                                      std::max(top + minSize, mCanvasDragY + mCanvasDragH + dy));
                selected->transformacao["x"] = left;
                selected->transformacao["y"] = top;
                selected->transformacao["largura"] = right - left;
                selected->transformacao["altura"] = bottom - top;
                ClampElementCornerRadii(*selected);
            }

            mCanvasDragChanged = true;
            mProjectDirty = true;
            applyCursor(mCanvasDragMode);
        }

        if (mCanvasDragMode != 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (mCanvasDragChanged)
            {
                if (mCanvasDragMode == 1)
                    mStatusMsg = std::to_string(mCanvasGroupStarts.size()) +
                                 " elemento(s) movido(s)";
                else if (mCanvasDragMode >= 10 && mCanvasDragMode <= 13)
                    mStatusMsg = "Arredondamento da quina alterado";
                else
                    mStatusMsg = "Elemento redimensionado";
                mStatusMsgUntil = GetTime() + 4.0;
                TraceLog(LOG_INFO, "M05 selecao: transformacao alterada (%s)",
                         mSelectedElementId.c_str());
            }
            mCanvasDragMode = 0;
            mCanvasDragChanged = false;
            mCanvasGroupStarts.clear();
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
                // espaço para a barra terminar exatamente onde começa o workspace.
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);

                const float availY = ImGui::GetContentRegionAvail().y - kStatusBarHeight;

                ImGui::BeginChild("##toolbar", ImVec2(kToolbarWidth, availY), false,
                                  ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoScrollWithMouse);
                DrawToolbar();
                ImGui::EndChild();

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
                    if (wheel > 0.0f) mCanvasZoom = std::min(4.0f, mCanvasZoom * 1.1f);
                    if (wheel < 0.0f) mCanvasZoom = std::max(0.25f, mCanvasZoom / 1.1f);
                    if (mCurrentTool == Tool::Zoom)
                    {
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            mCanvasZoom = std::min(4.0f, mCanvasZoom * 1.25f);
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                            mCanvasZoom = std::max(0.25f, mCanvasZoom / 1.25f);
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    }
                }
                HandleCanvasInteraction(canvasHovered);
                CanvasDraw(mHasProject ? &mProject : nullptr, mTelaAtiva, mModoAtivo,
                           &mSelectedElementIds, mSelectedElementId.c_str(),
                           mSelectedCornerMask, mRulersVisible, mCanvasZoom,
                           mCanvasPanX, mCanvasPanY);
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
    }

    void App::DrawMenuBar()
    {
        if (!ImGui::BeginMenuBar()) return;

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
            MenuItemSoon("Duplicar", "M05");
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
            if (ImGui::MenuItem("Snap de 8 unidades", nullptr, mSnapEnabled))
                mSnapEnabled = !mSnapEnabled;
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
            ? !mSelectedElementId.empty() : mSelectedElementIds.size() >= 2);
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
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::SetNextItemWidth(54.0f);
        ImGui::InputFloat("##space_h", &mHorizontalSpacing, 0.0f, 0.0f, "H %.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Espaçamento horizontal");
        ImGui::SameLine();
        if (ImGui::Button("↔##distribute_h", ImVec2(28.0f, button)))
            DistribuirElementosSelecionados(true);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Aplicar espaçamento horizontal");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(54.0f);
        ImGui::InputFloat("##space_v", &mVerticalSpacing, 0.0f, 0.0f, "V %.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Espaçamento vertical");
        ImGui::SameLine();
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
        if (mSnapEnabled) ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x4f8cff, 0.30f));
        if (IconButton(IconId::Grid, mSnapEnabled ? "Snap ativo · clique para desligar"
                                                  : "Snap inativo · clique para ligar", button))
            mSnapEnabled = !mSnapEnabled;
        if (mSnapEnabled) ImGui::PopStyleColor();
        ImGui::SameLine();
        if (mRulersVisible) ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x4f8cff, 0.30f));
        if (IconButton(IconId::List, mRulersVisible ? "Réguas visíveis · clique para ocultar"
                                                    : "Réguas ocultas · clique para mostrar", button))
            mRulersVisible = !mRulersVisible;
        if (mRulersVisible) ImGui::PopStyleColor();

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
        ToolButton(IconId::Rectangle, "Criar retângulo · arraste no canvas", kFamilyCreate);
        ToolButton(IconId::Ellipse, "Criar elipse · arraste no canvas", kFamilyCreate);
        ToolButton(IconId::Polygon, "Criar polígono · arraste no canvas", kFamilyCreate);
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
        if (mCurrentTool == Tool::Zoom)
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x4f8cff, 0.30f));
        if (IconButton(IconId::ZoomIn, "Zoom In - clique ou use Z no canvas",
                       kToolButtonSize, kFamilyNav))
        {
            mCurrentTool = Tool::Zoom;
            mCanvasZoom = std::min(4.0f, mCanvasZoom * 1.25f);
        }
        if (mCurrentTool == Tool::Zoom) ImGui::PopStyleColor();
        ImGui::SetCursorPosX((kToolbarWidth - kToolButtonSize) * 0.5f);
        if (IconButton(IconId::ZoomOut, "Zoom Out - botao direito com Z",
                       kToolButtonSize, kFamilyNav))
        {
            mCurrentTool = Tool::Zoom;
            mCanvasZoom = std::max(0.25f, mCanvasZoom / 1.25f);
        }
        ToolButton(IconId::Pan, "Navegação · Mão (H)", kFamilyNav);
        ToolButton(IconId::Grid, "Visualização · Grade (G)", kFamilyNav);
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

        if (PanelBegin(IconId::List, "HIERARQUIA"))
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
        }
        if (PanelBegin(IconId::Inspector, "INSPETOR"))
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
                ImGui::Separator();

                ImGui::TextUnformatted("Alinhar");
                const char* alignTips[] = {
                    "Alinhar bordas esquerdas", "Alinhar centros horizontais",
                    "Alinhar bordas direitas", "Alinhar bordas superiores",
                    "Alinhar centros verticais", "Alinhar bordas inferiores"
                };
                const bool alignEnabled = selected->tipo == "grupo" ||
                    (mAlignTarget == 2 ? !mSelectedElementId.empty()
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
                ImGui::TextColored(Theme::TextDisabled,
                    mAlignTarget == 0 ? "Usa os limites da selecao"
                  : mAlignTarget == 1 ? "Mantem o elemento principal parado"
                                      : "Usa os limites da tela base");
                ImGui::Separator();
            }

            if (!selected)
                PanelHint("Selecione um elemento na Hierarquia ou no canvas.");
        }

        if (PanelBegin(IconId::Plus, "BIBLIOTECA"))
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
        }
        if (PanelBegin(IconId::Directives, "DIRETRIZES"))
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
        }

        if (PanelBegin(IconId::Assets, "RECURSOS"))
        {
            PanelHint("Importe SVGs, imagens e fontes. Gerenciador completo no M08.");
        }

        if (PanelBegin(IconId::History, "HISTÓRICO"))
        {
            PanelHint("Desfazer/refazer com histórico chega no M05.");
        }
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
            // No modo de revisão, só ficam visíveis dados relacionados à revisão.
            // Zoom/resolução/mouse voltam no modo normal, evitando congestionamento.
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(Theme::AccentOrange,
                               "● Modo Debug · Anotações: %d", (int)mAnnot.items.size());
            ImGui::SameLine(0, 14);
            if (ImGui::Button("Exportar todas (.txt + print)##status_export"))
            {
                mAnnot.selected = -1;
                mAnnot.editedIndex = -1;
                ExportDirectives();
            }
            ImGui::SameLine(0, 14);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(Theme::TextSecondary, "captura global sem a janela de edição");
        }
        else
        {
            ImGui::AlignTextToFramePadding();
            if (!mStatusMsg.empty() && GetTime() < mStatusMsgUntil)
                ImGui::TextColored(Theme::Success, "%.*s", 52, mStatusMsg.c_str());
            else
                ImGui::TextColored(Theme::TextSecondary, "Pronto");

            ImGui::SameLine(0, 16);
            ImGui::TextColored(Theme::TextSecondary, "● Modo Normal");
            ImGui::SameLine(0, 18);
            ImGui::TextColored(Theme::TextSecondary, "Zoom: %.0f%%", mCanvasZoom * 100.0f);
            ImGui::SameLine(0, 18);
            ImGui::TextColored(Theme::TextSecondary, "1280×720");

            const ImGuiIO& io = ImGui::GetIO();
            ImGui::SameLine(0, 18);
            if (fabsf(io.MousePos.x) > 1.0e30f || fabsf(io.MousePos.y) > 1.0e30f)
                ImGui::TextColored(Theme::TextSecondary, "Mouse: (—, —)");
            else
                ImGui::TextColored(Theme::TextSecondary, "Mouse: (%.0f, %.0f)",
                                   io.MousePos.x, io.MousePos.y);
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
