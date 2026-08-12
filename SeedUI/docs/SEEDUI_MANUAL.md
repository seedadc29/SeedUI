# Manual do SeedUI — Editor Visual de Interfaces

> Este manual é a documentação interna do programa SeedUI.
> Ele é exibido dentro do próprio editor (atalho `F1` do SeedUI) e é incluído
> automaticamente em todo projeto exportado (arquivo `README.md`), para que
> qualquer IA ou desenvolvedor que receba um projeto entenda o programa e o
> formato sem precisar de explicações adicionais.
>
> Documentos de apoio: `FORMATO_PROJETO.md` (formato), `IDENTIDADE_VISUAL.md`
> (identidade visual) e `MILESTONES.md` (planejamento por etapas).

## 1. O que é o SeedUI

O SeedUI é uma ferramenta **independente** para criar e editar visualmente
interfaces de um jogo ou aplicação: menus, HUD, inventários, janelas, painéis,
telas de configuração e interfaces de desenvolvimento (como o editor aberto
pela tecla F1 e as ferramentas da tecla F2 da engine).

- O SeedUI **não substitui a engine** e não substitui o F1/F2 da engine.
- O SeedUI cria **projetos de interface** (arquivos `projeto.ui.json`) que
  descrevem o visual e a intenção de comportamento de cada elemento.
- Uma **IA (ou um desenvolvedor)** lê esse projeto e conecta as funções reais
  aos elementos visuais — sem que o usuário precise programar nada.
- O SeedUI funciona **totalmente offline** (sem internet). A versão web é
  apenas um caso opcional para o futuro.

## 2. Para quem foi feito

- **Para o usuário**: desenhar a interface sem escrever código, de forma
  visual (WYSIWYG — o que você vê é o que será exportado).
- **Para a IA**: receber um projeto + este manual e entender, de cara, o que
  cada elemento deve fazer e como implementar.

## 3. Como o programa funciona

O SeedUI é dividido em painéis:

| Painel | Função |
|---|---|
| **Área de projeto (canvas)** | Onde a interface é montada. Selecionar, mover, redimensionar, rotacionar, duplicar, alinhar, usar guias e grade, zoom. |
| **Hierarquia** | Árvore de elementos: renomear, reorganizar, criar pais/filhos, bloquear, ocultar, pesquisar por nome/ID/tipo. |
| **Biblioteca de componentes** | Elementos prontos para arrastar para o canvas: painel, janela, botão, texto, campo numérico, slider, lista, barra de progresso etc. |
| **Inspetor de propriedades** | Edição numérica e visual de posição, tamanho, layout, cores, bordas, fontes, estados. |
| **Diretrizes** | Caixa de texto comum onde você escreve o que cada elemento deve fazer (ver seção 5). |
| **Anotar (modo debug)** | Marque áreas em QUALQUER parte da janela (menus, ícones, painéis, canvas), escreva o que alterar e exporte um .txt + captura para a IA. Ferramenta "Anotar" (tecla A), rótulo com seta e caixa flutuante de texto (ver seção 5.1). |
| **Manual** | Este documento, aberto dentro do programa (atalho `F1` do SeedUI). |

### 3.1 Aparência do editor (identidade Blender + Photoshop)

O SeedUI funde as linguagens visuais do **Photoshop** (base escura neutra,
plana e nítida) e do **Blender** (painéis com faixa de título e destaque do
modo ativo), com **clareza e intuição acima de tudo**: cada cor tem um papel,
cada painel tem cabeçalho, nada compete pela atenção. A especificação
completa está em `IDENTIDADE_VISUAL.md`.

Paleta principal (padrão escuro):

| Elemento | Cor |
|---|---|
| Fundo da janela | `#1e1e1e` |
| Painéis e barras | `#2b2b2b` |
| Cabeçalho de painel (strip) | `#3a3a3a` |
| Menus e cabeçalhos | `#323232` |
| Bordas (1px) | `#3f3f3f` |
| Texto principal | `#ececec` |
| Texto secundário | `#a0a0a0` |
| Destaque / seleção / ação | `#4f8cff` (azul) |
| Modo ativo / edição | `#f57900` (laranja Blender) |
| Canvas (área de edição) | `#1a1a1a` com grade sutil |

