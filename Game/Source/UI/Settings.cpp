#include "UI/Settings.h"

#include <fstream>
#include <string>

namespace game
{
    static bool gRetroGraphicsActive = false;
    static bool gCrtGraphicsActive = false;
    static int gCrtTextureLimit = 128;

    static const char *SettingsFile(const Settings &settings)
    {
        if (settings.synthMode) return "settings_synth.ini";
        if (settings.crtMode) return "settings_crt.ini";
        return settings.retroMode ? "settings_retro.ini" : "settings.ini";
    }

    static int ClampInt(int value, int min, int max)
    {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    static float ClampFloat(float value, float min, float max)
    {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    void LoadSettings(Settings &settings)
    {
        std::ifstream file(SettingsFile(settings));
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line))
        {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);

            if (key == "windowWidth") settings.windowWidth = std::stoi(value);
            else if (key == "windowHeight") settings.windowHeight = std::stoi(value);
            else if (key == "fullscreen") settings.fullscreen = (value == "1");
            else if (key == "vsync") settings.vsync = (value == "1");
            else if (key == "shadowQuality") settings.shadowQuality = std::stoi(value);
            else if (key == "mouseSensitivity") settings.mouseSensitivity = std::stof(value);
            else if (key == "masterVolume") settings.masterVolume = std::stof(value);
            else if (key == "characterGender")
                settings.characterGender = std::stoi(value) == 1
                    ? CharacterGender::Female : CharacterGender::Male;
            else if (key == "skinTone")
            {
                // Compatibility with the previous two-preset format.
                const bool light = std::stoi(value) == 1;
                settings.skinHue = light ? 0.075f : 0.045f;
                settings.skinSaturation = light ? 0.48f : 0.78f;
            }
            else if (key == "skinHue") settings.skinHue = std::stof(value);
            else if (key == "skinSaturation") settings.skinSaturation = std::stof(value);
            else if (key == "characterLightIntensity") settings.characterLightIntensity = std::stof(value);
            else if (key == "crtScanlines") settings.crtScanlines = (value == "1");
            else if (key == "crtScanlineStrength") settings.crtScanlineStrength = std::stof(value);
            else if (key == "crtScanlineSpacing") settings.crtScanlineSpacing = std::stoi(value);
            else if (key == "crtInternalWidth") settings.crtInternalWidth = std::stoi(value);
            else if (key == "crtAliasingStrength") settings.crtAliasingStrength = std::stof(value);
            else if (key == "crtTextureSize") settings.crtTextureSize = std::stoi(value);
            else if (key == "synthInternalWidth") settings.synthInternalWidth = std::stoi(value);
            else if (key == "synthTextureSize") settings.synthTextureSize = std::stoi(value);
            else if (key == "synthWireframe") settings.synthWireframe = (value == "1");
            else if (key == "synthPointFilter") settings.synthPointFilter = (value == "1");
            else if (key == "synthQuantizeStrength") settings.synthQuantizeStrength = std::stof(value);
            else if (key == "synthColorLevels") settings.synthColorLevels = std::stoi(value);
            else if (key == "synthMonochromeStrength") settings.synthMonochromeStrength = std::stof(value);
            else if (key == "synthPaletteMode") settings.synthPaletteMode = std::stoi(value);
            else if (key == "synthPaletteStrength") settings.synthPaletteStrength = std::stof(value);
            else if (key == "synthDitherStrength") settings.synthDitherStrength = std::stof(value);
            else if (key == "synthEdgeStrength") settings.synthEdgeStrength = std::stof(value);
            else if (key == "synthBinary") settings.synthBinary = (value == "1");
            else if (key == "synthBinaryStrength") settings.synthBinaryStrength = std::stof(value);
            else if (key == "synthBinaryCellSize") settings.synthBinaryCellSize = std::stoi(value);
            else if (key == "synthBinaryThreshold") settings.synthBinaryThreshold = std::stof(value);
            else if (key == "synthGrid") settings.synthGrid = (value == "1");
            else if (key == "synthGridStrength") settings.synthGridStrength = std::stof(value);
            else if (key == "synthGridSpacing") settings.synthGridSpacing = std::stoi(value);
            else if (key == "synthScanlines") settings.synthScanlines = (value == "1");
            else if (key == "synthScanlineStrength") settings.synthScanlineStrength = std::stof(value);
            else if (key == "synthScanlineSpacing") settings.synthScanlineSpacing = std::stoi(value);
            else if (key == "synthCurvature") settings.synthCurvature = std::stof(value);
            else if (key == "synthRgbSplit") settings.synthRgbSplit = std::stof(value);
            else if (key == "synthVignette") settings.synthVignette = std::stof(value);
            else if (key == "synthInvert") settings.synthInvert = std::stof(value);
            else if (key == "synthEnemyPolygonRatio") settings.synthEnemyPolygonRatio = std::stof(value);
            else if (key == "synthPlayerPolygonRatio") settings.synthPlayerPolygonRatio = std::stof(value);
            else if (key == "synthScenePolygonRatio") settings.synthScenePolygonRatio = std::stof(value);
        }

