#include "App.h"
#include "Scene.h"
#include "MenuScene.h"
#include "GameScene.h"

#include "raylib.h"
#include <algorithm>
#if !defined(NDEBUG)
#include "imgui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

namespace game
{
    struct App::RetroPresentation
    {
        RenderTexture2D target = {};
        Shader shader = {};
        int scanlinesLocation = -1;
        int scanlineStrengthLocation = -1;
        int scanlineSpacingLocation = -1;
        int aliasingStrengthLocation = -1;
        int synthLocations[21] = {};
    };

    namespace
    {
        constexpr int RetroWorldWidth = 960;
        constexpr int RetroWorldHeight = 540;

        const char *RetroFragmentShader = R"GLSL(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
out vec4 finalColor;

float SeedGrain(vec2 pixel)
{
    return fract(sin(dot(floor(pixel), vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec4 source = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec3 color = source.rgb;
    float light = dot(color, vec3(0.2126, 0.7152, 0.0722));

    // Assinatura Bruma: sombras frias, luz levemente dourada e cor contida.
    vec3 shadowTone = vec3(0.90, 0.96, 1.07);
    vec3 lightTone = vec3(1.06, 1.02, 0.92);
    color *= mix(shadowTone, lightTone, smoothstep(0.18, 0.82, light));
    color = mix(vec3(light), color, 1.08);

    // Quantizacao suave com grao estavel. Mantem leitura moderna sem imitar PS1.
    const float levels = 28.0;
    float grain = (SeedGrain(gl_FragCoord.xy) - 0.5) * 0.62 / levels;
    color = floor(clamp(color + grain, 0.0, 1.0) * levels + 0.5) / levels;

    vec2 centered = fragTexCoord * 2.0 - 1.0;
    float vignette = smoothstep(1.30, 0.28, dot(centered, centered));
    color *= mix(0.92, 1.0, vignette);
    finalColor = vec4(color, source.a);
}
)GLSL";

        const char *CrtFragmentShader = R"GLSL(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float scanlinesEnabled;
uniform float scanlineStrength;
uniform float scanlineSpacing;
uniform float aliasingStrength;
out vec4 finalColor;

void main()
{
    vec2 centered = fragTexCoord * 2.0 - 1.0;
    float radius = dot(centered, centered);
    vec2 uv = centered * (1.0 + radius * 0.055) * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0)
    {
        finalColor = vec4(0.006, 0.008, 0.012, 1.0);
        return;
    }

    vec2 texel = 1.0 / vec2(textureSize(texture0, 0));
    float split = mix(0.15, 1.15, aliasingStrength);
    vec3 color;
    color.r = texture(texture0, uv + vec2(texel.x * split, 0.0)).r;
    color.g = texture(texture0, uv).g;
    color.b = texture(texture0, uv - vec2(texel.x * split, 0.0)).b;

    vec3 neighborhood = (texture(texture0, uv + vec2(texel.x, 0.0)).rgb +
                         texture(texture0, uv - vec2(texel.x, 0.0)).rgb +
                         texture(texture0, uv + vec2(0.0, texel.y)).rgb +
                         texture(texture0, uv - vec2(0.0, texel.y)).rgb) * 0.25;
    color = mix(color, clamp(color + (color - neighborhood) * 0.85, 0.0, 1.0),
                aliasingStrength);

    vec3 reduced = floor(clamp(color, 0.0, 1.0) * 13.0 + 0.5) / 13.0;
    color = mix(color, reduced, 0.48 + aliasingStrength * 0.45);

    float spacing = max(1.0, scanlineSpacing);
    float lineWave = 0.5 + 0.5 * cos(6.2831853 * gl_FragCoord.y / spacing);
    float lineDarkening = scanlinesEnabled * scanlineStrength *
                          mix(0.12, 0.58, lineWave);
    color *= 1.0 - lineDarkening;

    // Mascara RGB de fosforo muito sutil, separada das scanlines.
    float phosphor = mod(floor(gl_FragCoord.x), 3.0);
    vec3 mask = phosphor < 1.0 ? vec3(1.0, 0.94, 0.94) :
                phosphor < 2.0 ? vec3(0.94, 1.0, 0.94) :
                                 vec3(0.94, 0.94, 1.0);
    color *= mix(vec3(1.0), mask, 0.34);
    float vignette = smoothstep(1.18, 0.22, radius);
    color *= mix(0.60, 1.0, vignette);
    finalColor = vec4(color, 1.0) * colDiffuse * fragColor;
}
)GLSL";

