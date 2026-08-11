# M07 — Chat de IA no SeedUI (planejamento para validação)

> Este documento descreve **como** o chat de IA funcionará dentro do SeedUI, **por que**
> ele depende dos Milestones anteriores e **o que precisa ser validado** antes de programar.
> Nada aqui foi implementado ainda — é a proposta para aprovação.

---

## 1. Contexto e objetivo

O usuário quer conversar com uma IA **dentro do SeedUI**, sem sair do programa, e que a IA
modifique a interface **em tempo real**, funcionando em conjunto com o F5.

A diretriz original (anotação sobre a caixa de diálogo flutuante):

> "Seria possível que essa caixa de diálogo flutuante pudesse ser interativa com um modelo
> de IA open-source, para que não precisasse sair dessa interface, possibilitando funcionar
> em conjunto com a função F5, em que pudesse ser modificada em tempo real a interface?"

Objetivo do M07:

1. Um **painel de chat de IA** dentro do SeedUI (janela fixa ou flutuante);
2. Campo de **chave de API** (o usuário já desenhou: um botão ao lado do "Confirmar");
3. A IA recebe o **contexto do projeto** (manual + projeto + anotação) e devolve
   **edições estruturadas** que o SeedUI aplica **na hora**, com desfazer;
4. Funciona **100% offline** quando não há chave/internet (o chat simplesmente fica
   desativado; o resto do SeedUI não muda).

---

## 2. Restrições confirmadas (regras do projeto)

| Regra | Como o M07 respeita |
|---|---|
| Offline-first | Chat é **opcional**. Sem chave ou sem internet → recurso desativado, SeedUI normal. |
| PC fraco | Nada pesado roda local. A IA roda na **nuvem** (OpenRouter). O app só faz HTTP + JSON (leve). |
| Sem código arbitrário | A IA **nunca** escreve código executável. Ela devolve **JSON do formato do projeto**; o SeedUI valida e aplica. |
| Não quebrar o programa | Toda edição da IA entra com **undo** e botão **Aplicar / Descartar**. |
| Formato legível por IAs | O `projeto.ui.json` é a linguagem comum entre SeedUI, engine e IAs (documentado no M03). |
| Web opcional | O chat usa a internet **só quando o usuário ativar** com a chave. Não é dependência. |

---

## 3. Provedores de IA

### 3.1 OpenRouter (provedor principal — nuvem)

- **Uma chave de API** dá acesso a dezenas de modelos open-source: DeepSeek, Qwen, Llama,
  Mistral, Gemma, etc.
- Existem modelos **gratuitos** no OpenRouter (com limites de uso) e modelos pagos por uso
  (centavos por conversa). O usuário escolhe o modelo numa lista dentro do chat.
- Requer internet e a chave criada em openrouter.ai (conta simples, sem cartão para o plano
  gratuito).

**Modelo padrão escolhido (validação do usuário):** `deepseek/deepseek-v4-flash:free` — o
mesmo modelo que ele já usa no Freebuff, **confirmado disponível gratuitamente no OpenRouter**
(sem cartão). Alternativa de teste: `qwen/qwen3-coder:free`. Lista será carregada do OpenRouter
no próprio app, com os gratuitos em primeiro.
- **Por que OpenRouter e não outro:** é o único que junta todos os modelos open-source num
  só lugar, com uma chave só, e fala o protocolo padrão "OpenAI-compatible" — o mesmo código
  serve para outros provedores no futuro.

### 3.2 Ollama (local) — opcional, para outros usuários

- Modelo rodando no próprio PC, gratuito e offline.
- **Decisão registrada: o usuário atual NÃO usa** (PC sem potência). Fica como opção futura
  no mesmo código (mesmo protocolo HTTP), mas **fora do escopo do M07** para este usuário.

### 3.3 O que NÃO é viável embutir (resposta às opções citadas)

| Ferramenta | Papel real | No SeedUI |
|---|---|---|
| **Freebuff (este chat)** | Assistente que roda fora do programa | Não embutível. A ponte já existe: exportar diretrizes → colar aqui → "mandei". O formato exportado foi feito para isso. |
| **OpenCode / MonkeyCode** | Agentes de terminal | Não embutíveis de forma confiável. Desnecessários com o chat no app. |
| **OpenRouter** | Agregador de modelos open-source (nuvem) | **SIM — provedor principal do M07.** |
| **Ollama** | Modelo local | Opção futura para PCs fortes; fora do escopo deste usuário. |

---

## 4. Arquitetura técnica

### 4.1 Dependências novas (as únicas do M07)

| Biblioteca | O que faz | Licença | Por quê |
|---|---|---|---|
| **WinHTTP** (do Windows) | Conexão HTTPS com o OpenRouter | Nativa do Windows | Sem dependência nova; resolve TLS/criptografia sozinho. |
| **nlohmann/json** (1 header) | Ler/escrever JSON do projeto e das respostas da IA | MIT (venda permitida) | Header único, padrão de mercado, compatível com o formato do projeto. |

Registrar ambas no `THIRD_PARTY.md` (licenças permitem venda — conferir antes de fechar o M07).