        settings.windowWidth = ClampInt(settings.windowWidth, 640, 3840);
        settings.windowHeight = ClampInt(settings.windowHeight, 480, 2160);
        settings.shadowQuality = ClampInt(settings.shadowQuality, 0, 2);
        settings.mouseSensitivity = ClampFloat(settings.mouseSensitivity, 0.0005f, 0.01f);
        settings.masterVolume = ClampFloat(settings.masterVolume, 0.0f, 1.0f);
        settings.skinHue = ClampFloat(settings.skinHue, 0.0f, 1.0f);
        settings.skinSaturation = ClampFloat(settings.skinSaturation, 0.0f, 1.0f);
        settings.characterLightIntensity = ClampFloat(settings.characterLightIntensity, 0.35f, 1.65f);
        settings.crtScanlineStrength = ClampFloat(settings.crtScanlineStrength, 0.0f, 1.0f);
        settings.crtScanlineSpacing = ClampInt(settings.crtScanlineSpacing, 1, 12);
        settings.crtInternalWidth = ClampInt(settings.crtInternalWidth, 64, 960);
        settings.crtAliasingStrength = ClampFloat(settings.crtAliasingStrength, 0.0f, 1.0f);
        settings.crtTextureSize = ClampInt(settings.crtTextureSize, 32, 512);
        settings.synthInternalWidth = ClampInt(settings.synthInternalWidth, 64, 960);
        settings.synthTextureSize = ClampInt(settings.synthTextureSize, 32, 512);
        settings.synthQuantizeStrength = ClampFloat(settings.synthQuantizeStrength, 0.0f, 1.0f);
        settings.synthColorLevels = ClampInt(settings.synthColorLevels, 2, 256);
        settings.synthMonochromeStrength = ClampFloat(settings.synthMonochromeStrength, 0.0f, 1.0f);
        settings.synthPaletteMode = ClampInt(settings.synthPaletteMode, 0, 5);
        settings.synthPaletteStrength = ClampFloat(settings.synthPaletteStrength, 0.0f, 1.0f);
        settings.synthDitherStrength = ClampFloat(settings.synthDitherStrength, 0.0f, 1.0f);
        settings.synthEdgeStrength = ClampFloat(settings.synthEdgeStrength, 0.0f, 1.0f);
        settings.synthBinaryStrength = ClampFloat(settings.synthBinaryStrength, 0.0f, 1.0f);
        settings.synthBinaryCellSize = ClampInt(settings.synthBinaryCellSize, 4, 48);
        settings.synthBinaryThreshold = ClampFloat(settings.synthBinaryThreshold, 0.0f, 1.0f);
        settings.synthGridStrength = ClampFloat(settings.synthGridStrength, 0.0f, 1.0f);
        settings.synthGridSpacing = ClampInt(settings.synthGridSpacing, 2, 64);
        settings.synthScanlineStrength = ClampFloat(settings.synthScanlineStrength, 0.0f, 1.0f);
        settings.synthScanlineSpacing = ClampInt(settings.synthScanlineSpacing, 1, 12);
        settings.synthCurvature = ClampFloat(settings.synthCurvature, 0.0f, 1.0f);
        settings.synthRgbSplit = ClampFloat(settings.synthRgbSplit, 0.0f, 1.0f);
        settings.synthVignette = ClampFloat(settings.synthVignette, 0.0f, 1.0f);
        settings.synthInvert = ClampFloat(settings.synthInvert, 0.0f, 1.0f);
        settings.synthEnemyPolygonRatio = ClampFloat(settings.synthEnemyPolygonRatio, 0.05f, 1.0f);
        settings.synthPlayerPolygonRatio = ClampFloat(settings.synthPlayerPolygonRatio, 0.05f, 1.0f);
        settings.synthScenePolygonRatio = ClampFloat(settings.synthScenePolygonRatio, 0.05f, 1.0f);
    }

    void SaveSettings(const Settings &settings)
    {
        std::ofstream file(SettingsFile(settings));
        if (!file.is_open()) return;

        file << "windowWidth=" << settings.windowWidth << "\n";
        file << "windowHeight=" << settings.windowHeight << "\n";
        file << "fullscreen=" << (settings.fullscreen ? 1 : 0) << "\n";
        file << "vsync=" << (settings.vsync ? 1 : 0) << "\n";
        file << "shadowQuality=" << settings.shadowQuality << "\n";
        file << "mouseSensitivity=" << settings.mouseSensitivity << "\n";
        file << "masterVolume=" << settings.masterVolume << "\n";
        file << "characterGender=" << static_cast<int>(settings.characterGender) << "\n";
        file << "skinHue=" << settings.skinHue << "\n";
        file << "skinSaturation=" << settings.skinSaturation << "\n";
        file << "characterLightIntensity=" << settings.characterLightIntensity << "\n";
        if (settings.crtMode)
        {
            file << "crtScanlines=" << (settings.crtScanlines ? 1 : 0) << "\n";
            file << "crtScanlineStrength=" << settings.crtScanlineStrength << "\n";
            file << "crtScanlineSpacing=" << settings.crtScanlineSpacing << "\n";
            file << "crtInternalWidth=" << settings.crtInternalWidth << "\n";
            file << "crtAliasingStrength=" << settings.crtAliasingStrength << "\n";
            file << "crtTextureSize=" << settings.crtTextureSize << "\n";
        }
        if (settings.synthMode)
        {
            file << "synthInternalWidth=" << settings.synthInternalWidth << "\n";
            file << "synthTextureSize=" << settings.synthTextureSize << "\n";
            file << "synthWireframe=" << (settings.synthWireframe ? 1 : 0) << "\n";
            file << "synthPointFilter=" << (settings.synthPointFilter ? 1 : 0) << "\n";
            file << "synthQuantizeStrength=" << settings.synthQuantizeStrength << "\n";
            file << "synthColorLevels=" << settings.synthColorLevels << "\n";
            file << "synthMonochromeStrength=" << settings.synthMonochromeStrength << "\n";
            file << "synthPaletteMode=" << settings.synthPaletteMode << "\n";
            file << "synthPaletteStrength=" << settings.synthPaletteStrength << "\n";
            file << "synthDitherStrength=" << settings.synthDitherStrength << "\n";
            file << "synthEdgeStrength=" << settings.synthEdgeStrength << "\n";
            file << "synthBinary=" << (settings.synthBinary ? 1 : 0) << "\n";
            file << "synthBinaryStrength=" << settings.synthBinaryStrength << "\n";
            file << "synthBinaryCellSize=" << settings.synthBinaryCellSize << "\n";
            file << "synthBinaryThreshold=" << settings.synthBinaryThreshold << "\n";
            file << "synthGrid=" << (settings.synthGrid ? 1 : 0) << "\n";
            file << "synthGridStrength=" << settings.synthGridStrength << "\n";
            file << "synthGridSpacing=" << settings.synthGridSpacing << "\n";
            file << "synthScanlines=" << (settings.synthScanlines ? 1 : 0) << "\n";
            file << "synthScanlineStrength=" << settings.synthScanlineStrength << "\n";
            file << "synthScanlineSpacing=" << settings.synthScanlineSpacing << "\n";
            file << "synthCurvature=" << settings.synthCurvature << "\n";
            file << "synthRgbSplit=" << settings.synthRgbSplit << "\n";
            file << "synthVignette=" << settings.synthVignette << "\n";
            file << "synthInvert=" << settings.synthInvert << "\n";
            file << "synthEnemyPolygonRatio=" << settings.synthEnemyPolygonRatio << "\n";
            file << "synthPlayerPolygonRatio=" << settings.synthPlayerPolygonRatio << "\n";
            file << "synthScenePolygonRatio=" << settings.synthScenePolygonRatio << "\n";
        }
    }

    void SetRetroGraphicsActive(bool active)
    {
        gRetroGraphicsActive = active;
    }

    bool IsRetroGraphicsActive()
    {
        return gRetroGraphicsActive;
    }

    void SetCrtGraphicsActive(bool active)
    {
        gCrtGraphicsActive = active;
    }

    bool IsCrtGraphicsActive()
    {
        return gCrtGraphicsActive;
    }

    void SetCrtTextureLimit(int pixels)
    {
        gCrtTextureLimit = ClampInt(pixels, 32, 512);
    }

    int GetCrtTextureLimit()
    {
        return gCrtTextureLimit;
    }
}
