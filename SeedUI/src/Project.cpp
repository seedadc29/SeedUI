#include "Project.h"

#include <ctime>

namespace seedui
{
    namespace
    {
        std::string NowStamp()
        {
            const time_t t = time(nullptr);
            struct tm tmv;
            localtime_s(&tmv, &t);
            char buf[32];
            strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tmv);
            return buf;
        }

        // ---- Serialização de Element ----

        void ToJson(const Element& e, nlohmann::json& out)
        {
            out["id"] = e.id;
            out["tipo"] = e.tipo;
            if (!e.nome.empty()) out["nome"] = e.nome;
            if (!e.visivel) out["visivel"] = false;
            if (e.bloqueado) out["bloqueado"] = true;
            if (!e.transformacao.empty()) out["transformacao"] = e.transformacao;
            if (!e.layout.empty()) out["layout"] = e.layout;
            if (!e.estilos.empty()) out["estilos"] = e.estilos;
            if (!e.estados.empty()) out["estados"] = e.estados;
            if (!e.propriedades.empty()) out["propriedades"] = e.propriedades;
            if (!e.diretriz.empty()) out["diretriz"] = e.diretriz;
            if (!e.filhos.empty())
            {
                out["filhos"] = nlohmann::json::array();
                for (const Element& f : e.filhos)
                {
                    nlohmann::json jf;
                    ToJson(f, jf);
                    out["filhos"].push_back(jf);
                }
            }
        }

        void FromJson(Element& e, const nlohmann::json& in)
        {
            e.id = in.value("id", std::string());
            e.tipo = in.value("tipo", std::string());
            e.nome = in.value("nome", std::string());
            e.visivel = in.value("visivel", true);
            e.bloqueado = in.value("bloqueado", false);
            if (in.contains("transformacao")) e.transformacao = in["transformacao"];
            if (in.contains("layout")) e.layout = in["layout"];
            if (in.contains("estilos")) e.estilos = in["estilos"];
            if (in.contains("estados")) e.estados = in["estados"];
            if (in.contains("propriedades")) e.propriedades = in["propriedades"];
            e.diretriz = in.value("diretriz", std::string());
            if (in.contains("filhos") && in["filhos"].is_array())
            {
                e.filhos.clear();
                for (const auto& jf : in["filhos"])
                {
                    Element f;
                    FromJson(f, jf);
                    e.filhos.push_back(std::move(f));
                }
            }
        }

        void ToJson(const Modo& m, nlohmann::json& out)
        {
            out["id"] = m.id;
            out["nome"] = m.nome;
            out["raiz"] = nlohmann::json::array();
            for (const Element& e : m.raiz)
            {
                nlohmann::json je;
                ToJson(e, je);
                out["raiz"].push_back(je);
            }
        }

        void FromJson(Modo& m, const nlohmann::json& in)
        {
            m.id = in.value("id", std::string());
            m.nome = in.value("nome", std::string());
            m.raiz.clear();
            if (in.contains("raiz") && in["raiz"].is_array())
            {
                for (const auto& je : in["raiz"])
                {
                    Element e;
                    FromJson(e, je);
                    m.raiz.push_back(std::move(e));
                }
            }
        }

        void ToJson(const Tela& t, nlohmann::json& out)
        {
            out["id"] = t.id;
            out["nome"] = t.nome;
            out["modos"] = nlohmann::json::array();
            for (const Modo& m : t.modos)
            {
                nlohmann::json jm;
                ToJson(m, jm);
                out["modos"].push_back(jm);
            }
        }

