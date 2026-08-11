#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

namespace game
{
    // Full-screen loading overlay presented while the game world initializes
    // synchronously. Each Draw() call presents one frame to the window (and
    // keeps it responsive), so it can be called between heavy loading stages
    // to show real progress.
    class LoadingScreen
    {
    public:
        // Presents a frame with the given progress (0..1) and stage label.
        void Draw(float progress, const char *stage);
        // Presents a final 100% frame ("Pronto!").
        void Finish();
    };
}

#endif // LOADINGSCREEN_H
