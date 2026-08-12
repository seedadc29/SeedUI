#include "Project.h"

#include "Geo.h"

#include <algorithm>
#include <cfloat>
#include <ctime>

namespace seedui
{
    namespace
    {
        struct ElementSlot
        {
            std::vector<Element>* container = nullptr;
            size_t index = 0;
        };

        bool FindElementSlot(std::vector<Element>& elements, const std::string& id,
                             ElementSlot& slot)
        {
            for (size_t i = 0; i < elements.size(); ++i)
            {
                if (elements[i].id == id)
                {
                    slot.container = &elements;
                    slot.index = i;
                    return true;
                }
                if (FindElementSlot(elements[i].filhos, id, slot)) return true;
            }
            return false;
        }

        bool ElementContains(const Element& root, const std::string& id)
        {
            if (root.id == id) return true;
            for (const Element& child : root.filhos)
                if (ElementContains(child, id)) return true;
            return false;
        }

        void CollectCopies(const std::vector<Element>& elements,
                           const std::vector<std::string>& selectedIds,
                           std::vector<Element>& out)
        {
            for (const Element& element : elements)
            {
                const bool selected = std::find(selectedIds.begin(), selectedIds.end(),
                                                element.id) != selectedIds.end();
                if (selected)
                    out.push_back(element);
                else
                    CollectCopies(element.filhos, selectedIds, out);
            }
        }

        void AssignFreshIds(Element& element, Project& project,
                            std::vector<std::string>& reservedIds)
        {
            const std::string base = element.tipo.empty() ? "elemento" : element.tipo;
            for (int index = 1; index < 100000; ++index)
            {
                const std::string candidate = base + "_" + std::to_string(index);
                const bool reserved = std::find(reservedIds.begin(), reservedIds.end(),
                                                candidate) != reservedIds.end();
                if (!reserved && !Project::ResolverId(project, candidate))
                {
                    element.id = candidate;
                    reservedIds.push_back(candidate);
                    break;
                }
            }
            for (Element& child : element.filhos)
                AssignFreshIds(child, project, reservedIds);
        }

        void OffsetElementTreeXY(Element& element, float dx, float dy,
                                 float canvasWidth, float canvasHeight)
        {
            const float width = element.transformacao.value("largura", 160.0f);
            const float height = element.transformacao.value("altura", 32.0f);
            const float x = element.transformacao.value("x", 0.0f) + dx;
            const float y = element.transformacao.value("y", 0.0f) + dy;
            element.transformacao["x"] = std::max(0.0f,
                std::min(std::max(0.0f, canvasWidth - width), x));
            element.transformacao["y"] = std::max(0.0f,
                std::min(std::max(0.0f, canvasHeight - height), y));
            for (Element& child : element.filhos)
                OffsetElementTreeXY(child, dx, dy, canvasWidth, canvasHeight);
        }

        void OffsetElementTree(Element& element, float delta,
                               float canvasWidth, float canvasHeight)
        {
            OffsetElementTreeXY(element, delta, delta, canvasWidth, canvasHeight);
        }

