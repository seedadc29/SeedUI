// Diálogo nativo do Windows "Salvar como" para o PNG anotado.
// Este arquivo isola <windows.h> do restante (nomes como CloseWindow,
// DrawText etc. colidiriam com o raylib) — mesmo padrão do FileDialogs.cpp
// do SeedUI.
#include "SaveDialog.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

#include <cstring>

namespace seednotas
{
    std::string SalvarDialogoPNG(const std::string& nomeSugerido, void* ownerHwnd)
    {
        constexpr const char* kFilter =
            "Imagem PNG (*.png)\0*.png\0Todos os arquivos (*.*)\0*.*\0\0";
        char buf[MAX_PATH] = { 0 };
        strncpy(buf, nomeSugerido.c_str(), sizeof buf - 1);
        OPENFILENAMEA ofn;
        std::memset(&ofn, 0, sizeof ofn);
        ofn.lStructSize = sizeof ofn;
        ofn.hwndOwner = (HWND)ownerHwnd; // diálogo sempre ACIMA da janela dona
        ofn.lpstrFilter = kFilter;
        ofn.lpstrFile = buf;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = "Salvar anotação SeedNotas";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        ofn.lpstrDefExt = "png";
        if (!GetSaveFileNameA(&ofn)) return std::string();
        return std::string(buf);
    }
}
