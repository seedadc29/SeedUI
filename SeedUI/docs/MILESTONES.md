# Milestones do SeedUI — Planejamento por Etapas

> Este documento registra o planejamento completo do SeedUI em marcos
> numerados (Milestone 01, 02, ...), espelhando a prática usada no
> desenvolvimento da engine (`Agente.md`). Cada marco termina **executável e
> testável**: só se avança para o próximo após validação. Nenhuma etapa deve
> quebrar o que já funciona — o SeedUI é independente da engine.

## Regras de conduta (boas práticas)

1. **Documentar antes de programar**: cada decisão importante entra neste
   plano e nos manuais.
2. **Executar por partes**: um marco por vez, com teste e aprovação ao final.
3. **Não inventar APIs da engine**: verificar o código existente antes de
   qualquer integração.
4. **Não misturar aparência e lógica** no arquivo exportado.
5. **IDs estáveis como contrato** — nunca conectar por posição na tela ou texto.
6. Ao final de cada marco: resumo do que mudou, arquivos, testes realizados e
   pendências.

## Visão geral

| Milestone | Foco | Status |
|---|---|---|
| 01 | Fundação: documentação, formato, identidade visual | Em andamento |
| 02 | Esqueleto executável do editor | Concluído |
| 03 | Modelo de dados e salvamento (`projeto.ui.json`) | Concluído |
| 04 | Hierarquia e biblioteca de componentes | Em andamento |
| 05 | Canvas interativo (seleção, transformação, undo/redo) | Planejado |
| 06 | Inspetor e diretrizes | Planejado |
| 07 | Texto e fontes | Planejado |
| 08 | Recursos externos (PNG/JPG/WebP/SVG) | Planejado |
| 09 | Modelos de interface (templates estilo Canva) | Planejado |
| 10 | Telas, modos e workspaces completos | Planejado |
| 11 | Estados e pré-visualização interativa | Planejado |
| 12 | Temas, variáveis e componentes reutilizáveis | Planejado |
| 13 | Exportação e relatório para a IA | Planejado |
| 14 | Qualidade, acessibilidade e desempenho | Planejado |
| 15 | Integração com a engine (futuro, opcional) | Planejado |

## Milestone 01 — Fundação e documentação

**Objetivo**: definir arquitetura, formato e identidade; documentar tudo antes
de programar.

**Entregáveis**:
- `docs/SEEDUI_MANUAL.md` — manual interno (funcionamento, fluxo com a IA, diretrizes).
- `docs/FORMATO_PROJETO.md` — especificação do `projeto.ui.json` (telas, modos, elementos, diretrizes).
- `docs/IDENTIDADE_VISUAL.md` — identidade visual Blender + Photoshop.
- `docs/MILESTONES.md` — este plano.

**Decisões registradas**:
- Web = caso opcional; SeedUI desktop, offline.
- Comportamento = campo de diretrizes em texto comum (sem código no arquivo).
- Manual interno embutido e exportado com o projeto (`README.md`).
- SeedUI standalone primeiro; integração com a engine só no Milestone 15.
- Identidade visual = fusão Blender + Photoshop (clareza acima de tudo).
- Sistema de modelos de interface (padrões + "Meus Modelos"), estilo Canva.
- Formato com telas (múltiplas interfaces) e modos (workspaces estilo Blender).

**Critérios de aceitação**: documentos revisados e aprovados pelo usuário.

**Testes**: leitura e validação dos documentos (ainda não há código).

## Milestone 02 — Esqueleto executável do editor

**Objetivo**: o programa abre com o visual profissional escuro (identidade
Blender + Photoshop) e o workspace montado, sem tocar na engine.

**Entregáveis**:
- Projeto Visual Studio `SeedUI.sln` (C++17 + raylib), executável standalone.
- Barra de menus, barra de ferramentas, canvas central (grade/réguas), painéis
  acoplados, barra de status.
- Tema escuro da identidade visual implementado.
- **Sistema de ícones**: Phosphor (MIT) + Lucide (ISC) embutidos em
  `assets/icons/`, rasterizados com lunasvg (MIT); ícone + tooltip em toda
  ação.