        Element* HitElement(Element& element, float x, float y)
        {
            if (!element.visivel) return nullptr;
            const float left = element.transformacao.value("x", 0.0f);
            const float top = element.transformacao.value("y", 0.0f);
            const float width = element.transformacao.value("largura", 160.0f);
            const float height = element.transformacao.value("altura", 32.0f);
            bool inside = false;
            if (element.tipo == "caminho")
            {
                // Caminho: clique próximo ao contorno tessellado.
                std::vector<ImVec2> pts;
                Geo::OutlineProject(element, pts, 64);
                const int n = (int)pts.size();
                if (n >= 2)
                {
                    const bool closed = element.transformacao.value("fechado", 0.0f) > 0.5f;
                    const int segments = closed ? n : n - 1;
                    float best = FLT_MAX;
                    for (int i = 0; i < segments; ++i)
                    {
                        const ImVec2& a = pts[i];
                        const ImVec2& b = pts[(i + 1) % n];
                        const float dx = b.x - a.x, dy = b.y - a.y;
                        const float len2 = dx * dx + dy * dy;
                        float t = len2 > 0.0f
                            ? ((x - a.x) * dx + (y - a.y) * dy) / len2 : 0.0f;
                        t = std::max(0.0f, std::min(1.0f, t));
                        const float px = a.x + t * dx, py = a.y + t * dy;
                        const float ddx = x - px, ddy = y - py;
                        best = std::min(best, ddx * ddx + ddy * ddy);
                    }
                    inside = best <= 64.0f; // raio 8px
                }
            }
            else if (element.tipo == "linha")
            {
                // Linha: clique próximo ao traço (distância ao segmento).
                float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
                Geo::LineEndpointsProject(element, x1, y1, x2, y2);
                const float dx = x2 - x1, dy = y2 - y1;
                const float len2 = dx * dx + dy * dy;
                if (len2 > 0.0f)
                {
                    float t = ((x - x1) * dx + (y - y1) * dy) / len2;
                    t = std::max(0.0f, std::min(1.0f, t));
                    const float px = x1 + t * dx, py = y1 + t * dy;
                    const float ddx = x - px, ddy = y - py;
                    inside = (ddx * ddx + ddy * ddy) <= 64.0f; // raio 8px
                }
            }
            else if (Geo::ElementRotation(element) != 0.0f)
            {
                float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
                Geo::RotatedAABB(element, minX, minY, maxX, maxY);
                inside = x >= minX && x <= maxX && y >= minY && y <= maxY;
            }
            else
            {
                inside = x >= left && x <= left + width &&
                         y >= top && y <= top + height;
            }
            if (element.tipo == "grupo" && inside) return &element;
            for (auto it = element.filhos.rbegin(); it != element.filhos.rend(); ++it)
                if (Element* hit = HitElement(*it, x, y)) return hit;
            return inside ? &element : nullptr;
        }

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

    Element* Project::ResolverId(Modo& modo, const std::string& id)
    {
        ElementSlot slot;
        return FindElementSlot(modo.raiz, id, slot)
            ? &(*slot.container)[slot.index]
            : nullptr;
    }

    Element* Project::ElementoNoPonto(Modo& modo, float x, float y)
    {
        for (auto it = modo.raiz.rbegin(); it != modo.raiz.rend(); ++it)
            if (Element* hit = HitElement(*it, x, y)) return hit;
        return nullptr;
    }

    bool Project::ExcluirElemento(Modo& modo, const std::string& id)
    {
        ElementSlot slot;
        if (!FindElementSlot(modo.raiz, id, slot)) return false;
        if ((*slot.container)[slot.index].bloqueado) return false;
        slot.container->erase(slot.container->begin() + slot.index);
        return true;
    }

    bool Project::MoverElemento(Modo& modo, const std::string& id, int delta)
    {
        ElementSlot slot;
        if (!FindElementSlot(modo.raiz, id, slot)) return false;
        if ((*slot.container)[slot.index].bloqueado) return false;
        const int from = (int)slot.index;
        const int to = from + delta;
        if (to < 0 || to >= (int)slot.container->size()) return false;
        std::swap((*slot.container)[from], (*slot.container)[to]);
        return true;
    }

    bool Project::ReparentearElemento(Modo& modo, const std::string& id,
                                      const std::string& novoPaiId)
    {
        ElementSlot sourceSlot;
        if (!FindElementSlot(modo.raiz, id, sourceSlot)) return false;
        Element& source = (*sourceSlot.container)[sourceSlot.index];
        if (source.bloqueado) return false;
        if (novoPaiId == id || (!novoPaiId.empty() && ElementContains(source, novoPaiId)))
            return false;

        Element* newParent = novoPaiId.empty() ? nullptr : ResolverId(modo, novoPaiId);
        if (!novoPaiId.empty() && (!newParent || newParent->bloqueado)) return false;

        Element moved = std::move(source);
        sourceSlot.container->erase(sourceSlot.container->begin() + sourceSlot.index);
        if (novoPaiId.empty())
        {
            modo.raiz.push_back(std::move(moved));
            return true;
        }

        newParent = ResolverId(modo, novoPaiId);
        if (!newParent) return false;
        newParent->filhos.push_back(std::move(moved));
        return true;
    }

