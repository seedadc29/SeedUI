#include "MenuScene.h"
#include "Scene.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>
#include <cstdio>

namespace game
{
    static const char *kGameTitle = "ELDORIA";
    static const char *kGameSubtitle = "Um RPG de mundo aberto";
    static const char *kCreditsAuthor = "Leonardo Seedorf";
    static const char *kBuildVersion = "0.3.0";

#ifndef GAME_BUILD_MODE
#define GAME_BUILD_MODE Unknown
#endif

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

    static const char *BuildModeString() { return STRINGIFY(GAME_BUILD_MODE); }

    MenuScene::MenuScene(Settings &settings) : Scene(settings)
    {
    }

    MenuScene::~MenuScene() = default;

    void MenuScene::Init()
    {
        EnableCursor();
        mShowSettings = false;
        mOrbitAngle = 0.0f;
        mOrbitCamera.target = { 0, 1.4f, 0 };
        mOrbitCamera.up = { 0, 1, 0 };
        mOrbitCamera.fovy = 50.0f;
        mOrbitCamera.projection = CAMERA_PERSPECTIVE;
    }

    void MenuScene::Update(float deltaTime)
    {
        mOrbitAngle += deltaTime * 0.3f;
        mOrbitCamera.position = { cosf(mOrbitAngle) * mOrbitRadius, 6.0f, sinf(mOrbitAngle) * mOrbitRadius };
        mOrbitCamera.target = { 0, 1.4f, 0 };
    }

    void MenuScene::Draw()
    {
        BeginMode3D(mOrbitCamera);
            DrawGrid(24, 1.0f);
            DrawCube({ 0, 0.75f, 0 }, 4.0f, 1.5f, 4.0f, { 46, 51, 64, 255 });
            DrawCubeWires({ 0, 0.75f, 0 }, 4.0f, 1.5f, 4.0f, { 120, 130, 150, 200 });
            DrawCube({ 0, 2.25f, 0 }, 1.6f, 1.6f, 1.6f, RED);
            DrawCubeWires({ 0, 2.25f, 0 }, 1.6f, 1.6f, 1.6f, MAROON);
        EndMode3D();

        if (mShowSettings)
        {
            if (mSettingsScreen.Draw(mSettings))
            {
                SaveSettings(mSettings);
                mShowSettings = false;
            }
            return;
        }

        DrawMenu();
    }

    void MenuScene::DrawMenu()
    {
        float scale = ui::UiScale();
        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();

        ui::DrawTextCentered(kGameTitle, sw * 0.5f, sh * 0.10f, (int)(60 * scale), ui::TextPrimary());
        ui::DrawTextCentered(kGameSubtitle, sw * 0.5f, sh * 0.10f + 78 * scale, (int)(20 * scale), ui::TextDim());

        float btnW = 270 * scale;
        float btnH = 54 * scale;
        float gap = 18 * scale;
        float bx = (sw - btnW) * 0.5f;
        float by = sh * 0.42f;

        if (ui::DrawButton("Jogar", { bx, by, btnW, btnH })) SetRequest(SceneRequest::Play);
        by += btnH + gap;
        if (ui::DrawButton("Configurações", { bx, by, btnW, btnH })) { mShowSettings = true; }
        by += btnH + gap;
        if (ui::DrawButton("Sair", { bx, by, btnW, btnH })) SetRequest(SceneRequest::Quit);

        DrawCredits();
    }

    void MenuScene::DrawCredits()
    {
        float scale = ui::UiScale();
        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();
        int fs = (int)(15 * scale);

        ui::DrawText(kCreditsAuthor, { 20, sh - 34 * scale }, fs, ui::TextDim());

        char build[64];
        snprintf(build, sizeof build, "Build %s (%s)", kBuildVersion, BuildModeString());
        float bw = MeasureTextEx(GetFontDefault(), build, (float)fs, 1.0f).x;
        ui::DrawText(build, { sw - bw - 20, sh - 34 * scale }, fs, ui::TextDim());
    }
}