- Tela inicial (novo projeto, modelos, recentes, abrir).
- Manual acessível dentro do app (atalho F1 do SeedUI).

**Critérios**: abrir o programa, navegar painéis, redimensionar docks,
fechar/reabrir sem erros; visual conforme identidade.

**Status**: concluído.

**Notas**: reutiliza raylib + Dear ImGui da engine (`../Game/ThirdParty`);
ícones Phosphor (peso *thin*) embutidos em `assets/icons/` e rasterizados com
nanosvg; fonte Inter em `assets/fonts/`; F12 captura a tela; `--capture` tira
capturas automaticamente para teste; manual aberto com F1.

**Testes**: build nas 3 configurações; execução sem crash (capturas da tela
inicial e do workspace verificadas visualmente).

## Milestone 03 — Modelo de dados e salvamento

**Objetivo**: o editor lê e grava `projeto.ui.json` (v1) com telas e modos.

**Entregáveis**: criar/abrir/salvar projeto; telas e modos; indicador "não
salvo"; conversão de versão do formato; caminhos relativos de recursos.

**Critérios**: salvar e reabrir sem perda; arquivo legível por humanos; IDs
estáveis preservados.

**Testes**: criar projeto, adicionar tela/modo, salvar, fechar, reabrir,
conferir o JSON.

**Status**: concluído.

**Notas**:
- Dependência nova: **nlohmann/json** (header único, MIT) em
  `ThirdParty/json/` — registrado no `THIRD_PARTY.md`.
- `raylib` e `imgui` foram **vendidos para dentro** de `SeedUI/ThirdParty/`
  (com cópias dos backends GLFW/OpenGL3), tornando o SeedUI **standalone**:
  não depende mais de `../Game/ThirdParty`. O `.vcxproj` foi ajustado.
- `Project.h/cpp` implementa o modelo (telas → modos → árvore de elementos)
  com serialização v1. Blocos opcionais (transformacao, layout, estilos,
  estados, propriedades) são preservados sem perda como JSON puro — M06
  (inspetor) os edita com tipos.
- Diálogos de arquivo nativos do Windows (`FileDialogs.cpp`, commdlg32):
  Abrir (`Ctrl+O`), Salvar (`Ctrl+S`), Salvar como (`Ctrl+Shift+S`).
- Novo projeto cria tela "Tela principal" + modo "Padrão" com a base da
  resolução escolhida; seletores de Tela/Modo no menu (estilo Blender) e no
  painel HIERARQUIA funcionam; status bar mostra "● Alterações não salvas".
- Canvas desenha os elementos da tela/modo ativa (caixas simples com ID).

**Testes realizados**: build nas 3 configurações; `--capture` sem crash;
criar/abrir/salvar/reabrir `projeto.ui.json` e conferir o JSON.

## Milestone 04 — Hierarquia e biblioteca de componentes

**Objetivo**: montar a árvore de elementos visualmente.

**Entregáveis**: painel Hierarquia (árvore, renomear, reordenar, pais/filhos,
bloquear, ocultar, pesquisar); Biblioteca de componentes (arrastar para o
canvas); criação de elementos básicos (painel, janela, botão, texto, campo
numérico, slider).

**Critérios**: arrastar componente → aparece no canvas e na árvore; renomear
ID; reordenar; salvar/recarregar mantém a estrutura.

**Progresso implementado**:
- Hierarquia pesquisável por nome, ID ou tipo.
- Seleção, renomeação visual (F2), edição de ID único no Inspetor, excluir
  (Delete), ocultar/mostrar, bloquear/desbloquear e mover acima/abaixo.
- Drag-and-drop entre elementos para criar relações pai/filho; soltar no modo
  ativo devolve o elemento à raiz, com proteção contra ciclos.
- Biblioteca com componentes básicos clicáveis e arrastáveis; drop no canvas
  converte a posição para coordenadas da tela base e cria o elemento no ponto.
