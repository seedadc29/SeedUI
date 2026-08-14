#ifndef SEEDUI_FILEDIALOGS_H
#define SEEDUI_FILEDIALOGS_H

#include <string>

namespace seedui
{
    // Diálogos de arquivo nativos do Windows (commdlg32).
    // Retorna caminho completo selecionado, ou vazio se cancelado.

    std::string AbrirDialogoProjeto();
    std::string SalvarDialogoProjeto(const std::string& nomeSugerido);
    // Diálogo de exportação SVG (*.svg).
    std::string SalvarDialogoSVG(const std::string& nomeSugerido);
}

#endif // SEEDUI_FILEDIALOGS_H