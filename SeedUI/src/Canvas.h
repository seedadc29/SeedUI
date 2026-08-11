#ifndef SEEDUI_CANVAS_H
#define SEEDUI_CANVAS_H

#include "Project.h"

namespace seedui
{
    // Desenha o canvas (grade, réguas, moldura) e os elementos da tela/modo.
    // projeto nulo = apenas o visual demonstrativo.
    void CanvasDraw(const Project* projeto = nullptr, int telaAtiva = 0, int modoAtivo = 0);
}

#endif // SEEDUI_CANVAS_H