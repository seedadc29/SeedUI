// Entry point: subsistema Windows sem console, mas com main(argc, argv)
// funcional (o CRT monta os argumentos reais da linha de comando).
#pragma comment(linker, "/ENTRY:mainCRTStartup")

#include <cstring>
#include <fstream>
#include <string>

#include "App.h"
#include "Geo.h"
#include "Project.h"
#include "SmartGuides.h"
#include "AlignUtils.h"

namespace
{
    int RunM04SelfTest()
    {
        using namespace seedui;
        std::ofstream report("m04-self-test.txt", std::ios::trunc);
        int failures = 0;
        auto check = [&](bool condition, const char* description)
        {
            report << (condition ? "PASS " : "FAIL ") << description << '\n';
            if (!condition) ++failures;
        };
        auto makeElement = [](const char* id, const char* type)
        {
            Element e;
            e.id = id;
            e.nome = id;
            e.tipo = type;
            return e;
        };

        Project project;
        project.CriarNovo("Autoteste M04", 1280, 720);
        check(project.telas.size() == 1 && project.telas[0].modos.size() == 1,
              "novo projeto possui tela e modo padrao");
        Modo& mode = project.telas[0].modos[0];

        Element a = makeElement("a", "painel");
        Element x = makeElement("x", "texto");
        x.filhos.push_back(makeElement("y", "botao"));
        a.filhos.push_back(std::move(x));
        mode.raiz.push_back(std::move(a));
        mode.raiz.push_back(makeElement("b", "botao"));
        mode.raiz.push_back(makeElement("c", "slider"));

        check(Project::MoverElemento(mode, "c", -1), "reordena elemento na raiz");
        check(mode.raiz.size() == 3 && mode.raiz[1].id == "c" && mode.raiz[2].id == "b",
              "ordem da raiz foi atualizada");
        check(!Project::MoverElemento(mode, "a", -1), "recusa movimento alem do limite");
        check(Project::ReparentearElemento(mode, "b", "a"), "cria relacao pai e filho");
        check(Project::ResolverId(mode, "a")->filhos.back().id == "b",
              "filho aparece dentro do pai");
        check(!Project::ReparentearElemento(mode, "a", "y"),
              "impede ciclo ao mover pai para descendente");

        Element* b = Project::ResolverId(mode, "b");
        b->bloqueado = true;
        check(!Project::MoverElemento(mode, "b", -1), "bloqueio impede reordenar");
        check(!Project::ReparentearElemento(mode, "b", std::string()),
              "bloqueio impede reparentear");
        check(!Project::ExcluirElemento(mode, "b"), "bloqueio impede excluir");

        Element* aPtr = Project::ResolverId(mode, "a");
        aPtr->bloqueado = true;
        check(!Project::ReparentearElemento(mode, "c", "a"),
              "pai bloqueado recusa novo filho");
        aPtr->bloqueado = false;
        b->bloqueado = false;
        check(Project::ReparentearElemento(mode, "b", std::string()),
              "elemento desbloqueado volta para a raiz");

        Element* xPtr = Project::ResolverId(mode, "x");
        xPtr->filhos.push_back(makeElement("temporario", "texto"));
        check(Project::ExcluirElemento(mode, "temporario"), "exclui elemento aninhado");
        check(Project::ResolverId(mode, "temporario") == nullptr,
              "elemento aninhado saiu da arvore");

        b = Project::ResolverId(mode, "b");
        b->bloqueado = true;
        b->visivel = false;
        b->transformacao = { { "x", 321.0f }, { "y", 123.0f },
                            { "largura", 180.0f }, { "altura", 42.0f } };
        b->estilos["raio_quinas"] = {
            { "superior_esquerda", 4.0f }, { "superior_direita", 8.0f },
            { "inferior_direita", 12.0f }, { "inferior_esquerda", 16.0f }
        };

        Modo hitMode;
        Element base = makeElement("base", "painel");
        base.transformacao = { { "x", 10.0f }, { "y", 10.0f },
                              { "largura", 100.0f }, { "altura", 100.0f } };
        Element child = makeElement("child", "botao");
        child.bloqueado = true;
        child.transformacao = { { "x", 20.0f }, { "y", 20.0f },
                               { "largura", 30.0f }, { "altura", 30.0f } };
        base.filhos.push_back(std::move(child));
        Element overlay = makeElement("overlay", "texto");
        overlay.transformacao = { { "x", 15.0f }, { "y", 15.0f },
                                 { "largura", 40.0f }, { "altura", 40.0f } };
        hitMode.raiz.push_back(std::move(base));
        hitMode.raiz.push_back(std::move(overlay));
        check(Project::ElementoNoPonto(hitMode, 25.0f, 25.0f)->id == "overlay",
              "clique escolhe o elemento visualmente no topo");
        hitMode.raiz.back().visivel = false;
        check(Project::ElementoNoPonto(hitMode, 25.0f, 25.0f)->id == "child",
              "clique ignora ocultos e permite selecionar bloqueados");
        check(Project::ElementoNoPonto(hitMode, 500.0f, 500.0f) == nullptr,
              "clique fora dos elementos limpa a selecao");

        Project clipboardProject;
        clipboardProject.CriarNovo("Clipboard", 1280, 720);
        Modo& clipboardMode = clipboardProject.telas[0].modos[0];
        Element clipboardPanel = makeElement("painel_1", "painel");
        clipboardPanel.transformacao = { { "x", 10.0f }, { "y", 20.0f },
                                         { "largura", 320.0f }, { "altura", 160.0f } };
        clipboardPanel.estilos["raio_quinas"]["superior_esquerda"] = 24.0f;
        clipboardPanel.filhos.push_back(makeElement("texto_1", "texto"));
        clipboardMode.raiz.push_back(std::move(clipboardPanel));
        const std::vector<Element> clipboard = Project::CopiarElementos(
            clipboardMode, { "painel_1", "texto_1" });
        check(clipboard.size() == 1 && clipboard[0].filhos.size() == 1,
              "copiar selecao nao duplica filho quando o pai tambem esta selecionado");
        const std::vector<std::string> pasted = Project::ColarElementos(
            clipboardProject, clipboardMode, clipboard, 16.0f);
        Element* pastedPanel = pasted.empty()
            ? nullptr : Project::ResolverId(clipboardMode, pasted[0]);
        check(pastedPanel && pastedPanel->id != "painel_1" &&
              pastedPanel->transformacao.value("x", 0.0f) == 26.0f,
              "colar cria ID novo e desloca a copia no canvas");
        check(pastedPanel && pastedPanel->filhos.size() == 1 &&
              pastedPanel->filhos[0].id != "texto_1",
              "colar renomeia IDs dos descendentes");
        check(pastedPanel && pastedPanel->estilos["raio_quinas"].value(
                  "superior_esquerda", 0.0f) == 24.0f,
              "colar preserva estilos e raios de quina");

        // Duplicar (Ctrl+D): deslocamento independente X/Y com repetição do
        // último passo (estilo CorelDRAW).
        const std::vector<std::string> dup1 = Project::ColarElementosOffset(
            clipboardProject, clipboardMode, clipboard, 16.0f, 24.0f);
        Element* dup1Panel = dup1.empty()
            ? nullptr : Project::ResolverId(clipboardMode, dup1[0]);
        check(dup1Panel && dup1Panel->transformacao.value("x", 0.0f) == 26.0f &&
              dup1Panel->transformacao.value("y", 0.0f) == 44.0f,
              "colar com offset X/Y independente desloca nos dois eixos");
        const std::vector<std::string> dup2 = Project::ColarElementosOffset(
            clipboardProject, clipboardMode, clipboard, 26.0f, 44.0f);
        Element* dup2Panel = dup2.empty()
            ? nullptr : Project::ResolverId(clipboardMode, dup2[0]);
        check(dup2Panel && dup2Panel->transformacao.value("x", 0.0f) == 36.0f &&
              dup2Panel->transformacao.value("y", 0.0f) == 64.0f,
              "repetir o ultimo deslocamento acumula o passo (Ctrl+D x2)");

        // Espelhar (flip CorelDRAW): inverte a posição em torno do centro da
        // caixa conjunta e alterna o flag de geometria das formas vetoriais.
        Modo mirrorMode;
        Element mirrorA = makeElement("mirror_a", "poligono");
        mirrorA.transformacao = { { "x", 100.0f }, { "y", 100.0f },
                                  { "largura", 100.0f }, { "altura", 100.0f } };
        Element mirrorB = makeElement("mirror_b", "retangulo");
        mirrorB.transformacao = { { "x", 300.0f }, { "y", 100.0f },
                                  { "largura", 100.0f }, { "altura", 100.0f } };
        mirrorMode.raiz.push_back(std::move(mirrorA));
        mirrorMode.raiz.push_back(std::move(mirrorB));
        Project::EspelharElementos(mirrorMode, { "mirror_a", "mirror_b" }, true);
        Element* mA = Project::ResolverId(mirrorMode, "mirror_a");
        Element* mB = Project::ResolverId(mirrorMode, "mirror_b");
        // Centro X do conjunto = 250; A (100..200) vai para 300..400;
        // B (300..400) vai para 100..200.
        check(mA && mA->transformacao.value("x", 0.0f) == 300.0f &&
              mA->transformacao.value("espelhado_h", 0.0f) == 1.0f,
              "espelhar H inverte posicao em torno do centro do grupo");
        check(mB && mB->transformacao.value("x", 0.0f) == 100.0f,
              "espelhar H espelha todos os selecionados");
        check(mA && mA->transformacao.value("espelhado_v", 0.0f) == 0.0f,
              "espelhar H nao altera o flag vertical");
        Project::EspelharElementos(mirrorMode, { "mirror_a" }, false);
        Element* mA2 = Project::ResolverId(mirrorMode, "mirror_a");
        check(mA2 && mA2->transformacao.value("y", 0.0f) == 100.0f &&
              mA2->transformacao.value("espelhado_v", 0.0f) == 1.0f,
              "espelhar elemento unico mantem posicao (espelha no lugar)");

        // Linha: extremos (0,0)->(w,h) da caixa e clique por distância ao
        // segmento (o traço fino é difícil de acertar por AABB).
        Modo lineMode;
        Element lineEl = makeElement("linha_1", "linha");
        lineEl.transformacao = { { "x", 100.0f }, { "y", 100.0f },
                                 { "largura", 200.0f }, { "altura", 100.0f } };
        lineMode.raiz.push_back(std::move(lineEl));
        float lx1 = 0.0f, ly1 = 0.0f, lx2 = 0.0f, ly2 = 0.0f;
        Element* lineRef = Project::ResolverId(lineMode, "linha_1");
        Geo::LineEndpointsProject(*lineRef, lx1, ly1, lx2, ly2);
        check(lx1 == 100.0f && ly1 == 100.0f && lx2 == 300.0f && ly2 == 200.0f,
              "linha: extremos seguem a diagonal da caixa");
        check(Project::ElementoNoPonto(lineMode, 200.0f, 150.0f) &&
              Project::ElementoNoPonto(lineMode, 200.0f, 150.0f)->id == "linha_1",
              "linha: clique no meio do traco seleciona");
        check(Project::ElementoNoPonto(lineMode, 200.0f, 400.0f) == nullptr,
              "linha: clique longe do traco nao seleciona");

        Modo groupMode;
        Element groupA = makeElement("painel_a", "painel");
        groupA.transformacao = { { "x", 10.0f }, { "y", 20.0f },
                                 { "largura", 100.0f }, { "altura", 80.0f } };
        Element groupB = makeElement("painel_b", "painel");
        groupB.transformacao = { { "x", 180.0f }, { "y", 140.0f },
                                 { "largura", 120.0f }, { "altura", 60.0f } };
        groupMode.raiz.push_back(std::move(groupA));
        groupMode.raiz.push_back(std::move(groupB));
        const std::string groupId = Project::AgruparElementos(
            groupMode, { "painel_a", "painel_b" });
        Element* grouped = Project::ResolverId(groupMode, groupId);
        check(!groupId.empty() && grouped && grouped->filhos.size() == 2,
              "Ctrl+G cria um grupo com os elementos selecionados");
        check(grouped && grouped->transformacao.value("x", -1.0f) == 10.0f &&
              grouped->transformacao.value("largura", 0.0f) == 290.0f,
              "grupo calcula os limites visuais da selecao");
        check(Project::ElementoNoPonto(groupMode, 30.0f, 40.0f) == grouped,
              "clique dentro do grupo seleciona a unidade agrupada");

        // Desagrupar (M04): filhos voltam ao nível do grupo preservando posição.
        Element* groupedPtr = Project::ResolverId(groupMode, groupId);
        groupedPtr->bloqueado = true;
        check(!Project::DesagruparElementos(groupMode, groupId),
              "grupo bloqueado recusa desagrupar");
        groupedPtr->bloqueado = false;
        check(Project::DesagruparElementos(groupMode, groupId),
              "desagrupar remove o grupo");
        check(groupMode.raiz.size() == 2 &&
              Project::ResolverId(groupMode, "painel_a") &&
              Project::ResolverId(groupMode, "painel_b"),
              "desagrupar devolve os filhos ao nivel do grupo");
        Element* ungroupedA = Project::ResolverId(groupMode, "painel_a");
        Element* ungroupedB = Project::ResolverId(groupMode, "painel_b");
        check(ungroupedA && ungroupedA->transformacao.value("x", -1.0f) == 10.0f &&
              ungroupedB && ungroupedB->transformacao.value("y", -1.0f) == 140.0f,
              "desagrupar preserva a posicao dos elementos");

        // Clone com o botão direito (M04): cópia com IDs novos ao lado do
        // original, posição preservada; bloqueado recusa.
        {
            Modo cloneMode;
            Element original = makeElement("origem", "painel");
            original.transformacao = { { "x", 40.0f }, { "y", 60.0f },
                                       { "largura", 200.0f }, { "altura", 80.0f } };
            original.filhos.push_back(makeElement("origem_filho", "texto"));
            cloneMode.raiz.push_back(std::move(original));
            Project cloneProject;
            cloneProject.CriarNovo("Clone", 1280, 720);
            const std::string cloneId =
                Project::ClonarElemento(cloneMode, cloneProject, "origem");
            check(!cloneId.empty() && cloneId != "origem",
                  "clone recebe ID novo");
            check(cloneMode.raiz.size() == 2 &&
                  cloneMode.raiz[1].id == cloneId,
                  "clone entra logo apos o original");
            Element* clone = Project::ResolverId(cloneMode, cloneId);
            check(clone && clone->transformacao.value("x", -1.0f) == 40.0f &&
                  clone->filhos.size() == 1 &&
                  clone->filhos[0].id != "origem_filho",
                  "clone preserva posicao e renomeia descendentes");
            cloneMode.raiz[0].bloqueado = true;
            check(Project::ClonarElemento(cloneMode, cloneProject, "origem").empty(),
                  "elemento bloqueado recusa clone");
        }

        // Guias inteligentes (M04): encaixe de bordas/centros no arraste.
        {
            Modo guideMode;
            Element moving = makeElement("move", "painel");
            moving.transformacao = { { "x", 100.0f }, { "y", 100.0f },
                                     { "largura", 200.0f }, { "altura", 50.0f } };
            Element target = makeElement("alvo", "painel");
            target.transformacao = { { "x", 500.0f }, { "y", 100.0f },
                                     { "largura", 120.0f }, { "altura", 50.0f } };
            guideMode.raiz.push_back(std::move(moving));
            guideMode.raiz.push_back(std::move(target));

            std::vector<SmartGuides::Rect> starts = { { 100.0f, 100.0f, 200.0f, 50.0f } };
            std::vector<std::string> selection = { "move" };

            // Borda direita a 3px da esquerda do alvo -> encaixa em dx=200.
            float dx = 197.0f, dy = 5.0f, gx = -1.0f, gy = -1.0f;
            SmartGuides::Apply(guideMode, starts, selection, dx, dy,
                               1280.0f, 720.0f, 5.0f, gx, gy);
            check(dx == 200.0f && gx == 500.0f,
                  "guia inteligente encaixa borda direita na esquerda do alvo");
            check(dy == 5.0f && gy == -1.0f,
                  "sem encaixe vertical fora da tolerancia");

            // Centro do elemento puxado ao centro da tela (640).
            dx = 441.8f; dy = 0.0f; gx = gy = -1.0f;
            starts[0] = { 100.0f, 100.0f, 200.0f, 50.0f };
            SmartGuides::Apply(guideMode, starts, selection, dx, dy,
                               1280.0f, 720.0f, 5.0f, gx, gy);
            check(dx == 440.0f && gx == 640.0f,
                  "guia inteligente encaixa centro no centro da tela");

            // Fora da tolerância: nenhum ajuste.
            dx = 150.0f; dy = 0.0f; gx = gy = -1.0f;
            starts[0] = { 100.0f, 100.0f, 200.0f, 50.0f };
            SmartGuides::Apply(guideMode, starts, selection, dx, dy,
                               1280.0f, 720.0f, 5.0f, gx, gy);
            check(dx == 150.0f && gx == -1.0f,
                  "guia inteligente ignora posicao fora da tolerancia");

            // Seleção contendo os dois: sem candidatas além da tela.
            selection.push_back("alvo");
            dx = 197.0f; dy = 0.0f; gx = gy = -1.0f;
            starts = { { 100.0f, 100.0f, 200.0f, 50.0f },
                       { 500.0f, 100.0f, 120.0f, 50.0f } };
            SmartGuides::Apply(guideMode, starts, selection, dx, dy,
                               1280.0f, 720.0f, 5.0f, gx, gy);
            check(gx == -1.0f && gy == -1.0f,
                  "guias ignoram elementos da propria selecao");

            // Moldura da tela-base: snap FORTE (tolerância 8x). Elemento a
            // 14px da borda esquerda (dentro de 40px) encaixa na moldura,
            // mesmo estando fora da tolerância normal de 5px.
            Modo frameMode;
            Element fm = makeElement("move", "painel");
            fm.transformacao = { { "x", 10.0f }, { "y", 100.0f },
                                 { "largura", 100.0f }, { "altura", 40.0f } };
            frameMode.raiz.push_back(std::move(fm));
            selection = { "move" };
            dx = -24.0f; dy = 0.0f; gx = gy = -1.0f;
            starts = { { 10.0f, 100.0f, 100.0f, 40.0f } };
            SmartGuides::Apply(frameMode, starts, selection, dx, dy,
                               1280.0f, 720.0f, 5.0f, gx, gy);
            check(dx == -10.0f && gx == 0.0f && dy == 0.0f && gy == -1.0f,
                  "moldura tem snap forte (8x tolerancia) e vence os demais");

            // Força AMPLIADA: o menor desvio do elemento à borda é 35px —
            // dentro da tolerância ampliada (8x=40px) mas FORA da antiga
            // (5x=25px): a moldura magnetiza de mais longe agora.
            Modo frameMode2;
            Element fm2 = makeElement("move", "painel");
            fm2.transformacao = { { "x", 40.0f }, { "y", 100.0f },
                                  { "largura", 100.0f }, { "altura", 40.0f } };
            frameMode2.raiz.push_back(std::move(fm2));
            selection = { "move" };
            dx = -5.0f; dy = 0.0f; gx = gy = -1.0f;
            starts = { { 40.0f, 100.0f, 100.0f, 40.0f } };
            SmartGuides::Apply(frameMode2, starts, selection, dx, dy,
                               1280.0f, 720.0f, 5.0f, gx, gy);
            // selLeft = 35 (35 < 40); centro 85, direita 135.
            // O ponto mais próximo da borda 0 é a esquerda (35px) — engata:
            // dLeft = 0-35 = -35; dx += -35 -> dx = -40, esquerda em 0.
            check(dx == -40.0f && gx == 0.0f && gy == -1.0f,
                  "moldura: tolerancia ampliada (8x) magnetiza de mais longe");
        }

        // Snap de GUIA arrastada da régua (M05): a guia gruda nas laterais/
        // centros das formas e na moldura — a régua vira ferramenta
        // funcional de alinhamento, não só estética.
        {
            Modo guideSnapMode;
            Element form = makeElement("forma", "painel");
            form.transformacao = { { "x", 200.0f }, { "y", 150.0f },
                                   { "largura", 120.0f }, { "altura", 60.0f } };
            Element filho = makeElement("filho", "botao");
            filho.transformacao = { { "x", 340.0f }, { "y", 250.0f },
                                    { "largura", 40.0f }, { "altura", 20.0f } };
            form.filhos.push_back(std::move(filho));
            guideSnapMode.raiz.push_back(std::move(form));

            // Guia vertical perto da lateral ESQUERDA da forma (202 -> 200).
            bool snapped = false;
            float v = SmartGuides::SnapGuideToShapes(
                202.0f, false, guideSnapMode, 1280.0f, 720.0f, 10.0f, snapped);
            check(snapped && v == 200.0f,
                  "guia vertical gruda na lateral esquerda da forma");

            // Guia vertical perto do CENTRO da forma (260 -> 260).
            snapped = false;
            v = SmartGuides::SnapGuideToShapes(
                259.0f, false, guideSnapMode, 1280.0f, 720.0f, 10.0f, snapped);
            check(snapped && v == 260.0f,
                  "guia vertical gruda no centro da forma");

            // Guia vertical perto da lateral DIREITA da forma (321 -> 320).
            snapped = false;
            v = SmartGuides::SnapGuideToShapes(
                321.0f, false, guideSnapMode, 1280.0f, 720.0f, 10.0f, snapped);
            check(snapped && v == 320.0f,
                  "guia vertical gruda na lateral direita da forma");

            // Guia HORIZONTAL perto do topo do FILHO (255 -> 250).
            snapped = false;
            v = SmartGuides::SnapGuideToShapes(
                255.0f, true, guideSnapMode, 1280.0f, 720.0f, 10.0f, snapped);
            check(snapped && v == 250.0f,
                  "guia horizontal gruda no topo de um filho do grupo");

            // Guia horizontal perto da BASE da forma (212 -> 210).
            snapped = false;
            v = SmartGuides::SnapGuideToShapes(
                212.0f, true, guideSnapMode, 1280.0f, 720.0f, 10.0f, snapped);
            check(snapped && v == 210.0f,
                  "guia horizontal gruda na base da forma");

            // Guia FORA da tolerância: permanece onde o usuário soltou.
            snapped = false;
            v = SmartGuides::SnapGuideToShapes(
                50.0f, true, guideSnapMode, 1280.0f, 720.0f, 10.0f, snapped);
            check(!snapped && v == 50.0f,
                  "guia fora da tolerancia nao se move");

            // Moldura: guia perto do CENTRO da tela (646 -> 640) vence.
            snapped = false;
            v = SmartGuides::SnapGuideToShapes(
                646.0f, false, guideSnapMode, 1280.0f, 720.0f, 10.0f, snapped);
            check(snapped && v == 640.0f,
                  "guia gruda no centro da moldura da tela-base");

            // Moldura: guia perto da borda esquerda (3 -> 0) gruda nela.
            snapped = false;
            v = SmartGuides::SnapGuideToShapes(
                3.0f, false, guideSnapMode, 1280.0f, 720.0f, 10.0f, snapped);
            check(snapped && v == 0.0f,
                  "guia gruda na borda esquerda da moldura da tela-base");
        }

        // Snap de ARESTA no resize às guias fixas das réguas (M05): ao
        // redimensionar, a aresta arrastada encaixa na guia mais próxima.
        {
            float gx = -1.0f, gy = -1.0f;
            // Aresta direita a 7px de uma guia vertical em 500: encaixa.
            float left = 100.0f, right = 493.0f, top = 50.0f, bottom = 90.0f;
            std::vector<float> guidesV = { 500.0f };
            std::vector<float> guidesH = { 300.0f };
            SmartGuides::SnapResizeToGuides(
                left, right, top, bottom,
                false, true, false, false, // só resizeRight
                guidesV, guidesH, 12.0f, false, 0.0f, 0.0f, gx, gy);
            check(right == 500.0f && gx == 500.0f && left == 100.0f,
                  "resize: aresta direita encaixa na guia vertical");
            check(gy == -1.0f && top == 50.0f && bottom == 90.0f,
                  "resize: eixo sem alça nao muda");

            // Aresta inferior a 9px de uma guia horizontal em 300: encaixa.
            left = 100.0f; right = 200.0f; top = 50.0f; bottom = 291.0f;
            gx = gy = -1.0f;
            SmartGuides::SnapResizeToGuides(
                left, right, top, bottom,
                false, false, false, true, // só resizeBottom
                guidesV, guidesH, 12.0f, false, 0.0f, 0.0f, gx, gy);
            check(bottom == 300.0f && gy == 300.0f,
                  "resize: aresta inferior encaixa na guia horizontal");

            // Fora da tolerância: nenhum encaixe.
            left = 100.0f; right = 520.0f; top = 50.0f; bottom = 90.0f;
            gx = gy = -1.0f;
            SmartGuides::SnapResizeToGuides(
                left, right, top, bottom,
                false, true, false, false,
                guidesV, guidesH, 12.0f, false, 0.0f, 0.0f, gx, gy);
            check(right == 520.0f && gx == -1.0f,
                  "resize: aresta fora da tolerancia nao se move");

            // ESPELHADO (Shift): aresta direita encaixa em 500 e a esquerda
            // espelha a partir do pivô (px=150 -> left = 2*150-500 = -200).
            left = 100.0f; right = 493.0f; top = 50.0f; bottom = 90.0f;
            gx = gy = -1.0f;
            SmartGuides::SnapResizeToGuides(
                left, right, top, bottom,
                false, true, false, false,
                guidesV, guidesH, 12.0f, true, 150.0f, 0.0f, gx, gy);
            check(right == 500.0f && left == -200.0f && gx == 500.0f,
                  "resize espelhado: oposta reflete a partir do pivô");
        }

        // Snap de ARESTA no resize à MOLDURA da tela-base (M05): o
        // delimitador principal magnetiza as arestas também ao
        // redimensionar — tolerância forte (8x), igual ao mover.
        {
            float gx = -1.0f, gy = -1.0f;
            // Aresta direita a 7px da borda direita (1280): encaixa.
            float left = 100.0f, right = 1273.0f, top = 50.0f, bottom = 90.0f;
            SmartGuides::SnapResizeToFrame(
                left, right, top, bottom,
                false, true, false, false, // só resizeRight
                1280.0f, 720.0f, 12.0f, false, 0.0f, 0.0f, gx, gy);
            check(right == 1280.0f && gx == 1280.0f && left == 100.0f,
                  "resize: aresta direita trava na borda 1280 da moldura");
            check(gy == -1.0f && top == 50.0f && bottom == 90.0f,
                  "resize: moldura nao mexe no eixo sem alça");

            // Aresta inferior a 9px do CENTRO vertical (360): encaixa no
            // centro (candidato da moldura).
            left = 100.0f; right = 200.0f; top = 50.0f; bottom = 351.0f;
            gx = gy = -1.0f;
            SmartGuides::SnapResizeToFrame(
                left, right, top, bottom,
                false, false, false, true, // só resizeBottom
                1280.0f, 720.0f, 12.0f, false, 0.0f, 0.0f, gx, gy);
            check(bottom == 360.0f && gy == 360.0f,
                  "resize: aresta inferior trava no centro da moldura");

            // Fora da tolerância forte: nenhum encaixe.
            left = 100.0f; right = 1250.0f; top = 50.0f; bottom = 90.0f;
            gx = gy = -1.0f;
            SmartGuides::SnapResizeToFrame(
                left, right, top, bottom,
                false, true, false, false,
                1280.0f, 720.0f, 12.0f, false, 0.0f, 0.0f, gx, gy);
            check(right == 1250.0f && gx == -1.0f,
                  "resize: aresta longe da moldura nao se move");

            // ESPELHADO (Shift): aresta direita trava em 1280 e a esquerda
            // reflete a partir do pivô (px=640 -> left = 2*640-1280 = 0).
            left = 100.0f; right = 1273.0f; top = 50.0f; bottom = 90.0f;
            gx = gy = -1.0f;
            SmartGuides::SnapResizeToFrame(
                left, right, top, bottom,
                false, true, false, false,
                1280.0f, 720.0f, 12.0f, true, 640.0f, 0.0f, gx, gy);
            check(right == 1280.0f && left == 0.0f && gx == 1280.0f,
                  "resize espelhado: moldura trava e oposta reflete pelo pivô");
        }

        // Guias de espaçamento (M04): replicam espaços repetidos.
        {
            Modo spacingMode;
            Element a = makeElement("a", "painel");
            a.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 120.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element c = makeElement("c", "painel");
            c.transformacao = { { "x", 240.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element mov = makeElement("move", "painel");
            mov.transformacao = { { "x", 358.0f }, { "y", 0.0f },
                                  { "largura", 100.0f }, { "altura", 40.0f } };
            spacingMode.raiz.push_back(std::move(a));
            spacingMode.raiz.push_back(std::move(b));
            spacingMode.raiz.push_back(std::move(c));
            spacingMode.raiz.push_back(std::move(mov));

            std::vector<SmartGuides::Rect> s = { { 358.0f, 0.0f, 100.0f, 40.0f } };
            std::vector<std::string> sel = { "move" };
            float dx = 0.0f, dy = 0.0f;
            float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
            SmartGuides::ApplySpacing(spacingMode, s, sel, dx, dy, 4.0f,
                                       0.0f, 0.0f, x1, x2, y1, y2);
            // Espaço de 18px -> 20px (repetido entre a/b e b/c): dx += 2 e
            // guias em 340 e 360 delimitando o espaço repetido.
            check(dx == 2.0f && x1 == 340.0f && x2 == 360.0f,
                  "guia de espacamento replica espaco repetido dos vizinhos");
            check(y1 == -1.0f && y2 == -1.0f,
                  "sem guia de espacamento em eixo sem repeticao");
        }

        // Preview de espaçamento com Shift (M04): linhas-guia acima/abaixo da
        // fileira + ticks delimitando cada espaço entre peças.
        {
            Modo previewMode;
            Element p1 = makeElement("p1", "painel");
            p1.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                 { "largura", 100.0f }, { "altura", 40.0f } };
            Element p2 = makeElement("p2", "painel");
            p2.transformacao = { { "x", 120.0f }, { "y", 0.0f },
                                 { "largura", 100.0f }, { "altura", 40.0f } };
            Element p3 = makeElement("p3", "painel");
            p3.transformacao = { { "x", 240.0f }, { "y", 0.0f },
                                 { "largura", 100.0f }, { "altura", 40.0f } };
            Element pmove = makeElement("move", "painel");
            pmove.transformacao = { { "x", 360.0f }, { "y", 0.0f },
                                    { "largura", 100.0f }, { "altura", 40.0f } };
            previewMode.raiz.push_back(std::move(p1));
            previewMode.raiz.push_back(std::move(p2));
            previewMode.raiz.push_back(std::move(p3));
            previewMode.raiz.push_back(std::move(pmove));

            std::vector<std::string> sel = { "move" };
            std::vector<SmartGuides::GuideLine> lines;
            std::vector<SmartGuides::GuideLabel> labels;
            SmartGuides::ComputeSpacingPreview(previewMode, sel, 4.0f,
                                               7.0f, -1.0f, lines, labels);
            // Fileira de 4: 4 objetos x 4 traços = 16 traços nas quinas das
            // laterais (todos horizontais); 3 rótulos de distância.
            check(lines.size() == 16,
                  "preview Shift: 16 tracos nas quinas das laterais (4 objetos)");
            check(labels.size() == 3,
                  "preview Shift rotula os 3 espacos");
            int horizontal = 0;
            for (const auto& line : lines)
                if (line.horizontal) ++horizontal;
            check(horizontal == 16,
                  "preview Shift: todos os tracos da fileira sao horizontais");

            // Com valor previsto (engatado): os rótulos do espaço previsto
            // ficam destacados para o usuário.
            SmartGuides::ComputeSpacingPreview(previewMode, sel, 4.0f, 7.0f,
                                               20.0f, lines, labels);
            int highlighted = 0;
            for (const auto& label : labels)
                if (label.highlight) ++highlighted;
            check(highlighted == 3,
                  "preview Shift destaca os rotulos do espaco previsto (20)");

            // Coluna vertical: traços verticais nos cantos de topo/base.
            Modo colMode;
            Element c1 = makeElement("c1", "painel");
            c1.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                 { "largura", 100.0f }, { "altura", 40.0f } };
            Element c2 = makeElement("c2", "painel");
            c2.transformacao = { { "x", 0.0f }, { "y", 120.0f },
                                 { "largura", 100.0f }, { "altura", 40.0f } };
            Element cmove = makeElement("cmove", "painel");
            cmove.transformacao = { { "x", 0.0f }, { "y", 240.0f },
                                    { "largura", 100.0f }, { "altura", 40.0f } };
            colMode.raiz.push_back(std::move(c1));
            colMode.raiz.push_back(std::move(c2));
            colMode.raiz.push_back(std::move(cmove));
            std::vector<std::string> selC = { "cmove" };
            SmartGuides::ComputeSpacingPreview(colMode, selC, 4.0f, 7.0f,
                                               -1.0f, lines, labels);
            check(lines.size() == 12 && labels.size() == 2,
                  "preview Shift: coluna gera 12 tracos verticais e 2 rotulos");
            horizontal = 0;
            for (const auto& line : lines)
                if (line.horizontal) ++horizontal;
            check(horizontal == 0,
                  "preview Shift: tracos da coluna sao verticais");

            // Elemento com ALTURA diferente não forma fileira: sem traços.
            Modo mismatchMode;
            Element m1 = makeElement("m1", "painel");
            m1.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                 { "largura", 100.0f }, { "altura", 40.0f } };
            Element m2 = makeElement("m2", "painel");
            m2.transformacao = { { "x", 120.0f }, { "y", 0.0f },
                                 { "largura", 100.0f }, { "altura", 60.0f } };
            mismatchMode.raiz.push_back(std::move(m1));
            mismatchMode.raiz.push_back(std::move(m2));
            std::vector<std::string> sel2 = { "m2" };
            SmartGuides::ComputeSpacingPreview(mismatchMode, sel2, 4.0f,
                                               7.0f, -1.0f, lines, labels);
            check(lines.empty() && labels.empty(),
                  "preview Shift ignora elementos de alturas diferentes");
        }

