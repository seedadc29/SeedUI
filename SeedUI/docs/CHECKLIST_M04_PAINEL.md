# Checklist M04 — Validação do Painel (objeto vetorial)

Rastreio das etapas de validação do **Painel** como objeto vetorial editável
(estilo CorelDRAW/Illustrator), aprovado em 2026-08-12. Uma etapa por vez,
**validada pelo usuário antes de seguir**. As ferramentas novas entram na
barra lateral esquerda (famílias existentes), com ícone + tooltip + atalho.

> Regra do formato: guias e medições são **auxílio de edição** — não entram
> no `projeto.ui.json`. O que persiste é a posição/tamanho/rotação finais.

| Etapa | Conteúdo | Status |
|---|---|---|
| 1 | **Cores de preenchimento e contorno**: seletor de cor (`cor_fundo`/`cor_borda`) na barra lateral e no Inspetor, swatch no botão, conta-gotas aplicando no selecionado | Parcial — paleta inferior + seletor 3 modelos prontos; falta na barra lateral/Inspetor |
| 2 | **Transformações de precisão**: rotação (alça no canvas + valor no Inspetor), espelhar H/V, nudge (setas 1px / Shift 10px), proporção travada (Shift no resize) | Parcial — **rotação pronta** |
| 3 | **Grupo avançado**: desagrupar (filhos sobem ao nível do pai preservando posição), redimensionar grupo escalando filhos, duplicar (Ctrl+D) com deslocamento fixo, repetir último deslocamento | Parcial — **redimensionar grupo e desagrupar prontos** |
| 4 | **Estilo de linha e sombra**: contorno tracejado, pontas (caps), junções (joins), sombra (`sombra`: cor, deslocamento, desfoque) | Pendente |
| 5 | **Auxílio de precisão**: guias arrastadas da régua com snap e bloquear/apagar, guias inteligentes (centro/bordas no arraste), ferramenta medir (distância/ângulo com rótulo, sem entrar no JSON) | Parcial — guias inteligentes prontas e **snap reforçado** (tol. 10px, grade em espaço de projeto); régua e medir pendentes |

## O que foi implementado (2026-08-12)

### Base vetorial — `src/Geo.h`
- Tesselação do contorno do elemento (retângulo com quinas, elipse, polígono)
  em coordenadas locais/projeto/tela.
- Rotação em torno de pivô (`transformacao.rotacao` em graus +
  `transformacao.centro_rotacao` opcional; default = centro).
- AABB rotacionado (`Geo::RotatedAABB`) para hit-test aproximado.

### Rotação (etapa 2 — parcial)
- Render do Canvas: elemento rotacionado desenhado como polígono tessellado
  preenchido + contorno com junções arredondadas.
- Seleção: contorno acompanha a rotação (sem caixa "gorda" nas quinas);
  alça de rotação acima do topo rotacionado; marcadores de quina ficam
  ocultos em elementos rotacionados.
- Interação: arrastar a alça gira em torno do pivô; **Shift = passo de 15°**;
  cursor `ResizeAll`; hit-test por AABB rotacionado em `Project.cpp`;
  mensagem de status "Rotação ajustada".

### Redimensionar grupo e multi-seleção (etapa 3 — quase completa)
- Multi-seleção + arrastar alça da caixa conjunta → todos escalam
  proporcionalmente a partir da caixa inicial comum (preserva layout
  relativo). Tamanho mínimo respeitado.
- **Caixa de seleção única** cobrindo TODO o conjunto (união dos limites
  rotacionados) com 8 alças + alça de rotação; contornos individuais leves;
  clique em espaço vazio dentro da caixa move o conjunto.
- **Rotação em conjunto**: todos os selecionados (e filhos de grupos) giram
  juntos em torno do centro da caixa conjunta, preservando cada rotação
  inicial (`CanvasTransformStart.rot`); Shift = passo de 15°.
- **Cores em conjunto**: paleta e seletor aplicam a TODOS os selecionados
  (mensagem indica quantos elementos foram coloridos).
- Pendente: duplicar com Ctrl+D (deslocamento fixo + repetir último).

### Desagrupar (etapa 3 — parcial)
- `Project::DesagruparElementos` devolve os filhos ao nível do grupo
  preservando posição (coordenadas absolutas); grupo bloqueado recusa.
- Atalho **Ctrl+Shift+G**, menu **Objeto → Desagrupar** e botão na barra de
  ações (ícone `corners-out`, Lucide/ISC).