- Estrutura continua serializada pelo formato v1 (`filhos`, ordem, visibilidade,
  bloqueio e transformação).

**Validação atual**:
- `SeedUI.exe --self-test-m04`: 23 verificações aprovadas, cobrindo ordem,
  pai/filho, proteção contra ciclos, exclusão aninhada, bloqueio efetivo,
  seleção por ponto e persistência JSON da estrutura e dos estados.
- Builds Debug, Development e Release aprovados; autoteste passa nas três configurações e a captura automática abre/renderiza/fecha sem crash.
- A seleção simples por clique no canvas foi antecipada do M05 para tornar a
  Hierarquia e o Inspetor utilizáveis já no M04.
- Ainda requer validação manual do gesto de drag-and-drop com o mouse (Biblioteca
  → canvas e elemento → pai); o código compila e os efeitos de modelo estão
  cobertos, mas isso não será declarado concluído sem testar o gesto real.

### Escopo de validação do Painel (objeto vetorial) — aprovado em 2026-08-12

O **Painel** é o retângulo vetorial editável que serve de base da composição e,
quando habilitado a uma função, de elemento funcional. Para validá-lo ele precisa
do conjunto completo de ferramentas vetoriais (estilo CorelDRAW/Illustrator).
**Todas as ferramentas entram na barra lateral esquerda** (grupos existentes:
Seleção/Transformação, Criação/Aparência, Navegação/Visualização, Revisão),
alinhadas em grade, com ícone + tooltip + atalho. Uma etapa por vez, validada
pelo usuário antes de seguir. Rastreio em `docs/CHECKLIST_M04_PAINEL.md`.

| Etapa | Conteúdo | Status |
|---|---|---|
| 1 | **Cores de preenchimento e contorno**: seletor de cor (`cor_fundo`/`cor_borda`) na barra lateral e no Inspetor, swatch no botão, conta-gotas aplicando no selecionado | Pendente |
| 2 | **Transformações de precisão**: rotação (alça no canvas + valor no Inspetor), espelhar H/V, nudge (setas 1px / Shift 10px), proporção travada (Shift no resize) | Pendente |
| 3 | **Grupo avançado**: desagrupar (filhos sobem ao nível do pai preservando posição), redimensionar grupo escalando filhos, duplicar (Ctrl+D) com deslocamento fixo, repetir último deslocamento | Pendente |
| 4 | **Estilo de linha e sombra**: contorno tracejado, pontas (caps), junções (joins), sombra (`sombra`: cor, deslocamento, desfoque) | Pendente |
| 5 | **Auxílio de precisão**: guias arrastadas da régua com snap e bloquear/apagar, guias inteligentes (centro/bordas no arraste), ferramenta medir (distância/ângulo com rótulo, sem entrar no JSON) | Pendente |

Critérios da etapa: painel com todas as funções vetoriais acima; toda ferramenta
nova na barra esquerda com ícone + tooltip + atalho; refletir no
`projeto.ui.json` (guias e medições são só auxílio de edição, não entram no
arquivo); autoteste ampliado; avançar uma etapa por vez após validação do usuário.

### Identidade premium dos ícones (barra lateral) — parte do M04

- **Alinhamento**: ícones alinhados em grade consistente na barra (mesmo centro,
  mesmo grid, separadores por família) — sempre, em qualquer nova ferramenta.
- **Modelo de ícone estilo Blender**: traço fino consistente (~1.5px), cantos
  arredondados, glifos simples e legíveis, substituindo o desenho atual onde
  houver distância do padrão — sem quebrar o que já existe, apenas refinando.
- **Cor por família** (paleta restrita da identidade, sem arco-íris):

| Família (grupo da barra) | Cor de destaque |
|---|---|
| Seleção / Transformação | azul `#4f8cff` |
| Criação | verde `#2ecc71` |
| Aparência | lilás `#a78bfa` (validar com o usuário) |
| Navegação / Visualização | neutro (sem cor) |
| Revisão / Anotações | laranja `#f57900` |

