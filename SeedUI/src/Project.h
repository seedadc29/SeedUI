#ifndef SEEDUI_PROJECT_H
#define SEEDUI_PROJECT_H

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace seedui
{
    // Elemento da árvore da interface. Os blocos opcionais (transformacao,
    // layout, estilos, estados, propriedades) são preservados como JSON puro:
    // M03 não edita esses detalhes ainda (M06/inspetor), só os mantém sem perda.
    struct Element
    {
        std::string id;
        std::string tipo;
        std::string nome;
        bool visivel = true;
        bool bloqueado = false;
        std::string diretriz;
        nlohmann::json transformacao = nlohmann::json::object();
        nlohmann::json layout = nlohmann::json::object();
        nlohmann::json estilos = nlohmann::json::object();
        nlohmann::json estados = nlohmann::json::object();
        nlohmann::json propriedades = nlohmann::json::object();
        std::vector<Element> filhos;

    };

    struct Modo
    {
        std::string id;
        std::string nome;
        std::vector<Element> raiz;

    };

    struct Tela
    {
        std::string id;
        std::string nome;
        std::vector<Modo> modos;

    };

    class Project
    {
    public:
        static constexpr const char* kFormato = "seedui.projeto";
        static constexpr int kVersao = 1;
        static constexpr const char* kGerador = "SeedUI 0.1 (M03)";

        // Estado serializável (formato v1)
        std::string formato = kFormato;
        int versao = kVersao;
        std::string nome;
        std::string descricao;
        std::string criadoEm;
        std::string modificadoEm;
        std::string gerador = kGerador;
        int telaBaseLargura = 1280;
        int telaBaseAltura = 720;
        std::vector<int> resolucoes = { 1280, 720 }; // pares largura,altura — M10
        nlohmann::json variaveis = nlohmann::json::object();
        nlohmann::json temas = nlohmann::json::object();
        nlohmann::json recursos = nlohmann::json::array();
        std::string diretrizes;
        std::vector<Tela> telas;

        // Estado não serializado (controle do editor)
        std::string caminhoArquivo; // onde o projeto foi salvo/aberto


        // Cria um projeto novo com uma tela e um modo padrão.
        void CriarNovo(const std::string& nomeProjeto, int largura, int altura);

        // Serialização. Retorna std::string vazio em caso de sucesso.
        static std::string Serializar(const Project& projeto);
        static std::string Desserializar(Project& projeto, const std::string& texto);

        // Resolve um id dentro das telas/modos e retorna ponteiro (ou nullptr).
        // Varre raiz recursivamente. Se "modoFixo" for não-nulo, busca só nele.
        static Element* ResolverId(Project& projeto, const std::string& id,
                                   Modo** modoEncontrado = nullptr);

        // Operacoes estruturais usadas pela Hierarquia. Elementos bloqueados
        // nao podem ser movidos, reparentados ou excluidos.
        static Element* ResolverId(Modo& modo, const std::string& id);
        static Element* ElementoNoPonto(Modo& modo, float x, float y);
        static bool ExcluirElemento(Modo& modo, const std::string& id);
        static bool MoverElemento(Modo& modo, const std::string& id, int delta);
        static bool ReparentearElemento(Modo& modo, const std::string& id,
                                        const std::string& novoPaiId);
        static std::vector<Element> CopiarElementos(
            const Modo& modo, const std::vector<std::string>& ids);
        static std::vector<std::string> ColarElementos(
            Project& projeto, Modo& modo, const std::vector<Element>& elementos,
            float deslocamento);
        static std::string AgruparElementos(Modo& modo,
                                             const std::vector<std::string>& ids);
        // Desagrupa: filhos sobem para o nível do grupo preservando posição.
        static bool DesagruparElementos(Modo& modo, const std::string& groupId);
        // Clona um elemento (com descendentes) com IDs novos, logo após o
        // original. Retorna o ID do clone (vazio se bloqueado/ausente).
        static std::string ClonarElemento(Modo& modo, Project& projeto,
                                          const std::string& id);

        // Camadas (estilo CorelDRAW): reordena o elemento dentro do seu
        // contêiner (raiz do modo ou pai). delta > 0 sobe (renderiza depois,
        // fica na frente); delta < 0 desce. Retorna false se ausente/bloqueado.
        static bool MoverCamada(Modo& modo, const std::string& id, int delta);

        // Timestamp atual no formato do formato ("YYYY-MM-DD HH:MM:SS").
        static std::string StampAtual();
    };
}

#endif // SEEDUI_PROJECT_H
