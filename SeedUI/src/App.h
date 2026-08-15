#ifndef SEEDUI_APP_H
#define SEEDUI_APP_H

#include "Annotations.h"
#include "Icons.h"
#include "Project.h"
#include "SmartGuides.h"

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
        Rectangle,
        Ellipse,
        Polygon,
        Line,
        Pen,
        Measure,
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

        // Gera o SVG de um modo (testável sem UI — usado pelo autoteste).
        static std::string GerarSVG(const Project& projeto, const Modo& modo);

    private:
        void DrawWorkspace();
        void DrawMenuBar();
        void DrawActionBar();
        void DrawPropertyBar(); // barra contextual (CorelDRAW): X/Y/L/A, unidade, zoom
        void DrawToolbar();
        void DrawRightPanels();
        void DrawRightRail();
        void DrawStatusBar();
        void DrawColorBar();
        void DrawColorPickerWindow();
        void DrawStartScreen();
        void DrawNovoProjetoDialog();
        void DrawManualWindow();
        void DrawAboutWindow();
        void ToolButton(IconId id, const char* tip, ImU32 familyColor);
        void DrawToolFamilySeparator(ImU32 familyColor);
        void ExportDirectives();
        void DoExportDirectives();
        void ZoomCanvas(float factor); // zoom com encaminhamento ao cursor
        void CriarRetanguloTelaBase(); // duplo clique na ferramenta retângulo
        void MoverCamadaSelecionada(int delta); // Ctrl+setas (CorelDRAW)
        void NovoProjeto();
        void AbrirProjeto();
        void SalvarProjeto(bool salvarComo);
        void FecharProjeto();
        void MarcarTudoSalvo();
        bool PossuiModoAtivo() const;
        bool AdicionarComponente(const char* tipo, const char* nome,
                                 float projectX = -1.0f, float projectY = -1.0f);
        void CopiarElementosSelecionados();
        void RecortarElementosSelecionados();
        void ColarElementosCopiados();
        void DuplicarSelecao();
        void EspelharSelecao(bool horizontal);
        void ConverterEmCaminho(); // Ctrl+Q: forma -> caminho editável por nós
        void HandleMeasureTool(bool canvasHovered);
        void DesenharMedicao();
        void ZoomPara(float targetScale, float centerX, float centerY);
        void Zoom100();
        void ZoomFit();
        void ZoomFitSelection();
        void SelecionarTodos();
        void ExportarSVG();
        void HandlePenTool(bool canvasHovered);
        void AddPenPoint(Element& path, float projectX, float projectY,
                         float handleDX, float handleDY, bool curved);
        void RecalcularCaixaCaminho(Element& path);
        void ApagarElementosSelecionados();
        void AlternarModoAnotacao();
        void AlinharElementosSelecionados(int operacao);
        void DistribuirElementosSelecionados(bool horizontal);
        void AgruparElementosSelecionados();
        void DesagruparElementosSelecionados();
        void CriarPowerClipSelecao();
        void EntrarEdicaoPowerClip(const std::string& frameId = std::string());
        void SairEdicaoPowerClip();
        void ExtrairConteudoPowerClip();
        void AjustarConteudoPowerClip(int modo);
        void SelecionarConteudoPowerClip();
        void AdicionarConteudoPowerClip(bool substituir);
        void RemoverConteudoSelecionadoPowerClip();
        void RemoverPowerClip();
        std::string PowerClipContextFrameId();
        void ResetarHistorico();
        void CapturarHistorico();
        void Desfazer();
        void Refazer();
        void DrawElementTree(Element& element);
        void HandleCanvasInteraction(bool canvasHovered);
        void HandleGuidesInteraction(); // guias arrastadas das réguas
        void DesenharGuias();           // linhas das guias sobre o canvas
        void SnapGuias(float& dx, float& dy,
                       const std::vector<SmartGuides::Rect>& starts);
        float UnitToPixels() const;     // fator da unidade atual -> px
        float PixelsToUnit(float px) const;

    public:
        // Estrutura de duplicação e repetição de transformação estilo CorelDRAW (Ctrl+D / Ctrl+R)
        struct DuplicateTransformSnapshot
        {
            std::string id;
            float x = 0.0f;
            float y = 0.0f;
            float rot = 0.0f;
            float w = 160.0f;
            float h = 32.0f;
        };

        struct DuplicateSystem
        {
            float deltaX = 16.0f;
            float deltaY = 16.0f;
            float deltaRot = 0.0f;
            float factorW = 1.0f;
            float factorH = 1.0f;

            std::string lastDuplicatedId;
            float sourceX = 0.0f;
            float sourceY = 0.0f;
            float sourceRot = 0.0f;
            float sourceW = 160.0f;
            float sourceH = 32.0f;
            bool hasSourceSnapshot = false;

            void ResetToDefault()
            {
                deltaX = 16.0f;
                deltaY = 16.0f;
                deltaRot = 0.0f;
                factorW = 1.0f;
                factorH = 1.0f;
                lastDuplicatedId.clear();
                hasSourceSnapshot = false;
            }
        };

        DuplicateSystem mDup;
        void AtualizarDeltaDuplicacaoManual(const Element& el);

    private:

        float SnapTol(float basePx) const;
        const char* SnapStrengthLabel() const;
        void DrawSnapStrengthPopup();

        Project mProject;      // projeto em memória (vazio = tela inicial)
        bool mHasProject = false;
        bool mProjectDirty = false;
        int mTelaAtiva = 0;    // índice em mProject.telas
        int mModoAtivo = 0;    // índice em mProject.telas[mTelaAtiva].modos
        std::string mUltimoCaminho; // caminho do último salvar/abrir

        // Barra de propriedades contextual (estilo CorelDRAW)
        int mUnit = 0;        // 0 px, 1 mm, 2 cm, 3 in, 4 pt
        float mPrecision = 1.0f; // incremento dos campos numéricos
        float mNudgeDistance = 1.0f; // distância de deslocamento pelas setas (na unidade atual)

        // Guias fixas arrastadas das réguas (coordenadas de projeto; NÃO
        // entram no projeto.ui.json — são auxílio de edição).
        std::vector<float> mGuidesH; // guias horizontais (y)
        std::vector<float> mGuidesV; // guias verticais (x)
        int mGuideDragKind = 0;      // 0 nenhum; 1 cria H; 2 cria V; 3 arrasta H; 4 arrasta V
        int mGuideDragIndex = -1;
        bool mGuideSnapEngaged = false; // guia em arrasto encaixada (destaque visual)

        struct CanvasTransformStart
        {
            std::string id;
            float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
            float rot = 0.0f; // rotação no início do arraste (rotação em conjunto)
        };
        std::string mSelectedElementId;
        std::vector<std::string> mSelectedElementIds;
        std::string mAnchorElementId; // âncora de alinhamento (duplo clique)
        std::vector<CanvasTransformStart> mCanvasGroupStarts;
        bool mCanvasMarquee = false;
        bool mCanvasMarqueeAdditive = false;
        float mCanvasMarqueeStartX = 0.0f;
        float mCanvasMarqueeStartY = 0.0f;
        float mCanvasMarqueeEndX = 0.0f;
        float mCanvasMarqueeEndY = 0.0f;
        int mCanvasDragMode = 0; // 1 mover; 2..9 tamanho; 10..13 raio das quinas; 14 rotacao
        float mCanvasDragMouseX = 0.0f;
        float mCanvasDragMouseY = 0.0f;
        float mCanvasDragX = 0.0f;
        float mCanvasDragY = 0.0f;
        float mCanvasDragW = 0.0f;
        float mCanvasDragH = 0.0f;
        float mCanvasDragRotation = 0.0f;
        float mCanvasDragPivotX = 0.0f;
        float mCanvasDragPivotY = 0.0f;
        // Delta aplicado no FRAME ANTERIOR do arrasto — a previsão de
        // espaçamento o usa para detectar quando o cursor "pulou" por cima
        // de um alvo (arrasto rápido) e ainda assim engatar no alvo exato.
        float mCanvasPrevDX = 0.0f;
        float mCanvasPrevDY = 0.0f;
        float mCanvasCornerRadiusStarts[4] = { 0, 0, 0, 0 };
        unsigned int mSelectedCornerMask = 0;
        unsigned int mCanvasCornerDragMask = 0;
        std::string mCornerSelectionElementId;
        bool mCanvasDragChanged = false;
        // Guias inteligentes (M04): posição em coordenadas do projeto da guia
        // ativa durante o arraste (-1 = nenhuma). Desenhadas no canvas.
        float mGuideSnapX = -1.0f;
        float mGuideSnapY = -1.0f;
        // Guias FIXAS (das réguas) em que a seleção ENGATOU durante o arrasto
        // (-1 = nenhuma). Desenhadas em destaque para dar o feedback visual
        // do encaixe forma->guia.
        float mGuideFixedSnapX = -1.0f;
        float mGuideFixedSnapY = -1.0f;
        // Guias de espaçamento (M04): duas linhas delimitando um espaço
        // repetido entre vizinhos (par de posições; -1 = nenhuma).
        float mGuideSpacingX1 = -1.0f;
        float mGuideSpacingX2 = -1.0f;
        float mGuideSpacingY1 = -1.0f;
        float mGuideSpacingY2 = -1.0f;
        // Preview de espaçamento com Shift (estilo CorelDRAW): linhas-guia
        // acima/abaixo da fileira e ticks entre cada peça, durante o mover.
        std::vector<SmartGuides::GuideLine> mShiftGuides;
        std::vector<SmartGuides::GuideLabel> mShiftGuidesLabels;
        // Clone com o botão direito (estilo CorelDRAW): pressionar sobre um
        // elemento e arrastar cria uma cópia e move a cópia.
        bool mRightDragArmed = false;
        std::string mRightDragElementId;
        bool mCloneDragging = false;
        float mRightDragStartX = 0.0f;
        float mRightDragStartY = 0.0f;
        DuplicateTransformSnapshot mRightDragSourceA;
        // Clone durante o arrasto (fork): mover com o botão esquerdo e, no
        // meio do arrasto, apertar o direito faz o ORIGINAL voltar ao ponto
        // de partida e o CLONE continuar seguindo o cursor até soltar.
        bool mCloneForked = false;
        DuplicateTransformSnapshot mCloneForkSourceA;
        // Limiar de arrasto: o objeto fica FIXO ao clicar e só se move depois
        // que o mouse ultrapassa ~4px de tela (evita "arrasto acidental" no
        // clique). Trava depois de cruzado (não volta atrás no mesmo arrasto).
        bool mCanvasDragPastThreshold = false;
        std::vector<Element> mElementClipboard;
        int mElementPasteGeneration = 0;
        bool mElementClipboardFromCut = false;
        // Ferramenta Medir: medição transitória (não entra no JSON).
        bool mMeasureDragging = false;
        float mMeasureX1 = 0.0f, mMeasureY1 = 0.0f;
        float mMeasureX2 = 0.0f, mMeasureY2 = 0.0f;
        // Caneta (caminho Bezier): desenho, rubber banding e edição de nós.
        bool mPenDrawing = false;
        bool mPenDragging = false;
        float mPenDragStartX = 0.0f, mPenDragStartY = 0.0f;
        float mPenDragX = 0.0f, mPenDragY = 0.0f;
        float mPenMouseProjectX = 0.0f, mPenMouseProjectY = 0.0f;
        int mPenHoverSegmentIndex = -1;
        float mPenHoverT = 0.0f;
        float mPenHoverProjX = 0.0f, mPenHoverProjY = 0.0f;
        bool mPenSnapActive = false;
        float mPenSnapProjX = 0.0f;
        float mPenSnapProjY = 0.0f;
        bool mPenSnapIsClose = false;
        bool mPenPreview = true;
        bool mPenAutoAddDelete = true;
        float mPenConstrainAngle = 15.0f;
        std::string mPowerClipEditFrameId;
        // Moldura do filho escolhido por Ctrl+clique fora do ambiente interno.
        // Mantém o recorte ativo e permite voltar/entrar pelo botão no canvas.
        std::string mPowerClipDirectFrameId;
        int mPathEditIndex = -1;  // nó em edição (dragMode 16 = nó, 17 = alça)
        int mAlignTarget = 0; // 0 selecao, 1 elemento principal, 2 tela
        float mHorizontalSpacing = 16.0f;
        float mVerticalSpacing = 16.0f;
        bool mSnapEnabled = true;
        float mSnapStrength = 1.0f; // força do snap (0.0x a 3x; 0 = desligado)
        bool mRulersVisible = true;
        bool mRulersLocked = false; // bloqueio da régua (proteção contra edição)
        bool mGridVisible = true;   // ocultar a grade NÃO desliga o snap
        bool mGuidesVisible = true; // ocultar as linhas guia NÃO desliga o snap
        bool mWireframeMode = false; // modo wireframe (só contornos)
        bool mZoomToMouse = true; // zoom encaminha para o cursor (qualquer controle)
        bool mMarqueeContainOnly = true; // seleção exige cobertura TOTAL do elemento
        bool mColorPickerOpen = false; // janela do seletor de cor (3 modelos)
        int mColorPickerTarget = 0;    // 0 = cor_fundo (preenchimento), 1 = cor_borda (contorno)
        float mCanvasZoom = 1.0f;
        float mCanvasPanX = 0.0f;
        float mCanvasPanY = 0.0f;
        bool mShapeCreating = false;
        float mShapeStartX = 0.0f;
        float mShapeStartY = 0.0f;
        float mShapeEndX = 0.0f;
        float mShapeEndY = 0.0f;
        std::vector<std::string> mHistory;
        int mHistoryIndex = -1;
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