        enum SynthUniform
        {
            SynthQuantize = 0,
            SynthColorLevels,
            SynthMonochrome,
            SynthPaletteMode,
            SynthPaletteStrength,
            SynthDither,
            SynthEdges,
            SynthBinary,
            SynthBinaryStrength,
            SynthBinaryCell,
            SynthBinaryThreshold,
            SynthGrid,
            SynthGridStrength,
            SynthGridSpacing,
            SynthScanlines,
            SynthScanlineStrength,
            SynthScanlineSpacing,
            SynthCurvature,
            SynthRgbSplit,
            SynthVignette,
            SynthInvert,
            SynthUniformCount
        };

        const char *SynthUniformNames[SynthUniformCount] = {
            "quantizeStrength", "colorLevels", "monochromeStrength",
            "paletteMode", "paletteStrength", "ditherStrength",
            "edgeStrength", "binaryEnabled", "binaryStrength",
            "binaryCellSize", "binaryThreshold", "gridEnabled",
            "gridStrength", "gridSpacing", "scanlinesEnabled",
            "scanlineStrength", "scanlineSpacing", "curvatureStrength",
            "rgbSplitStrength", "vignetteStrength", "invertStrength"
        };

        const char *SynthFragmentShader = R"GLSL(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float quantizeStrength;
uniform float colorLevels;
uniform float monochromeStrength;
uniform float paletteMode;
uniform float paletteStrength;
uniform float ditherStrength;
uniform float edgeStrength;
uniform float binaryEnabled;
uniform float binaryStrength;
uniform float binaryCellSize;
uniform float binaryThreshold;
uniform float gridEnabled;
uniform float gridStrength;
uniform float gridSpacing;
uniform float scanlinesEnabled;
uniform float scanlineStrength;
uniform float scanlineSpacing;
uniform float curvatureStrength;
uniform float rgbSplitStrength;
uniform float vignetteStrength;
uniform float invertStrength;
out vec4 finalColor;

float Luma(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

float Hash(vec2 p)
{
    return fract(sin(dot(floor(p), vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 ApplyPalette(float value, float mode)
{
    if (mode < 0.5) return vec3(value);
    if (mode < 1.5) return vec3(value * 0.16, value, value * 0.34);       // terminal verde
    if (mode < 2.5) return vec3(value, value * 0.58, value * 0.10);       // ambar
    if (mode < 3.5) return mix(vec3(0.01, 0.04, 0.10), vec3(0.35, 0.90, 1.0), value); // blueprint
    if (mode < 4.5) return vec3(smoothstep(0.25, 0.75, value),
                                smoothstep(0.0, 0.55, value) * (1.0 - smoothstep(0.68, 1.0, value)),
                                1.0 - smoothstep(0.18, 0.62, value));    // diagnostico termico
    return mix(vec3(0.025, 0.055, 0.035), vec3(0.45, 1.0, 0.38), value); // data moss
}

float BinaryGlyph(vec2 p, float bit)
{
    float outerX = step(abs(p.x - 0.5), 0.30);
    float outerY = step(abs(p.y - 0.5), 0.40);
    float innerX = step(abs(p.x - 0.5), 0.14);
    float innerY = step(abs(p.y - 0.5), 0.25);
    float zero = outerX * outerY * (1.0 - innerX * innerY);
    float stem = step(abs(p.x - 0.52), 0.09) *
                 step(0.12, p.y) * step(p.y, 0.86);
    float foot = step(abs(p.x - 0.52), 0.23) *
                 step(0.76, p.y) * step(p.y, 0.88);
    float one = max(stem, foot);
    return mix(zero, one, bit);
}

void main()
{
    vec2 centered = fragTexCoord * 2.0 - 1.0;
    float radius = dot(centered, centered);
    vec2 uv = centered * (1.0 + radius * curvatureStrength * 0.085) * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0)
    {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 textureResolution = vec2(textureSize(texture0, 0));
    vec2 texel = 1.0 / textureResolution;
    float split = rgbSplitStrength * 2.5;
    vec3 color;
    color.r = texture(texture0, uv + vec2(texel.x * split, 0.0)).r;
    color.g = texture(texture0, uv).g;
    color.b = texture(texture0, uv - vec2(texel.x * split, 0.0)).b;
    float luminance = Luma(color);

    vec2 sourcePixel = uv * textureResolution;
    float noise = (Hash(sourcePixel) - 0.5) * ditherStrength;
    float levels = max(2.0, colorLevels);
    vec3 quantized = floor(clamp(color + noise / levels, 0.0, 1.0) *
                           levels + 0.5) / levels;
    color = mix(color, quantized, quantizeStrength);
    luminance = Luma(color);
    color = mix(color, vec3(luminance), monochromeStrength);
    color = mix(color, ApplyPalette(luminance, paletteMode), paletteStrength);

    float left = Luma(texture(texture0, uv - vec2(texel.x, 0.0)).rgb);
    float right = Luma(texture(texture0, uv + vec2(texel.x, 0.0)).rgb);
    float up = Luma(texture(texture0, uv - vec2(0.0, texel.y)).rgb);
    float down = Luma(texture(texture0, uv + vec2(0.0, texel.y)).rgb);
    float edge = clamp((abs(right - left) + abs(down - up)) * 3.2, 0.0, 1.0);
    color = mix(color, mix(color, vec3(0.015), edge), edgeStrength);

    float cellSize = max(4.0, binaryCellSize);
    vec2 cellIndex = floor(sourcePixel / cellSize);
    vec2 cellUv = (cellIndex + 0.5) * cellSize / textureResolution;
    float cellLight = Luma(texture(texture0, cellUv).rgb);
    float bit = step(binaryThreshold, cellLight);
    vec2 glyphUv = fract(sourcePixel / cellSize);
    float glyph = BinaryGlyph(glyphUv, bit);
    vec3 codeInk = ApplyPalette(max(0.35, cellLight), max(1.0, paletteMode));
    vec3 binaryColor = mix(vec3(0.004, 0.006, 0.008), codeInk, glyph);
    color = mix(color, binaryColor, binaryEnabled * binaryStrength);

    float spacing = max(2.0, gridSpacing);
    vec2 gridCell = mod(sourcePixel, spacing);
    float gridLine = max(step(gridCell.x, 0.65), step(gridCell.y, 0.65));
    color = mix(color, color * 0.20, gridLine * gridEnabled * gridStrength);

    float scanSpacing = max(1.0, scanlineSpacing);
    float scan = 0.5 + 0.5 * cos(6.2831853 * gl_FragCoord.y / scanSpacing);
    color *= 1.0 - scanlinesEnabled * scanlineStrength * mix(0.08, 0.56, scan);
    color *= mix(1.0, smoothstep(1.25, 0.20, radius), vignetteStrength);
    color = mix(color, vec3(1.0) - color, invertStrength);
    finalColor = vec4(clamp(color, 0.0, 1.0), 1.0) * colDiffuse * fragColor;
}
)GLSL";
    }

    App::App(VisualProfile profile) : mVisualProfile(profile)
    {
        mSettings.retroMode = profile != VisualProfile::Standard;
        mSettings.crtMode = profile == VisualProfile::CrtLow;
        mSettings.synthMode = profile == VisualProfile::Synth;
        if (mSettings.retroMode)
        {
            mSettings.shadowQuality = 0;
            mRetroBadgeTime = 7.0f;
        }
        SetRetroGraphicsActive(mSettings.retroMode);
        SetCrtGraphicsActive(mSettings.crtMode || mSettings.synthMode);
    }
    App::~App() { Shutdown(); }

    void App::InitWindowAndContext()
    {
        unsigned int flags = mVisualProfile == VisualProfile::Standard
            ? FLAG_MSAA_4X_HINT : 0u;
        if (mSettings.vsync) flags |= FLAG_VSYNC_HINT;

        SetConfigFlags(flags);
        const char *windowTitle = "Eldoria";
        if (mVisualProfile == VisualProfile::Bruma)
            windowTitle = "Eldoria - Perfil Semente // Bruma";
        else if (mVisualProfile == VisualProfile::CrtLow)
            windowTitle = "Eldoria - Perfil Semente // Tubo CRT";
        else if (mVisualProfile == VisualProfile::Synth)
            windowTitle = "Eldoria - SeedSynth // Sintetizador Grafico";
        InitWindow(mSettings.windowWidth, mSettings.windowHeight, windowTitle);
        SetExitKey(0);
        SetTargetFPS(60);
        SetTextureFilter(GetFontDefault().texture, TEXTURE_FILTER_BILINEAR);

        if (mSettings.fullscreen) ToggleFullscreen();
        mAppliedSettings = mSettings;

        if (mVisualProfile != VisualProfile::Standard) InitRetroPresentation();
    }

    void App::InitRetroPresentation()
    {
        mRetroPresentation = new RetroPresentation();
        RecreateRetroTarget();
        const char *fragment = RetroFragmentShader;
        if (mVisualProfile == VisualProfile::CrtLow) fragment = CrtFragmentShader;
        else if (mVisualProfile == VisualProfile::Synth) fragment = SynthFragmentShader;
        mRetroPresentation->shader = LoadShaderFromMemory(nullptr, fragment);
        if (mVisualProfile == VisualProfile::CrtLow &&
            mRetroPresentation->shader.id != 0)
        {
            mRetroPresentation->scanlinesLocation = GetShaderLocation(
                mRetroPresentation->shader, "scanlinesEnabled");
            mRetroPresentation->scanlineStrengthLocation = GetShaderLocation(
                mRetroPresentation->shader, "scanlineStrength");
            mRetroPresentation->scanlineSpacingLocation = GetShaderLocation(
                mRetroPresentation->shader, "scanlineSpacing");
            mRetroPresentation->aliasingStrengthLocation = GetShaderLocation(
                mRetroPresentation->shader, "aliasingStrength");
        }
        else if (mVisualProfile == VisualProfile::Synth &&
                 mRetroPresentation->shader.id != 0)
        {
            for (int index = 0; index < SynthUniformCount; ++index)
                mRetroPresentation->synthLocations[index] = GetShaderLocation(
                    mRetroPresentation->shader, SynthUniformNames[index]);
        }
    }

    void App::RecreateRetroTarget()
    {
        if (!mRetroPresentation) return;
        if (mRetroPresentation->target.id != 0)
            UnloadRenderTexture(mRetroPresentation->target);
        int width = RetroWorldWidth;
        if (mVisualProfile == VisualProfile::CrtLow)
            width = std::max(64, std::min(960, mSettings.crtInternalWidth));
        else if (mVisualProfile == VisualProfile::Synth)
            width = std::max(64, std::min(960, mSettings.synthInternalWidth));
        const int height = std::max(36, (int)roundf(width * 9.0f / 16.0f));
        mRetroPresentation->target = LoadRenderTexture(width, height);
        if (mRetroPresentation->target.id != 0)
            SetTextureFilter(mRetroPresentation->target.texture,
                (mVisualProfile == VisualProfile::CrtLow ||
                 (mVisualProfile == VisualProfile::Synth && mSettings.synthPointFilter))
                    ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);
    }

    void App::Run()
    {
        LoadSettings(mSettings);
        if (mSettings.crtMode) SetCrtTextureLimit(mSettings.crtTextureSize);
        if (mSettings.synthMode) SetCrtTextureLimit(mSettings.synthTextureSize);
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

            if (mVisualProfile != VisualProfile::Standard && mRetroPresentation &&
                mRetroPresentation->target.id != 0)
            {
                if (mRetroBadgeTime > 0.0f) mRetroBadgeTime -= dt;
                DrawRetroFrame();
            }
            else
            {
                BeginDrawing();
                ClearBackground({ 14, 16, 22, 255 });
                mActiveScene->Draw();
#if !defined(NDEBUG)
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
                EndDrawing();
            }
        }

        Shutdown();
    }

    void App::DrawRetroFrame()
    {
        BeginTextureMode(mRetroPresentation->target);
        ClearBackground({ 14, 16, 22, 255 });
        mActiveScene->DrawWorld();
        EndTextureMode();

        BeginDrawing();
        ClearBackground({ 8, 9, 13, 255 });
        const int worldWidth = mRetroPresentation->target.texture.width;
        const int worldHeight = mRetroPresentation->target.texture.height;
        const float scaleX = (float)GetScreenWidth() / (float)worldWidth;
        const float scaleY = (float)GetScreenHeight() / (float)worldHeight;
        const float scale = fminf(scaleX, scaleY);
        const Rectangle destination = {
            ((float)GetScreenWidth() - worldWidth * scale) * 0.5f,
            ((float)GetScreenHeight() - worldHeight * scale) * 0.5f,
            worldWidth * scale,
            worldHeight * scale
        };
        const Rectangle source = {
            0.0f, 0.0f,
            (float)mRetroPresentation->target.texture.width,
            -(float)mRetroPresentation->target.texture.height
        };
        if (mVisualProfile == VisualProfile::CrtLow &&
            mRetroPresentation->shader.id != 0)
        {
            const float scanlines = mSettings.crtScanlines ? 1.0f : 0.0f;
            const float spacing = (float)mSettings.crtScanlineSpacing;
            SetShaderValue(mRetroPresentation->shader,
                           mRetroPresentation->scanlinesLocation,
                           &scanlines, SHADER_UNIFORM_FLOAT);
            SetShaderValue(mRetroPresentation->shader,
                           mRetroPresentation->scanlineStrengthLocation,
                           &mSettings.crtScanlineStrength, SHADER_UNIFORM_FLOAT);
            SetShaderValue(mRetroPresentation->shader,
                           mRetroPresentation->scanlineSpacingLocation,
                           &spacing, SHADER_UNIFORM_FLOAT);
            SetShaderValue(mRetroPresentation->shader,
                           mRetroPresentation->aliasingStrengthLocation,
                           &mSettings.crtAliasingStrength, SHADER_UNIFORM_FLOAT);
        }
        else if (mVisualProfile == VisualProfile::Synth &&
                 mRetroPresentation->shader.id != 0)
        {
            auto setSynth = [&](int uniform, float value)
            {
                SetShaderValue(mRetroPresentation->shader,
                               mRetroPresentation->synthLocations[uniform],
                               &value, SHADER_UNIFORM_FLOAT);
            };
            setSynth(SynthQuantize, mSettings.synthQuantizeStrength);
            setSynth(SynthColorLevels, (float)mSettings.synthColorLevels);
            setSynth(SynthMonochrome, mSettings.synthMonochromeStrength);
            setSynth(SynthPaletteMode, (float)mSettings.synthPaletteMode);
            setSynth(SynthPaletteStrength, mSettings.synthPaletteStrength);
            setSynth(SynthDither, mSettings.synthDitherStrength);
            setSynth(SynthEdges, mSettings.synthEdgeStrength);
            setSynth(SynthBinary, mSettings.synthBinary ? 1.0f : 0.0f);
            setSynth(SynthBinaryStrength, mSettings.synthBinaryStrength);
            setSynth(SynthBinaryCell, (float)mSettings.synthBinaryCellSize);
            setSynth(SynthBinaryThreshold, mSettings.synthBinaryThreshold);
            setSynth(SynthGrid, mSettings.synthGrid ? 1.0f : 0.0f);
            setSynth(SynthGridStrength, mSettings.synthGridStrength);
            setSynth(SynthGridSpacing, (float)mSettings.synthGridSpacing);
            setSynth(SynthScanlines, mSettings.synthScanlines ? 1.0f : 0.0f);
            setSynth(SynthScanlineStrength, mSettings.synthScanlineStrength);
            setSynth(SynthScanlineSpacing, (float)mSettings.synthScanlineSpacing);
            setSynth(SynthCurvature, mSettings.synthCurvature);
            setSynth(SynthRgbSplit, mSettings.synthRgbSplit);
            setSynth(SynthVignette, mSettings.synthVignette);
            setSynth(SynthInvert, mSettings.synthInvert);
        }
        if (mRetroPresentation->shader.id != 0)
            BeginShaderMode(mRetroPresentation->shader);
        DrawTexturePro(mRetroPresentation->target.texture, source, destination,
                       { 0.0f, 0.0f }, 0.0f, WHITE);
        if (mRetroPresentation->shader.id != 0) EndShaderMode();

        mActiveScene->DrawOverlay();
        if (mRetroBadgeTime > 0.0f)
        {
            const char *badge = "PERFIL SEMENTE // BRUMA  960x540";
            if (mVisualProfile == VisualProfile::CrtLow)
                badge = TextFormat("PERFIL SEMENTE // TUBO CRT  %dx%d", worldWidth, worldHeight);
            else if (mVisualProfile == VisualProfile::Synth)
                badge = TextFormat("SEEDSYNTH // PATCH ATIVO  %dx%d", worldWidth, worldHeight);
            const int fontSize = 13;
            const int width = MeasureText(badge, fontSize);
            DrawRectangle(GetScreenWidth() - width - 28, GetScreenHeight() - 34,
                          width + 18, 24, { 10, 14, 20, 205 });
            DrawText(badge, GetScreenWidth() - width - 19,
                     GetScreenHeight() - 29, fontSize, { 169, 199, 207, 235 });
        }
#if !defined(NDEBUG)
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
        EndDrawing();
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

        if (mVisualProfile == VisualProfile::CrtLow &&
            s.crtInternalWidth != mAppliedSettings.crtInternalWidth)
            RecreateRetroTarget();
        if (mVisualProfile == VisualProfile::Synth &&
            (s.synthInternalWidth != mAppliedSettings.synthInternalWidth ||
             s.synthPointFilter != mAppliedSettings.synthPointFilter))
            RecreateRetroTarget();

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

        if (mRetroPresentation && mRetroPresentation->shader.id != 0)
        {
            UnloadShader(mRetroPresentation->shader);
            mRetroPresentation->shader = {};
        }
        if (mRetroPresentation && mRetroPresentation->target.id != 0)
        {
            UnloadRenderTexture(mRetroPresentation->target);
            mRetroPresentation->target = {};
        }
        delete mRetroPresentation;
        mRetroPresentation = nullptr;

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