    std::vector<Element> Project::CopiarElementos(
        const Modo& modo, const std::vector<std::string>& ids)
    {
        std::vector<Element> copies;
        CollectCopies(modo.raiz, ids, copies);
        return copies;
    }

    std::vector<std::string> Project::ColarElementos(
        Project& projeto, Modo& modo, const std::vector<Element>& elementos,
        float deslocamento)
    {
        return ColarElementosOffset(projeto, modo, elementos,
                                    deslocamento, deslocamento);
    }

    std::vector<std::string> Project::ColarElementosOffset(
        Project& projeto, Modo& modo, const std::vector<Element>& elementos,
        float dx, float dy)
    {
        std::vector<std::string> reservedIds;
        std::vector<std::string> pastedRootIds;
        for (const Element& source : elementos)
        {
            Element copy = source;
            AssignFreshIds(copy, projeto, reservedIds);
            OffsetElementTreeXY(copy, dx, dy, (float)projeto.telaBaseLargura,
                                (float)projeto.telaBaseAltura);
            pastedRootIds.push_back(copy.id);
            modo.raiz.push_back(std::move(copy));
        }
        return pastedRootIds;
    }

    namespace
    {
        // Inverte a posição do elemento (e de toda a subárvore) em torno do
        // eixo central, alternando o flag de espelhamento das formas.
        void FlipElementTree(Element& element, float center, bool horizontal)
        {
            const float x = element.transformacao.value("x", 0.0f);
            const float y = element.transformacao.value("y", 0.0f);
            const float w = element.transformacao.value("largura", 160.0f);
            const float h = element.transformacao.value("altura", 32.0f);
            if (horizontal)
                element.transformacao["x"] = 2.0f * center - (x + w);
            else
                element.transformacao["y"] = 2.0f * center - (y + h);
            if (element.tipo == "retangulo" || element.tipo == "painel" ||
                element.tipo == "elipse" || element.tipo == "poligono" ||
                element.tipo == "linha")
            {
                const char* key = horizontal ? "espelhado_h" : "espelhado_v";
                element.transformacao[key] =
                    element.transformacao.value(key, 0.0f) > 0.5f ? 0.0f : 1.0f;
            }
            for (Element& child : element.filhos)
                FlipElementTree(child, center, horizontal);
        }
    }

    void Project::EspelharElementos(Modo& modo,
                                    const std::vector<std::string>& ids,
                                    bool horizontal)
    {
        // Centro da caixa conjunta da seleção (eixo de espelhamento).
        float left = FLT_MAX, top = FLT_MAX, right = -FLT_MAX, bottom = -FLT_MAX;
        bool found = false;
        for (Element& element : modo.raiz)
        {
            if (std::find(ids.begin(), ids.end(), element.id) == ids.end())
                continue;
            const float x = element.transformacao.value("x", 0.0f);
            const float y = element.transformacao.value("y", 0.0f);
            const float w = element.transformacao.value("largura", 160.0f);
            const float h = element.transformacao.value("altura", 32.0f);
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x + w);
            bottom = std::max(bottom, y + h);
            found = true;
        }
        if (!found) return;
        const float center = horizontal ? (left + right) * 0.5f
                                        : (top + bottom) * 0.5f;

        // Aplica recursivamente: inverte a posição em torno do centro e
        // alterna o flag de geometria nas formas vetoriais.
        for (Element& element : modo.raiz)
        {
            if (std::find(ids.begin(), ids.end(), element.id) != ids.end())
                FlipElementTree(element, center, horizontal);
        }
    }

