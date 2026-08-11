# Identidade Visual do SeedUI — Fusão Blender + Photoshop

> Documento de referência da aparência do próprio editor SeedUI.
> Princípio guia: **clareza e intuição acima de tudo**. Se um elemento visual
> não ajuda a entender, ele não entra.

## 1. Conceito

Fusão de duas escolas de design de ferramentas profissionais:

- **Photoshop** — a base: fundo escuro neutro, visual plano e nítido, bordas
  de 1px, cantos discretos, tipografia compacta.
- **Blender** — a estrutura: painéis com **faixa de título** (cabeçalho) no
  topo, seletor de modo visível com o modo ativo em destaque, acento quente
  para "o que está ativo agora".

Resultado: um programa sério, organizado e óbvio — o usuário entende onde
está e o que está selecionado sem pensar.

## 2. Princípios visuais

1. Fundo escuro neutro (grays do Photoshop).
2. Todo painel tem cabeçalho (strip) — herança do Blender.
3. Plano e nítido: bordas de 1px, sem sombras pesadas, raio discreto.
4. **Cor com significado**: cada cor tem um papel único (tabela abaixo).
5. Tipografia calma e compacta.
6. Ícones em linha fina, sempre com dica (tooltip).
7. Espaçamento em grade de 4px — ritmo consistente.

## 3. Paleta oficial

| Papel | Cor | Origem |
|---|---|---|
| Fundo da janela | `#212121` | Blender |
| Painéis | `#2b2b2b` | Photoshop |
| Cabeçalho de painel (strip) | `#3a3a3a` | Blender |
| Menus / barra superior | `#323232` | Photoshop |
| Bordas (1px) | `#3f3f3f` | Photoshop |
| Texto principal | `#ececec` | — |
| Texto secundário | `#a0a0a0` | — |
| Texto desabilitado | `#6a6a6a` | — |
| Destaque / seleção / ação | `#4f8cff` (azul) | Photoshop |
| Modo ativo / edição | `#f57900` (laranja) | Blender |
| Sucesso | `#2ecc71` | — |
| Aviso | `#f1c40f` | — |
| Erro | `#e74c3c` | — |
| Canvas (área de edição) | `#1a1a1a` + grade `#262626` | — |
| Seleção no canvas | azul `#4f8cff` com preenchimento 20% | Photoshop |
| Checkerboard (transparência) | `#2b2b2b` / `#323232` | Photoshop |

**Regra de uso da cor**: azul = "você está interagindo com isto"; laranja =
"você está DENTRO deste modo" (apenas um por vez); verde/amarelo/vermelho =
estado (ok / atenção / erro). Nada além disso.

## 4. Tipografia

- Família: **Inter** (embutida), fallback Segoe UI (Windows).
- Tamanhos: 11px dados/status · 12px UI padrão · 13px títulos de painel ·
  15px nomes de elemento · 24px tela inicial.
- Pesos: 400 regular · 500 medium (títulos) · 600 semibold (destaques).

## 5. Espaçamento e forma

- Grade de 4px: 4 / 8 / 12 / 16.
- Padding de painel: 8px; gap entre painéis acoplados: 1px (a própria borda).
- Raio: 4px controles · 6px janelas flutuantes/modais · 0px painéis acoplados.
- Borda padrão: 1px `#3f3f3f`.

## 6. Ícones

Ícones vêm de **bibliotecas consolidadas** (nada de desenho amador):

- **Phosphor** (MIT) — peso *thin/light*: traço fino que casa com a
  identidade; ~9.000 ícones, qualidade de estúdio.
- **Lucide** (ISC) — complemento para ícones que faltarem; 24×24, traço 2px,
  visualmente consistente.
- Os SVGs são **embutidos no programa** (`assets/icons/`) e rasterizados na
  inicialização com **lunasvg** (MIT) — nítidos em qualquer tamanho/DPI,
  funcionam 100% offline.

Regras de uso:

