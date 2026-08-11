// Não incluímos <windows.h> inteiro: nomes dele (CloseWindow, ShowCursor, DrawText...)
// colidem com funções do raylib. Declaramos apenas o que realmente usamos.
#pragma comment(lib, "Shell32.lib")
extern "C"
{
    // HINSTANCE é um ponteiro; retornamos void* (o valor não é usado)
    __declspec(dllimport) void* ShellExecuteA(void* hwnd, const char* operation,
                                              const char* file, const char* parameters,
                                              const char* directory, int showCmd);
}

#include "App.h"

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
        constexpr float kRightPanelWidth = 320.0f;
        constexpr float kStatusBarHeight = 26.0f;
        constexpr float kToolButtonSize = 34.0f;

        void MenuItemSoon(const char* label, const char* milestone)
        {
            ImGui::MenuItem(label, milestone, false, false);
        }

        // Cabeçalho de painel no estilo Blender (faixa de título)
        bool PanelBegin(const char* title)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, Theme::PanelHeader);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Theme::BorderLight);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, Theme::PanelHeader);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 7));
            const bool open = ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen);
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
        mStatusMsg = "Diretrizes exportadas (pasta diretrizes/ aberta)";
        mStatusMsgUntil = GetTime() + 8.0;

        // Abre a pasta para o usuário ver o .txt e o print (SW_SHOWDEFAULT = 10)
        ShellExecuteA(nullptr, "open", dir.c_str(), nullptr, nullptr, 10);
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
                const float availY = ImGui::GetContentRegionAvail().y - kStatusBarHeight;

                ImGui::BeginChild("##toolbar", ImVec2(kToolbarWidth, availY), false,
                                  ImGuiWindowFlags_NoScrollbar);
                DrawToolbar();
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("##canvas", ImVec2(-kRightPanelWidth, availY), true);
                CanvasDraw();
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("##panels", ImVec2(kRightPanelWidth, availY), false);
                DrawRightPanels();
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
                    AnnotationsPopupRectFor(mAnnot.items[mAnnot.selected].label, vp->Size);
                popupMin = pr.min;
                popupMax = pr.max;
                clipPopup = true;
            }
            AnnotationsDraw(mAnnot, clipPopup, popupMin, popupMax);
        }

        // Popup de edição da anotação selecionada (pode pedir exportação)
        if (mHasProject)
        {
            const int action = AnnotationsEditWindow(mAnnot);
            if (action == AnnotationsEdit_Export) ExportDirectives();
        }

        if (mShowManual) DrawManualWindow();
        if (mShowAbout) DrawAboutWindow();
    }

    void App::DrawMenuBar()
    {
        if (!ImGui::BeginMenuBar()) return;

        if (ImGui::BeginMenu("Arquivo"))
        {
            if (mHasProject && ImGui::MenuItem("Fechar projeto (voltar à tela inicial)"))
                mHasProject = false;
            MenuItemSoon("Novo projeto", "M03");
            MenuItemSoon("Novo a partir de modelo", "M09");
            MenuItemSoon("Abrir projeto", "M03");
            ImGui::Separator();
            MenuItemSoon("Salvar", "M03");
            MenuItemSoon("Salvar como", "M03");
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
            MenuItemSoon("Painel", "M04");
            MenuItemSoon("Janela", "M04");
            MenuItemSoon("Texto", "M04");
            MenuItemSoon("Botão", "M04");
            MenuItemSoon("Campo numérico", "M04");
            MenuItemSoon("Slider", "M04");
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

        // Seletor de tela/modo (estilo Blender) — funcional a partir do M03/M10
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 310.0f);
        ImGui::BeginDisabled();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::Combo("##tela", &mToolComboDummy, "Tela: — (M03)\0\0");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::Combo("##modo", &mModeComboDummy, "Modo: — (M03)\0\0");
        ImGui::EndDisabled();

        ImGui::EndMenuBar();
    }

    void App::DrawToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
        ImGui::Spacing();

        ToolButton(IconId::Select, "Selecionar (V)");
        ToolButton(IconId::Move, "Mover (M)");
        ToolButton(IconId::Text, "Texto (T)");
        ToolButton(IconId::ZoomIn, "Zoom (+)");
        ToolButton(IconId::Pan, "Mão / navegar (H)");
        ToolButton(IconId::Color, "Conta-gotas / cor (I)");
        ToolButton(IconId::Grid, "Grade (G)");
        ToolButton(IconId::Annotate,
                   "Anotar (modo debug) — marque a área e escreva a alteração para a IA (A)");

        ImGui::Separator();
        ImGui::Spacing();

        IconButton(IconId::New, "Novo projeto (M03)", kToolButtonSize);
        IconButton(IconId::Open, "Abrir projeto (M03)", kToolButtonSize);
        IconButton(IconId::Save, "Salvar (M03)", kToolButtonSize);

        ImGui::Separator();
        ImGui::Spacing();

        IconButton(IconId::Undo, "Desfazer (M05)", kToolButtonSize);
        IconButton(IconId::Redo, "Refazer (M05)", kToolButtonSize);
        IconButton(IconId::Copy, "Copiar (M05)", kToolButtonSize);
        IconButton(IconId::Paste, "Colar (M05)", kToolButtonSize);
        IconButton(IconId::Trash, "Apagar (M05)", kToolButtonSize);

        ImGui::Separator();
        ImGui::Spacing();

        IconButton(IconId::Model, "Galeria de modelos (M09)", kToolButtonSize);
        IconButton(IconId::Download, "Exportar pacote (M13)", kToolButtonSize);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (IconButton(IconId::Question, "Manual (F1)", kToolButtonSize)) mShowManual = true;
        IconButton(IconId::Gear, "Preferências (M14)", kToolButtonSize);

        ImGui::PopStyleVar();
    }

    void App::ToolButton(IconId id, const char* tip)
    {
        const bool active = (mCurrentTool == ToolFromIcon(id));
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, Theme::Hex(0x4f8cff, 0.30f));
        if (IconButton(id, tip, kToolButtonSize)) mCurrentTool = ToolFromIcon(id);
        if (active) ImGui::PopStyleColor();
    }

    void App::DrawRightPanels()
    {
        if (PanelBegin("HIERARQUIA"))
        {
            PanelHint("Nenhum projeto aberto. Crie um projeto (Arquivo → Novo, M03) ou use um modelo (M09).");
        }

        if (PanelBegin("INSPETOR"))
        {
            PanelHint("Selecione um elemento no canvas para editar posição, tamanho, cores e estados (M06).");
        }

        if (PanelBegin("BIBLIOTECA"))
        {
            static const char* kComponents[] = {
                "Painel", "Janela", "Caixa", "Grupo", "Texto", "Título",
                "Botão", "Botão com ícone", "Alternância", "Caixa de seleção",
                "Campo de texto", "Campo numérico", "Campo de senha", "Área de texto",
                "Lista", "Lista suspensa", "Slider", "Slider com valor",
                "Barra de progresso", "Indicador circular", "Seletor de cor",
                "Separador", "Barra de rolagem", "Tooltip", "Menu de contexto",
                "Janela modal", "Diálogo", "Notificação", "Inspetor", "Viewport",
            };

            ImGui::BeginDisabled();
            for (const char* name : kComponents)
            {
                ImGui::Selectable(name);
            }
            ImGui::EndDisabled();

            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDisabled);
            ImGui::TextWrapped("Arraste para o canvas a partir do M04.");
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        if (PanelBegin("DIRETRIZES"))
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
            if (ImGui::Button("Exportar diretrizes (.txt + print)", ImVec2(-1.0f, 0.0f)))
                ExportDirectives();
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDisabled);
            ImGui::TextWrapped("Use a ferramenta \"Anotar\" (A) para marcar QUALQUER parte do SeedUI — menus, ícones, painéis ou canvas. Depois exporte as diretrizes: você só precisa dizer aqui no chat \"veja as novas alterações\".");
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        if (PanelBegin("RECURSOS"))
        {
            PanelHint("Importe SVGs, imagens e fontes. Gerenciador completo no M08.");
        }

        if (PanelBegin("HISTÓRICO"))
        {
            PanelHint("Desfazer/refazer com histórico chega no M05.");
        }
    }

    void App::DrawStatusBar()
    {
        ImGui::Separator();
        ImGui::BeginChild("##status", ImVec2(0, kStatusBarHeight), false, ImGuiWindowFlags_NoScrollbar);

        if (!mStatusMsg.empty() && GetTime() < mStatusMsgUntil)
        {
            ImGui::TextColored(Theme::Success, "%.*s", 60, mStatusMsg.c_str());
        }
        else
        {
            ImGui::TextColored(Theme::TextSecondary, "Pronto");
        }
        ImGui::SameLine(0, 16);
        if (mCurrentTool == Tool::Annotate)
        {
            ImGui::TextColored(Theme::AccentOrange,
                               "● Modo Debug: Anotações (%d)", (int)mAnnot.items.size());
        }
        else
        {
            ImGui::TextColored(Theme::TextSecondary, "● Modo Normal");
        }
        ImGui::SameLine(0, 20);
        ImGui::TextColored(Theme::TextSecondary, "Zoom: 100%%");
        ImGui::SameLine(0, 18);
        ImGui::TextColored(Theme::TextSecondary, "Tela base: 1280x720");

        const ImGuiIO& io = ImGui::GetIO();
        ImGui::SameLine(0, 18);
        if (fabsf(io.MousePos.x) > 1.0e30f || fabsf(io.MousePos.y) > 1.0e30f)
        {
            ImGui::TextColored(Theme::TextSecondary, "Mouse: (—, —)");
        }
        else
        {
            ImGui::TextColored(Theme::TextSecondary, "Mouse: (%.0f, %.0f)", io.MousePos.x, io.MousePos.y);
        }

        // "Sem alterações não salvas" só quando houver espaço de verdade
        // (janelas estreitas / mensagem de status longa -> esconde para não sobrepor)
        const char* savedLabel = "Sem alterações não salvas";
        const float needW = ImGui::CalcTextSize(savedLabel).x + 12.0f;
        if (ImGui::GetCursorPosX() + needW <= ImGui::GetWindowContentRegionMax().x)
        {
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - needW);
            ImGui::TextColored(Theme::TextSecondary, "%s", savedLabel);
        }

        ImGui::EndChild();
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
        ImGui::BeginDisabled();
        if (ImGui::Button("Novo projeto", ImVec2(bw, 42))) {}
        ImGui::SameLine(0, gap);
        if (ImGui::Button("Abrir projeto", ImVec2(bw, 42))) {}
        ImGui::SameLine(0, gap);
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
        const char* foot = "Milestone 02 — esqueleto executável · Recursos de arquivo chegam no M03 (projetos) e M09 (modelos)";
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
        ImGui::TextColored(Theme::TextSecondary, "Versão 0.1 — Milestone 02 (esqueleto executável)");
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