        // Previsão ASSERTIVA: dois objetos com espaço de 62 preveem 62 para
        // a próxima peça (referência única — não precisa estar repetida).
        {
            Modo predictMode;
            Element a = makeElement("a", "painel");
            a.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 162.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element mov = makeElement("move", "painel");
            mov.transformacao = { { "x", 320.0f }, { "y", 0.0f },
                                  { "largura", 100.0f }, { "altura", 40.0f } };
            predictMode.raiz.push_back(std::move(a));
            predictMode.raiz.push_back(std::move(b));
            predictMode.raiz.push_back(std::move(mov));

            std::vector<SmartGuides::Rect> s = { { 320.0f, 0.0f, 100.0f, 40.0f } };
            std::vector<std::string> sel = { "move" };
            float dx = 0.0f, dy = 0.0f;
            float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
            SmartGuides::ApplySpacing(predictMode, s, sel, dx, dy, 10.0f,
                                       0.0f, 0.0f, x1, x2, y1, y2);
            // Espaço atual 58 (320-262); prevê 62: dx += 4, guias 262..324.
            check(dx == 4.0f && x1 == 262.0f && x2 == 324.0f,
                  "previsao assertiva: dois objetos com 62 preveem 62");
            check(y1 == -1.0f && y2 == -1.0f,
                  "sem previsao no eixo vertical");
        }

