#pragma once

#include <string>
#include <vector>

namespace seedui
{
    // Carrega um recurso embutido no executavel (RCDATA) pelo nome.
    // Ex.: LoadEmbeddedResource("Inter.ttf", out) le o bloco
    //      "Inter.ttf" RCDATA "assets/fonts/Inter.ttf" declarado no .rc.
    // Retorna false se o recurso nao existir.
    bool LoadEmbeddedResource(const char* name, std::vector<unsigned char>& out);

    // Conveniencia: devolve o conteudo como texto (SVG/manual).
    bool LoadEmbeddedText(const char* name, std::string& out);
}
