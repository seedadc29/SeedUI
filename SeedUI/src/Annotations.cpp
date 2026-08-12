#include "Annotations.h"

#include "Theme.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <cstring>

namespace seedui
{
    namespace
    {
        constexpr float kLabelW = 240.0f;
        constexpr float kLabelMinH = 68.0f;
        constexpr float kPopupW = 360.0f;
        constexpr float kPopupH = 202.0f; // edição individual compacta; exportação é global na barra inferior

        const ImU32 kAnnotColors[] = {
            IM_COL32(241, 196, 15, 255),   // amarelo
            IM_COL32(236, 72, 153, 255),   // magenta
            IM_COL32(34, 211, 238, 255),   // ciano
            IM_COL32(46, 204, 113, 255),   // verde
            IM_COL32(168, 85, 247, 255),   // violeta
        };
        constexpr int kColorCount = 5;

        float LabelHeight(const Annotation& a)
        {
            const char* preview = a.text.empty() ? "Clique para editar..." : a.text.c_str();
            const float textH = ImGui::CalcTextSize(preview, nullptr, false,
                                                    kLabelW - 16.0f).y;
            return std::max(kLabelMinH, 34.0f + textH + 8.0f);
        }
        bool PointInRect(const ImVec2& p, const ImVec2& a, const ImVec2& b)
        {
            return p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y;
        }

        bool RectsOverlap(const ImVec2& a0, const ImVec2& a1,
                          const ImVec2& b0, const ImVec2& b1)
        {
            return a0.x < b1.x && a1.x > b0.x && a0.y < b1.y && a1.y > b0.y;
        }

        void ArrowHead(ImDrawList* dl, const ImVec2& to, const ImVec2& dir, ImU32 col, float size)
        {
            const float d0 = -dir.y * 0.5f;
            const float d1 = dir.x * 0.5f;
            dl->AddLine(to, ImVec2(to.x - dir.x * size + d0 * size, to.y - dir.y * size + d1 * size), col, 1.5f);
            dl->AddLine(to, ImVec2(to.x - dir.x * size - d0 * size, to.y - dir.y * size - d1 * size), col, 1.5f);
        }
    }

    AnnotationsPopupRect AnnotationsPopupRectFor(const Annotation& annotation,
                                                 const ImVec2& viewportSize)
    {
        const ImVec2 label = annotation.label;
        ImVec2 anchor(label.x, label.y + LabelHeight(annotation) + 8.0f); // abaixo do rótulo
        if (anchor.x + kPopupW > viewportSize.x) anchor.x = viewportSize.x - kPopupW;
        if (anchor.x < 0.0f) anchor.x = 0.0f;
        if (anchor.y + kPopupH > viewportSize.y)
        {
            // Não coube embaixo: coloca acima do rótulo (ou na borda de baixo)
            anchor.y = label.y - kPopupH - 8.0f;
            if (anchor.y < 0.0f) anchor.y = viewportSize.y - kPopupH;
            if (anchor.y < 0.0f) anchor.y = 0.0f;
        }
        AnnotationsPopupRect r;
        r.min = anchor;
        r.max = ImVec2(anchor.x + kPopupW, anchor.y + kPopupH);
        return r;
    }

    void AnnotationsAdd(AnnotationState& st, const ImVec2& min, const ImVec2& max, const char* text)
    {
        Annotation a;
        a.id = st.nextId++;
        a.min = min;
        a.max = max;
        a.text = text ? text : "";
        a.label = ImVec2(max.x + 14.0f, max.y - LabelHeight(a) - 8.0f);
        a.color = kAnnotColors[(a.id - 1) % kColorCount];
        st.items.push_back(a);
        st.selected = (int)st.items.size() - 1;
        st.editedIndex = -1;
    }