- Traço 1.5–2px; 16px na barra de ferramentas, 14px em menus, 20px em destaques.
- **Toda ação tem ícone + tooltip** — o usuário entende a função sem ler.
- Feedback visual por ícone + cor: azul = interagindo, laranja = dentro do modo.
- Nada de ícone "morto": se um ícone não comunica, substitua ou use texto.

## 7. Composição do workspace

```
┌────────────────────────────────────────────────────────────────┐
│ Arquivo  Editar  Exibir  Inserir  Objeto  Modelos  Ajuda       │
│ Tela: hud_jogo ▾   Modo: Combate ▾  (laranja quando ativo)     │
├──────────┬──────────────────────────────────────┬──────────────┤
│ Ferram.  │                                      │  HIERARQUIA  │
│ ⬚ Seleção│         CANVAS                        │──────────────│
│ ✥ Mover  │   réguas · grade · guias · zoom      │  INSPETOR    │
│ T Texto  │                                      │──────────────│
│ ...      │                                      │  DIRETRIZES  │
│          │                                      │──────────────│
│          │                                      │  RECURSOS    │
├──────────┴──────────────────────────────────────┴──────────────┤
│ Zoom 100% · 1280×720 · (x: 240, y: 60) · ● Não salvo           │
└────────────────────────────────────────────────────────────────┘
```

- **Barra de menus**: menus + seletor de tela/modo (estilo Blender).
- **Barra de ações globais** (horizontal, sob os menus): Arquivo (novo, abrir,
  salvar), Histórico (desfazer/refazer), Edição (copiar/colar/apagar), Projeto
  (modelos/exportação) e Sistema (manual/preferências).
- **Barra de ferramentas contextuais** (esquerda): somente ações diretas no
  canvas, agrupadas em Seleção/Transformação, Criação/Aparência,
  Navegação/Visualização e Revisão; uma ferramenta ativa por vez.
- **Canvas** (centro): superfície escura com grade, réguas, guias e área segura.
- **Painéis** (direita): cada um com faixa de título (strip), colapsável.
- **Barra de status**: zoom, resolução, coordenadas, indicador de não salvo.

## 8. Tela inicial (estilo Photoshop/Canva)

- "Novo projeto em branco", galeria de **Modelos** (com miniaturas),
  "Recentes", "Abrir projeto".
- Categorias de modelos: Jogos (HUD, inventário, menu) · Profissionais
  (Blender, Photoshop, Maya, CorelDRAW, Illustrator, Krita, ZBrush) ·
  Meus Modelos.

## 9. Workspaces do editor

- O usuário salva disposições dos painéis ("Desenho", "Diretrizes",
  "Revisão") e alterna pelo menu Exibir → Workspace.
- O workspace ativo não muda dados do projeto — apenas a organização do editor.

## 10. Licenciamento comercial (venda permitida)

Todo ativo do SeedUI pode ser usado em produto **vendável**:

| Ativo | Licença | Observação |
|---|---|---|
| Ícones Phosphor | MIT | Venda livre; sem atribuição visível. |
| Ícones Lucide | ISC | Idem (ISC ≈ MIT). |
| Renderizador lunasvg | MIT | Venda livre. |
| Fonte Inter | OFL 1.1 | Pode ser embutida e vendida dentro do software (não revender a fonte isolada). |
| raylib | zlib | Venda livre. |

Regra prática: manter os avisos de licença num arquivo `THIRD_PARTY.md`
dentro do programa — é o único requisito das licenças MIT/ISC/OFL para uso
comercial. **Evitar** Font Awesome Free (CC BY 4.0, exige atribuição) e
conjuntos pagos sem licença comercial.

## 11. O que NÃO fazer

- Nada de cores berrantes fora dos papéis definidos.
- Laranja apenas para o modo ativo (nunca para seleção).
- Sem gradientes chamativos, sem sombras grandes, sem ícones pesados.
- Sem cantos exagerados; sem texto pequeno demais (mínimo 11px).