        // ENCADEAMENTO COMPLETO (grade 8px + previsão): com a referência de
        // 32, qualquer posição bruta próxima pousa EXATAMENTE em 32 — não em
        // 31 ou 33 (a previsão vence a grade e o alvo é o valor exato).
        {
            Modo m;
            Element a = makeElement("a", "painel");
            a.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 132.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            m.raiz.push_back(std::move(a));
            m.raiz.push_back(std::move(b));

            // Posições brutas do clone que a grade levaria a 31/33/32:
            // todas devem terminar com gap EXATO 32 (clone.left == 264).
            const float rawPositions[] = { 259.0f, 261.0f, 263.0f, 265.0f, 267.0f };
            bool allExact = true;
            for (float raw : rawPositions)
            {
                float dx = 0.0f, dy = 0.0f;
                float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
                std::vector<SmartGuides::Rect> s = { { raw, 0.0f, 100.0f, 40.0f } };
                // Grade de 8px primeiro (como no App).
                dx = roundf((raw + dx) / 8.0f) * 8.0f - raw;
                // Re-puxada (sem candidatas) + previsão por último.
                SmartGuides::ApplySpacing(m, s, { "move" }, dx, dy, 12.0f,
                                          0.0f, 0.0f, x1, x2, y1, y2);
                const float finalX = raw + dx;
                const float gap = finalX - 232.0f;
                if (fabsf(gap - 32.0f) > 0.01f) allExact = false;
            }
            check(allExact,
                  "grade+previsao pousam EXATAMENTE em 32 (nunca 31 ou 33)");
        }

