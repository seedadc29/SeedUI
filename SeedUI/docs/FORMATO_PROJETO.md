# Formato de Projeto do SeedUI — `projeto.ui.json` (v1)

> Documento de referência do formato de projeto usado pelo SeedUI.
> Faz parte da documentação interna do programa e acompanha todo projeto
> exportado (uma cópia resumida do manual vai no `README.md` do pacote).

## 1. Princípios do formato

1. **Três camadas separadas**: estrutura (hierarquia), aparência (estilos) e
   comportamento (diretrizes em texto). Uma alteração em uma camada nunca apaga a outra.
2. **IDs estáveis**: todo elemento tem um `id` único e permanente. A engine e a IA
   se referem ao elemento por esse ID — nunca pela posição na tela nem pelo texto exibido.
3. **Sem caminhos absolutos**: recursos (imagens, SVGs, fontes) são referenciados por
   caminho relativo dentro da pasta do projeto.
4. **Sem código executável**: o arquivo não contém scripts. O comportamento é descrito
   em linguagem natural no campo `diretriz`, que será interpretado por uma IA ou
   desenvolvedor na etapa de implementação.
5. **Legível e versionado**: o campo `versao` controla a evolução do formato com
   migração explícita entre versões.

## 2. Estrutura de pastas de um projeto

```
InterfaceProjeto/
├── projeto.ui.json      ← o projeto (único arquivo obrigatório)
├── README.md            ← cópia do manual do SeedUI (contexto para a IA)
├── temas/               ← temas salvos (opcional)
├── componentes/         ← componentes reutilizáveis (opcional)
├── fontes/              ← fontes importadas (.ttf, .otf)
├── imagens/             ← PNG, JPG, WebP
├── svg/                 ← SVGs importados
└── preview/             ← capturas de pré-visualização (opcional)
```

## 3. Estrutura do `projeto.ui.json`

### 3.1 Nível raiz

| Campo         | Tipo   | Obrigatório | Descrição |
|---------------|--------|-------------|-----------|
| `formato`     | texto  | sim         | Sempre `"seedui.projeto"`. |
| `versao`      | número | sim         | Versão do formato (atualmente `1`). |
| `metadados`   | objeto | sim         | `nome`, `descricao`, `criado_em`, `modificado_em`, `gerador`. |
| `tela_base`   | objeto | sim         | `largura` e `altura` da tela de referência (ex.: 1280×720). |
| `resolucoes`  | lista  | não         | Resoluções extras para teste de responsividade. |
| `variaveis`   | objeto | não         | Variáveis globais (cores, espaçamentos, raios, fontes). |
| `temas`       | objeto | não         | Temas e paletas salvos. |
| `recursos`    | lista  | não         | Recursos importados (SVG, imagem, fonte). |
| `diretrizes`  | texto  | não         | Instruções gerais do projeto, em linguagem natural. |
| `telas`       | lista  | sim         | Lista de telas do projeto; cada tela contém `modos`, e cada modo contém `raiz` (ver seção 3.2). |

### 3.2 Telas e Modos (workspaces)

Um projeto pode conter **várias telas** — as diferentes janelas/interfaces do
programa ou jogo que está sendo editado (menu principal, HUD, inventário,
editor F1, ferramentas F2, configurações...).

Cada tela pode ter **vários modos** — os modos de trabalho, no estilo dos
workspaces do Blender (Viewport, Sculpt, Edit...) ou dos espaços de trabalho
do Photoshop. Cada modo é um layout completo e independente daquela tela,
trocável em tempo de execução.

- `telas` — lista de telas do projeto.
- Tela → `modos` — lista de modos da tela.
- Modo → `raiz` — lista de elementos (a árvore da interface naquele modo).

Exemplo conceitual (estilo Blender):

```json
"telas": [
  {
    "id": "tela_editor_cena",
    "nome": "Editor de Cena",
    "modos": [
      { "id": "modo_viewport", "nome": "Viewport", "raiz": [ ... ] },
      { "id": "modo_sculpt",   "nome": "Sculpt",   "raiz": [ ... ] },
      { "id": "modo_edit",     "nome": "Edit",     "raiz": [ ... ] }
    ]
  }
]
```

Aplicado ao jogo (integração futura com a engine): a tela ativa e o modo ativo
são controlados pelo jogo em execução — ex.: tela `hud_jogo` com modos
`exploracao`, `combate` e `inventario`; tela `editor_f1` com modos `terreno`,
`objetos` e `inimigos`. O SeedUI permite projetar e pré-visualizar cada tela e
cada modo separadamente.