### 4.2 Onde o chat fica na interface

**Decisão do usuário (validada): janela flutuante, minimizável e acoplável.**

- O chat abre como uma **janela flutuante** sobre o workspace (arrastável, como o popup de
  anotação);
- Pode ser **minimizada** (vira um botão/aba discreto) e reaberta;
- Pode ser **acoplada na lateral direita** (encaixe simples na borda, estilo modular) — v1
  implementa flutuar + minimizar + encaixar na direita; **docking completo** (arrastar entre
  bordas, abas múltiplas) fica para depois, porque exigiria trocar o ImGui para a versão com
  docking (a mesma biblioteca é compartilhada com a engine — risco a evitar no M07).

Conteúdo da janela:
  - Seletor de **modelo** (lista dos open-source do OpenRouter, gratuitos primeiro);
  - Campo da **chave de API** (salva nas preferências do usuário — nunca no arquivo do projeto);
  - Histórico da conversa + campo de digitação + botão Enviar (+ botão **Parar**);
  - Botão **"Aplicar" / "Descartar"** quando a IA devolver uma edição;
  - Indicador de status: `Sem chave (chat desativado)` · `Conectando…` · `Online · modelo X`.
- **Integração com a caixa da anotação** (como o usuário desenhou): um **botão de chave 🔑
  ao lado do "Confirmar"** abre as configurações da IA direto da anotação; e um botão
  **"Enviar esta anotação para a IA"** transforma a anotação em pedido do chat.

### 4.3 O que a IA recebe (contexto automático)

Para a IA entender sem explicações repetidas, cada pedido envia:

1. **Resumo do manual do SeedUI** (o "documento de manual" que já existe — versão compacta
   com propósito, formato, IDs, eventos e regras);
2. **O projeto atual** (`projeto.ui.json` — a parte relevante, ou resumido se muito grande);
3. **O pedido do usuário** (texto do chat ou o texto da anotação marcada).

Isso realiza a ideia original: *"quando eu for comentar na caixa de diretrizes, a IA já
entende de cara qual alteração e onde será implementada, sem eu precisar explicar toda vez"*.

### 4.4 Formato da resposta da IA (sem código)

A IA não escreve código. Ela devolve **edições estruturadas** no formato do projeto:

```json
{
  "resposta": "Alterei a cor de fundo do painel para azul escuro.",
  "edicoes": [
    { "id": "painel_objetos", "propriedade": "cor_fundo", "valor": "#1e293b" },
    { "id": "botao_adicionar_objeto", "propriedade": "raio_canto", "valor": 8 }
  ]
}
```

Regras da resposta:

- **Só** referência por **ID** (nunca por posição ou texto) — regra do projeto;
- **Só** propriedades conhecidas pelo SeedUI (lista validada no inspetor);
- O SeedUI **valida** cada edição antes de aplicar (propriedade existe? valor válido?).

### 4.5 Aplicação em tempo real (o "F5" da edição)

1. Usuário digita o pedido no chat (ou envia uma anotação) e clica em Enviar;
2. SeedUI monta o contexto e chama o OpenRouter (HTTPS via WinHTTP);
3. IA devolve o JSON de edições;
4. SeedUI valida, aplica no modelo do projeto **em memória** e **re-renderiza na hora**;
5. A edição entra no histórico de **undo** (desfazer) e fica marcada como *proposta da IA*
   com **Aplicar / Descartar**;
6. Só persiste no arquivo quando o usuário salva (igual a qualquer edição manual).

**Resultado:** a mudança aparece em tempo real, sem fechar o programa. O **F5** continua
existindo como recarga/fallback (recarrega tema, ícones e manual dos arquivos), mas deixa de
ser necessário para edições.

### 4.6 Segurança

- A IA **nunca** executa nada — só devolve JSON validado;
- Valores fora da lista de propriedades conhecidas → rejeitados com aviso;
- A chave de API fica nas preferências locais, nunca no `projeto.ui.json` nem no export;
- O pedido pode ser **cancelado** (botão Parar);
- **Memória das conversas (decisão do usuário):** o histórico é salvo por projeto num arquivo
  separado (`projeto.conversas.json` — fora do `projeto.ui.json`, para o arquivo visual
  continuar limpo e comparável). Quando o histórico passar do limite configurado (nº de
  mensagens ou tamanho em KB), as mensagens **mais antigas que já foram executadas e
  finalizadas** (aplicadas ou descartadas) são removidas automaticamente; mensagens ainda
  pendentes (propostas em aberto) nunca são apagadas. Limite ajustável em Preferências.

---

## 5. Experiência do usuário (leigo, designer)

- **Não precisa saber nada de IA**: escolhe o modelo na lista, cola a chave, e conversa em
  português normal ("deixa o fundo do painel azul escuro", "aumenta esse botão");
- **Sem sair do programa**: tudo acontece dentro do SeedUI;
- **Feedback claro**: a resposta da IA aparece no chat; as mudanças aplicadas aparecem no
  canvas na hora; cada mudança pode ser **desfeita** com um clique;