        // Referência NÃO inteira: uma referência de 32.4 prevê 32.4 exato
        // (sem arredondar a 0.5 — antes pousava em 32.5 e exibia 33).
        {
            Modo m;
            Element a = makeElement("a", "painel");
            a.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 132.4f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            m.raiz.push_back(std::move(a));
            m.raiz.push_back(std::move(b));

            std::vector<SmartGuides::Rect> s = { { 264.1f, 0.0f, 100.0f, 40.0f } };
            float dx = 0.0f, dy = 0.0f;
            float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
            SmartGuides::ApplySpacing(m, s, { "move" }, dx, dy, 12.0f,
                                      0.0f, 0.0f, x1, x2, y1, y2);
            // gap = 264.1 - 232.4 = 31.7 -> alvo 32.4 exato: dx += 0.7.
            check(fabsf(dx - 0.7f) < 0.01f && fabsf(x2 - x1 - 32.4f) < 0.01f,
                  "referencia nao inteira prevê valor exato (32.4)");
        }

        // AS 4 DIREÇÕES do encaixe de espaçamento (M04): a previsão precisa
        // pousar EXATAMENTE no valor de referência em qualquer orientação.
        // Referência vertical: a(0,0,100,40) e b(0,72,100,40) -> gap 32.
        {
            Modo m;
            Element a = makeElement("a", "painel");
            a.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 0.0f }, { "y", 72.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            m.raiz.push_back(std::move(a));
            m.raiz.push_back(std::move(b));

            // Seleção ABAIXO do vizinho (gap = selTop - (o.y + o.h)):
            // bruto 28 em vez de 32 -> deve pousar exato em 32.
            {
                std::vector<SmartGuides::Rect> s = { { 0.0f, 140.0f, 100.0f, 40.0f } };
                float dx = 0.0f, dy = 0.0f;
                float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
                SmartGuides::ApplySpacing(m, s, { "move" }, dx, dy, 12.0f,
                                          0.0f, 0.0f, x1, x2, y1, y2);
                const float gap = (140.0f + dy) - 112.0f;
                check(fabsf(gap - 32.0f) < 0.01f && y1 == 112.0f && y2 == 144.0f,
                      "vertical ABAIXO: previsao pousa exatamente em 32");
            }
            // Seleção ACIMA do vizinho (gap = o.y - selBottom):
            // bruto 28 -> deve pousar exato em 32.
            {
                std::vector<SmartGuides::Rect> s = { { 0.0f, -68.0f, 100.0f, 40.0f } };
                float dx = 0.0f, dy = 0.0f;
                float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
                SmartGuides::ApplySpacing(m, s, { "move" }, dx, dy, 12.0f,
                                          0.0f, 0.0f, x1, x2, y1, y2);
                const float gap = 0.0f - ((-68.0f + dy) + 40.0f);
                check(fabsf(gap - 32.0f) < 0.01f && y1 == -32.0f && y2 == 0.0f,
                      "vertical ACIMA: previsao pousa exatamente em 32");
            }
        }
        // Referência horizontal DIREITA (gap = selLeft - (o.x + o.w)):
        // a(0,0,100,40) e b(132,0,100,40) -> gap 32; bruto 28 -> exato 32.
        {
            Modo m;
            Element a = makeElement("a", "painel");
            a.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 132.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            m.raiz.push_back(std::move(a));
            m.raiz.push_back(std::move(b));
            std::vector<SmartGuides::Rect> s = { { 260.0f, 0.0f, 100.0f, 40.0f } };
            float dx = 0.0f, dy = 0.0f;
            float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
            SmartGuides::ApplySpacing(m, s, { "move" }, dx, dy, 12.0f,
                                      0.0f, 0.0f, x1, x2, y1, y2);
            const float gap = (260.0f + dx) - 232.0f;
            check(fabsf(gap - 32.0f) < 0.01f && x1 == 232.0f && x2 == 264.0f,
                  "horizontal DIREITA: previsao pousa exatamente em 32");
        }