O próprio editor também usa esse conceito: ele salva **workspaces**
(disposições dos painéis do SeedUI), como o Photoshop.

### 3.3 Elemento (nó da árvore)

| Campo            | Tipo    | Obrigatório | Descrição |
|------------------|---------|-------------|-----------|
| `id`             | texto   | sim         | Identificador único e estável (ex.: `botao_restaurar_vida`). |
| `tipo`           | texto   | sim         | Tipo do elemento (ver catálogo na seção 5). |
| `nome`           | texto   | não         | Nome de exibição (pode mudar; o `id` não). |
| `visivel`        | bool    | não         | Default `true`. |
| `bloqueado`      | bool    | não         | Default `false` — impede edição visual. |
| `transformacao`  | objeto  | não         | `x`, `y`, `largura`, `altura`, `largura_min`, `largura_max`, `altura_min`, `altura_max`, `escala`, `rotacao`, `origem_x`, `origem_y`, `opacidade`, `ordem`. |
| `layout`         | objeto  | não         | `tipo` (`livre`, `horizontal`, `vertical`, `grade`), `alinhamento`, `espacamento`, `margem`, `preenchimento`, `ancoras`, `expandir`, `quebrar_linha`, `rolagem`. |
| `estilos`        | objeto  | não         | Aparência: `cor_fundo`, `cor_texto`, `borda`, `raio_cantos`, `sombra`, `gradiente`, `fonte`, `tamanho_fonte`, etc. |
| `estados`        | objeto  | não         | Variações por estado: `normal`, `hover`, `pressionado`, `selecionado`, `ativo`, `desativado`, `foco`, `erro`, `aviso`, `edicao`. |
| `propriedades`   | objeto  | não         | Propriedades específicas do tipo (ver catálogo). |
| `diretriz`       | texto   | não         | **Instrução em linguagem natural** sobre o comportamento/objetivo do elemento. Ver seção 6. |
| `filhos`         | lista   | não         | Elementos filhos (para contêineres). |

Exemplo mínimo de elemento:

```json
{
  "id": "botao_restaurar_vida",
  "tipo": "botao",
  "nome": "Botão Restaurar Vida",
  "transformacao": { "x": 40, "y": 60, "largura": 180, "altura": 36 },
  "estilos": { "cor_fundo": "#2ecc71", "cor_texto": "#ffffff", "raio_cantos": 6 },
  "propriedades": { "texto": "Restaurar vida", "icone": "svg/icone_coracao.svg" },
  "diretriz": "Ao clicar, deve restaurar a vida do personagem ao valor máximo e exibir uma mensagem de confirmação."
}
```

## 4. Metadados e identificação

- `id` só pode conter letras minúsculas, números e `_`. Sem acentos, espaços ou hífens.
- Convenção de nome: `<tipo>_<ação>_<objeto>` — `botao_adicionar_objeto`,
  `campo_vida_personagem`, `slider_velocidade`, `janela_editor_f1`.
- O `id` **não muda** ao mover, estilizar ou reorganizar o elemento.

## 5. Catálogo de tipos (v1)