- A cor aparece **sutil**: ícone ativo/hover e barra fina no separador do grupo;
  ícones inativos ficam em cinza neutro. Detalhes em `IDENTIDADE_VISUAL.md` §6.
## Milestone 05 — Canvas interativo

**Objetivo**: manipulação visual direta dos elementos.

**Entregáveis**: seleção e multisseleção; mover; redimensionar (cantos e
laterais); duplicar; copiar/colar; apagar; alinhar/distribuir; grade/snap;
guias; zoom; navegação; **desfazer/refazer** (histórico de comandos).

**Critérios**: operações de transformação refletem no JSON; undo/redo
consistente.

## Milestone 06 — Inspetor e diretrizes

**Objetivo**: edição numérica e visual das propriedades.

**Entregáveis**: transformação (X/Y, largura/altura, escala, rotação,
opacidade, ordem), layout, aparência (cor, borda, raio, sombra, gradiente),
tipografia, propriedades por tipo, estados (normal, hover, pressionado...),
**painel de diretrizes** (texto comum por elemento).

**Critérios**: alterar propriedades reflete no canvas e no JSON; diretriz
salva e reabre; aviso de elemento interativo sem diretriz.

## Milestone 07 — Texto e fontes

**Objetivo**: tipografia completa.

**Entregáveis**: importar fontes TTF/OTF (cópia para `fontes/`); família,
tamanho, peso, cor, alinhamento, espaçamento, quebra de linha, reticências;
aviso de fonte ausente; lista de fontes do projeto.

## Milestone 08 — Recursos externos

**Objetivo**: importar e gerenciar PNG/JPG/WebP/SVG.

**Entregáveis**: importação com cópia para o projeto; gerenciador de recursos
(nome, tipo, dimensões, onde é usado, ausentes, duplicados, substituição
mantendo referências); uso como ícone, fundo, moldura; proteção contra SVG
inseguro (sanitização antes de rasterizar); rasterização via **lunasvg**
(já embutido no Milestone 02).

**Critérios**: importar um SVG e usá-lo em um botão; reabrir o projeto sem
depender do caminho original.

## Milestone 09 — Modelos de interface (templates)

**Objetivo**: galeria de modelos no estilo Canva.

**Entregáveis**:
- Modelos padrão inspirados em interfaces consagradas: **Blender** (workspace
  3D), **Photoshop** (edição de imagem), **Maya** (modelagem),
  **CorelDRAW/Illustrator** (vetorial), **Krita** (pintura), **ZBrush**
  (escultura) — além de modelos de jogo (HUD, inventário, menu, pausa) e
  modelos vazios por resolução.
- Salvar a própria interface do usuário como modelo ("Meus Modelos").
- Aplicar modelo → projeto novo pronto para editar; IDs sem conflito (sufixo
  automático).
- Miniaturas (preview) para cada modelo.

**Critérios**: abrir a galeria, escolher um modelo, editar, salvar como modelo,
reutilizar.

## Milestone 10 — Telas, modos e workspaces do editor

**Objetivo**: suporte completo a múltiplas telas/modos + workspaces.

**Entregáveis**: seletor de tela/modo (estilo Blender, modo ativo em laranja);
criar/duplicar/renomear telas e modos; pré-visualização por resolução
(1280×720 etc.); responsividade (âncoras, tamanhos mín/máx); modo de
comparação de resoluções; workspaces salvos do editor.

**Critérios**: alternar telas/modos sem perda; pré-visualizar em 2+ resoluções.

## Milestone 11 — Estados e pré-visualização interativa

**Objetivo**: testar o comportamento visual sem a engine.

**Entregáveis**: simular hover, clique, digitação, abrir/fechar, rolagem,
tooltips; ações simuladas (abrir painel, trocar aba, mostrar mensagem...);
transições.

**Critérios**: a pré-visualização interativa responde sem lógica real.

## Milestone 12 — Temas, variáveis e componentes reutilizáveis

**Objetivo**: estilos globais.

