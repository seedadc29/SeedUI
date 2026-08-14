#ifndef SEEDUI_ANNOTATIONS_H
#define SEEDUI_ANNOTATIONS_H

#include "imgui.h"

#include <string>
#include <vector>

namespace seedui
{
    struct Annotation
    {
        int id = 0;
        ImVec2 min = ImVec2(0, 0);    // área selecionada (coordenadas da JANELA do SeedUI)
        ImVec2 max = ImVec2(0, 0);
        ImVec2 label = ImVec2(0, 0);  // canto sup. esquerdo do rótulo flutuante
        std::string text;             // alteração/diretriz escrita pelo usuário
        ImU32 color = 0;
        int icon = 0;                 // índice do catálogo de ícones (0 = nenhum)
        int iconPos = 0;              // 0 = sup. dir., 1 = inf. dir., 2 = inf. esq.
        float fontSize = 13.0f;       // tamanho da fonte do texto (px; 8..64)
        float labelW = 240.0f;        // largura do rótulo (redimensionável)
        float labelH = 0.0f;          // altura manual mínima do rótulo (0 = automática pelo texto)
    };

    // Posições do ícone no rótulo da anotação
    enum AnnotationsIconPos
    {
        AnnotationsIconPos_TopRight = 0,
        AnnotationsIconPos_BottomRight = 1,
        AnnotationsIconPos_BottomLeft = 2,
    };

    // Hook opcional para o seletor de ícones: desenhado DENTRO da janela de
    // edição da anotação (após o campo de texto). SeedNotas registra para
    // mostrar a grade de ícones; o SeedUI (modo debug) deixa nulo e o
    // comportamento não muda.
    typedef void (*AnnotationsIconPicker)(Annotation& a);

    struct AnnotationState
    {
        std::vector<Annotation> items;
        int nextId = 1;
        int selected = -1;            // índice da anotação em edição
        int dragging = -1;            // índice sendo arrastado
        int dragMode = 0;             // 1=área, 2=rótulo, 3=borda dir., 4=borda inf., 5=canto
        ImVec2 dragOffset = ImVec2(0, 0);
        bool creating = false;        // arrastando para criar uma nova
        ImVec2 createStart = ImVec2(0, 0);
        char editBuf[4096] = { 0 };
        int editedIndex = -1;
        AnnotationsIconPicker iconPicker = nullptr;  // opcional (SeedNotas)
    };

    struct AnnotationsPopupRect
    {
        ImVec2 min = ImVec2(0, 0);
        ImVec2 max = ImVec2(0, 0);
    };

    // Retângulo do popup de edição de uma anotação, sempre DENTRO da janela:
    // fica abaixo do rótulo; se não couber, aparece acima dele. A altura leva
    // em conta o seletor de ícones quando o hook estiver registrado.
    AnnotationsPopupRect AnnotationsPopupRectFor(const AnnotationState& st,
                                                 const Annotation& annotation,
                                                 const ImVec2& viewportSize);

    // Altura atual do rótulo flutuante (cresce com o texto).
    float AnnotationsLabelHeight(const Annotation& a);

    // Fonte de alta resolução para o TEXTO das anotações (opcional).
    // SeedNotas registra uma fonte de atlas grande (64px) para o texto
    // ampliado sair nítido; SeedUI deixa nulo e usa a fonte atual do ImGui.
    // Medição de altura e desenho usam a MESMA fonte (consistência).
    void AnnotationsSetHighResFont(ImFont* font);

    // Cria uma anotação programaticamente (ex.: amostra em modo captura)
    void AnnotationsAdd(AnnotationState& st, const ImVec2& min, const ImVec2& max,
                        const char* text);

    // Input GLOBAL: arrastar sobre QUALQUER parte da janela (menus, ícones,
    // painéis, canvas) cria/move anotações. Chamar dentro do escopo da janela
    // do workspace.
    void AnnotationsUpdate(AnnotationState& st, bool toolActive,
                           const ImVec2& viewportSize);

    // Desenha as anotações por cima de toda a interface (draw list de primeiro
    // plano). Se o popup de edição estiver aberto (clipPopup=true), as anotações
    // que ficariam por cima dele são puladas para não escondê-lo.
    void AnnotationsDraw(const AnnotationState& st, bool clipPopup,
                         const ImVec2& popupMin, const ImVec2& popupMax);

    // Ações possíveis da janela de edição de anotação
    enum AnnotationsEditAction
    {
        AnnotationsEdit_None = 0,   // popup fechado ou sem ação
        AnnotationsEdit_Confirmed,  // Confirmar ✓ / X / Esc (fecha mantendo o texto)
        AnnotationsEdit_Deleted,    // Apagar anotação
    };

    // Janela flutuante de digitação da anotação selecionada.
    // Retorna a ação escolhida pelo usuário.
    int AnnotationsEditWindow(AnnotationState& st);

    // Texto pronto para colar/exportar para a IA.
    std::string AnnotationsToText(const AnnotationState& st, const std::string& globalComment);
}

#endif // SEEDUI_ANNOTATIONS_H
