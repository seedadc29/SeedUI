#include <windows.h>

#include "App.h"

int main()
{
    game::App app;
    app.Run();

    return 0;
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main();
}