    std::string Project::AgruparElementos(
        Modo& modo, const std::vector<std::string>& ids)
    {
        std::vector<size_t> indices;
        for (size_t index = 0; index < modo.raiz.size(); ++index)
        {
            const Element& element = modo.raiz[index];
            if (!element.bloqueado &&
                std::find(ids.begin(), ids.end(), element.id) != ids.end())
                indices.push_back(index);
        }
        if (indices.size() < 2) return std::string();

        float left = FLT_MAX, top = FLT_MAX, right = -FLT_MAX, bottom = -FLT_MAX;
        for (size_t index : indices)
        {
            const Element& element = modo.raiz[index];
            const float x = element.transformacao.value("x", 0.0f);
            const float y = element.transformacao.value("y", 0.0f);
            const float w = element.transformacao.value("largura", 160.0f);
            const float h = element.transformacao.value("altura", 32.0f);
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x + w);
            bottom = std::max(bottom, y + h);
        }

        std::string groupId;
        for (int number = 1; number < 100000; ++number)
        {
            groupId = "grupo_" + std::to_string(number);
            if (!ResolverId(modo, groupId)) break;
        }

        Element group;
        group.id = groupId;
        group.tipo = "grupo";
        group.nome = "Grupo";
        group.transformacao = {
            { "x", left }, { "y", top },
            { "largura", right - left }, { "altura", bottom - top }
        };

        const size_t insertAt = indices.front();
        for (auto it = indices.rbegin(); it != indices.rend(); ++it)
        {
            group.filhos.insert(group.filhos.begin(), std::move(modo.raiz[*it]));
            modo.raiz.erase(modo.raiz.begin() + *it);
        }
        modo.raiz.insert(modo.raiz.begin() + std::min(insertAt, modo.raiz.size()),
                         std::move(group));
        return groupId;
    }

    bool Project::DesagruparElementos(Modo& modo, const std::string& groupId)
    {
        ElementSlot slot;
        if (!FindElementSlot(modo.raiz, groupId, slot)) return false;
        Element& group = (*slot.container)[slot.index];
        if (group.tipo != "grupo" || group.bloqueado) return false;

        // Os filhos usam coordenadas absolutas (o agrupar move elementos da
        // raiz sem alterar x/y) — sobem para o nível do grupo intactos.
        std::vector<Element> children = std::move(group.filhos);
        slot.container->erase(slot.container->begin() + slot.index);
        size_t at = std::min(slot.index, slot.container->size());
        for (Element& child : children)
            slot.container->insert(slot.container->begin() + at++, std::move(child));
        return true;
    }

    std::string Project::ClonarElemento(Modo& modo, Project& projeto,
                                        const std::string& id)
    {
        ElementSlot slot;
        if (!FindElementSlot(modo.raiz, id, slot)) return std::string();
        Element& original = (*slot.container)[slot.index];
        if (original.bloqueado) return std::string();

        Element copy = original;
        std::vector<std::string> reservedIds;
        AssignFreshIds(copy, projeto, reservedIds);
        slot.container->insert(slot.container->begin() + slot.index + 1,
                               std::move(copy));
        return (*slot.container)[slot.index + 1].id;
    }

    bool Project::MoverCamada(Modo& modo, const std::string& id, int delta)
    {
        ElementSlot slot;
        if (!FindElementSlot(modo.raiz, id, slot)) return false;
        Element& element = (*slot.container)[slot.index];
        if (element.bloqueado) return false;
        if (delta == 0) return false;

        std::vector<Element>& container = *slot.container;
        size_t index = slot.index;
        size_t target = delta > 0 ? index + 1 : index - 1;
        if (target >= container.size()) return false;

        // Troca de posição com o vizinho (sobe = renderiza depois = frente;
        // desce = renderiza antes = atrás). Move o elemento completo.
        std::swap(container[index], container[target]);
        return true;
    }
}
