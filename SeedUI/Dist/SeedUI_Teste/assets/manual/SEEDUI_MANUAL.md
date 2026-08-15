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
  Ele cria, edita e exporta projetos de interface.
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