Espaçamento consistente em unidades de 4px/8px; ícones em linha fina de
bibliotecas consolidadas (**Phosphor** + **Lucide**, com ícone + tooltip em
toda ação); cantos discretos (raio 4–6px); sem sombras pesadas — o visual é
plano e nítido.

Workspace do editor (disposição padrão):

- **Barra de menus** no topo: Arquivo, Editar, Exibir, Inserir, Objeto,
  Modelos, Ajuda — com o **seletor de tela/modo** no estilo Blender (o modo
  ativo aparece em laranja).
- **Barra de ferramentas** à esquerda: selecionar, mover, redimensionar,
  texto, guia, zoom, conta-gotas de cor...
- **Canvas** ao centro, com réguas, grade, guias e área segura.
- **Painéis acoplados à direita**: Hierarquia, Inspetor, Biblioteca,
  Diretrizes, Histórico, Recursos.
- **Barra de status** embaixo: zoom, coordenadas, resolução e indicador de
  alterações não salvas.

O editor salva **workspaces** (disposições dos painéis) — você pode ter um
workspace "Desenho", outro "Diretrizes" e outro "Revisão", como no
Photoshop. Tema claro também estará disponível como opção.

### 3.2 Modelos de interface (estilo Canva)

Para quem está começando, ter **sugestões prontas** ajuda muito — por isso o
SeedUI terá uma galeria de **modelos de interface**, no estilo do Canva:

- **Modelos padrão** (embutidos no programa), inspirados em interfaces
  consagradas: Blender (workspace 3D), Photoshop (edição de imagem), Maya
  (modelagem), CorelDRAW/Illustrator (vetorial), Krita (pintura), ZBrush
  (escultura) — além de modelos de jogo (HUD, inventário, menu, tela de
  pausa) e modelos vazios por resolução.
- **Meus Modelos**: salve qualquer interface que você criar como modelo
  pessoal, para reutilizar depois.
- **Aplicar**: escolher um modelo gera um projeto novo pronto para editar;
  IDs que conflitarem ganham sufixo para nunca duplicar.
- Cada modelo tem uma **miniatura** (imagem de pré-visualização).

A galeria aparece na tela inicial do SeedUI e também em "Arquivo → Novo a
partir de modelo".

### 3.3 Manipulação de precisão no canvas (M04)

O canvas ganhou ferramentas de precisão estilo CorelDRAW/Illustrator:

- **Rotação**: selecione um elemento e arraste a **alça circular acima do
  topo** dele — o elemento gira em torno do próprio centro. Segure **Shift**
  durante o arraste para travar em incrementos de 15°. O contorno da seleção
  acompanha a rotação e o clique continua acertando o elemento rotacionado.
- **Guias inteligentes** (magenta, como no CorelDRAW): ao **mover** um
  elemento, o programa encaixa automaticamente bordas e centros dele com a
  **borda e o centro da tela** e com **bordas e centros de outros elementos
  visíveis**, mostrando a linha-guia na hora do encaixe. Funcionam com o
  **Snap ativo** (botão de grade na barra de ações ou menu Exibir).
- **Força do snap (ímã) ajustável**: o **botão de ímã** na barra de ações
  (ou **Exibir → Força do snap**) abre um controle de **0.00× a 3.00×** —
  quanto maior, mais longe os objetos "grudam" (tolerâncias ampliadas);
  quanto menor, mais precisão manual é exigida; **0.00× desliga o snap
  completamente** (todas as tolerâncias zeram). O multiplicador vale para
  **todas** as referências: guias inteligentes, moldura, guias das réguas,
  arestas no resize e espaçamento (o encaixe continua **exato** no ponto —
  a força só muda a distância de captura, nunca o valor final).
