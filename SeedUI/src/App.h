#ifndef SEEDUI_APP_H
#define SEEDUI_APP_H

#include "Annotations.h"
#include "Icons.h"
#include "Project.h"

namespace seedui
{
    enum class Tool
    {
        Select,
        Move,
        Text,
        Zoom,
        Pan,
        Color,
        Grid,
        Annotate,
    };

    class App
    {
    public:
        App() = default;
        ~App() = default;

        App(const App&) = delete;
        App& operator=(const App&) = delete;

        void Run(bool captureAfterBoot = false);

    private:
        void DrawWorkspace();
        void DrawMenuBar();
        void DrawActionBar();
        void DrawToolbar();
        void DrawRightPanels();
        void DrawRightRail();
        void DrawStatusBar();
        void DrawStartScreen();
        void DrawNovoProjetoDialog();
        void DrawManualWindow();
        void DrawAboutWindow();
        void ToolButton(IconId id, const char* tip);
        void ExportDirectives();
        void DoExportDirectives();
        void NovoProjeto();
        void AbrirProjeto();
        void SalvarProjeto(bool salvarComo);
        void FecharProjeto();
        void MarcarTudoSalvo();
        void AdicionarComponente(const char* tipo, const char* nome);
        void DrawElementTree(Element& element);

        Project mProject;      // projeto em memória (vazio = tela inicial)
        bool mHasProject = false;
        bool mProjectDirty = false;
        int mTelaAtiva = 0;    // índice em mProject.telas
        int mModoAtivo = 0;    // índice em mProject.telas[mTelaAtiva].modos
        std::string mUltimoCaminho; // caminho do último salvar/abrir

        std::string mSelectedElementId;
        float mRightPanelWidth = 320.0f;
        bool mRightPanelCollapsed = false;

        bool mShowNovoProjeto = false;
        char mNovoNome[128] = { 0 };
        char mNovoDescricao[512] = { 0 };
        int mNovoLargura = 1280;
        int mNovoAltura = 720;

        bool mRunning = true;
        bool mCaptureAfterBoot = false;
        int mFrameCount = 0;
        bool mShowManual = false;
        bool mShowAbout = false;
        Tool mCurrentTool = Tool::Select;

        // Modo debug: anotações para direcionar alterações à IA
        AnnotationState mAnnot;
        std::string mGlobalDirectives;
        char mGlobalBuf[8192] = { 0 };
        bool mGlobalBufLoaded = false;
        bool mPendingExport = false;
        std::string mStatusMsg;
        double mStatusMsgUntil = 0.0;
    };
}

#endif // SEEDUI_APP_H