- 4 verificações no autoteste: bloqueado recusa, remove o grupo, devolve os
  filhos, posição preservada.

### Cores (etapa 1 — parcial)
- Canvas renderiza `estilos.cor_fundo`/`cor_borda` (`#rrggbb`), fallback
  neutro; `src/ColorUtils.h` (parse/hex + HSV) e `src/ColorPicker.{h,cpp}`
  (seletor com 3 modelos: Triângulo, Quadrado, Barras) — novo arquivo
  registrado no `.vcxproj`.
- Paleta fixa **acima da barra de status** (`DrawColorBar`): clique
  esquerdo = preenchimento, direito = contorno; botão abre o seletor.
- Seletor com 3 modelos (Triângulo/Quadrado/Barras) — triângulo com
  subdivisão em quads e bordas suaves (sem serrilhado), alças maiores e
  prévia com borda.

### Seleção e zoom (refinamento)
- Seleção por caixa com **modo alternável**: padrão **cobertura total**
  (elemento só selecionado se a caixa cobrir o corpo inteiro); opção
  **"Selecionar ao encostar"** no menu para o comportamento antigo.
- **Zoom no cursor** aplicado a todos os controles (roda, botões, atalho Z)
  até o limite de 16×.

### Guias inteligentes (etapa 5 — parcial)
- `src/SmartGuides.h` (header-only, testável pelo autoteste).
- Durante o **mover**, bordas e centros da seleção encaixam na tela-base
  (bordas + centro) e em bordas/centros de elementos visíveis fora da
  seleção (pais com filhos selecionados movem inteiros).
- **Snap forte da moldura**: a tela-base (0/centro/W/H) tem tolerância 3×
  — o delimitador principal vence os demais snaps (re-puxada após a grade).
- **Guias de espaçamento (assertivas)**: QUALQUER espaço existente entre
  dois vizinhos é alvo de previsão — a seleção é puxada para replicá-lo com
  ímã forte (~10px) e o rótulo do espaço previsto fica destacado
  (`SmartGuides::ApplySpacing`; sem linhas longas — representação unificada
  nos traços das quinas). A previsão é **exata nas 4 direções** (esquerda,
  direita, acima, abaixo) — os sinais das ramificações "esquerda" e
  "acima" foram corrigidos para o encaixe vertical pousar exatamente no
  valor de referência (nunca 31/33).
- **Preview de espaçamento com Shift**: durante o mover (incluindo o clone
  por fork), segurar **Shift** mostra **pequenos traços ciano nos cantos
  das laterais** de cada objeto da fileira (mesmos topo/base) + **valor da
  distância** de cada espaço (`SmartGuides::ComputeSpacingPreview`); idem
  para colunas verticais (traços verticais nos cantos de topo/base). Só
  orientação visual, não entra no JSON.
- **Limiar de arrasto**: clique seleciona sem mover; o objeto só se move
  após ~4px de arrasto real (trava por gesto).
- **Alinhar ao Conjunto** (`AlignUtils.h`, testável): novo alvo de
  alinhamento "Conjunto" — a referência é o bounding box dos VIZINHOS
  (visíveis fora da seleção); centralizar H+V coloca a forma no centro exato
  do conjunto com distâncias uniformes nos 4 lados. 4 verificações novas no
  autoteste.
- **Organização do painel direito em abas** e **debug/exportar no menu bar**
  (nada sufoca o rodapé); paleta centralizada na faixa; ponto de sangria do
  topo alinhado à régua da coluna de ferramentas.
