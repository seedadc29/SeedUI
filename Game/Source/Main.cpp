#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <string>

#include "App.h"

static game::VisualProfile DetectVisualProfile()
{
    wchar_t executablePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return game::VisualProfile::Standard;
    std::wstring name(executablePath, length);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](wchar_t c) { return (wchar_t)std::towlower(c); });
    if (name.find(L"crt") != std::wstring::npos ||
        name.find(L"lowpoly") != std::wstring::npos ||
        name.find(L"ps1") != std::wstring::npos ||
        name.find(L"tubo") != std::wstring::npos)
        return game::VisualProfile::CrtLow;
    if (name.find(L"retro") != std::wstring::npos ||
        name.find(L"bruma") != std::wstring::npos)
        return game::VisualProfile::Bruma;
    return game::VisualProfile::Standard;
}

int main()
{
    game::App app(DetectVisualProfile());
    app.Run();

    return 0;
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main();
}
