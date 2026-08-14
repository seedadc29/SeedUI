#ifndef SETTINGS_H
#define SETTINGS_H

namespace game
{
    enum class CharacterGender : int
    {
        Male = 0,
        Female = 1,
    };

    struct Settings
    {
        int   windowWidth = 1280;
        int   windowHeight = 720;
        bool  fullscreen = false;
        bool  vsync = true;
        int   shadowQuality = 2;   // 0 = Low, 1 = Medium, 2 = High
        float mouseSensitivity = 0.0025f;
        float masterVolume = 1.0f; // 0..1
        CharacterGender characterGender = CharacterGender::Male;
        float skinHue = 0.06f;             // 0..1
        float skinSaturation = 0.72f;      // 0..1
        float characterLightIntensity = 1.0f; // 0.35..1.65
        // Perfil experimental isolado. Nao e gravado no settings.ini normal.
        bool retroMode = false;
        bool crtMode = false;
        bool crtScanlines = true;
        float crtScanlineStrength = 0.72f;
        int crtScanlineSpacing = 3;
        int crtInternalWidth = 480;
        float crtAliasingStrength = 0.78f;
        int crtTextureSize = 128;
        // SeedSynth: cadeia modular salva em settings_synth.ini.
        bool synthMode = false;
        int synthInternalWidth = 480;
        int synthTextureSize = 128;
        bool synthWireframe = false;
        bool synthPointFilter = true;
        float synthQuantizeStrength = 0.45f;
        int synthColorLevels = 16;
        float synthMonochromeStrength = 0.0f;
        int synthPaletteMode = 0;
        float synthPaletteStrength = 0.0f;
        float synthDitherStrength = 0.20f;
        float synthEdgeStrength = 0.0f;
        bool synthBinary = false;
        float synthBinaryStrength = 1.0f;
        int synthBinaryCellSize = 12;
        float synthBinaryThreshold = 0.50f;
        bool synthGrid = false;
        float synthGridStrength = 0.45f;
        int synthGridSpacing = 12;
        bool synthScanlines = false;
        float synthScanlineStrength = 0.45f;
        int synthScanlineSpacing = 3;
        float synthCurvature = 0.0f;
        float synthRgbSplit = 0.0f;
        float synthVignette = 0.15f;
        float synthInvert = 0.0f;
    };

    void LoadSettings(Settings &settings);
    void SaveSettings(const Settings &settings);
    void SetRetroGraphicsActive(bool active);
    bool IsRetroGraphicsActive();
    void SetCrtGraphicsActive(bool active);
    bool IsCrtGraphicsActive();
    void SetCrtTextureLimit(int pixels);
    int GetCrtTextureLimit();
}

#endif // SETTINGS_H
