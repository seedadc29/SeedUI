// SeedNotas — ferramenta standalone de anotações de tela EM TEMPO REAL.
//
// Dois modos:
//   1) BARRA FLUTUANTE: janela pequena, sem moldura, sempre no topo
//      (FLAG_WINDOW_UNDECORATED + FLAG_WINDOW_TOPMOST). Fica sobre qualquer
//      programa; arraste para posicionar.
//   2) OVERLAY AO VIVO: clicou em "Anotar", a janela expande para a tela
//      inteira e fica TRANSPARENTE — o desktop/programas aparecem AO VIVO
//      por trás (sem tirar print). As anotações são desenhadas por cima em
//      tempo real. Esc / "Voltar" retorna à barra flutuante.
//
// "Print / Salvar": captura o desktop atual + as anotações, abre o diálogo
// nativo do Windows (Salvar como) para o usuário escolher a pasta, e grava
// um PNG anotado + arquivo .txt de diretrizes ao lado.
//
// Reutiliza do SeedUI (via caminho relativo, sem duplicar código):
//   - Annotations.h/.cpp  (o mesmo sistema de anotação do modo debug)
//   - Theme.h/.cpp         (o mesmo tema premium escuro)
//   - ThirdParty (raylib + imgui + backends)

#pragma comment(linker, "/ENTRY:mainCRTStartup")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Dwmapi.lib")
extern "C"
{
    __declspec(dllimport) void* ShellExecuteA(void* hwnd, const char* operation,
                                              const char* file, const char* parameters,
                                              const char* directory, int showCmd);
    __declspec(dllimport) int DwmSetWindowAttribute(void* hwnd, unsigned int attribute,
                                                    const void* value, unsigned int valueSize);
    __declspec(dllimport) unsigned long GetModuleFileNameA(void* hModule,
                                                           char* lpFilename,
                                                           unsigned long nSize);
    // Mostra/esconde a janela (0 = esconder, 5 = mostrar) — usada para
    // capturar o desktop limpo no momento do print.
    __declspec(dllimport) int ShowWindow(void* hwnd, int nCmdShow);
    // Posição GLOBAL do cursor (para arrastar a barra sem tremor).
    struct SNPoint { long x, y; };
    __declspec(dllimport) int GetCursorPos(SNPoint* lpPoint);
    __declspec(dllimport) void Sleep(unsigned long milliseconds);
    // Estado REAL do botão do mouse (não depende da fila de eventos do ImGui,
    // então o arrasto nunca fica "preso" se o clique terminar fora da janela).
    __declspec(dllimport) short GetAsyncKeyState(int vKey);
}

#include <direct.h> // _mkdir

#include "Annotations.h"
#include "Theme.h"
#include "AnnotIcons.h"
#include "SaveDialog.h"
#include "ScreenCapture.h"

