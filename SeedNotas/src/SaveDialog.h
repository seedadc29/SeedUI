#ifndef SEEDNOTAS_SAVEDIALOG_H
#define SEEDNOTAS_SAVEDIALOG_H

#include <string>

namespace seednotas
{
    // Diálogo nativo do Windows "Salvar como" para arquivos PNG.
    // ownerHwnd = janela dona (HWND). Sem dono, o diálogo pode abrir ATRÁS
    // de janelas sempre-no-topo (como o overlay em tela cheia), parecendo
    // travado. Retorna o caminho completo, ou vazio se o usuário cancelar.
    std::string SalvarDialogoPNG(const std::string& nomeSugerido, void* ownerHwnd);
}

#endif // SEEDNOTAS_SAVEDIALOG_H
