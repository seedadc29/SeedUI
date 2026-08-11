#include "App.h"
#include "Scene.h"
#include "MenuScene.h"
#include "GameScene.h"

#include "raylib.h"
#if !defined(NDEBUG)
#include "imgui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

namespace game
{
    App::App() = default;
    App::~App() { Shutdown(); }

    void App::InitWindowAndContext()
    {
        unsigned int flags = FLAG_MSAA_4X_HINT;
        if (mSettings.vsync) flags |= FLAG_VSYNC_HINT;

        SetConfigFlags(flags);
        InitWindow(mSettings.windowWidth, mSettings.windowHeight, "Eldoria");
        SetExitKey(0);
        SetTargetFPS(60);
        SetTextureFilter(GetFontDefault().texture, TEXTURE_FILTER_BILINEAR);

        if (mSettings.fullscreen) ToggleFullscreen();
        mAppliedSettings = mSettings;
    }

    void App::Run()
    {
        LoadSettings(mSettings);
        InitWindowAndContext();

#if !defined(NDEBUG)
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        // GetWindowHandle() returns HWND on Windows. The ImGui GLFW backend
        // requires GLFW's internal window pointer instead.
        ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
        ImGui_ImplOpenGL3_Init("#version 330");
        mImGuiInitialized = true;
#endif

        mMenuScene = new MenuScene(mSettings);
        mGameScene = new GameScene(mSettings);
#if defined(NDEBUG)
        mActiveScene = mMenuScene;
#else
        // Debug and Development builds skip the front-end for faster iteration.
        // Release remains the player-facing build and starts at the main menu.
        mActiveScene = mGameScene;
#endif
        mActiveScene->Init();

        while (!WindowShouldClose() && !mShouldClose)
        {
            const float dt = GetFrameTime();

#if !defined(NDEBUG)
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
#endif

            mActiveScene->Update(dt);
            HandleSceneRequest();
            ApplySettingsChanges();

            BeginDrawing();
            ClearBackground({ 14, 16, 22, 255 });
            mActiveScene->Draw();
#if !defined(NDEBUG)
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
            EndDrawing();
        }

        Shutdown();
    }

    void App::HandleSceneRequest()
    {
        SceneRequest request = mActiveScene ? mActiveScene->ConsumeRequest() : SceneRequest::None;

        switch (request)
        {
        case SceneRequest::Play:
            mGameScene->Shutdown();
            mGameScene->Init();
            mActiveScene = mGameScene;
            break;
        case SceneRequest::MainMenu:
            mMenuScene->Init();
            mActiveScene = mMenuScene;
            break;
        case SceneRequest::Quit:
            mShouldClose = true;
            break;
        case SceneRequest::None:
        default:
            break;
        }
    }

    void App::ApplySettingsChanges()
    {
        const Settings &s = mSettings;

        if (s.windowWidth != mAppliedSettings.windowWidth ||
            s.windowHeight != mAppliedSettings.windowHeight)
        {
            SetWindowSize(s.windowWidth, s.windowHeight);
        }

        if (s.fullscreen != mAppliedSettings.fullscreen)
        {
            ToggleFullscreen();
        }

        if (s.vsync != mAppliedSettings.vsync)
        {
            if (s.vsync) SetWindowState(FLAG_VSYNC_HINT);
            else ClearWindowState(FLAG_VSYNC_HINT);
        }

        if (s.shadowQuality != mAppliedSettings.shadowQuality ||
            s.mouseSensitivity != mAppliedSettings.mouseSensitivity ||
            s.characterGender != mAppliedSettings.characterGender ||
            s.skinHue != mAppliedSettings.skinHue ||
            s.skinSaturation != mAppliedSettings.skinSaturation ||
            s.characterLightIntensity != mAppliedSettings.characterLightIntensity)
        {
            if (mGameScene) mGameScene->ApplySettings();
        }

        mAppliedSettings = mSettings;
    }

    void App::Shutdown()
    {
        if (mMenuScene)
        {
            mMenuScene->Shutdown();
            delete mMenuScene;
            mMenuScene = nullptr;
        }
        if (mGameScene)
        {
            mGameScene->Shutdown();
            delete mGameScene;
            mGameScene = nullptr;
        }
        mActiveScene = nullptr;

#if !defined(NDEBUG)
        if (mImGuiInitialized)
        {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            mImGuiInitialized = false;
        }
#endif

        if (IsWindowReady())
            CloseWindow();
    }
}