#include "raylib.h"
#include "imgui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace seednotas
{
    namespace
    {
        enum class Mode
        {
            FloatingBar,  // barra pequena sempre no topo
            Overlay,      // tela cheia transparente, anotação AO VIVO
        };

        constexpr int kBarW = 620;
        constexpr int kBarH = 44;

        Mode gMode = Mode::FloatingBar;
        bool gQuit = false;

        // Barra recolhida ("minimizada") — vira uma pílula compacta na tela.
        bool gBarCollapsed = false;
        constexpr int kPillW = 150;
        constexpr int kPillH = 44;

        // Posição atual da barra (persistida entre as transições de modo)
        int gBarX = 0;
        int gBarY = 0;

        // Largura REAL da barra: acompanha o conteúdo (sem espaço vazio).
        int gBarW = kBarW;
        int gPillW = kPillW;

        // Tamanho do monitor usado pelo overlay (definido no EnterOverlay).
        int gOverlayW = 0;
        int gOverlayH = 0;

        // Encolhe a janela atual até a largura do último item desenhado
        // (+ margem), removendo o espaço sem informação. O CENTRO horizontal
        // fica fixo durante o redimensionamento — a barra não "anda" para
        // os lados enquanto cresce/encolhe.
        void FitBarWidth(int maxHeight)
        {
            const float contentEnd = ImGui::GetItemRectMax().x -
                                     ImGui::GetWindowPos().x;
            const int need = (int)(contentEnd + 14.0f);
            const int cur = GetScreenWidth();
            if (std::abs(need - cur) <= 1) return; // sem oscilação
            // Posição REAL da janela (GLFW/raylib) — o ImGui::GetWindowPos()
            // pode reportar (0,0) no primeiro frame e jogar a barra fora do
            // centro. Mantém o CENTRO horizontal fixo ao redimensionar.
            const Vector2 pos = GetWindowPosition();
            const int centerX = (int)pos.x + cur / 2;
            gBarX = centerX - need / 2;
            gBarY = (int)pos.y;
            SetWindowSize(need, maxHeight);
            SetWindowPosition(gBarX, gBarY);
        }

        // Arrasto da barra (posição absoluta global — sem tremor)
        bool gBarDrag = false;
        SNPoint gDragStartCursor = { 0, 0 };
        Vector2 gDragStartWin = { 0, 0 };

        seedui::AnnotationState gAnnot;
        char gGlobalComment[4096] = { 0 };

        // A captura/print é gerada apenas no momento de SALVAR.
        bool gPendingPrint = false;

        // Modo de autoteste do print: salva num caminho fixo, sem diálogo.
        bool gExportTest = false;

        // Região da barra de ferramentas do overlay: anotações não devem ser
        // criadas sobre ela (o AnnotationsUpdate captura cliques em toda a janela).
        ImVec2 gToolbarMin(0, 0);
        ImVec2 gToolbarMax(0, 0);

        // Fonte para os rótulos do PNG exportado
        Font gExportFont = { 0 };

        // Seletor de ícones DENTRO da janela de edição da anotação (hook
        // registrado no AnnotationState). Grade com os ícones preenchidos da
        // pasta tabler-icons + escolha da posição no rótulo.
        void AnnotIconPickerUI(seedui::Annotation& a)
        {
            ImGui::TextColored(seedui::Theme::TextDisabled,
                               "Ícone da anotação (tipo da nota):");
            const float size = 34.0f;
            const int perRow = 8;
            const int total = AnnotIconCount() + 1; // 0 = nenhum

            for (int i = 0; i < total; ++i)
            {
                if (i > 0 && (i % perRow) != 0) ImGui::SameLine();
                const bool selected = (a.icon == i);
                ImVec2 p0(0, 0), p1(0, 0);
                bool clicked = false;
                if (i == 0)
                {
                    // Botão "sem ícone" (×)
                    clicked = ImGui::Button("##ic_none", ImVec2(size, size));
                    p0 = ImGui::GetItemRectMin();
                    p1 = ImGui::GetItemRectMax();
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    const ImVec2 c((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
                    const float r = size * 0.24f;
                    dl->AddLine(ImVec2(c.x - r, c.y - r),
                                ImVec2(c.x + r, c.y + r),
                                IM_COL32(205, 205, 205, 255), 2.0f);
                    dl->AddLine(ImVec2(c.x + r, c.y - r),
                                ImVec2(c.x - r, c.y + r),
                                IM_COL32(205, 205, 205, 255), 2.0f);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Sem ícone");
                }
                else
                {
                    char tip[96];
                    snprintf(tip, sizeof tip, "%s (%s)",
                             AnnotIconName(i), AnnotIconFileName(i));
                    clicked = AnnotIconImageButton(i, size,
                                                   IM_COL32(228, 228, 228, 255),
                                                   tip);
                    p0 = ImGui::GetItemRectMin();
                    p1 = ImGui::GetItemRectMax();
                }
                if (selected)
                {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddRect(ImVec2(p0.x - 1.0f, p0.y - 1.0f),
                                ImVec2(p1.x + 1.0f, p1.y + 1.0f),
                                a.color, 4.0f, 0, 2.0f);
                }
                if (clicked) a.icon = i;
            }

            ImGui::TextColored(seedui::Theme::TextDisabled,
                               "Posição no rótulo:");
            const char* posLabels[] = { "Sup. dir.", "Inf. dir.", "Inf. esq." };
            for (int p = 0; p < 3; ++p)
            {
                if (p > 0) ImGui::SameLine();
                const bool active = a.iconPos == p;
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, a.color);
                if (ImGui::Button(posLabels[p])) a.iconPos = p;
                if (active) ImGui::PopStyleColor();
            }
        }

        void EnableDarkTitleBar()
        {
            void* hwnd = GetWindowHandle();
            if (!hwnd) return;
            const int enabled = 1;
            DwmSetWindowAttribute(hwnd, 20, &enabled, sizeof enabled);
            DwmSetWindowAttribute(hwnd, 19, &enabled, sizeof enabled);
            const unsigned int dark = 0x001e1e1e;
            DwmSetWindowAttribute(hwnd, 35, &dark, sizeof dark);
            DwmSetWindowAttribute(hwnd, 36, &dark, sizeof dark);
        }

        void ClearAll()
        {
            gAnnot.items.clear();
            gAnnot.selected = -1;
            TraceLog(LOG_INFO, "SeedNotas: anotações limpas");
        }

        void EnterOverlay()
        {
            TraceLog(LOG_INFO, "SeedNotas: ENTER OVERLAY");
            // Expande para a tela inteira (coordenadas 1:1 com o desktop)
            const GLFWvidmode* m = glfwGetVideoMode(glfwGetPrimaryMonitor());
            const int mw = m ? m->width : 1600;
            const int mh = m ? m->height : 900;
            SetWindowPosition(0, 0);
            SetWindowSize(mw, mh);
            gMode = Mode::Overlay;
            gOverlayW = mw;
            gOverlayH = mh;
            // Traz a janela para a FRENTE e dá o FOCO. Sem isto, o primeiro
            // clique depois do F5 pode cair no programa por baixo (ex.:
            // seleciona o texto do Freebuff) em vez de criar a anotação.
            SetWindowFocused();
        }

        void LeaveOverlay()
        {
            gMode = Mode::FloatingBar;
            SetWindowPosition(gBarX, gBarY);
            SetWindowSize(gBarW, kBarH);
        }

        std::vector<std::string> WrapText(const std::string& text, float maxWidth,
                                          const Font& font, float fontSize)
        {
            std::vector<std::string> lines;
            std::string current;
            const char* p = text.c_str();
            while (*p)
            {
                while (*p && *p != '\n')
                {
                    std::string next = current + *p;
                    float w = MeasureTextEx(font, next.c_str(), fontSize, 0.0f).x;
                    if (w > maxWidth && !current.empty())
                    {
                        lines.push_back(current);
                        current.clear();
                    }
                    current += *p;
                    ++p;
                }
                if (*p == '\n')
                {
                    lines.push_back(current);
                    current.clear();
                    ++p;
                }
            }
            if (!current.empty()) lines.push_back(current);
            if (lines.empty()) lines.push_back("");
            return lines;
        }

        // Desenha uma anotação sobre a imagem capturada (para o PNG final).
        // No modo overlay, as coordenadas das anotações SÃO coordenadas da
        // tela (a janela cobre a tela toda em (0,0)), então a transformação
        // é identidade. scale: 1.0 = tamanho real da tela; 2.0 = alta
        // resolução (o PNG sai em dobro, com texto maior e nítido).
        void DrawAnnotationOnImage(const seedui::Annotation& a, int imgW, int imgH,
                                   float scale)
        {
            const float x0 = a.min.x * scale;
            const float y0 = a.min.y * scale;
            const float x1 = a.max.x * scale;
            const float y1 = a.max.y * scale;
            const float lx = a.label.x * scale;
            const float ly = a.label.y * scale;

            const Color col = {
                (unsigned char)((a.color >> 16) & 0xFF),
                (unsigned char)((a.color >> 8) & 0xFF),
                (unsigned char)(a.color & 0xFF),
                (unsigned char)((a.color >> 24) & 0xFF)
            };
            const Color fill = { col.r, col.g, col.b, 40 };
            const Color dark = { 43, 43, 43, 245 };
            const Color white = { 236, 236, 236, 255 };

            const float fw = x1 - x0;
            const float fh = y1 - y0;
            const float lineW = 2.0f * scale;

            // Área selecionada
            DrawRectangle((int)x0, (int)y0, (int)fw, (int)fh, fill);
            DrawRectangleLinesEx(Rectangle{ x0, y0, fw, fh }, lineW, col);

            // Seta do rótulo até a área
            const float sx = (x0 + x1) * 0.5f;
            const float sy = y0;
            const float ex = lx + 120.0f * scale;
            const float ey = ly + 20.0f * scale;
            DrawLine((int)sx, (int)sy, (int)ex, (int)ey, col);

            // Rótulo (largura redimensionável do usuário; mín. 100px)
            const float labelW = std::max(100.0f, a.labelW) * scale;
            // Fonte do texto: respeita o tamanho escolhido na anotação,
            // com mínimo de 15 (o visual atual do print é preservado).
            const float userFs = (a.fontSize > 0.0f ? a.fontSize : 15.0f);
            const float fs = std::max(15.0f, userFs) * scale;
            const float lineH = fs + 3.0f * scale;
            // Rótulo com ALTURA DINÂMICA: cresce conforme o texto, para a
            // informação final nunca ficar cortada no print.
            std::string body = a.text.empty() ? "(sem texto)" : a.text;
            std::vector<std::string> lines = WrapText(body, labelW - 16.0f * scale,
                                                      gExportFont, fs);
            // Teto de segurança: o rótulo nunca sai da imagem. Se o texto for
            // muito longo, mostra o que cabe + "…" (o .txt ao lado do PNG
            // sempre tem o texto completo).
            const int maxLines = std::max(1,
                (int)((imgH - ly - 44.0f * scale) / lineH));
            int shown = (int)lines.size();
            if (shown > maxLines) shown = maxLines;
            float labelH = 34.0f * scale + (float)shown * lineH +
                           10.0f * scale;
            // Altura manual mínima escolhida pelo usuário (0 = automática)
            if (a.labelH > 0.0f)
                labelH = std::max(labelH, a.labelH * scale);
            DrawRectangle((int)lx, (int)ly, (int)labelW, (int)labelH, dark);
            DrawRectangleLinesEx(Rectangle{ lx, ly, labelW, labelH }, lineW, col);

            char tb[64];
            snprintf(tb, sizeof tb, "Anotacao #%d", a.id);
            DrawTextEx(gExportFont, tb, Vector2{ lx + 8.0f * scale, ly + 6.0f * scale },
                       fs, 0.0f, col);

            float ty = ly + 6.0f * scale + fs + 6.0f * scale;
            for (int i = 0; i < shown; ++i)
            {
                DrawTextEx(gExportFont, lines[i].c_str(),
                           Vector2{ lx + 8.0f * scale, ty }, fs, 0.0f, white);
                ty += lineH;
            }
            if (shown < (int)lines.size())
                DrawTextEx(gExportFont, "...", Vector2{ lx + 8.0f * scale, ty },
                           fs, 0.0f, col);

            const float badgeR = 18.0f * scale;
            const float bx = x0 + badgeR;
            const float by = y0 + badgeR;
            DrawCircle((int)bx, (int)by, badgeR, col);
            char nb[16];
            snprintf(nb, sizeof nb, "%d", a.id);
            Vector2 ts = MeasureTextEx(gExportFont, nb, fs, 0.0f);
            DrawTextEx(gExportFont, nb, Vector2{ bx - ts.x * 0.5f, by - ts.y * 0.5f },
                       fs, 0.0f, Color{ 25, 25, 25, 255 });

            // Ícone da anotação no canto do rótulo (tingido com a cor)
            if (a.icon >= 1 && a.icon <= AnnotIconCount())
            {
                const Texture2D itex = AnnotIconTexture(a.icon);
                if (itex.id != 0)
                {
                    const float iconS = 22.0f * scale;
                    const float mg = 6.0f * scale;
                    Vector2 ip;
                    if (a.iconPos == seedui::AnnotationsIconPos_TopRight)
                        ip = Vector2{ lx + labelW - iconS - mg, ly + mg };
                    else if (a.iconPos == seedui::AnnotationsIconPos_BottomRight)
                        ip = Vector2{ lx + labelW - iconS - mg,
                                      ly + labelH - iconS - mg };
                    else
                        ip = Vector2{ lx + mg, ly + labelH - iconS - mg };
                    const float s = iconS / (float)itex.width;
                    DrawTextureEx(itex, ip, 0.0f, s, col);
                }
            }
        }

        // Captura o desktop + desenha as anotações por cima (PNG final).
        // O desktop é capturado com a nossa janela ESCONDIDA (desktop limpo);
        // as anotações são redesenhadas por cima com DrawAnnotationOnImage.
        void Export()
        {
            void* hwnd = GetWindowHandle();

            // 1) Esconde a janela e espera o DWM recompor o desktop limpo
            ShowWindow(hwnd, 0); // SW_HIDE
            Sleep(150);

            int w = 0, h = 0;
            unsigned char* pixels = nullptr;
            const bool ok = CaptureScreenRGBA(w, h, pixels);

            ShowWindow(hwnd, 5); // SW_SHOW

            if (!ok || !pixels)
            {
                TraceLog(LOG_WARNING, "SeedNotas: falha ao capturar a tela para o print");
                if (pixels) free(pixels);
                return;
            }

            // 2) Desenha as anotações por cima do desktop (RenderTexture)
            Image base = { 0 };
            base.data = pixels;
            base.width = w;
            base.height = h;
            base.mipmaps = 1;
            base.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            Texture2D baseTex = LoadTextureFromImage(base);
            free(pixels);

            // ALTA RESOLUÇÃO (2x): o PNG sai com o dobro dos pixels da tela
            // (2732x1536 numa tela 1366x768). O desktop é re-escalado e as
            // anotações são desenhadas em dobro — texto maior e nítido para
            // ler e para a IA processar.
            const float scale = 2.0f;
            const int outW = w * (int)scale;
            const int outH = h * (int)scale;
            RenderTexture2D rt = LoadRenderTexture(outW, outH);
            BeginTextureMode(rt);
            ClearBackground(BLACK);
            DrawTextureEx(baseTex, Vector2{ 0, 0 }, 0.0f, scale, WHITE);
            for (const seedui::Annotation& a : gAnnot.items)
                DrawAnnotationOnImage(a, outW, outH, scale);
            EndTextureMode();
            UnloadTexture(baseTex);

            Image out = LoadImageFromTexture(rt.texture);
            ImageFlipVertical(&out); // confirmado por teste: flip é necessário
            UnloadRenderTexture(rt);

            // 3) Diálogo nativo do Windows para escolher onde salvar
            std::time_t now = std::time(nullptr);
            std::tm tmv;
            localtime_s(&tmv, &now);
            char stamp[64];
            snprintf(stamp, sizeof stamp, "seednotas_%04d%02d%02d_%02d%02d%02d.png",
                     tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                     tmv.tm_hour, tmv.tm_min, tmv.tm_sec);

            std::string pngPath;
            if (gExportTest)
            {
                pngPath = "seednotas_export_test.png";
                TraceLog(LOG_INFO,
                         "SeedNotas: EXPORT TEST resolucao capturada=%dx%d "
                         "alta-resolucao=2x",
                         w, h);
            }
            else
            {
                pngPath = SalvarDialogoPNG(stamp, hwnd);
                if (pngPath.empty())
                {
                    UnloadImage(out);
                    TraceLog(LOG_INFO, "SeedNotas: salvamento cancelado pelo usuário");
                    return;
                }
            }
            if (pngPath.size() < 4 ||
                pngPath.compare(pngPath.size() - 4, 4, ".png") != 0)
                pngPath += ".png";

            ExportImage(out, pngPath.c_str());
            UnloadImage(out);

            // 4) .txt com as diretrizes ao lado do PNG
            std::string txtPath = pngPath.substr(0, pngPath.size() - 4) + ".txt";
            std::string txt;
            txt += "SEEDNOTAS — ANOTAÇÕES DE TELA\n";
            txt += "=============================\n\n";
            txt += "COMENTÁRIO GERAL:\n";
            txt += (gGlobalComment[0] ? gGlobalComment : "(vazio)");
            txt += "\n\n";
            txt += "Captura: " + std::to_string(w) + "x" +
                   std::to_string(h) + "px (tela do computador)\n";
            txt += "Coordenadas em percentual da imagem (x→direita, y→baixo).\n\n";
            if (gAnnot.items.empty())
            {
                txt += "(nenhuma anotação)\n";
            }
            else
            {
                for (const seedui::Annotation& a : gAnnot.items)
                {
                    const char* iconName = "nenhum";
                    if (a.icon >= 1 && a.icon <= AnnotIconCount())
                        iconName = AnnotIconName(a.icon);
                    char line[1024];
                    snprintf(line, sizeof line,
                             "[%d] icone=%s área (%.1f%%, %.1f%%) %.1f%% x %.1f%%: %s\n",
                             a.id, iconName,
                             a.min.x / (float)w * 100.0f,
                             a.min.y / (float)h * 100.0f,
                             (a.max.x - a.min.x) / (float)w * 100.0f,
                             (a.max.y - a.min.y) / (float)h * 100.0f,
                             a.text.empty() ? "(sem texto)" : a.text.c_str());
                    txt += line;
                }
            }

            FILE* f = fopen(txtPath.c_str(), "w");
            if (f)
            {
                fputs(txt.c_str(), f);
                fclose(f);
            }

            TraceLog(LOG_INFO, "SeedNotas: print salvo em %s (+ txt)",
                     pngPath.c_str());
        }

        // ---------- Modo OVERLAY (tela cheia transparente, AO VIVO) ----------

        void DrawOverlayToolbar()
        {
            if (ImGui::Button("Print (Ctrl+S)", ImVec2(120, 0)))
                gPendingPrint = true;
            ImGui::SameLine();
            if (ImGui::Button("Limpar", ImVec2(70, 0)))
                ClearAll();
            ImGui::SameLine();
            ImGui::Separator();
            ImGui::SameLine();
            if (ImGui::Button("Voltar (Esc)", ImVec2(100, 0)))
            {
                LeaveOverlay();
            }
            ImGui::SameLine();
            ImGui::Separator();
            ImGui::SameLine();
            ImGui::Text("Comentário geral:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 8.0f);
            ImGui::InputText("##comentario", gGlobalComment, sizeof gGlobalComment);

            gToolbarMin = ImGui::GetWindowPos();
            gToolbarMax = ImVec2(gToolbarMin.x + ImGui::GetWindowWidth(),
                                 gToolbarMin.y + ImGui::GetWindowHeight());
        }

        // ---------- Modo BARRA FLUTUANTE ----------

        // Barra recolhida: uma pílula compacta com "retornar" (+).
        void DrawCollapsedPill()
        {
            const ImGuiViewport* vp = ImGui::GetMainViewport();

            ImGui::SetNextWindowPos(vp->Pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(vp->Size, ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(30, 30, 32, 250));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(79, 140, 255, 200));
            ImGui::Begin("##seednotas_pill", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoScrollbar);

            // Centraliza os itens no eixo vertical da barra
            ImGui::SetCursorPosY(std::max(0.0f,
                (ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeight()) * 0.5f));

            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(seedui::Theme::AccentBlue, "SeedNotas");
            ImGui::SameLine();
            if (!gAnnot.items.empty())
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(seedui::Theme::Warning, " %d",
                                   (int)gAnnot.items.size());
                const ImVec2 r0 = ImGui::GetItemRectMin();
                const ImVec2 r1 = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddCircleFilled(
                    ImVec2(r0.x + 5.0f, (r0.y + r1.y) * 0.5f), 3.5f,
                    ImGui::ColorConvertFloat4ToU32(seedui::Theme::Warning));
            }
            ImGui::SameLine();
            if (ImGui::Button("+", ImVec2(34, 0)))
            {
                // Expande de volta para a barra completa (mantém na tela).
                const GLFWvidmode* m = glfwGetVideoMode(glfwGetPrimaryMonitor());
                const int mw = m ? m->width : 1600;
                gBarCollapsed = false;
                gBarX = std::max(0, std::min(gBarX, mw - gBarW));
                SetWindowPosition(gBarX, gBarY);
                SetWindowSize(gBarW, kBarH);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Retornar a barra completa");

            // Pílula compacta: encolhe ao conteúdo (sem espaço vazio)
            gPillW = GetScreenWidth();
            FitBarWidth(kPillH);

            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);

            // Arrasto da pílula (mesma lógica da barra, sem tremor).
            const bool leftDown = (GetAsyncKeyState(0x01) & 0x8000) != 0;
            const bool overPill = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
            const bool onWidget = ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered();
            if (!gBarDrag)
            {
                if (leftDown && overPill && !onWidget)
                {
                    gBarDrag = true;
                    GetCursorPos(&gDragStartCursor);
                    gDragStartWin = GetWindowPosition();
                }
            }
            else
            {
                if (!leftDown)
                {
                    gBarDrag = false;
                }
                else
                {
                    SNPoint cur = { 0, 0 };
                    GetCursorPos(&cur);
                    int nx = (int)gDragStartWin.x + (cur.x - gDragStartCursor.x);
                    int ny = (int)gDragStartWin.y + (cur.y - gDragStartCursor.y);
                    const GLFWvidmode* m = glfwGetVideoMode(glfwGetPrimaryMonitor());
                    const int mw = m ? m->width : 1600;
                    const int mh = m ? m->height : 900;
                    const int w = GetScreenWidth();
                    const int h = GetScreenHeight();
                    nx = std::max(-w + 48, std::min(nx, mw - 48));
                    ny = std::max(-h + 48, std::min(ny, mh - 48));
                    SetWindowPosition(nx, ny);
                    gBarX = nx;
                    gBarY = ny;
                }
            }
        }

        void DrawFloatingBar()
        {
            const ImGuiViewport* vp = ImGui::GetMainViewport();

            ImGui::SetNextWindowPos(vp->Pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(vp->Size, ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(30, 30, 32, 250));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(79, 140, 255, 200));
            ImGui::Begin("##seednotas_bar", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoScrollbar);

            // Centraliza os itens no eixo vertical da barra
            ImGui::SetCursorPosY(std::max(0.0f,
                (ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeight()) * 0.5f));

            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(seedui::Theme::AccentBlue, "SeedNotas");
            ImGui::SameLine();
            ImGui::Separator();
            ImGui::SameLine();

            if (ImGui::Button("Anotar (F5)", ImVec2(110, 0)))
            {
                EnterOverlay();
            }
            ImGui::SameLine();
            if (ImGui::Button("Print", ImVec2(70, 0)))
                gPendingPrint = true;
            ImGui::SameLine();
            if (ImGui::Button("Limpar", ImVec2(70, 0)))
                ClearAll();
            ImGui::SameLine();
            if (ImGui::Button("Sair", ImVec2(60, 0)))
                gQuit = true;

            ImGui::SameLine();
            if (!gAnnot.items.empty())
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(seedui::Theme::Warning, " %d",
                                   (int)gAnnot.items.size());
                const ImVec2 r0 = ImGui::GetItemRectMin();
                const ImVec2 r1 = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddCircleFilled(
                    ImVec2(r0.x + 5.0f, (r0.y + r1.y) * 0.5f), 3.5f,
                    ImGui::ColorConvertFloat4ToU32(seedui::Theme::Warning));
            }
            else
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("-");
            }

            ImGui::SameLine();
            if (ImGui::Button("-", ImVec2(34, 0)))
            {
                gBarCollapsed = true;
                SetWindowSize(gPillW, kPillH);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Minimizar a barra");

            // Barra compacta: encolhe ao conteúdo (sem espaço vazio)
            gBarW = GetScreenWidth();
            FitBarWidth(kBarH);

            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);

            // Arrastar a barra com posição ABSOLUTA global (sem tremor):
            // novaPos = posiçãoInicial + (cursorGlobal - cursorInicialGlobal).
            // O pressionar/soltar é lido direto do sistema (GetAsyncKeyState) —
            // se o usuário soltar o botão fora da janela ou a janela perder o
            // foco, o arrasto NUNCA fica preso.
            const bool leftDown = (GetAsyncKeyState(0x01) & 0x8000) != 0;
            const bool overBar = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
            const bool onWidget = ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered();
            if (!gBarDrag)
            {
                if (leftDown && overBar && !onWidget)
                {
                    gBarDrag = true;
                    GetCursorPos(&gDragStartCursor);
                    gDragStartWin = GetWindowPosition();
                }
            }
            else
            {
                if (!leftDown)
                {
                    gBarDrag = false;
                }
                else
                {
                    SNPoint cur = { 0, 0 };
                    GetCursorPos(&cur);
                    int nx = (int)gDragStartWin.x + (cur.x - gDragStartCursor.x);
                    int ny = (int)gDragStartWin.y + (cur.y - gDragStartCursor.y);
                    // Mantém pelo menos 48px da barra visíveis no monitor atual
                    // (nunca deixa a barra "sumir" para fora da tela).
                    const GLFWvidmode* m = glfwGetVideoMode(glfwGetPrimaryMonitor());
                    const int mw = m ? m->width : 1600;
                    const int mh = m ? m->height : 900;
                    nx = std::max(-gBarW + 48, std::min(nx, mw - 48));
                    ny = std::max(-kBarH + 48, std::min(ny, mh - 48));
                    SetWindowPosition(nx, ny);
                    gBarX = nx;
                    gBarY = ny;
                }
            }
        }
    }

    void TraceToFile(int logLevel, const char* text, va_list args)
    {
        static FILE* f = nullptr;
        if (!f) f = fopen("seednotas_dbg.log", "w");
        char buf[2048];
        vsnprintf(buf, sizeof buf, text, args);
        if (f) { fprintf(f, "%s\r\n", buf); fflush(f); }
        (void)logLevel;
    }

    int Run(int argc, char** argv)
    {
        bool exportTest = false;
        for (int i = 1; i < argc; ++i)
            if (std::strcmp(argv[i], "--export-test") == 0) exportTest = true;

        SetTraceLogCallback(TraceToFile);
        glfwInit();
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        const int monitorW = mode ? mode->width : 1600;
        const int monitorH = mode ? mode->height : 900;

        SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_TRANSPARENT |
                       FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
        InitWindow(kBarW, kBarH, "SeedNotas");
        // A barra tem tamanho fixo (não é redimensionável pelo usuário).
        // Com o TRANSPARENT ativo, alguns drivers criam a janela no tamanho
        // do monitor: garante-se o tamanho da barra logo após a criação.
        SetWindowSize(kBarW, kBarH);
        TraceLog(LOG_INFO, "SeedNotas: apos init janela=%dx%d",
                 GetScreenWidth(), GetScreenHeight());
        EnableDarkTitleBar();
        gBarX = (monitorW - kBarW) / 2;
        gBarY = 24;
        SetWindowPosition(gBarX, gBarY);
        SetExitKey(0);
        SetTargetFPS(60);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        seedui::Theme::Apply();
        seedui::Theme::LoadFonts();

        // Fonte de ALTA RESOLUÇÃO para o TEXTO do rótulo no overlay: atlas de
        // 64px, escalada PARA BAIXO ao desenhar (8..64px). Como o desenho
        // sempre é uma redução, o texto ampliado sai nítido — sem pixelado.
        // Medição de altura e desenho usam a mesma fonte (via
        // AnnotationsSetHighResFont).
        {
            const char* candidates[] = {
                "assets/fonts/Inter.ttf",
                "../SeedUI/assets/fonts/Inter.ttf",
                "../../SeedUI/assets/fonts/Inter.ttf",
            };
            for (const char* p : candidates)
            {
                if (!FileExists(p)) continue;
                ImFont* hf = ImGui::GetIO().Fonts->AddFontFromFileTTF(
                    p, 64.0f, nullptr, ImGui::GetIO().Fonts->GetGlyphRangesDefault());
                if (hf)
                {
                    seedui::AnnotationsSetHighResFont(hf);
                    TraceLog(LOG_INFO, "Fonte alta-resolucao do rotulo: %s", p);
                }
                break;
            }
        }

        ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
        ImGui_ImplOpenGL3_Init("#version 330");

        {
            // Fonte com atlas de ALTA resolução (64px): os glifos são gerados
            // grandes e escalados para baixo ao desenhar no print — resultado
            // nítido em QUALQUER tamanho de fonte (8..64px), sem serrilhado.
            int cp[224];
            for (int i = 0; i < 224; ++i) cp[i] = 32 + i;
            if (FileExists("assets/fonts/Inter.ttf"))
                gExportFont = LoadFontEx("assets/fonts/Inter.ttf", 64, cp, 224);
            else if (FileExists("../SeedUI/assets/fonts/Inter.ttf"))
                gExportFont = LoadFontEx("../SeedUI/assets/fonts/Inter.ttf", 64, cp, 224);
            else if (FileExists("../../SeedUI/assets/fonts/Inter.ttf"))
                gExportFont = LoadFontEx("../../SeedUI/assets/fonts/Inter.ttf", 64, cp, 224);
            if (gExportFont.texture.id == 0) gExportFont = GetFontDefault();
        }

        // Catálogo de ícones de anotação (tabler-icons preenchidos) + seletor
        // no popup de edição.
        AnnotIconsLoad();
        gAnnot.iconPicker = AnnotIconPickerUI;

        // Autoteste do PRINT: captura a tela e exporta o PNG anotado para
        // um caminho fixo, SEM o diálogo "Salvar como" — permite medir a
        // resolução real do arquivo gerado (diagnóstico de "print baixo").
        if (exportTest)
        {
            gExportTest = true;
            seedui::Annotation a;
            a.id = 1;
            a.min = ImVec2(300, 200);
            a.max = ImVec2(600, 320);
            a.label = ImVec2(620, 200);
            a.text = "Texto de teste longo para medir a resolucao do print";
            a.color = 0xFFC4F13E;
            a.icon = AnnotIcon_AlertTriangle;
            a.fontSize = 22.0f;
            a.iconPos = seedui::AnnotationsIconPos_BottomRight;
            gAnnot.items.push_back(a);
            gGlobalComment[0] = 'T';
            gGlobalComment[1] = 'e';
            gGlobalComment[2] = 's';
            gGlobalComment[3] = 't';
            gGlobalComment[4] = 0;
            Export();
            CloseWindow();
            return 0;
        }

        // Reafirma o estado da janela no modo overlay: o resize/foco do
        // EnterOverlay pode ser atrasado pelo gerenciador de janelas, e nesse
        // intervalo o clique do usuário cai no programa por baixo (ex.:
        // seleciona o texto do Freebuff). Reaplicar periodicamente garante que
        // o overlay está em tela cheia, no topo e com foco.
        int sOverlayGuardFrame = 0;
        while (!WindowShouldClose() && !gQuit)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            if (gMode == Mode::Overlay && gOverlayW > 0 &&
                (sOverlayGuardFrame++ % 15) == 0)
            {
                if ((int)GetScreenWidth() != gOverlayW ||
                    (int)GetScreenHeight() != gOverlayH ||
                    (int)GetWindowPosition().x != 0 ||
                    (int)GetWindowPosition().y != 0)
                {
                    SetWindowPosition(0, 0);
                    SetWindowSize(gOverlayW, gOverlayH);
                }
                SetWindowFocused();
            }

            const ImGuiViewport* vp = ImGui::GetMainViewport();

            if (gMode == Mode::FloatingBar)
            {
                if (gBarCollapsed) DrawCollapsedPill();
                else DrawFloatingBar();

                // F5 independente de FOCo: lê o estado GLOBAL da tecla (como o
                // arrasto da barra já faz). Se outra janela estiver com foco
                // (a barra é sempre-no-topo mas pode não ter o teclado), o
                // ImGui não recebe a tecla — este caminho funciona sempre.
                static bool sF5Down = false;
                const bool f5Down = (GetAsyncKeyState(0x74) & 0x8000) != 0;
                if (f5Down && !sF5Down) EnterOverlay();
                sF5Down = f5Down;
            }
            else
            {
                ImGui::SetNextWindowPos(vp->Pos, ImGuiCond_Always);
                ImGui::SetNextWindowSize(vp->Size, ImGuiCond_Always);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
                ImGui::Begin("##seednotas_overlay", nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoScrollbar);

                // Barra de ferramentas compacta no topo (fundo opaco — a janela
                // pai é transparente para o desktop aparecer ao vivo)
                ImGui::SetCursorPos(ImVec2(8, 8));
                ImGui::PushStyleColor(ImGuiCol_ChildBg,
                                      IM_COL32(30, 30, 32, 248));
                ImGui::BeginChild("##overtoolbar", ImVec2(vp->Size.x - 16, 34),
                                  true, ImGuiWindowFlags_NoScrollbar);
                DrawOverlayToolbar();
                ImGui::EndChild();
                ImGui::PopStyleColor();

                // O resto da janela é transparente: o desktop aparece AO VIVO.
                // Um botão invisível garante que o clique chegue ao overlay.
                ImGui::SetCursorPos(ImVec2(0, 50));
                ImGui::InvisibleButton("##overlay_bg",
                                       ImVec2(vp->Size.x, vp->Size.y - 50));

                ImGui::End();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(2);

                const ImVec2 mousePos = ImGui::GetMousePos();
                const bool overToolbar = mousePos.x >= gToolbarMin.x &&
                                         mousePos.x <= gToolbarMax.x &&
                                         mousePos.y >= gToolbarMin.y &&
                                         mousePos.y <= gToolbarMax.y;
                seedui::AnnotationsUpdate(gAnnot, !overToolbar, vp->Size);

                // O rótulo NÃO desenha por cima da caixa de digitação: quando
                // o popup de edição está aberto, as anotações que ficariam
                // sobre ele são puladas (mesmo comportamento do SeedUI).
                // Durante o ARRASTO a caixa de diálogo nem aparece — então o
                // recorte fica DESLIGADO para a anotação nunca sumir.
                bool clipPopup = false;
                ImVec2 popupMin(0, 0), popupMax(0, 0);
                if (gAnnot.selected >= 0 && gAnnot.dragging < 0 &&
                    !gAnnot.creating &&
                    gAnnot.selected < (int)gAnnot.items.size())
                {
                    const seedui::AnnotationsPopupRect pr =
                        seedui::AnnotationsPopupRectFor(
                            gAnnot, gAnnot.items[gAnnot.selected], vp->Size);
                    popupMin = pr.min;
                    popupMax = pr.max;
                    clipPopup = true;
                }
                seedui::AnnotationsDraw(gAnnot, clipPopup, popupMin, popupMax);

                // Ícones das anotações no canto do rótulo (tingidos com a cor)
                for (const seedui::Annotation& a : gAnnot.items)
                {
                    if (a.icon < 1 || a.icon > AnnotIconCount()) continue;
                    // Não desenha o ícone sobre a caixa de digitação aberta
                    if (clipPopup)
                    {
                        const float lw = std::max(100.0f, a.labelW);
                        const float lh = seedui::AnnotationsLabelHeight(a);
                        if (a.label.x < popupMax.x &&
                            a.label.x + lw > popupMin.x &&
                            a.label.y < popupMax.y &&
                            a.label.y + lh > popupMin.y)
                            continue;
                    }
                    const float lw = std::max(100.0f, a.labelW);
                    const float lh = seedui::AnnotationsLabelHeight(a);
                    const float iconS = 22.0f;
                    const float mg = 5.0f;
                    ImVec2 pos;
                    if (a.iconPos == seedui::AnnotationsIconPos_TopRight)
                        pos = ImVec2(a.label.x + lw - iconS - mg,
                                     a.label.y + mg);
                    else if (a.iconPos == seedui::AnnotationsIconPos_BottomRight)
                        pos = ImVec2(a.label.x + lw - iconS - mg,
                                     a.label.y + lh - iconS - mg);
                    else
                        pos = ImVec2(a.label.x + mg,
                                     a.label.y + lh - iconS - mg);
                    AnnotIconDrawOverlay(a.icon, pos, iconS, a.color);
                }

                seedui::AnnotationsEditWindow(gAnnot);

                if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl)
                    gPendingPrint = true;
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    LeaveOverlay();
                }
            }

            ImGui::Render();
            BeginDrawing();
            Color blank = { 0, 0, 0, 0 };
            ClearBackground(blank);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            EndDrawing();

            // Print: captura o desktop (janela escondida) + compõe anotações
            if (gPendingPrint)
            {
                gPendingPrint = false;
                Export();
            }
        }

        if (gExportFont.texture.id != 0) UnloadFont(gExportFont);
        AnnotIconsUnload();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        CloseWindow();
        return 0;
    }
}

int main(int argc, char** argv)
{
    return seednotas::Run(argc, argv);
}
