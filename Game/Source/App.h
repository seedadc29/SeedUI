#ifndef APP_H
#define APP_H

#include "UI/Settings.h"

namespace game
{
    enum class VisualProfile : unsigned char
    {
        Standard = 0,
        Bruma,
        CrtLow
    };

    class Scene;
    class GameScene;

    class App
    {
    public:
        explicit App(VisualProfile profile = VisualProfile::Standard);
        ~App();

        App(const App &) = delete;
        App &operator=(const App &) = delete;

        void Run();

    private:
        void InitWindowAndContext();
        void HandleSceneRequest();
        void ApplySettingsChanges();
        void InitRetroPresentation();
        void RecreateRetroTarget();
        void DrawRetroFrame();
        void Shutdown();

        Settings mSettings;
        Settings mAppliedSettings;
        Scene *mMenuScene = nullptr;
        GameScene *mGameScene = nullptr;
        Scene *mActiveScene = nullptr;
        bool mShouldClose = false;
        VisualProfile mVisualProfile = VisualProfile::Standard;
        struct RetroPresentation;
        RetroPresentation *mRetroPresentation = nullptr;
        float mRetroBadgeTime = 0.0f;
#if !defined(NDEBUG)
        bool mImGuiInitialized = false;
#endif
    };
}

#endif // APP_H
