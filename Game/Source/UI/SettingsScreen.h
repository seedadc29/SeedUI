#ifndef SETTINGSCREEN_H
#define SETTINGSCREEN_H

#include "UI/Settings.h"
#include "UI/GameUI.h"

namespace game
{
    class SettingsScreen
    {
    public:
        bool Draw(Settings &settings);

    private:
        ui::UiState mUiState;
    };
}

#endif // SETTINGSCREEN_H