| Tipo               | Descrição | Propriedades principais |
|--------------------|-----------|-------------------------|
| `painel`           | Retângulo de fundo para agrupar elementos | `cor_fundo`, `borda`, `raio_cantos` |
| `janela`           | Janela com título e área de conteúdo | `titulo`, `arrastavel`, `redimensionavel` |
| `caixa`            | Contêiner simples | — |
| `contêiner`        | Contêiner com layout | `layout`, `rolagem` |
| `grupo`            | Agrupamento de elementos | — |
| `texto`            | Texto simples | `texto`, `fonte`, `tamanho`, `cor`, `alinhamento` |
| `titulo`           | Título | `texto`, `nivel` |
| `botao`            | Botão de ação | `texto`, `icone`, `destacado` |
| `botao_icone`      | Botão apenas com ícone | `icone`, `dica` |
| `alternancia`      | Botão liga/desliga | `ativo`, `texto` |
| `caixa_selecao`    | Checkbox | `marcado`, `texto` |
| `opcao`            | Botão de opção (rádio) | `grupo`, `selecionado` |
| `campo_texto`      | Entrada de texto | `texto_inicial`, `placeholder`, `limite_caracteres` |
| `campo_senha`      | Entrada de senha | `texto_inicial`, `placeholder` |
| `campo_pesquisa`   | Entrada de pesquisa | `placeholder` |
| `campo_numerico`   | Entrada numérica | `minimo`, `maximo`, `casas_decimais`, `incremento` |
| `area_texto`       | Texto multilinha | `texto`, `altura_linhas` |
| `lista`            | Lista de itens | `itens`, `selecao_multipla` |
| `lista_suspensa`   | Dropdown | `itens`, `indice_selecionado` |
| `arvore`           | Árvore hierárquica | `itens` |
| `tabela`           | Tabela | `colunas`, `linhas` |
| `grade`            | Grade de células | `colunas`, `espacamento` |
| `slider`           | Controle deslizante | `minimo`, `maximo`, `valor`, `orientacao` |
| `slider_valor`     | Slider com rótulo numérico | `minimo`, `maximo`, `valor`, `casas_decimais` |
| `incremento`       | Controle +/− | `minimo`, `maximo`, `valor`, `passo` |
| `barra_progresso`  | Barra de progresso | `minimo`, `maximo`, `valor`, `formato` |
| `indicador_circular` | Progresso circular | `minimo`, `maximo`, `valor` |
| `seletor_cor`      | Seletor de cor | `cor` |
| `separador`        | Linha separadora | `orientacao` |
| `divisor`          | Divisor redimensionável | `orientacao` |
| `barra_rolagem`    | Barra de rolagem | `orientacao`, `valor`, `tamanho_indicador` |
| `tooltip`          | Dica de passagem do mouse | `texto` |
| `menu_contexto`    | Menu ao clicar com botão direito | `itens` |
| `janela_modal`     | Janela que bloqueia o restante | `titulo`, `fechavel` |
| `dialogo`          | Caixa de diálogo (pergunta/aviso) | `titulo`, `texto`, `botoes` |
| `notificacao`      | Aviso temporário | `texto`, `tipo` (`info`, `sucesso`, `erro`, `aviso`) |
| `inspetor`         | Painel de propriedades | `itens` |
| `viewport`         | Área de pré-visualização (ex.: cena 3D) | `rotulo` |
| `personalizado`    | Componente definido pelo usuário | `componente` |

> Novos tipos podem ser adicionados em versões futuras sem quebrar projetos existentes.

## 6. O campo `diretriz` — a ponte com a IA

O campo `diretriz` é uma **área de comentário em texto comum** (linguagem natural),
anexada a cada elemento. Ela não é código e nunca é executada. Ela serve para:

- Explicar o que aquele elemento deve fazer na interface final;
- Indicar qual função/objetivo a IA deve implementar;
- Registrar decisões que devem ser preservadas em alterações futuras.

**Como escrever uma boa diretriz** (quem → o quê → quando → resultado):

> "Ao clicar, este botão deve restaurar a vida do personagem ao valor máximo,
> atualizar a barra de vida do HUD e exibir a mensagem 'Vida restaurada!' por 2 segundos."

**Exemplos por tipo de elemento:**

- Botão: `"Ao clicar, abre a janela de inventário e atualiza a lista de itens do personagem."`
- Campo numérico: `"Edita a velocidade do personagem. Valor mínimo 0, máximo 200. Ao alterar, atualiza o slider de velocidade."`
- Barra de progresso: `"Exibe o XP atual do personagem em relação ao XP necessário para o próximo nível."`
- Painel: `"Painel de status do personagem. Deve aparecer no canto superior esquerdo do HUD."`

**Regras:**

- Escreva o comportamento desejado, não "como programar" (a IA decide a implementação).
- Uma diretriz por elemento; para instruções gerais, use `diretrizes` no nível raiz do projeto.
- Nunca escreva código na diretriz (o arquivo não executa nada).

## 7. Estados visuais

Cada elemento interativo pode ter estilos diferentes por estado:

`normal`, `hover` (mouse sobre), `pressionado`, `selecionado`, `ativo`,
`desativado`, `foco`, `erro`, `aviso`, `edicao`.

Exemplo:

```json
"estados": {
  "normal":       { "cor_fundo": "#2ecc71" },
  "hover":        { "cor_fundo": "#27ae60" },
  "pressionado":  { "cor_fundo": "#1e8449" },
  "desativado":   { "cor_fundo": "#95a5a6" }
}
```

## 8. Recursos externos

- Tipos suportados: `imagem` (PNG, JPG, WebP), `svg`, `fonte` (TTF, OTF).
- Ao importar, o arquivo é **copiado** para a pasta correta do projeto
  (`imagens/`, `svg/`, `fontes/`) e referenciado por caminho relativo.
