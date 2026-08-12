#ifndef SEEDUI_COLORPICKER_H
#define SEEDUI_COLORPICKER_H

namespace seedui
{
    // Seletor de cor com 3 modelos estilo Photoshop: triângulo (HSV),
    // quadrado (SV + barra de matiz) e barras (RGB). Desenha na janela
    // ImGui atual; altera rgb[] em 0..1. Uso:
    //     float rgb[3] = {...};
    //     if (ColorPicker::Widget("##cor", rgb)) { /* aplicar */ }
    namespace ColorPicker
    {
        // Desenha o seletor completo (abas de modelo + área). Retorna true
        // quando o usuário mudou a cor nesta chamada.
        bool Widget(const char* label, float rgb[3]);
    }
}

#endif // SEEDUI_COLORPICKER_H