        // CRUZAMENTO (arrasto rápido, M04): o gap pulou de 54 para 18 num
        // único frame, passando POR CIMA do alvo 32 — mesmo estando fora da
        // tolerância (|18-32|=14 > 12), o encaixe engata e pousa exato em 32.
        // Antes, o cursor "pulava" a zona do ímã e a previsão não disparava.
        {
            Modo m;
            Element a = makeElement("a", "painel");
            a.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 132.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            m.raiz.push_back(std::move(a));
            m.raiz.push_back(std::move(b));
            std::vector<SmartGuides::Rect> s = { { 250.0f, 0.0f, 100.0f, 40.0f } };
            float dx = 0.0f, dy = 0.0f;
            float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
            // Frame anterior em 286 (prevDx = 36): gap era 54; agora é 18.
            SmartGuides::ApplySpacing(m, s, { "move" }, dx, dy, 12.0f,
                                      36.0f, 0.0f, x1, x2, y1, y2);
            const float finalX = 250.0f + dx;
            check(fabsf(dx - 14.0f) < 0.01f && finalX == 264.0f &&
                  x1 == 232.0f && x2 == 264.0f,
                  "arrasto rapido: gap pula 54->18 e engata exato em 32");
        }

        // CONTEXTO DE FILEIRA (M04): a referência de espaçamento só vale para
        // a fileira em que a peça está sendo encaixada. Uma peça em outra
        // altura (y=200) NÃO prevê o espaçamento de 32 da fileira y=0 — a
        // previsão fica silenciosa em vez de puxar para um alvo errado.
        {
            Modo m;
            Element a = makeElement("a", "painel");
            a.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 132.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            m.raiz.push_back(std::move(a));
            m.raiz.push_back(std::move(b));
            // Peça em OUTRA fileira, mas com gap 28 (dentro da tolerância do
            // alvo 32): a previsão NÃO deve engatar (contexto diferente).
            std::vector<SmartGuides::Rect> s = { { 260.0f, 200.0f, 100.0f, 40.0f } };
            float dx = 0.0f, dy = 0.0f;
            float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
            SmartGuides::ApplySpacing(m, s, { "move" }, dx, dy, 12.0f,
                                      0.0f, 0.0f, x1, x2, y1, y2);
            check(dx == 0.0f && x1 == -1.0f && y1 == -1.0f,
                  "fora da fileira: nao prevê espacamento de outra linha");
        }

