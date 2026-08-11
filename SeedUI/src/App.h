#ifndef SEEDUI_APP_H
#define SEEDUI_APP_H

#include "Annotations.h"
#include "Icons.h"

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
        void DrawToolbar();
        void DrawRightPanels();
        void DrawStatusBar();
        void DrawStartScreen();
        void DrawManualWindow();
        void DrawAboutWindow();
        void ToolButton(IconId id, const char* tip);
        void ExportDirectives();
        void DoExportDirectives();

        bool mRunning = true;
        bool mCaptureAfterBoot = false;
        int mFrameCount = 0;
        bool mShowManual = false;
        bool mShowAbout = false;
        bool mHasProject = false; // M03: projetos reais chegam no Milestone 03
        Tool mCurrentTool = Tool::Select;
        int mToolComboDummy = 0;
        int mModeComboDummy = 0;

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
