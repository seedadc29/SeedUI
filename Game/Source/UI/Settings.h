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
    };

    void LoadSettings(Settings &settings);
    void SaveSettings(const Settings &settings);
}

#endif // SETTINGS_H
