// AnnotIcons.h — catálogo de ícones de anotação do SeedNotas.
//
// Ícones PREENCHIDOS (sem contorno) extraídos da pasta tabler-icons-main
// (icons/filled), rasterizados em branco com nanosvg e reutilizados em três
// lugares:
//   1. Seletor no popup de edição da anotação (ImGui);
//   2. Rótulo flutuante no overlay (ImGui, tingido com a cor da anotação);
//   3. Print/exportação PNG (raylib, tingido com a cor da anotação).
//
// O tingimento é feito na hora do desenho (textura branca x cor), então uma
// única textura por ícone serve para qualquer cor de anotação.

#ifndef SEEDNOTAS_ANNOTICONS_H
#define SEEDNOTAS_ANNOTICONS_H

#include "raylib.h"
#include "imgui.h"

namespace seednotas
{
    // Índices do catálogo (AnnotIconCount() ícones, índices 1..N; 0 = nenhum).
    // Ordem = ordem da grade no seletor.
    enum AnnotIconId
    {
        AnnotIcon_None = 0,
        AnnotIcon_ThumbUp,      // 1  Aprovação
        AnnotIcon_ThumbDown,    // 2  Desaprovação
        AnnotIcon_CircleCheck,  // 3  Aprovado
        AnnotIcon_CircleX,      // 4  Rejeitado
        AnnotIcon_AlertTriangle,// 5  Atenção
        AnnotIcon_AlertCircle,  // 6  Erro
        AnnotIcon_Bulb,         // 7  Ideia
        AnnotIcon_HelpCircle,   // 8  Dúvida
        AnnotIcon_InfoCircle,   // 9  Informação
        AnnotIcon_Heart,        // 10 Gostei
        AnnotIcon_Star,         // 11 Destaque
        AnnotIcon_Flag,         // 12 Bandeira
        AnnotIcon_Pin,          // 13 Fixar
        AnnotIcon_Bookmark,     // 14 Ler depois
        AnnotIcon_Edit,         // 15 Editar
        AnnotIcon_Eye,          // 16 Revisar
        AnnotIcon_Clock,        // 17 Prazo
        AnnotIcon_Sparkles,     // 18 Sugestão
        AnnotIcon_Quote,        // 19 Citação
        AnnotIcon_ClipboardCheck, // 20 Checklist
        AnnotIcon_ExclamationCircle, // 21 Importante
        AnnotIcon_Lock,         // 22 Bloqueado
        AnnotIcon_BarrierBlock, // 23 Proibido
    };

    // Carrega (ou recarrega) as texturas dos ícones. Chamar após InitWindow
    // (precisa de contexto OpenGL/raylib). Inofensivo chamar várias vezes.
    void AnnotIconsLoad();

    // Libera as texturas. Chamar antes de CloseWindow.
    void AnnotIconsUnload();

    // Quantidade de ícones do catálogo (23).
    int AnnotIconCount();

    // Nome legível em pt-BR do ícone (ex.: "Atenção"). index de 1..Count.
    const char* AnnotIconName(int index);

    // Nome do arquivo SVG de origem (ex.: "alert-triangle.svg").
    const char* AnnotIconFileName(int index);

    // Textura branca do ícone (para desenhar com ImGui/raylib e tingir).
    Texture2D AnnotIconTexture(int index);

    // Helper ImGui: botão de ícone tingido para a grade do seletor.
    // Retorna true se clicado.
    bool AnnotIconImageButton(int index, float size, ImU32 tint,
                              const char* tooltip);

    // Helper ImGui: desenha o ícone num ponto (coordenadas da janela),
    // tingido, usando a draw list de primeiro plano.
    void AnnotIconDrawOverlay(int index, const ImVec2& pos, float size,
                              ImU32 tint);
}

#endif // SEEDNOTAS_ANNOTICONS_H