    void AnnotationsUpdate(AnnotationState& st, bool toolActive, const ImVec2& viewportSize)
    {
        if (!toolActive)
        {
            st.creating = false;
            st.dragging = -1;
            st.dragMode = 0;
            return;
        }

        // Coordenadas GLOBAIS (janela do SeedUI inteira): a anotação pode cobrir
        // menus, ícones da barra lateral, painéis e o canvas.
        const ImVec2 mouse = ImGui::GetMousePos();
        // O canvas usa um InvisibleButton que fica ativo durante o arraste;
        // depender de IsWindowHovered bloqueava falsamente as anotações.
        const bool hovered = PointInRect(mouse, ImVec2(0.0f, 0.0f), viewportSize);

        // O popup da anotação precisa receber clique e teclado. Sem esta
        // exclusão, a captura global desmarcava a anotação antes do campo focar.
        if (!st.creating && st.dragging < 0 && st.selected >= 0 &&
            st.selected < (int)st.items.size())
        {
            const AnnotationsPopupRect popup =
                AnnotationsPopupRectFor(st.items[st.selected], viewportSize);
            if (PointInRect(mouse, popup.min, popup.max)) return;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered)
        {
            int hitLabel = -1;
            int hitRect = -1;
            for (int i = (int)st.items.size() - 1; i >= 0; --i)
            {
                const Annotation& a = st.items[i];
                if (PointInRect(mouse, a.label, ImVec2(a.label.x + kLabelW, a.label.y + LabelHeight(a))))
                {
                    hitLabel = i;
                    break;
                }
                if (PointInRect(mouse, a.min, a.max))
                {
                    hitRect = i;
                    break;
                }
            }

            if (hitLabel >= 0)
            {
                st.selected = hitLabel;
                st.dragging = hitLabel;
                st.dragMode = 2;
                st.dragOffset = ImVec2(mouse.x - st.items[hitLabel].label.x,
                                       mouse.y - st.items[hitLabel].label.y);
            }
            else if (hitRect >= 0)
            {
                st.selected = hitRect;
                st.dragging = hitRect;
                st.dragMode = 1;
                st.dragOffset = ImVec2(mouse.x - st.items[hitRect].min.x,
                                       mouse.y - st.items[hitRect].min.y);
            }
            else
            {
                st.selected = -1;
                st.creating = true;
                st.createStart = mouse;
            }
        }

        if (st.dragging >= 0 && st.dragMode != 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            Annotation& a = st.items[st.dragging];
            if (st.dragMode == 1)
            {
                const ImVec2 np(mouse.x - st.dragOffset.x, mouse.y - st.dragOffset.y);
                const ImVec2 delta(np.x - a.min.x, np.y - a.min.y);
                a.min.x += delta.x;
                a.min.y += delta.y;
                a.max.x += delta.x;
                a.max.y += delta.y;
                a.label.x += delta.x;
                a.label.y += delta.y;
            }
            else
            {
                a.label.x = mouse.x - st.dragOffset.x;
                a.label.y = mouse.y - st.dragOffset.y;
                const float boundX = std::max(0.0f, viewportSize.x - kLabelW);
                const float boundY = std::max(0.0f, viewportSize.y - LabelHeight(a));
                a.label.x = std::min(std::max(a.label.x, 0.0f), boundX);
                a.label.y = std::min(std::max(a.label.y, 0.0f), boundY);
            }
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (st.creating)
            {
                const ImVec2 a0 = st.createStart;
                const float dx = mouse.x - a0.x;
                const float dy = mouse.y - a0.y;
                if (fabsf(dx) > 10.0f && fabsf(dy) > 10.0f)
                {
                    const ImVec2 mn(std::min(a0.x, mouse.x), std::min(a0.y, mouse.y));
                    const ImVec2 mx(std::max(a0.x, mouse.x), std::max(a0.y, mouse.y));
                    AnnotationsAdd(st, mn, mx, "");
                }
                st.creating = false;
            }
            st.dragging = -1;
            st.dragMode = 0;
        }

        // Clique direito fora das anotações = desmarca
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hovered)
        {
            bool onAny = false;
            for (const Annotation& a : st.items)
            {
                if (PointInRect(mouse, a.min, a.max) ||
                    PointInRect(mouse, a.label, ImVec2(a.label.x + kLabelW, a.label.y + LabelHeight(a))))
                {
                    onAny = true;
                    break;
                }
            }
            if (!onAny) st.selected = -1;
        }
    }

    void AnnotationsDraw(const AnnotationState& st, bool clipPopup,
                         const ImVec2& popupMin, const ImVec2& popupMax)
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImFont* font = ImGui::GetFont();

        for (const Annotation& a : st.items)
        {
            // Não desenha por cima da janela de edição aberta
            const bool overPopup = clipPopup &&
                (RectsOverlap(a.min, a.max, popupMin, popupMax) ||
                 RectsOverlap(a.label, ImVec2(a.label.x + kLabelW, a.label.y + LabelHeight(a)),
                              popupMin, popupMax));
            if (overPopup) continue;

            // Coordenadas já são globais (janela do SeedUI)
            const ImVec2 r0 = a.min;
            const ImVec2 r1 = a.max;
            const ImU32 col = a.color;
            const ImU32 fill = (col & 0x00FFFFFF) | 0x22000000;
            const bool isSel = (st.selected >= 0 && &a == &st.items[st.selected]);

            // Área selecionada
            dl->AddRectFilled(r0, r1, fill);
            dl->AddRect(r0, r1, col, 0.0f, 0, isSel ? 2.5f : 1.5f);

            // Número (badge)
            const float badge = 18.0f;
            const ImVec2 bc(r0.x + badge, r0.y + badge);
            dl->AddCircleFilled(bc, badge, col);
            char nb[16];
            snprintf(nb, sizeof nb, "%d", a.id);
            const ImVec2 nts = font->CalcTextSizeA(12.0f, FLT_MAX, 0.0f, nb);
            dl->AddText(font, 12.0f, ImVec2(bc.x - nts.x * 0.5f, bc.y - nts.y * 0.5f),
                        IM_COL32(25, 25, 25, 255), nb);

            // Rótulo flutuante
            const ImVec2 l0 = a.label;
            const ImVec2 l1(l0.x + kLabelW, l0.y + LabelHeight(a));

            // Seta do rótulo para a área
            const ImVec2 from((l0.x + l1.x) * 0.5f, l1.y);
            const ImVec2 to((r0.x + r1.x) * 0.5f, r0.y);
            ImVec2 dir(to.x - from.x, to.y - from.y);
            const float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
            if (len > 1.0f)
            {
                dir.x /= len;
                dir.y /= len;
            }
            dl->AddLine(from, to, col, 1.5f);
            ArrowHead(dl, to, dir, col, 6.0f);

            dl->AddRectFilled(l0, l1, IM_COL32(43, 43, 43, 245), 4.0f);
            dl->AddRect(l0, l1, col, 4.0f, 0, isSel ? 2.0f : 1.5f);

            char tb[32];
            snprintf(tb, sizeof tb, "Anotação #%d", a.id);
            dl->AddText(ImVec2(l0.x + 8.0f, l0.y + 6.0f), col, tb);

            const char* preview = a.text.empty() ? "Clique para editar..." : a.text.c_str();
            dl->PushClipRect(ImVec2(l0.x + 8.0f, l0.y + 24.0f),
                             ImVec2(l1.x - 8.0f, l1.y - 6.0f), true);
            dl->AddText(font, 13.0f, ImVec2(l0.x + 8.0f, l0.y + 26.0f),
                        IM_COL32(236, 236, 236, 255), preview, nullptr, kLabelW - 16.0f);
            dl->PopClipRect();
        }

        // Pré-visualização do retângulo em criação
        if (st.creating)
        {
            const ImVec2 a0 = st.createStart;
            const ImVec2 m = ImGui::GetMousePos();
            dl->AddRect(a0, m, kAnnotColors[0], 0.0f, 0, 1.5f);
            const char* hint = "Solte para criar a anotação";
            dl->AddText(font, 13.0f, ImVec2(m.x + 12.0f, m.y + 8.0f),
                        IM_COL32(241, 196, 15, 255), hint);
        }
    }

    int AnnotationsEditWindow(AnnotationState& st)
    {
        if (st.selected < 0 || st.selected >= (int)st.items.size())
        {
            st.editedIndex = -1;
            return AnnotationsEdit_None;
        }

        Annotation& a = st.items[st.selected];
        bool focusEditor = false;

        if (st.editedIndex != st.selected)
        {
            focusEditor = true;
            strncpy(st.editBuf, a.text.c_str(), sizeof(st.editBuf) - 1);
            st.editBuf[sizeof(st.editBuf) - 1] = 0;
            st.editedIndex = st.selected;
        }

        // Popup sempre DENTRO da janela (abaixo do rótulo; acima dele se não couber)
        const AnnotationsPopupRect pr =
            AnnotationsPopupRectFor(a, ImGui::GetMainViewport()->Size);
        ImGui::SetNextWindowPos(pr.min, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(kPopupW, kPopupH), ImGuiCond_Always);

        char title[48];
        snprintf(title, sizeof title, "Anotação #%d — escreva a alteração", a.id);

        bool open = true;
        if (!ImGui::Begin(title, &open,
                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                          ImGuiWindowFlags_NoResize))
        {
            ImGui::End();
            return AnnotationsEdit_None;
        }

        // Fechou pelo X ou Esc = confirma e fecha a caixa
        if (!open || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            st.selected = -1;
            st.editedIndex = -1;
            ImGui::End();
            return AnnotationsEdit_Confirmed;
        }

        if (focusEditor) ImGui::SetKeyboardFocusHere();
        ImGui::InputTextMultiline("##texto", st.editBuf, sizeof(st.editBuf),
                                  ImVec2(-1.0f, 84.0f));
        a.text = st.editBuf; // salvo automaticamente: o rótulo atualiza em tempo real

        ImGui::TextColored(Theme::TextDisabled,
                           "Área: (%.0f, %.0f) %.0f x %.0f — janela do SeedUI",
                           a.min.x, a.min.y, a.max.x - a.min.x, a.max.y - a.min.y);

        const float btnW = (ImGui::GetContentRegionAvail().x - 8.0f) * 0.5f;
        if (ImGui::Button("Confirmar ✓", ImVec2(btnW, 0)))
        {
            st.selected = -1;
            st.editedIndex = -1;
            ImGui::End();
            return AnnotationsEdit_Confirmed;
        }
        ImGui::SameLine();
        if (ImGui::Button("Apagar anotação", ImVec2(btnW, 0)))
        {
            st.items.erase(st.items.begin() + st.selected);
            st.selected = -1;
            st.editedIndex = -1;
            st.dragging = -1;
            st.dragMode = 0;
            ImGui::End();
            return AnnotationsEdit_Deleted;
        }


        ImGui::TextColored(Theme::TextDisabled,
                           "Salvamento automático · Esc fecha");
        ImGui::End();
        return AnnotationsEdit_None;
    }

    std::string AnnotationsToText(const AnnotationState& st, const std::string& globalComment)
    {
        std::string out = "SEEDUI — DIRETRIZES DE ALTERAÇÃO (modo debug)\n";
        out += "=============================================\n\n";
        out += "COMENTÁRIO GERAL DO PROGRAMA:\n";
        out += globalComment.empty() ? std::string("(vazio)\n") : globalComment + "\n";
        out += "\nANOTAÇÕES SOBRE ÁREAS DA INTERFACE (coordenadas da janela do SeedUI):\n";
        if (st.items.empty())
        {
            out += "(nenhuma)\n";
        }
        else
        {
            for (const Annotation& a : st.items)
            {
                char line[1024];
                snprintf(line, sizeof line,
                         "[%d] área (%.0f, %.0f) %.0f x %.0f: %s\n",
                         a.id, a.min.x, a.min.y, a.max.x - a.min.x, a.max.y - a.min.y,
                         a.text.empty() ? "(sem texto)" : a.text.c_str());
                out += line;
            }
        }
        return out;
    }
}
