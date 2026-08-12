#ifndef SEEDUI_CANVAS_H
#define SEEDUI_CANVAS_H

#include "Project.h"

namespace seedui
{
    // Desenha o canvas (grade, réguas, moldura) e os elementos da tela/modo.
    // projeto nulo = apenas o visual demonstrativo.
    void CanvasDraw(const Project* projeto = nullptr, int telaAtiva = 0, int modoAtivo = 0,
                    const std::vector<std::string>* elementosSelecionados = nullptr,
                    const char* elementoPrincipalId = nullptr,
                    unsigned int quinasSelecionadas = 0,
                    bool exibirReguas = true,
                    bool reguasBloqueadas = false,
                    bool exibirGrade = true,
                    float zoom = 1.0f,
                    float panX = 0.0f, float panY = 0.0f,
                    float unidadeEmPixels = 1.0f);

    // Converte coordenadas da janela para coordenadas da tela base do projeto.
    bool CanvasScreenToProject(const Project* projeto, float screenX, float screenY,
                               float& projectX, float& projectY,
                               bool limitarNaMoldura = false,
                               float zoom = 1.0f,
                               float panX = 0.0f, float panY = 0.0f);

    // Converte um ponto da tela-base para a janela e devolve a escala visual.
    bool CanvasProjectToScreen(const Project* projeto, float projectX, float projectY,
                               float& screenX, float& screenY, float& scale,
                               float zoom = 1.0f,
                               float panX = 0.0f, float panY = 0.0f);
}

#endif // SEEDUI_CANVAS_H