- Tolerância de 10px (em tela); ativas junto com o **Snap**.
- Linha-guia **magenta** (`Theme::SmartGuide`, #d946ef) desenhada no canvas
  na posição encaixada; some ao soltar.
- 8 verificações no autoteste M04 (`--self-test-m04`): encaixe de borda,
  centro da tela, fora da tolerância, seleção ignorando a si mesma, snap
  forte da moldura, e guias de espaçamento (X replica + Y sem repetição).

### Espaço de trabalho livre e área de segurança
- Objetos **não ficam presos** à moldura: mover/redimensionar/criar/marquee
  funcionam no canvas inteiro (as conversões `CanvasScreenToProject`/
  `CanvasProjectToScreen` devolvem coordenadas além da tela-base e seguem o
  zoom até 32×).
- **Grade pontilhada percorre o canvas inteiro** (alinhada às unidades de
  projeto 8/40 — continua em compasso com o snap em qualquer ponto).
- Ao sair da tela-base com elementos selecionados, um **contorno vermelho
  fino tracejado** contorna a moldura (aviso de área de segurança); o
  usuário pode voltar arrastando de volta.

### Correção de bug pré-existente
- `Missing PopStyleColor()` travava o build Debug no clique dos toggles
  **Snap**, **Réguas** e **Zoom In**: o estado era invertido entre o
  `PushStyleColor` e o `PopStyleColor`. Corrigido capturando o estado antes
  do botão (padrão do `ToolButton`). Ver MILESTONES → Histórico de decisões.

## Próximos passos da etapa 5
- Guias arrastadas da régua (snap, bloquear/apagar).
- Ferramenta **medir** (distância/ângulo com rótulo, sem entrar no JSON).

### Manipulação de conjunto, camadas e atalhos (2026-08-12)
- **Rotação em conjunto (corpo rígido)**: multi-seleção e grupos giram como
  corpo único orbitando o centro da caixa de seleção (`dragMode 14` + pivô do
  conjunto em `App.cpp`); rotação individual permanece como função secundária.
- **Grupo editável em conjunto**: alças de resize/rotação disponíveis em
  `dragModeAt` para grupos; starts coletados dos filhos (`CollectTransformStarts`
  exclui o próprio grupo); caixa do grupo recalculada no release do arrasto.
- **Cor recursiva em grupos**: paleta e seletor propagam `cor_fundo`/`cor_borda`
  a todos os filhos do grupo (`AplicarCorRecursiva`).
- **Seleção temporária (Ctrl+arrastar)**: marquee habilitado em qualquer
  ferramenta ao segurar Ctrl (`HandleCanvasInteraction`).
- **Snap reforçado da tela base**: tolerância da moldura maior que os demais
  snaps (`SmartGuides::Apply` — área de segurança segura por mais tempo).
- **Duplo clique na ferramenta de criar retângulo**: cria retângulo no tamanho
  da tela base (1280×720) centralizado (`CriarRetanguloTelaBase`).
- **Camadas (CorelDRAW)**: `Project::MoverCamada` (subir/descer/primeiro/
  último, recursivo em grupos) + menu Objeto → Camadas + atalhos Ctrl+↑/↓.
- **Mover com setas**: setas movem a seleção 1px (10px com Shift) via
  `ApplyPositionDelta`; sem modificador, sem conflito com camadas.

### Previsão confiável, janela e toolbar (2026-08-12, tarde)
- **`SmartGuides::ApplySpacing` reescrito**: (1) referências filtradas pela
  **fileira/coluna** da seleção (gaps de outras linhas não viram alvo); (2)
  **grupos/containers contam como bloco único** (filhos aninhados
  deduplicados); (3) ímã com **detecção de cruzamento** — o gap que pula por
  cima do alvo entre frames (arrasto rápido, zoom alto) ainda engata exato;
  (4) erro efetivo do `matchGap` decide a disputa entre vizinhos (0 para
  cruzamento). 4 testes novos.
- **Janela com limite máximo** (`SetWindowMaxSize` + `glfwGetMonitorWorkarea`,
  até a área de trabalho COMPLETA — ex. 1366×768): maximizar preenche a tela
  toda e redimensionar nunca cria janela maior que o monitor (o que antes
  empurrava o rodapé para fora da tela). `mCanvasPrevDX/DY` alimentam a
  previsão com o delta do frame anterior (zerados no início do arrasto e no
  fork).
- **Toolbar com rolagem fina**: child sem `NoScrollbar` + `ScrollbarSize=6`;
  roda continua sendo zoom (mantido `NoScrollWithMouse`).

## Validação
- Builds Debug e Development aprovados (via MSBuild direto; devenv com lock
  da reinicialização pendente do instalador do VS).
- `SeedUI.exe --self-test-m04`: **76 verificações PASS, 0 failures** (4 novas
  de previsão: cruzamento, fora da fileira, grupo como bloco único ×2).
- Captura automática (`--capture`) sem crash/asserts (Debug e Development);
  limite máx. 1366×768 aplicado na inicialização.
- **Pendente de validação manual (usuário)**: engate da previsão em arrasto
  rápido, janela maximizada com rodapé visível e rolagem da toolbar.