        void FromJson(Tela& t, const nlohmann::json& in)
        {
            t.id = in.value("id", std::string());
            t.nome = in.value("nome", std::string());
            t.modos.clear();
            if (in.contains("modos") && in["modos"].is_array())
            {
                for (const auto& jm : in["modos"])
                {
                    Modo m;
                    FromJson(m, jm);
                    t.modos.push_back(std::move(m));
                }
            }
        }
    }

    void Project::CriarNovo(const std::string& nomeProjeto, int largura, int altura)
    {
        *this = Project(); // zera tudo (mantém defaults)

        nome = nomeProjeto;
        criadoEm = NowStamp();
        modificadoEm = criadoEm;
        telaBaseLargura = largura > 0 ? largura : 1280;
        telaBaseAltura = altura > 0 ? altura : 720;

        Tela t;
        t.id = "tela_principal";
        t.nome = "Tela principal";
        Modo m;
        m.id = "modo_padrao";
        m.nome = "Padrão";
        t.modos.push_back(std::move(m));
        telas.push_back(std::move(t));
    }

    std::string Project::StampAtual()
    {
        return NowStamp();
    }

    std::string Project::Serializar(const Project& p)
    {
        try
        {
            nlohmann::json root;
            root["formato"] = Project::kFormato;
            root["versao"] = Project::kVersao;

            nlohmann::json mete;
            mete["nome"] = p.nome;
            mete["descricao"] = p.descricao;
            mete["criado_em"] = p.criadoEm;
            mete["modificado_em"] = p.modificadoEm;
            mete["gerador"] = p.gerador;
            root["metadados"] = mete;

            root["tela_base"] = { { "largura", p.telaBaseLargura },
                                  { "altura", p.telaBaseAltura } };

            if (!p.resolucoes.empty())
                root["resolucoes"] = p.resolucoes;
            if (!p.variaveis.empty())
                root["variaveis"] = p.variaveis;
            if (!p.temas.empty())
                root["temas"] = p.temas;
            if (!p.recursos.empty())
                root["recursos"] = p.recursos;
            if (!p.diretrizes.empty())
                root["diretrizes"] = p.diretrizes;

            root["telas"] = nlohmann::json::array();
            for (const Tela& t : p.telas)
            {
                nlohmann::json jt;
                ToJson(t, jt);
                root["telas"].push_back(jt);
            }

            return root.dump(2); // indentado, legível por humanos e por IA
        }
        catch (const std::exception& e)
        {
            return std::string("Falha ao serializar: ") + e.what();
        }
    }

    std::string Project::Desserializar(Project& p, const std::string& texto)
    {
        try
        {
            nlohmann::json root = nlohmann::json::parse(texto);

            if (root.value("formato", std::string()) != Project::kFormato)
                return "Arquivo inválido: campo \"formato\" deve ser \"seedui.projeto\".";

            p = Project(); // zera antes de carregar (dados novos sobrescrevem)

            p.formato = Project::kFormato;
            p.versao = root.value("versao", Project::kVersao);

            if (root.contains("metadados") && root["metadados"].is_object())
            {
                const auto& m = root["metadados"];
                p.nome = m.value("nome", std::string());
                p.descricao = m.value("descricao", std::string());
                p.criadoEm = m.value("criado_em", std::string());
                p.modificadoEm = m.value("modificado_em", std::string());
                p.gerador = m.value("gerador", std::string());
            }

            if (root.contains("tela_base") && root["tela_base"].is_object())
            {
                const auto& tb = root["tela_base"];
                p.telaBaseLargura = tb.value("largura", 0);
                p.telaBaseAltura = tb.value("altura", 0);
                if (p.telaBaseLargura <= 0) p.telaBaseLargura = 1280;
                if (p.telaBaseAltura <= 0) p.telaBaseAltura = 720;
            }

            if (root.contains("resolucoes") && root["resolucoes"].is_array())
                p.resolucoes = root["resolucoes"].get<std::vector<int>>();
            if (root.contains("variaveis")) p.variaveis = root["variaveis"];
            if (root.contains("temas")) p.temas = root["temas"];
            if (root.contains("recursos")) p.recursos = root["recursos"];
            p.diretrizes = root.value("diretrizes", std::string());

            if (root.contains("telas") && root["telas"].is_array())
            {
                for (const auto& jt : root["telas"])
                {
                    Tela t;
                    FromJson(t, jt);
                    p.telas.push_back(std::move(t));
                }
            }

            return std::string();
        }
        catch (const nlohmann::json::exception& e)
        {
            return std::string("Falha ao ler o JSON: ") + e.what();
        }
        catch (const std::exception& e)
        {
            return std::string("Falha ao ler o projeto: ") + e.what();
        }
    }

    Element* Project::ResolverId(Project& projeto, const std::string& id, Modo** modoEncontrado)
    {
        if (modoEncontrado) *modoEncontrado = nullptr;
        if (id.empty()) return nullptr;

        for (Tela& t : projeto.telas)
        {
            for (Modo& m : t.modos)
            {
                std::vector<Element*> pilha;
                for (Element& e : m.raiz) pilha.push_back(&e);
                while (!pilha.empty())
                {
                    Element* e = pilha.back();
                    pilha.pop_back();
                    if (e->id == id)
                    {
                        if (modoEncontrado) *modoEncontrado = &m;
                        return e;
                    }
                    for (Element& f : e->filhos) pilha.push_back(&f);
                }
            }
        }
        return nullptr;
    }
}
