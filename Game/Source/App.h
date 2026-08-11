#ifndef APP_H
#define APP_H

#include "UI/Settings.h"

namespace game
{
    class Scene;
    class GameScene;

    class App
    {
    public:
        App();
        ~App();

        App(const App &) = delete;
        App &operator=(const App &) = delete;

        void Run();

    private:
        void InitWindowAndContext();
        void HandleSceneRequest();
        void ApplySettingsChanges();
        void Shutdown();

        Settings mSettings;
        Settings mAppliedSettings;
        Scene *mMenuScene = nullptr;
        GameScene *mGameScene = nullptr;
        Scene *mActiveScene = nullptr;
        bool mShouldClose = false;
#if !defined(NDEBUG)
        bool mImGuiInitialized = false;
#endif
    };
}

#endif // APP_H
