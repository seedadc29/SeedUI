#ifndef MENUSCENE_H
#define MENUSCENE_H

#include "Scene.h"
#include "UI/SettingsScreen.h"

#include "raylib.h"

namespace game
{
    class MenuScene : public Scene
    {
    public:
        MenuScene(Settings &settings);
        ~MenuScene() override;

        void Init() override;
        void Update(float deltaTime) override;
        void Draw() override;
        void DrawWorld() override;
        void DrawOverlay() override;

    private:
        void DrawMenu();
        void DrawCredits();

        Camera mOrbitCamera = {};
        float mOrbitAngle = 0.0f;
        float mOrbitRadius = 11.0f;
        bool mShowSettings = false;
        SettingsScreen mSettingsScreen;
    };
}

#endif // MENUSCENE_H