**Entregáveis**: paletas; estilos de texto/botão/painel; variáveis globais
(mudou → atualiza tudo); desvincular elemento do estilo; componentes
reutilizáveis; temas (minimalista, industrial, medieval, futurista, claro,
escuro).

## Milestone 13 — Exportação e relatório para a IA

**Objetivo**: pacote pronto para a IA/engine.

**Entregáveis**: exportar pasta com `projeto.ui.json` + recursos + `README.md`
(manual); relatório do projeto (telas, componentes, IDs, tipos, diretrizes,
recursos, avisos); validação de compatibilidade.

**Critérios**: abrir a pasta exportada em outro lugar sem recursos quebrados;
relatório legível por humanos e por IA.

## Milestone 14 — Qualidade, acessibilidade e desempenho

**Objetivo**: robustez, inclusive em máquinas fracas.

**Entregáveis**: validações de acessibilidade (contraste, tamanho mínimo,
foco, texto cortado...); diagnóstico e registro de erros copiáveis;
desempenho (cache de recursos, efeitos opcionais, avisos de custo);
preferências; atalhos configuráveis; tema claro; salvamento automático e
recuperação.

## Milestone 15 — Integração com a engine (futuro, opcional)

**Objetivo**: carregar os projetos exportados dentro do jogo.

**Entregáveis**: runtime declarativo na engine (lê `projeto.ui.json` e desenha
com raylib); importar interfaces atuais (F1, F2, HUD, inventário, menus) com
relatório de divergências; conexão de funções por ID; preservação de atalhos
e comportamentos.

**Risco**: é o item mais arriscado — só começa após o Milestone 14 aprovado,
com plano próprio e sem alterar silenciosamente o comportamento atual da engine.

## Critérios de aceitação final (diretriz original)

1. Abrir um projeto de interface.
2. Criar painéis, textos, botões, campos numéricos e sliders.
3. Posicionar e redimensionar componentes visualmente.
4. Editar valores numericamente.
5. Alterar cores, bordas, arredondamento, fontes e espaçamento.
6. Organizar componentes em hierarquia.
7. Criar IDs estáveis.
8. Importar e utilizar um SVG externo.
9. Salvar e reabrir sem perda de dados.
10. Desfazer e refazer alterações.
11. Testar estados de interação.
12. Exportar a interface em formato legível.
13. Carregar a interface exportada dentro da engine (Milestone 15).
14. Associar um botão exportado a uma função real.
15. Modificar o visual sem apagar a função associada.
16. Alterar uma função sem modificar o visual.
17. Visualizar o resultado em diferentes resoluções.
18. Executar adequadamente em computador de baixo desempenho.
19. Converter ou reproduzir as interfaces atuais do F1 e do F2.
20. Gerar diagnóstico compreensível em caso de erro.

## Histórico de decisões

