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

#include <cfloat>
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

        Tool ToolFromIcon(IconId id)
        {
            switch (id)
            {
            case IconId::Select: return Tool::Select;
            case IconId::Move:   return Tool::Move;
            case IconId::Text:   return Tool::Text;
            case IconId::ZoomIn: return Tool::Zoom;
            case IconId::Pan:    return Tool::Pan;
            case IconId::Color:  return Tool::Color;
            case IconId::Grid:   return Tool::Grid;
            case IconId::Annotate: return Tool::Annotate;
            default:             return Tool::Select;
            }
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
                    mHasProject = true; // segunda captura mostra o workspace
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

    void App::AdicionarComponente(const char* tipo, const char* nome)
    {
        if (!mHasProject || mProject.telas.empty()) return;
        if (mTelaAtiva < 0 || mTelaAtiva >= (int)mProject.telas.size()) return;

        Tela& tela = mProject.telas[mTelaAtiva];
        if (tela.modos.empty()) return;
        if (mModoAtivo < 0 || mModoAtivo >= (int)tela.modos.size()) mModoAtivo = 0;

        Modo& modo = tela.modos[mModoAtivo];
        const std::string base = tipo ? tipo : "elemento";
        std::string id;
        for (int i = 1; i < 10000; ++i)
        {
            id = base + "_" + std::to_string(i);
            if (!Project::ResolverId(mProject, id)) break;
        }

        const int index = (int)modo.raiz.size();
        Element e;
        e.id = id;
        e.tipo = base;
        e.nome = nome ? nome : base.c_str();
        e.transformacao = {
            { "x", 80 + (index % 5) * 28 },
            { "y", 80 + (index % 7) * 24 },
            { "largura", 180 },
            { "altura", 42 }
        };

        if (base == "janela" || base == "painel")
        {
            e.transformacao["largura"] = 320;
            e.transformacao["altura"] = base == "janela" ? 220 : 160;
        }
        else if (base == "texto")
        {
            e.transformacao["largura"] = 220;
            e.transformacao["altura"] = 32;
            e.propriedades["texto"] = "Texto";
        }
        else if (base == "botao")
        {
            e.propriedades["texto"] = "Botao";
        }
        else if (base == "campo_numerico")
        {
            e.propriedades["valor"] = 0;
        }
        else if (base == "slider")
        {
            e.propriedades["valor"] = 50;
            e.propriedades["min"] = 0;
            e.propriedades["max"] = 100;
        }

        modo.raiz.push_back(std::move(e));
        mSelectedElementId = id;
        mProjectDirty = true;
        mStatusMsg = "Componente adicionado: " + id;
        mStatusMsgUntil = GetTime() + 5.0;
    }

    void App::DrawElementTree(Element& element)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;
        if (element.filhos.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
        if (element.id == mSelectedElementId) flags |= ImGuiTreeNodeFlags_Selected;

        const std::string label = element.nome.empty()
            ? element.id + "##" + element.id
            : element.nome + "##" + element.id;

        const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
        if (ImGui::IsItemClicked())
            mSelectedElementId = element.id;

        if (ImGui::BeginPopupContextItem())
        {
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
            ImGui::EndPopup();
        }

        if (open)
        {
            ImGui::TextColored(Theme::TextDisabled, "%s", element.id.c_str());
            for (Element& child : element.filhos)
                DrawElementTree(child);
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
        if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_A, false))
            mCurrentTool = Tool::Annotate;
        if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_V, false))
            mCurrentTool = Tool::Select;
        if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift &&
            ImGui::IsKeyPressed(ImGuiKey_C, false))
        {
            ImGui::SetClipboardText(AnnotationsToText(mAnnot, mGlobalDirectives).c_str());
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
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;

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
                                  ImGuiWindowFlags_NoScrollbar);
                DrawToolbar();
                ImGui::EndChild();

                ImGui::SameLine();

                const float rightWidth = mRightPanelCollapsed ? kRightRailWidth : mRightPanelWidth;
                ImGui::BeginChild("##canvas", ImVec2(-(rightWidth + 6.0f), availY), true);
                if (mHasProject)
                    CanvasDraw(&mProject, mTelaAtiva, mModoAtivo);
                else
                    CanvasDraw(nullptr);
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
                AnnotationsUpdate(mAnnot, mCurrentTool == Tool::Annotate, vp->Size);
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
            MenuItemSoon("Desfazer", "M05");
            MenuItemSoon("Refazer", "M05");
            ImGui::Separator();
            MenuItemSoon("Copiar", "M05");
            MenuItemSoon("Colar", "M05");
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
            MenuItemSoon("Guias", "M05");
            MenuItemSoon("Grade", "M05");
            ImGui::Separator();
            MenuItemSoon("Workspace", "M10");
            MenuItemSoon("Tema claro/escuro", "M14");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Inserir"))
        {
            if (!mHasProject) ImGui::BeginDisabled();
            if (ImGui::MenuItem("Painel")) AdicionarComponente("painel", "Painel");
            if (ImGui::MenuItem("Janela")) AdicionarComponente("janela", "Janela");
            if (ImGui::MenuItem("Texto")) AdicionarComponente("texto", "Texto");
            if (ImGui::MenuItem("Botao")) AdicionarComponente("botao", "Botao");
            if (ImGui::MenuItem("Campo numerico")) AdicionarComponente("campo_numerico", "Campo numerico");
            if (ImGui::MenuItem("Slider")) AdicionarComponente("slider", "Slider");
            if (!mHasProject) ImGui::EndDisabled();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Objeto"))
        {
            MenuItemSoon("Alinhar", "M05");
            MenuItemSoon("Distribuir", "M05");
            MenuItemSoon("Agrupar", "M04");
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
        IconButton(IconId::Undo, "Histórico · Desfazer (M05)", button);
        ImGui::SameLine();
        IconButton(IconId::Redo, "Histórico · Refazer (M05)", button);

        separator();
        IconButton(IconId::Copy, "Edição · Copiar (M05)", button);
        ImGui::SameLine();
        IconButton(IconId::Paste, "Edição · Colar (M05)", button);
        ImGui::SameLine();
        IconButton(IconId::Trash, "Edição · Apagar (M05)", button);

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
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
        ImGui::Spacing();

        ToolButton(IconId::Select, "Seleção · Selecionar (V)");
        ToolButton(IconId::Move, "Transformação · Mover (M)");

        ImGui::Separator();
        ImGui::Spacing();
        ToolButton(IconId::Text, "Criação · Texto (T)");
        ToolButton(IconId::Color, "Aparência · Conta-gotas (I)");

        ImGui::Separator();
        ImGui::Spacing();
        ToolButton(IconId::ZoomIn, "Navegação · Zoom (+)");
        ToolButton(IconId::Pan, "Navegação · Mão (H)");
        ToolButton(IconId::Grid, "Visualização · Grade (G)");

        ImGui::Separator();
        ImGui::Spacing();
        ToolButton(IconId::Annotate,
                   "Revisão · Anotar para a IA (A)");

        ImGui::PopStyleVar();
    }
    void App::ToolButton(IconId id, const char* tip)
    {
        // Centro geométrico da coluna: (50 - 32) / 2 = 9 px.
        ImGui::SetCursorPosX((kToolbarWidth - kToolButtonSize) * 0.5f);
        const bool active = (mCurrentTool == ToolFromIcon(id));
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x4f8cff, 0.30f));
        if (IconButton(id, tip, kToolButtonSize)) mCurrentTool = ToolFromIcon(id);
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
            if (!mHasProject)
            {
                PanelHint("Nenhum projeto aberto. Crie um projeto (Arquivo → Novo) ou use um modelo (M09).");
            }
            else
            {
                if (mProject.telas.empty())
                {
                    PanelHint("Projeto sem telas. A primeira tela vem com o modo \"Padrão\".");
                }
                else for (int ti = 0; ti < (int)mProject.telas.size(); ++ti)
                {
                    Tela& t = mProject.telas[ti];
                    ImGui::PushID(ti);
                    const bool isAtiva = (ti == mTelaAtiva);
                    if (isAtiva) ImGui::PushStyleColor(ImGuiCol_Header, Theme::Hex(0x4f8cff, 0.22f));
                    if (ImGui::TreeNodeEx(t.nome.c_str(),
                                          ImGuiTreeNodeFlags_DefaultOpen |
                                              ImGuiTreeNodeFlags_OpenOnArrow))
                    {
                        if (isAtiva)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextPrimary);
                            ImGui::TextColored(Theme::TextDisabled, "id: %s", t.id.c_str());
                            ImGui::PopStyleColor();
                        }
                        for (int mi = 0; mi < (int)t.modos.size(); ++mi)
                        {
                            Modo& mo = t.modos[mi];
                            const bool modoAtivo = (ti == mTelaAtiva && mi == mModoAtivo);
                            if (modoAtivo) ImGui::PushStyleColor(ImGuiCol_Text, Theme::AccentOrange);
                            if (ImGui::Selectable(("  " + mo.nome).c_str(), modoAtivo))
                            {
                                mTelaAtiva = ti;
                                mModoAtivo = mi;
                                mSelectedElementId.clear();
                            }
                            if (modoAtivo) ImGui::PopStyleColor();

                            if (modoAtivo)
                            {
                                if (mo.raiz.empty())
                                {
                                    ImGui::TextColored(Theme::TextDisabled,
                                                       "  Nenhum elemento neste modo.");
                                }
                                else
                                {
                                    ImGui::Indent(14.0f);
                                    for (Element& e : mo.raiz)
                                        DrawElementTree(e);
                                    ImGui::Unindent(14.0f);
                                }
                            }
                        }
                        ImGui::TreePop();
                    }
                    if (isAtiva) ImGui::PopStyleColor();
                    ImGui::PopID();
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
                ImGui::TextWrapped("%s", selected->nome.empty() ? selected->id.c_str() : selected->nome.c_str());
                ImGui::TextColored(Theme::TextDisabled, "id: %s", selected->id.c_str());
                ImGui::TextColored(Theme::TextDisabled, "tipo: %s", selected->tipo.c_str());

                const bool beforeVisible = selected->visivel;
                const bool beforeLocked = selected->bloqueado;
                ImGui::Checkbox("Visivel", &selected->visivel);
                ImGui::Checkbox("Bloqueado", &selected->bloqueado);
                if (beforeVisible != selected->visivel || beforeLocked != selected->bloqueado)
                    mProjectDirty = true;
                ImGui::Separator();
            }

            PanelHint("Selecione um elemento no canvas para editar posição, tamanho, cores e estados (M06).");
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
                if (!enabled) ImGui::EndDisabled();
                ImGui::PopID();
                return clicked && enabled;
            };

            if (mHasProject)
            {
                for (const ComponentButton& c : kBasicComponents)
                {
                    if (componentRow(c.tipo, c.nome, c.icon, true))
                        AdicionarComponente(c.tipo, c.nome);
                }
                ImGui::Separator();
            }

            ImGui::TextColored(Theme::TextSecondary, "Componentes planejados");
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

            ImGui::TextColored(Theme::TextDisabled,
                               "Clique para inserir. Arrastar para o canvas entra no M04.");
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
            ImGui::TextColored(Theme::TextSecondary, "Zoom: 100%%");
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
        if (ImGui::Button("Explorar o workspace (demo)", ImVec2(bw, 36))) mHasProject = true;

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