- **Snap forte da moldura**: a tela-base (o retângulo com o rótulo "tela
  base") é o delimitador principal — suas bordas e o centro têm um encaixe
  **oito vezes mais forte** que os demais snaps (magnetização ampliada),
  então o elemento "gruda" nela mesmo vindo de mais longe. Além disso, a
  **grade de 8px nunca desfaz esse encaixe**: quando a moldura (ou uma
  guia inteligente) engata no eixo, a quantização da grade é pulada — o
  objeto pousa EXATAMENTE na borda/centro (nunca em 1284 em vez de 1280).
- **Guias de espaçamento**: ao mover perto de uma sequência de elementos
  com espaçamento regular (ex.: três painéis com 20px entre si), o programa
  prevê o padrão e puxa o elemento para **replicar o mesmo espaço**,
  mostrando duas linhas tracejadas delimitando o intervalo.
- **Preview de espaçamento com Shift** (estilo CorelDRAW): durante o
  **mover** (vale também para o clone no arrasto), segure **Shift** — surgem
  **pequenos traços nos cantos das laterais** de cada objeto da fileira
  alinhada (traços horizontais nas laterais esquerda/direita; verticais nos
  cantos de topo/base em colunas), delimitando cada peça com clareza, e o
  **valor da distância** de cada espaço entre elas. Visual limpo, sem linhas
  longas atravessando a tela.
- **Organização do painel direito**: o painel usa **abas** (Hierarquia /
  Inspetor / Biblioteca / Diretrizes / Recursos / Histórico) — uma seção por
  vez, sem pilhas de cabeçalhos confusas.
- **Debug no topo**: em modo de revisão, o bloco de **debug/exportar** fica
  no **menu bar** (indicador de anotações + botão Exportar), nada sufoca o
  rodapé; a paleta de cores fica **centralizada verticalmente** na faixa.
- **Previsão de espaçamento assertiva e EXATA**: QUALQUER espaço existente
  entre dois objetos é um alvo de previsão — ex.: dois objetos com 32px de
  espaço entre si fazem a próxima peça **prever 32** e **travar exatamente
  em 32** (o alvo é o valor exato da referência, sem arredondamentos, e a
  previsão **vence a grade de 8px** — nunca pousa em 31 ou 33). Rótulos com
  **1 decimal** (ex.: "32.0") para você ver o valor real. Ao engatar, os
  traços aparecem e o **rótulo do espaço previsto fica destacado**: ali é o
  ponto onde você possivelmente quer estar — a trava indica, não prende.
  A previsão funciona nas **4 direções** (esquerda, direita, acima e abaixo)
  com a mesma precisão — o encaixe vertical também pousa exato em 32.
- **Previsão confiável em qualquer velocidade e contexto**: a previsão agora
  só usa **referências da própria fileira/coluna** em que a peça está sendo
  encaixada (espaçamentos de outras linhas não "poluem" o alvo) e
  **grupos/containers contam como um bloco único** (filhos aninhados não
  geram alvos falsos). Além disso, o ímã **detecta o cruzamento**: se o
  cursor **pular por cima** do alvo entre dois frames (arrasto rápido, zoom
  alto), a peça ainda **engata exato no alvo** — a previsão não "passa
  batido" nem pousa fora do valor.
- **Janela nunca esconde o rodapé**: a janela tem **limite máximo de tamanho**
  (a área de trabalho completa do monitor — ex. 1366×768) — **maximizar
  preenche a tela toda** e redimensionar nunca cria uma janela maior que o
  monitor (o que antes empurrava a barra de status e a paleta de cores para
  **fora da tela**); o conteúdo inferior fica sempre visível.
- **Barra de ferramentas com rolagem fina**: se as ferramentas não couberem
  na altura da janela, uma **barra de rolagem fina** aparece na borda da
  coluna (estilo Blender) — **nenhum ícone fica cortado ou invisível**; a
  roda do mouse continua sendo zoom do canvas.
- **Clique não move (limiar de arrasto)**: ao **clicar** num objeto ele fica
  **fixo no lugar** (seleciona, mas não se mexe). O objeto só passa a se
  mover depois que o mouse **arrasta além de ~4px** — sem deslocamentos
  acidentais no clique.
- **Alinhar ao Conjunto (distâncias uniformes)**: no inspetor, o alvo de
  alinhamento agora tem a opção **"Conjunto"** — a referência passa a ser
  o **bounding box dos vizinhos** (elementos visíveis fora da seleção), não
  a tela nem a própria seleção. Com uma forma selecionada, **Centralizar
  horizontal + vertical** a coloca **exatamente no centro do conjunto**, com
  distâncias idênticas em cima, embaixo, esquerda e direita (ex.: a forma
  vermelha no centro dos quadros cinza).
- **Redimensionar grupo**: com vários elementos selecionados, arraste uma
  alça da caixa conjunta — todos escalam proporcionalmente, preservando o
  layout relativo entre eles.
- **Multi-seleção em conjunto**: a caixa de seleção cobre **todo o corpo do
  conjunto** (não cada elemento isolado). Com vários selecionados,
  **mover, redimensionar, girar e colorir** valem para todos ao mesmo tempo
  — a rotação gira o conjunto em torno do centro da caixa conjunta e a
  paleta/seletor pintam todos de uma vez. Clicar em espaço vazio dentro da
  caixa e arrastar move o conjunto inteiro.

Estas ferramentas são **auxílio de edição**: não entram no arquivo do
projeto (`.ui.json`). O que fica salvo é só a posição, o tamanho e a rotação
finais de cada elemento.

### 3.4 Cores, clone e zoom (M04)

- **Paleta de cores**: barra fixa **acima da barra de status** (rodapé,
  estilo CorelDRAW). Com um elemento selecionado, **clique esquerdo** num
  swatch pinta o preenchimento (`cor_fundo`) e **clique direito** pinta o
  contorno (`cor_borda`).
- **Seletor de cor**: botão "Seletor de cor..." na paleta abre uma janela com
  **3 modelos estilo Photoshop** — **Triângulo** (HSV), **Quadrado** (SV com
  barra de matiz) e **Barras** (matiz + canais R/G/B). Aplica no
  preenchimento do elemento selecionado.
- **Clone durante o arrasto (fork)**: segure o **botão esquerdo** sobre o
  objeto e arraste (o movimento é só com o esquerdo, sem apertar o direito
  para mover). No meio do arrasto, **aperte o botão direito** uma vez: o
  **original volta ao ponto de partida** e a **cópia** (com IDs novos)
  assume o arrasto — continue movendo e **solte** para posicionar o clone
  no destino. (O modo antigo também existe: botão direito sobre o elemento
  e arrastar cria a cópia e move a cópia.)
- **Zoom**: a roda amplia de 10% até 1600%; com **"Zoom no cursor"** (menu
  Exibir) a ampliação segue o ponto sob o mouse em **todos** os controles
  (roda, botões +/−, atalho Z) — até o limite máximo.
- **Seleção por caixa**: padrão **cobertura total** — um elemento só é
  selecionado quando a caixa cobre o corpo inteiro (estilo Photoshop); no
  menu há a opção **"Selecionar ao encostar"** para voltar ao
  comportamento antigo (qualquer contato seleciona).
- **Snap forte**: bordas e centros encaixam com força (tolerância de 10px) e
  a **grade do canvas acompanha o snap** — o elemento não "escapa" do
  encaixe.
- **Grade matemática e discreta**: a grade é **pontos pequenos escuros**
  (baixo contraste, menos vibrante — o foco fica na tela-base e nas formas)
  desenhados em **espaço de projeto** a cada **8 unidades**, maiores a cada
  **40** — o **mesmo passo exato do snap** (`Geo::kGridStep` é a fonte
  única): cada ponto visível da grade é um ponto **exato** de encaixe em
  qualquer zoom (a grade nunca desalinha do snap, mesmo em escalas
  fracionárias).
- **Ocultar a grade mantém o snap**: menu **Exibir → Grade** ou o botão de
  grade na barra de ações oculta a grade **sem desligar o snap** — os
  objetos continuam encaixando nos pontos exatos da grade, mesmo sem vê-la.
- **Espaço de trabalho livre e área de segurança**: os objetos **não ficam
  presos** à moldura — você pode mover/redimensionar/criar em qualquer ponto
  do canvas e a **grade pontilhada percorre o canvas inteiro** (sempre
  alinhada ao snap). Ao sair da tela-base com elementos selecionados, um
  **contorno vermelho fino tracejado** contorna a moldura avisando que você
  saiu da área principal; basta arrastar de volta. Navegue com o **botão do
  meio** (arrastar = pan) e a **roda** (zoom que segue o mouse até o fim).

### 3.6 Barra de propriedades, guias das réguas e status (estilo CorelDRAW)

- **Barra de propriedades contextual**: abaixo da barra de ferramentas
  principal existe uma barra compacta estilo CorelDRAW com **X, Y, Largura,
  Altura e Rotação** da seleção com **entrada numérica direta** (digite e
  confirme — o objeto atualiza na hora). Seleção única edita o elemento;
  multi-seleção mostra a **caixa conjunta** (somente leitura). Inclui
  seletor de **Unidade** (px, mm, cm, in, pt) — os campos e as réguas
  passam a exibir na unidade escolhida — campo de **Precisão** (incremento)
  e **Zoom** editável com botão **Ajustar** (volta a página ao canvas).
- **Guias arrastáveis das réguas** (estilo CorelDRAW): clique na **régua
  horizontal** e arraste para criar uma **guia horizontal azul**; clique na
  **régua vertical** para uma guia vertical. As guias são independentes dos
  objetos, **não entram no arquivo do projeto** (são auxílio de edição),
  podem ser **arrastadas** para mudar de posição e **removidas** soltando
  na régua de origem ou fora do canvas.
- **Snap bidirecional guia ⇄ formas** (funcional, não só estético): ao
  **arrastar uma guia**, ela **gruda nas laterais e centros das formas**
  visíveis e na **moldura da tela-base** (bordas 0/fim e centro — snap
  forte do delimitador principal vence as formas em empate) — a régua vira
  ferramenta de alinhamento precisa. E, ao **mover uma forma**, ela
  **encaixa nas guias** com snap forte (referência intencional do usuário
  vence os demais snaps). Quando um encaixe acontece, a guia envolvida
  fica **destacada em laranja** (linha mais grossa) — feedback visual
  imediato do ponto exato de encaixe, nos dois sentidos.
- **Arestas encaixam nas guias ao REDIMENSIONAR** (estilo CorelDRAW): as
  guias da régua também são referência durante o **resize** — ao arrastar
  uma aresta (ou canto), ela **gruda na guia** mais próxima (vertical ou
  horizontal), inclusive no **resize espelhado** (Shift: a aresta oposta
  reflete a partir do pivô) e em **grupos** (a aresta da caixa conjunta
  encaixa). A guia engatada acende em laranja, igual ao mover.
- **A moldura magnetiza as arestas no REDIMENSIONAR**: antes, o snap da
  tela-base só existia ao **mover** o objeto inteiro — ao arrastar uma
  aresta (resize) ela passava pela lateral sem travar. Agora a **borda e o
  centro da tela-base** encaixam a aresta arrastada com a mesma força 8×
  do mover (linha magenta ao engatar), inclusive no espelhado (Shift) e
  em grupos — redimensionar até a borda "trava" claramente.
- **Bloqueio da régua** (proteção contra alterações acidentais): o menu
  **Exibir → Bloquear réguas** (ou o **cadeado** na barra de ferramentas
  principal) bloqueia/desbloqueia a régua. Bloqueada, a régua **continua
  visível e funcional** como referência espacial — as marcações seguem
  acompanhando zoom/pan/unidade e **as guias existentes continuam fazendo
  snap** normalmente — mas **não é possível interagir** com ela: não se
  cria nem se arrasta guia, e o cursor de hover some. Um **cadeado laranja
  discreto** aparece no canto onde as réguas se cruzam. Regra central:
  **régua bloqueada ≠ régua desativada** — o bloqueio impede a *edição* da
  régua, nunca o uso das referências que ela já estabeleceu.
- **Réguas sincronizadas**: as réguas horizontal e vertical agora mostram
  **coordenadas de projeto** (acompanham zoom e pan) em vez de pixels da
  janela, com ticks em passos "bonitos" (1/2/5 ×10^n) e números também na
  régua vertical; a unidade selecionada é aplicada aos números.
- **Status bar com detalhes**: quando há seleção, o rodapé mostra
  **Detalhes do objeto** (tipo · dimensões · posição) no lugar da mensagem
  e, à direita, a **cor do preenchimento em CMYK** (C/M/Y/K) com a
  espessura do contorno — referência profissional para impressão.
- **Redimensionamento com modificadores** (estilo CorelDRAW):
  - **Shift isolado** em qualquer alça = **espelhado a partir do ponto de
    origem** (pivô, no centro por padrão): puxar uma aresta faz a **oposta
    espelhar o movimento** para o lado contrário — a forma estica
    proporcionalmente para os dois lados (lateral e superior).
  - **Shift+Alt** nas **alças de canto** = **proporcional (uniforme)**:
    largura e altura escalam juntas preservando a proporção original
    (largura/altura = constante — 200×100 → 400×200; um quadrado/círculo
    100×100 → 200×200, nunca vira elipse). O **canto oposto à alça fica
    fixo** como âncora — o objeto não desloca.
  - **Sem modificador** = redimensionamento livre/deformação (intacto).
- **Ponto de origem (pivô) arrastável**: a forma selecionada mostra uma
  **"mira" laranja no centro** — clicar e arrastar reposiciona o ponto de
  origem, com **snap nos pontos-chave da própria forma** (centro, 4 cantos
  e 4 meios de aresta) + grade de 8px. O pivô é o centro do **resize
  espelhado** (Shift) e da **rotação**: mover a mira muda onde a forma
  "cresce" e em torno do que gira. Ele **acompanha o objeto** ao mover e
  redimensionar (mantém a posição relativa), e **Objeto → Redefinir ponto
  de origem** volta ao centro.

### 3.5 Seleção em conjunto, camadas e atalhos (M04)

- **Rotação em conjunto (corpo rígido)**: com vários elementos selecionados
  ou um **grupo**, girar a alça de rotação faz **todo o conjunto orbitar o
  centro da caixa de seleção** — os elementos giram juntos, como um objeto
  único (estilo CorelDRAW). A rotação **individual** continua disponível
  com a ferramenta/inspeção do elemento isolado (função secundária).
- **Grupos aceitam alterações em conjunto**: um **grupo** selecionado pode
  ser **redimensionado** pelas alças, **rotacionado** e **recolorido** —
  a alteração é aplicada a **todos os filhos** de uma vez (a caixa do grupo
  é recalculada após a transformação).
- **Seleção temporária com Ctrl**: em **qualquer** ferramenta (criar
  retângulo, elipse, texto etc.), segure **Ctrl** e arraste no canvas para
  abrir a **caixa de seleção** sem trocar de ferramenta — solte Ctrl para
  voltar à ferramenta atual.
- **Snap reforçado da moldura**: a **área de segurança** (tela base
  1280×720) tem snap **mais forte que todos os demais** — segura por mais
  tempo (tolerância maior), indicando que é o delimitador principal onde o
  trabalho deve acontecer. O usuário pode sair, mas o encaixe de volta é
  firme.
- **Criar retângulo com duplo clique**: dê **dois cliques rápidos** na
  ferramenta de criar retângulo e ela **cria automaticamente** um retângulo
  no tamanho da tela base definida (ex.: 1280×720) na posição central do
  canvas — independente da proporção configurada.
- **Camadas (CorelDRAW)**: no menu **Objeto → Camadas** (ou atalhos
  **Ctrl+↑ / Ctrl+↓**) o elemento selecionado **sobe uma camada** ou
  **desce uma camada** na ordem de renderização; o submenu traz também
  **"Trazer para frente"** e **"Enviar para trás"** (primeira/última
  posição).
- **Mover com as setas**: as **setas do teclado** movem o(s) elemento(s)
  selecionado(s) 1 unidade por pressionada (10 com Shift). Como exigido,
  **sem modificador** para não conflitar com Ctrl+setas das camadas.

## 4. O formato do projeto

- Um projeto é uma **pasta** com o arquivo principal `projeto.ui.json` e as
  pastas de recursos (`svg/`, `imagens/`, `fontes/`, ...).
- O arquivo é **JSON legível**, versionado (`versao`) e não contém código
  executável — apenas estrutura, aparência e diretrizes em texto.
- A especificação completa do formato está em `FORMATO_PROJETO.md`.

Regras de ouro do formato:

1. **IDs estáveis**: cada elemento tem um ID único e permanente
   (`botao_restaurar_vida`, `campo_vida_personagem`). A IA e a engine usam o
   ID — nunca a posição na tela ou o texto exibido.
2. **Três camadas separadas**: estrutura, aparência e comportamento. Mudar o
   visual não apaga o comportamento; mudar o comportamento não apaga o visual.
3. **Sem caminhos absolutos**: recursos são relativos à pasta do projeto.

Um projeto pode conter **várias telas** (as diferentes janelas/interfaces do
programa: menu, HUD, inventário, editor F1, ferramentas F2...) e cada tela
pode ter **vários modos** — os workspaces no estilo do Blender (Viewport,
Sculpt, Edit...) ou do Photoshop. Ex.: tela `hud_jogo` com modos
`exploracao`, `combate` e `inventario`. Cada modo tem sua própria árvore de
elementos, e o jogo (futuramente) troca de tela/modo em tempo de execução.

## 5. O campo de diretrizes — a ponte com a IA

Cada elemento tem uma **caixa de diretrizes**: um campo de texto comum onde
você escreve, em linguagem natural, o que aquele elemento deve fazer na
interface final. Não é código — é um comentário/instrução que direciona a IA.

Exemplo — você seleciona o botão "Restaurar vida" e escreve na diretriz:

> "Ao clicar, deve restaurar a vida do personagem ao valor máximo e exibir a
> mensagem 'Vida restaurada!' por 2 segundos."

Quando a IA receber o projeto, ela lê essa diretriz e sabe exatamente o que
implementar para aquele botão, sem precisar perguntar.

**Como escrever uma boa diretriz** — responda: quem? o quê? quando? resultado?

| Elemento | Diretriz boa |
|---|---|
| Botão "Adicionar objeto" | "Ao clicar, adiciona um objeto novo ao centro da cena e atualiza a lista de objetos." |
| Slider "Velocidade" | "Controla a velocidade do personagem (0 a 200). Ao arrastar, atualiza o valor exibido ao lado." |
| Campo "Vida" | "Edita a vida atual. Mínimo 0, máximo 999. Ao alterar, atualiza a barra de vida do HUD." |
| Painel "Inventário" | "Mostra os itens do personagem. Deve abrir com a tecla I e fechar com Esc." |

### 5.1 Modo debug — anotações para direcionar a IA

Além das diretrizes por elemento, o SeedUI tem um **modo debug de anotações**
para você apontar exatamente onde quer mudanças — **em qualquer parte da janela
do SeedUI**: menus, ícones da barra lateral, painéis ou canvas.

1. Ative a ferramenta **Anotar** na barra lateral (ou tecla **A**).
2. Arraste sobre **qualquer parte da janela** para **selecionar a área** que
   quer alterar — o retângulo cobre o que você marcar, inclusive os ícones.
3. Aparece uma **seta + rótulo** numerado e uma **caixa flutuante** onde você
   digita a alteração desejada.
4. Cada anotação tem um número (1, 2, 3...) e uma cor diferente, para você se
   referir a elas ("a anotação 2 e a 4").
5. Use o painel **DIRETRIZES** (à direita) para um **comentário geral** do
   programa.
6. **Exporte as diretrizes**: clique em **"Exportar diretrizes (.txt + print)"**
   (painel DIRETRIZES ou menu Editar). O SeedUI cria na pasta `diretrizes/`
   (ao lado do executável) um **arquivo .txt** com todas as anotações e o
   comentário geral, junto com uma **captura de tela** mostrando as áreas
   marcadas — e abre a pasta automaticamente. Depois é só dizer aqui no chat:
   *"veja as novas alterações"*.

Alternativa: **Ctrl+Shift+C** (ou menu Editar → "Copiar anotações para a IA")
cola tudo na área de transferência.

Dica: com a ferramenta Anotar ativa, a barra de status mostra
"● Modo Debug: Anotações". Pressione **V** para voltar à ferramenta
Selecionar e usar os menus normalmente.

As anotações são salvas junto do projeto a partir do Milestone 03.

## 6. Fluxo de trabalho completo

1. **Criar** um projeto novo no SeedUI — em branco ou a partir de um
   **modelo** da galeria (estilo Canva) — ou abrir um existente.
2. **Desenhar**: arraste componentes da biblioteca, posicione, redimensione,
   edite cores, bordas, fontes.
3. **Nomear**: dê IDs claros e escreva as **diretrizes** de cada elemento.
4. **Salvar e exportar**: o SeedUI gera a pasta do projeto com
   `projeto.ui.json` + recursos + o manual (`README.md`).
5. **Pedir à IA**: envie a pasta do projeto com um pedido (ver exemplos abaixo).
6. **A IA implementa**: ela lê o manual, o projeto e as diretrizes, e conecta
   as funções — sem alterar o visual.

## 7. Como pedir alterações à IA (prompts prontos)

Sempre **cite o ID** do elemento e descreva o comportamento esperado. Exemplos:

- "No projeto `editor_personagem.ui.json`, conecte o botão `botao_restaurar_vida`
  à função que restaura a vida do personagem. Preserve todo o restante."
- "Adicione um aviso visual (borda vermelha e texto 'Vida baixa!') ao campo
  `campo_vida_personagem` quando o valor ficar abaixo de 20."
- "Troque o ícone do botão `botao_adicionar_objeto` pelo `svg/icone_mais.svg`
  e mantenha as diretrizes intactas."
- "Remova o painel `painel_debug` e avise se houver funções associadas a ele."

Dica: comece o pedido citando o ID e termine com "preserve todo o restante"
quando a alteração for pontual.

## 8. Regras para quem implementa (a IA)

Se você é uma IA (ou desenvolvedor) trabalhando em um projeto SeedUI:

1. Leia primeiro este manual e o `FORMATO_PROJETO.md`, depois o
   `projeto.ui.json`.
2. Trabalhe sempre pelo `id` dos elementos — nunca por posição na tela ou pelo
   texto exibido.
3. **Nunca altere IDs**: eles são contratos estáveis entre o visual, a engine
   e o comportamento.
4. Alterações de visual não podem apagar `diretriz` (comportamento); alterações
   de comportamento não podem apagar `estilos` (aparência).
5. Não escreva código dentro do `projeto.ui.json` — o arquivo não executa nada.
   Implemente as funções no código da aplicação, usando os IDs para conectar.
6. Se um elemento com `diretriz` for removido, reporte a função que será perdida.
7. Ao final, informe o que foi alterado e o que ainda depende de decisão humana.

## 9. Convenções e boas práticas

- IDs: `<tipo>_<ação>_<objeto>` — `botao_adicionar_objeto`,
  `campo_vida_personagem`, `slider_velocidade`, `janela_editor_f1`.
  Somente letras minúsculas, números e `_`.
- Um elemento, um propósito. Prefira dividir do que sobrecarregar.
- Escreva diretrizes para todo elemento interativo (o editor avisa se faltar).
- Teste a interface nas resoluções suportadas (1280×720, 1600×900, 1920×1080...).
- Efeitos visuais pesados (sombras, transparências, gradientes) devem ser usados
  com moderação em máquinas fracas — o editor avisa sobre o custo.

## 10. Estado atual e próximas etapas

- **Agora**: o SeedUI é um editor **standalone** (não conectado à engine).
  Ele cria, edita e exporta projetos de interface. No canvas já funcionam
  seleção, mover, redimensionar, **rotação**, **redimensionar em grupo**,
  **desagrupar**, **guias inteligentes**, **cores** (paleta + seletor),
  **clone com o botão direito** e **zoom até 16×** (ver §3.3 e §3.4).
- **Futuro (opcional)**: integração com a engine — carregar os projetos
  exportados dentro do jogo e conectar funções reais aos IDs. O F1 e o F2
  continuam sendo da engine; o SeedUI apenas projeta as interfaces deles.

O plano completo por etapas está em `MILESTONES.md` (Milestone 01 a 15);
cada etapa é executada, testada e aprovada antes da próxima — nada de
avançar no escuro ou quebrar o que já funciona.

## 11. Solução de problemas

| Problema | Solução |
|---|---|
| O projeto não abre | Verifique se `projeto.ui.json` existe e se `versao` é compatível. O editor converte versões antigas. |
| Recurso ausente (imagem/SVG/fonte) | Importe o arquivo novamente; o editor copia para a pasta do projeto. |
| Elemento não aparece no canvas | Verifique `visivel`, posição dentro da tela e `ordem` (elemento atrás de outro). |
| Diretriz sumiu | Diretrizes são salvas junto com o projeto — confira se o projeto foi salvo após a alteração. |
| Quero projetar várias interfaces no mesmo projeto (menu, HUD, inventário...) | Use **telas** — um projeto pode ter várias telas, cada uma com seus modos (workspaces). |
| Não sei por onde começar | Use um **modelo** da galeria (Arquivo → Novo a partir de modelo) e edite. |
| Preciso de ajuda | Abra o menu "Ajuda → Manual" ou copie o registro de erros ("Diagnóstico") e envie. |