| Data | Decisão |
|---|---|
| 2026-08-11 | Web = caso opcional; SeedUI desktop, offline. |
| 2026-08-11 | Comportamento = campo de diretrizes em texto comum (sem código no arquivo). |
| 2026-08-11 | Manual interno embutido e exportado com o projeto (`README.md`). |
| 2026-08-11 | SeedUI standalone primeiro; integração com a engine só no Milestone 15. |
| 2026-08-11 | Identidade visual = fusão Blender + Photoshop (clareza acima de tudo). |
| 2026-08-11 | Sistema de modelos de interface (padrões + "Meus Modelos"), estilo Canva. |
| 2026-08-11 | Formato com telas (múltiplas interfaces) e modos (workspaces estilo Blender). |
| 2026-08-11 | Ícones = Phosphor (MIT) + Lucide (ISC) embutidos em SVG, rasterizados com lunasvg (MIT); licenças permitem venda (ver IDENTIDADE_VISUAL.md). |
| 2026-08-11 | Estratégia de venda definida (docs/PRECIFICACAO.md): faixas Gratuito/Pro/Studio; early access US$ 29 / R$ 149 no M13; v1.0 US$ 49 / R$ 199 no M14. |
| 2026-08-11 | Modo debug de anotações: selecionar área no canvas, rótulo com seta, caixa flutuante de texto e comentário geral (painel DIRETRIZES); copiar para a IA com Ctrl+Shift+C. |
| 2026-08-11 | Anotações do modo debug cobrem a janela INTEIRA (menus, ícones, painéis e canvas) em coordenadas da janela — não só o canvas. |
| 2026-08-11 | Exportação de diretrizes em arquivo: "Exportar diretrizes (.txt + print)" gera .txt + captura na pasta `diretrizes/` (ao lado do executável) e abre a pasta; o usuário só precisa dizer "veja as novas alterações". |
| 2026-08-11 | Janela do SeedUI abre menor que a tela (90% do monitor) com maximizar/restaurar (botão da janela, F11 e menu Exibir) — corrige a janela que "prendia" o usuário cobrindo a barra de tarefas. |
| 2026-08-11 | Popup de edição de anotação sempre DENTRO da janela (abaixo do rótulo; acima dele se não couber) — não sai mais do espaço de trabalho. |
| 2026-08-11 | Correção: ícones da barra de ferramentas estavam de cabeça para baixo (diretriz do usuário "Esta de cabeca pra baixo corrija", área da barra lateral). Causa: FlipVertical desnecessário — o nanosvg rasteriza de cima para baixo e o backend OpenGL do ImGui mostra a 1ª linha da textura no topo; o flip duplicava a inversão. Removido em Icons.cpp. |
| 2026-08-11 | Robustez dos ícones: carregamento ancorado na pasta do executável (não no diretório de trabalho) e IDs ImGui únicos por botão. Direção visual inspirada no Blender: hierarquia discreta, densidade consistente, áreas delimitadas, contexto local e estado ativo inequívoco. |
| 2026-08-11 | Base visual pré-M04 implementada: ações globais em barra horizontal sob os menus; lateral esquerda exclusiva para ferramentas contextuais agrupadas por família; conta-gotas com ícone próprio; painéis identificados por ícone + título; rótulos de anotação com altura adaptável; exportação global na barra inferior sem popup na captura. |
| 2026-08-11 | M07 planejado (docs/PLANEJAMENTO_M07_CHAT_IA.md): chat de IA dentro do SeedUI via OpenRouter (nuvem, open-source, uma chave) com edições em JSON aplicadas em tempo real (undo + Aplicar/Descartar). Ollama (local) fica fora do escopo — PC do usuário não tem potência. Dependências novas: WinHTTP (nativa) + nlohmann/json (MIT). Requer M03–M06 primeiro. |
| 2026-08-11 | M07 validado pelo usuário: modelo padrão `deepseek/deepseek-v4-flash:free` (gratuito confirmado no OpenRouter; Qwen como alternativa); chat em janela flutuante minimizável e acoplável à lateral (docking completo depois); streaming fora do v1; histórico da conversa salvo por projeto em `projeto.conversas.json` com limpeza automática de mensagens antigas já finalizadas (pendentes nunca apagadas). |
| 2026-08-12 | Metáfora visual da **Órbita** para a camada de programação visual (público não-programador): o **Sol** = elemento funcional (núcleo/ator, ex.: botão), os **planetas** = submecânicas/ações vinculadas (abrir painel, trocar aba, mostrar mensagem), a **órbita fina/serrilhada** = o vínculo funcional, e os **satélites** = comportamentos aninhados — conversando com o mapa mental e com a hierarquia parental existente. Camada de edição *sem código* em um **workspace próprio** (não junto do canvas), onde o autor arrasta ações para a órbita do elemento. |
| 2026-08-12 | Programação visual em **workspaces/salas** estilo Blender: o editor dividido em modos de nicho — **Layout** (workspace visual/atual: hierarquia, canvas, inspetor), **Editor/Edit** (parte visual aprofundada) e **Código/Code** (etapa programacional das órbitas, com auxílio de IA open-source) — uma sala por setor, alternáveis como no Blender; a interface visual já existente permanece intacta no workspace Layout, e o Code mode constrói a camada comportamental sobre ela. |
