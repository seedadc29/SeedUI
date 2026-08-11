// Entry point: subsistema Windows sem console, mas com main(argc, argv)
// funcional (o CRT monta os argumentos reais da linha de comando).
#pragma comment(linker, "/ENTRY:mainCRTStartup")

#include <cstring>

#include "App.h"

int main(int argc, char** argv)
{
    const bool capture = argc > 1 && std::strcmp(argv[1], "--capture") == 0;
    seedui::App app;
    app.Run(capture);
    return 0;
}