- O projeto nunca depende do caminho original do computador de origem.
- Em `recursos`:

```json
"recursos": [
  { "id": "icone_coracao",  "tipo": "svg",    "caminho": "svg/icone_coracao.svg" },
  { "id": "fundo_hud",      "tipo": "imagem", "caminho": "imagens/fundo_hud.png" },
  { "id": "fonte_principal","tipo": "fonte",  "caminho": "fontes/Inter-Regular.ttf" }
]
```

## 9. Versionamento e migração

- `versao` identifica a versão do formato. Projetos antigos continuam abrindo:
  o editor converte e grava na versão mais nova quando o usuário salva.
- Mudanças que quebram compatibilidade exigem migração explícita, com relatório
  do que foi convertido.

## 10. Validações

O editor valida o projeto e emite avisos (não bloqueia decisões intencionais):

- IDs duplicados ou vazios (inclusive entre telas e modos);
- IDs com caracteres inválidos;
- Recursos referenciados que não existem;
- Elementos interativos sem `diretriz`;
- Texto cortado / elementos fora da tela;
- Contraste insuficiente entre texto e fundo;
- Elementos sobrepostos sem intenção;
- Vínculos quebrados (futuro, na integração com a engine).

## 11. Exemplo completo (resumo)

```json
{
  "formato": "seedui.projeto",
  "versao": 1,
  "metadados": {
    "nome": "Editor de Atributos do Personagem",
    "descricao": "Interface aberta pela tecla F2 na engine",
    "criado_em": "2026-08-11",
    "gerador": "SeedUI 0.1"
  },
  "tela_base": { "largura": 1280, "altura": 720 },
  "variaveis": {
    "cor_fundo_principal": "#1a1a2e",
    "cor_destaque": "#e94560",
    "altura_botao_padrao": 36
  },
  "diretrizes": "Esta é a interface do editor de atributos do personagem, aberta pela tecla F2 na engine. O painel esquerdo mostra os atributos; o painel direito mostra ações rápidas.",
  "telas": [
    {
      "id": "tela_editor_f2",
      "nome": "Editor de Atributos",
      "modos": [
        {
          "id": "modo_padrao",
          "nome": "Padrão",
          "raiz": [
            {
              "id": "janela_editor_f2",
              "tipo": "janela",
              "transformacao": { "x": 120, "y": 80, "largura": 440, "altura": 520 },
              "propriedades": { "titulo": "Editor de Atributos" },
              "filhos": [
                {
                  "id": "campo_vida_personagem",
                  "tipo": "campo_numerico",
                  "transformacao": { "x": 20, "y": 40, "largura": 200, "altura": 32 },
                  "propriedades": { "valor": 100, "minimo": 0, "maximo": 999, "rotulo": "Vida" },
                  "diretriz": "Edita a vida atual do personagem. Ao alterar, atualiza o HUD do jogo."
                },
                {
                  "id": "botao_restaurar_vida",
                  "tipo": "botao",
                  "transformacao": { "x": 240, "y": 40, "largura": 180, "altura": 36 },
                  "propriedades": { "texto": "Restaurar vida" },
                  "diretriz": "Ao clicar, restaura a vida do personagem ao valor máximo e mostra a mensagem 'Vida restaurada!'."
                }
              ]
            }
          ]
        }
      ]
    }
  ]
}
```

## 12. Modelos de interface (templates)

Um **modelo** é um projeto SeedUI comum (`projeto.ui.json` + recursos),
armazenado na pasta de modelos do SeedUI — modelos embutidos no programa ou
salvos pelo usuário ("Meus Modelos").

- **Modelos padrão**: interfaces de referência inspiradas em programas
  consagrados (Blender, Photoshop, Maya, CorelDRAW, Illustrator, Krita,
  ZBrush) e modelos de jogo (HUD, inventário, menu, tela de pausa), além de
  modelos vazios por resolução.
- **Salvar como modelo**: exporta a tela/modo atual (ou o projeto inteiro)
  para a pasta de modelos, com miniatura em `preview/`.
- **Aplicar modelo**: copia estrutura e estilos para o novo projeto; IDs que
  conflitarem recebem sufixo (`_2`, `_3`...) para nunca duplicar.
- O formato de um modelo é **idêntico** ao de um projeto — não existe formato
  novo; um modelo é apenas um projeto guardado para reuso.
