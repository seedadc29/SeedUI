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
        bool PossuiModoAtivo() const;
        bool AdicionarComponente(const char* tipo, const char* nome,
                                 float projectX = -1.0f, float projectY = -1.0f);
        void CopiarElementosSelecionados();
        void ColarElementosCopiados();
        void AlternarModoAnotacao();
        void AlinharElementosSelecionados(int operacao);
        void DrawElementTree(Element& element);
        void HandleCanvasInteraction(bool canvasHovered);

        Project mProject;      // projeto em memória (vazio = tela inicial)
        bool mHasProject = false;
        bool mProjectDirty = false;
        int mTelaAtiva = 0;    // índice em mProject.telas
        int mModoAtivo = 0;    // índice em mProject.telas[mTelaAtiva].modos
        std::string mUltimoCaminho; // caminho do último salvar/abrir

        struct CanvasTransformStart
        {
            std::string id;
            float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
        };
        std::string mSelectedElementId;
        std::vector<std::string> mSelectedElementIds;
        std::vector<CanvasTransformStart> mCanvasGroupStarts;
        bool mCanvasMarquee = false;
        bool mCanvasMarqueeAdditive = false;
        float mCanvasMarqueeStartX = 0.0f;
        float mCanvasMarqueeStartY = 0.0f;
        float mCanvasMarqueeEndX = 0.0f;
        float mCanvasMarqueeEndY = 0.0f;
        int mCanvasDragMode = 0; // 1 mover; 2..9 tamanho; 10..13 raio das quinas
        float mCanvasDragMouseX = 0.0f;
        float mCanvasDragMouseY = 0.0f;
        float mCanvasDragX = 0.0f;
        float mCanvasDragY = 0.0f;
        float mCanvasDragW = 0.0f;
        float mCanvasDragH = 0.0f;
        float mCanvasCornerRadiusStarts[4] = { 0, 0, 0, 0 };
        unsigned int mSelectedCornerMask = 0;
        unsigned int mCanvasCornerDragMask = 0;
        std::string mCornerSelectionElementId;
        bool mCanvasDragChanged = false;
        std::vector<Element> mElementClipboard;
        int mElementPasteGeneration = 0;
        int mAlignTarget = 0; // 0 selecao, 1 elemento principal, 2 tela
        std::string mInspectorBufferedElementId;
        char mInspectorNameBuffer[128] = { 0 };
        char mInspectorIdBuffer[128] = { 0 };
        char mHierarchySearch[128] = { 0 };
        std::string mRenamingElementId;
        char mRenameBuffer[128] = { 0 };
        std::string mPendingDeleteElementId;
        std::string mPendingMoveElementId;
        int mPendingMoveDelta = 0;
        std::string mPendingReparentElementId;
        std::string mPendingReparentTargetId; // vazio = raiz do modo
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
        bool mSkipAnnotationInputThisFrame = false;

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
