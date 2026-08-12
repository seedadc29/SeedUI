#ifndef SEEDUI_ALIGNUTILS_H
#define SEEDUI_ALIGNUTILS_H

// Camada de ALINHAMENTO AO CONJUNTO: quando o usuário seleciona um elemento
// (ex.: uma forma básica) e pede para centralizar/alinha, a referência é o
// BOUNDING BOX DOS VIZINHOS (elementos visíveis fora da seleção) — não a
// tela nem a própria seleção. Isso garante distâncias uniformes nos 4 lados:
// centralizar horizontal + vertical coloca o elemento exatamente no centro
// do conjunto, com gaps iguais em cima/baixo/esquerda/direita.
//
// A matemática é pura (retângulos + operação) para ser testável pelo
// autoteste M04, exatamente como `SmartGuides` e `Geo`.

#include <algorithm>
#include <cfloat>
#include <vector>

namespace seedui
{
    namespace AlignUtils
    {
        struct Rect
        {
            float x = 0.0f;
            float y = 0.0f;
            float w = 0.0f;
            float h = 0.0f;
        };

        // Operações: 0 esquerda, 1 centro horizontal, 2 direita,
        // 3 topo, 4 centro vertical, 5 base (mesmo mapeamento da UI).
        inline int OpCount() { return 6; }

        // Bounding box de um conjunto de retângulos (os vizinhos).
        inline void SetBounds(const std::vector<Rect>& rects,
                              float& outL, float& outT,
                              float& outR, float& outB)
        {
            outL = FLT_MAX; outT = FLT_MAX;
            outR = -FLT_MAX; outB = -FLT_MAX;
            for (const Rect& r : rects)
            {
                outL = std::min(outL, r.x);
                outT = std::min(outT, r.y);
                outR = std::max(outR, r.x + r.w);
                outB = std::max(outB, r.y + r.h);
            }
        }

        // Nova posição X de um elemento de largura `w` alinhado ao conjunto.
        inline float AlignedX(float w, float setL, float setR, int operation)
        {
            if (operation == 0) return setL;
            if (operation == 1) return (setL + setR) * 0.5f - w * 0.5f;
            if (operation == 2) return setR - w;
            return setL; // eixo Y não altera X
        }

        // Nova posição Y de um elemento de altura `h` alinhado ao conjunto.
        inline float AlignedY(float h, float setT, float setB, int operation)
        {
            if (operation == 3) return setT;
            if (operation == 4) return (setT + setB) * 0.5f - h * 0.5f;
            if (operation == 5) return setB - h;
            return setT; // eixo X não altera Y
        }
    }
}

#endif // SEEDUI_ALIGNUTILS_H