- **Modo offline**: sem chave, o chat mostra "desativado" e o SeedUI continua 100% funcional;
- **Fluxo com anotações**: marcar a área → escrever → "Enviar para a IA" → conferir a
  proposta → Aplicar.

---

## 6. Pré-requisitos (por que o M07 vem depois)

| Milestone | O que entrega | Por que é pré-requisito |
|---|---|---|
| M03 | Criar/abrir/salvar `projeto.ui.json` | A IA só pode editar o que existe como projeto real. |
| M04 | Hierarquia e biblioteca funcionais | IDs estáveis + elementos reais no canvas. |
| M05 | Manipulação + undo/redo | "Aplicar / Descartar" depende do undo. |
| M06 | Inspetor completo | A lista de "propriedades conhecidas" que valida a IA vem do inspetor. |

**Sem M03–M06 não há o que a IA modificar** — hoje o workspace é demonstração.

---

## 7. Etapas de implementação (testáveis em partes)

| Etapa | Conteúdo | Teste |
|---|---|---|
| 7.1 | Adicionar WinHTTP + nlohmann/json e testar conexão com o OpenRouter (sem UI) | Log "conectado" + resposta bruta de um ping simples |
| 7.2 | Painel IA com chave, seletor de modelo e chat básico (texto) | Conversa simples funciona com chave real |
| 7.3 | Contexto automático (manual resumido + projeto) | Pedido "mude a cor do painel X" entende o projeto |
| 7.4 | Resposta → validação → aplicação com undo + Aplicar/Descartar | Edição aparece no canvas e desfaz |
| 7.5 | Integração com anotações (botão 🔑 e "Enviar para a IA") + botão Parar | Fluxo completo com anotação |
| 7.6 | Modo offline, avisos, limpeza e desempenho | Sem chave → chat desativado, resto intacto |

Cada etapa é validada pelo usuário antes de seguir (mesma disciplina dos milestones).

---

## 8. Critérios de aceitação (M07 completo)

1. Abrir o SeedUI **sem chave** → tudo funciona, chat mostra "desativado";
2. Colar chave + escolher modelo → conversar em português;
3. Pedir uma mudança → ela aparece **no canvas em tempo real**;
4. Desfazer a mudança da IA → volta ao estado anterior;
5. Enviar uma **anotação** para a IA → a IA responde sobre exatamente aquela área;
6. Fechar e reabrir o programa → preferências (chave) preservadas, projeto intacto;
7. **Nenhuma** alteração visual quebra a engine ou os vínculos (IDs estáveis).

---

## 9. Riscos e limitações (honestos)

- **Modelos open-source podem errar**: a edição é sempre **proposta** (Aplicar/Descartar),
  nunca automática sem revisão;
- **Latência**: depende da internet (1–10 s por resposta típica; streaming pode melhorar a
  sensação — avaliar na etapa 7.2);
- **Custos**: modelos gratuitos existem, mas com limite de uso; modelos melhores custam
  frações de centavo por conversa (avaliar na prática com o usuário);
- **Privacidade**: o texto do projeto é enviado ao provedor **só** quando o usuário usa o
  chat; quem quiser 100% privacidade continua usando o fluxo offline (exportar → Freebuff);
- **F5 não resolve tudo**: recarrega arquivos; edições da IA não precisam dele, mas código
  novo continua exigindo recompilar (processo normal do programa);
- **Chave vazada**: a chave fica no PC do usuário; orientação de não compartilhar o arquivo
  de preferências.

---

## 10. Decisões validadas (respostas do usuário em 11/08/2026)

| # | Pergunta | Resposta validada |
|---|---|---|
| 1 | Modelo padrão | **DeepSeek V4 Flash** (`deepseek/deepseek-v4-flash:free` — gratuito confirmado no OpenRouter), igual ao que ele usa no Freebuff; **Qwen** como segunda alternativa de teste. |
| 2 | Onde fica o chat | **Janela flutuante** minimizável, com opção de **acoplar na lateral** de forma modular (v1: flutuar + minimizar + encaixar à direita; docking completo depois — ImGui sem docking hoje). |
| 3 | Streaming | **Deixar para depois** (recomendação): v1 mostra "IA digitando…" enquanto responde; se a espera incomodar, adicionamos streaming (SSE) como melhoria pós-M07. |
| 4 | Histórico da conversa | **Salvar por projeto** (`projeto.conversas.json`), para o usuário não repetir o que já conversou. **Limpeza automática:** ao passar do limite configurado, apagam-se as mensagens mais antigas **já executadas e finalizadas** (aplicadas/descartadas); pendentes nunca são apagadas. Limite ajustável em Preferências. |
| 5 | Ordem de execução | **Confirmado pelo usuário:** "primeiro a gente cria o sistema de plantas" — M07 só começa depois do M03 (projeto real) e idealmente M04–M06 — ver seção 6. |

---

## 11. Impacto no desempenho (PC fraco)

- O chat é **HTTP + JSON**: praticamente zero custo de CPU/GPU quando ocioso;
- Nada de rede acontece sem o usuário usar o chat;
- As edições aplicadas reutilizam a renderização normal do SeedUI (sem efeitos extras);
- O ganho do OpenRouter é justamente **não** exigir potência local.
