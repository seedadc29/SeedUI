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
