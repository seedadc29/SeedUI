// Diálogos de arquivo nativos. Estes arquivos isolam <windows.h> do resto do
// código (nomes como CloseWindow, DrawText etc. colidiriam com o raylib).
#include "FileDialogs.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

#include <cstring>

namespace seedui
{
    namespace
    {
        constexpr const char* kFilter = "Projeto SeedUI (*.ui.json)\0*.ui.json\0Todos os arquivos (*.*)\0*.*\0\0";
        constexpr const char* kExtension = "ui.json";

        std::string RunOpenDialog()
        {
            char buf[MAX_PATH] = { 0 };
            OPENFILENAMEA ofn;
            std::memset(&ofn, 0, sizeof ofn);
            ofn.lStructSize = sizeof ofn;
            ofn.lpstrFilter = kFilter;
            ofn.lpstrFile = buf;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = "Abrir projeto SeedUI";
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            if (!GetOpenFileNameA(&ofn)) return std::string();
            return std::string(buf);
        }

        std::string RunSaveDialog(const std::string& nomeSugerido)
        {
            char buf[MAX_PATH] = { 0 };
            strncpy(buf, nomeSugerido.c_str(), sizeof buf - 1);
            OPENFILENAMEA ofn;
            std::memset(&ofn, 0, sizeof ofn);
            ofn.lStructSize = sizeof ofn;
            ofn.lpstrFilter = kFilter;
            ofn.lpstrFile = buf;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = "Salvar projeto SeedUI";
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            ofn.lpstrDefExt = kExtension;
            if (!GetSaveFileNameA(&ofn)) return std::string();
            return std::string(buf);
        }
    }

    std::string AbrirDialogoProjeto()
    {
        return RunOpenDialog();
    }

    std::string SalvarDialogoProjeto(const std::string& nomeSugerido)
    {
        return RunSaveDialog(nomeSugerido);
    }

    std::string SalvarDialogoSVG(const std::string& nomeSugerido)
    {
        constexpr const char* kFilterSvg =
            "SVG ("".svg)\0*.svg\0Todos os arquivos (*.*)\0*.*\0\0";
        char buf[MAX_PATH] = { 0 };
        strncpy(buf, nomeSugerido.c_str(), sizeof buf - 1);
        OPENFILENAMEA ofn;
        std::memset(&ofn, 0, sizeof ofn);
        ofn.lStructSize = sizeof ofn;
        ofn.lpstrFilter = kFilterSvg;
        ofn.lpstrFile = buf;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = "Exportar SVG";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        ofn.lpstrDefExt = "svg";
        if (!GetSaveFileNameA(&ofn)) return std::string();
        return std::string(buf);
    }
}