        // GRUPO COMO BLOCO ÚNICO (M04): um grupo (caixa 0..400) que contém um
        // filho (100..200) não polui as referências — o filho aninhado é
        // deduplicado. Ao lado do grupo, b(500..600) gera o único alvo real da
        // fileira: 100. Gap 90 engata em 100; gap 290 (que seria o artefato
        // filho->b de 300) NÃO engata.
        {
            Modo m;
            Element g = makeElement("g", "grupo");
            g.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 400.0f }, { "altura", 40.0f } };
            Element f = makeElement("f", "painel");
            f.transformacao = { { "x", 100.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            g.filhos.push_back(std::move(f));
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 500.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            m.raiz.push_back(std::move(g));
            m.raiz.push_back(std::move(b));
            {
                std::vector<SmartGuides::Rect> s = { { 690.0f, 0.0f, 100.0f, 40.0f } };
                float dx = 0.0f, dy = 0.0f;
                float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
                SmartGuides::ApplySpacing(m, s, { "move" }, dx, dy, 12.0f,
                                          0.0f, 0.0f, x1, x2, y1, y2);
                check(dx == 10.0f && x1 == 600.0f && x2 == 700.0f,
                      "grupo participa como bloco unico (gap 100 previsto)");
            }
            {
                std::vector<SmartGuides::Rect> s = { { 890.0f, 0.0f, 100.0f, 40.0f } };
                float dx = 0.0f, dy = 0.0f;
                float x1 = -1.0f, x2 = -1.0f, y1 = -1.0f, y2 = -1.0f;
                SmartGuides::ApplySpacing(m, s, { "move" }, dx, dy, 12.0f,
                                          0.0f, 0.0f, x1, x2, y1, y2);
                check(dx == 0.0f && x1 == -1.0f,
                      "filho aninhado nao gera alvo espurio de espacamento");
            }
        }

        // ALINHAMENTO AO CONJUNTO (M04): a referência é o bounding box dos
        // vizinhos (elementos fora da seleção), não a tela nem a própria
        // seleção — centralizar H+V coloca a forma exatamente no centro do
        // conjunto, com distâncias uniformes nos 4 lados.
        {
            // Cinzas formando uma "moldura" simétrica: left 0..100, right
            // 300..400, top 0..100, bottom 300..400 -> centro exato (200,200).
            std::vector<AlignUtils::Rect> vizinhos = {
                { 0.0f, 0.0f, 100.0f, 100.0f },
                { 300.0f, 0.0f, 100.0f, 100.0f },
                { 0.0f, 300.0f, 100.0f, 100.0f },
                { 300.0f, 300.0f, 100.0f, 100.0f },
            };
            float l = 0.0f, t = 0.0f, r = 0.0f, b = 0.0f;
            AlignUtils::SetBounds(vizinhos, l, t, r, b);
            check(l == 0.0f && t == 0.0f && r == 400.0f && b == 400.0f,
                  "conjunto: bounding box dos vizinhos (0,0)-(400,400)");
            // Forma vermelha 80x80 no canto (10,10): centralizar H+V deve
            // colocá-la em (160,160) — gap de 160 em TODOS os 4 lados.
            const float cx = AlignUtils::AlignedX(80.0f, l, r, 1);
            const float cy = AlignUtils::AlignedY(80.0f, t, b, 4);
            check(cx == 160.0f && cy == 160.0f,
                  "conjunto: centralizar H+V pousa exato no centro (160,160)");
            check(cx - 0.0f == 160.0f && (400.0f - (cx + 80.0f)) == 160.0f &&
                  cy - 0.0f == 160.0f && (400.0f - (cy + 80.0f)) == 160.0f,
                  "conjunto: distancias uniformes nos 4 lados (160 px)");
            // Alinhar à esquerda do conjunto / base do conjunto.
            check(AlignUtils::AlignedX(80.0f, l, r, 0) == 0.0f &&
                  AlignUtils::AlignedY(80.0f, t, b, 5) == 320.0f,
                  "conjunto: esquerda e base do conjunto");
        }

        // CAMADAS (estilo CorelDRAW, M04): MoverCamada reordena o elemento
        // dentro do seu contêiner — sobe (frente) ou desce (trás).
        {
            Modo m;
            Element a = makeElement("a", "painel");
            a.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element b = makeElement("b", "painel");
            b.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            Element c = makeElement("c", "painel");
            c.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 100.0f }, { "altura", 40.0f } };
            m.raiz.push_back(std::move(a));
            m.raiz.push_back(std::move(b));
            m.raiz.push_back(std::move(c));
            // Ordem inicial: a, b, c (a atrás, c na frente).
            check(Project::MoverCamada(m, "a", +1),
                  "camada: sobe um nível");
            check(m.raiz[0].id == "b" && m.raiz[1].id == "a" &&
                  m.raiz[2].id == "c",
                  "camada: ordem b, a, c após subir 'a'");
            check(Project::MoverCamada(m, "a", -1),
                  "camada: desce um nível");
            check(m.raiz[0].id == "a" && m.raiz[1].id == "b" &&
                  m.raiz[2].id == "c",
                  "camada: ordem a, b, c após descer 'a'");
            check(!Project::MoverCamada(m, "c", +1),
                  "camada: no topo nao sobe mais");
            check(!Project::MoverCamada(m, "a", -1),
                  "camada: no fundo nao desce mais");
            // Filhos de grupo: a camada opera dentro do contêiner.
            Element g = makeElement("g", "grupo");
            g.transformacao = { { "x", 0.0f }, { "y", 0.0f },
                                { "largura", 300.0f }, { "altura", 40.0f } };
            g.filhos.push_back(makeElement("g1", "painel"));
            g.filhos.push_back(makeElement("g2", "painel"));
            m.raiz.push_back(std::move(g));
            check(Project::MoverCamada(m, "g1", +1) &&
                  m.raiz[3].filhos[0].id == "g2" && m.raiz[3].filhos[1].id == "g1",
                  "camada: dentro do grupo sobe o filho");
        }

        const std::string json = Project::Serializar(project);
        Project loaded;
        const std::string error = Project::Desserializar(loaded, json);
        check(error.empty(), "JSON serializa e desserializa sem erro");
        check(loaded.telas.size() == 1 && loaded.telas[0].modos.size() == 1,
              "tela e modo persistem");
        if (!loaded.telas.empty() && !loaded.telas[0].modos.empty())
        {
            Modo& loadedMode = loaded.telas[0].modos[0];
            check(loadedMode.raiz.size() == 3 && loadedMode.raiz[0].id == "a" &&
                  loadedMode.raiz[1].id == "c" && loadedMode.raiz[2].id == "b",
                  "ordem da hierarquia persiste");
            Element* loadedX = Project::ResolverId(loadedMode, "x");
            check(loadedX && loadedX->filhos.size() == 1 && loadedX->filhos[0].id == "y",
                  "estrutura aninhada persiste");
            Element* loadedB = Project::ResolverId(loadedMode, "b");
            check(loadedB && loadedB->bloqueado && !loadedB->visivel,
                  "bloqueio e visibilidade persistem");
            check(loadedB && loadedB->transformacao.value("x", 0.0f) == 321.0f,
                  "transformacao persiste");
            check(loadedB && loadedB->estilos["raio_quinas"].value(
                      "inferior_esquerda", 0.0f) == 16.0f,
                  "quatro raios de quina persistem");
            // Sombra: objeto estilos.sombra (cor, deslocamento, desfoque)
            // persiste na serialização e no espelhamento não é alterado.
            Element shadowPanel = makeElement("sh", "retangulo");
            shadowPanel.estilos["sombra"] = nlohmann::json{
                { "cor", "#101010" }, { "deslocamento_x", 6.0 },
                { "deslocamento_y", -3.0 }, { "desfoque", 12.0 } };
            loadedMode.raiz.push_back(std::move(shadowPanel));
            const std::string json2 = Project::Serializar(loaded);
            Project loaded2;
            check(Project::Desserializar(loaded2, json2).empty(),
                  "sombra serializa sem erro");
            Element* shadowBack = Project::ResolverId(
                loaded2.telas[0].modos[0], "sh");
            check(shadowBack && shadowBack->estilos.contains("sombra") &&
                  shadowBack->estilos["sombra"].value("desfoque", 0.0f) == 12.0f &&
                  shadowBack->estilos["sombra"].value("deslocamento_y", 0.0f) == -3.0f,
                  "sombra persiste com cor, deslocamento e desfoque");
        }

        report << (failures == 0 ? "RESULT PASS" : "RESULT FAIL")
               << " failures=" << failures << '\n';
        return failures == 0 ? 0 : 1;
    }
}

int main(int argc, char** argv)
{
    if (argc > 1 && std::strcmp(argv[1], "--self-test-m04") == 0)
        return RunM04SelfTest();

    const bool capture = argc > 1 && std::strcmp(argv[1], "--capture") == 0;
    seedui::App app;
    app.Run(capture);
    return 0;
}
