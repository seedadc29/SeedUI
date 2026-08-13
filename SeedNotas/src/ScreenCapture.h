#ifndef SEEDNOTAS_SCREENCAPTURE_H
#define SEEDNOTAS_SCREENCAPTURE_H

// Captura a tela do monitor primário e devolve os pixels em RGBA
// (byte order R,G,B,A). Retorna false em caso de falha.
//
// Implementado com GDI clássico (BitBlt do desktop) para funcionar em
// qualquer Windows moderno sem dependências adicionais — mesmo padrão
// usado pela engine para captura de telas de anotação.

namespace seednotas
{
    bool CaptureScreenRGBA(int& width, int& height,
                           unsigned char*& outPixels /* malloc'ed, caller frees */);
}

#endif // SEEDNOTAS_SCREENCAPTURE_H
