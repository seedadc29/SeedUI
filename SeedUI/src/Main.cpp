// Entry point: subsistema Windows sem console, mas com main(argc, argv)
// funcional (o CRT monta os argumentos reais da linha de comando).
#pragma comment(linker, "/ENTRY:mainCRTStartup")

#include <cstring>
#include <fstream>
#include <string>

#include "App.h"
#include "Project.h"

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
