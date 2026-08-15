# Agente.md — Planejamento em Milestones do Mini-Editor 3D (Stove Reborn)

> **Documento de Diretrizes e Cadeia de Desenvolvimento**  
> Este documento define o plano mestre de construção do Mini-Editor 3D inspirado no Stove e no Blender.  
> Cada marco (**Milestone**) é incremental, autocontido, executável e testável antes do avanço para o próximo.  
> O projeto é otimizado para **máximo desempenho em computadores fracos (sem GPU dedicada / vídeo integrado)** e suporta execução **100% offline (Desktop .exe) e online (Navegador Web)**.

---

## 🧭 Regras de Conduta e Boas Práticas

1. **Estrutura em Cadeia Rigorosa:** Nunca pular etapas. O Milestone $N$ depende da estabilidade e validação do Milestone $N-1$.
2. **Desempenho em Primeiro Lugar (Low-Spec First):** 
   * Manter consumo de memória RAM abaixo de 150 MB.
   * Usar renderização WebGL/GPU leve (Phong/Lambert e Shaders otimizados).
   * Sem overhead de CPU em repouso (0% de CPU ociosa).
3. **Controle Total do Código-Fonte:** Todo o código é modular, limpo, tipado e com separação clara entre Lógica 3D, Interface (UI) e I/O de Arquivos.
4. **Atalhos e Usabilidade Padrão Blender:** Teclas `G` (Mover), `R` (Rotacionar), `S` (Escalar), `Tab` (Alternar Modos), `1`/`2`/`3` (Vértice/Aresta/Face), `Ctrl+Z` (Desfazer), `Ctrl+Y` (Refazer), `Shift+A` (Adicionar).
5. **Critério de Conclusão por Marco:** Cada milestone termina com testes visuais e funcionais aprovados.

---

## 🗺️ Tabela Geral de Milestones

| Milestone | Título / Foco | Entregável Principal | Status |
|---|---|---|:---:|
| **M01** | **Fundação & Viewport 3D Ultraleve** | Viewport 3D a 60 FPS, Câmera Orbital, Grid e Otimização Low-Spec | **Concluído** |
| **M02** | **Primitivas & Sistema de Transformação** | Geração de Primitivas (Cubo, Esfera, etc.), Gizmos 3D e Pivot | **Concluído** |
| **M03** | **Modo Modelagem: Seleção de Malha** | Raycasting e Seleção de Vértices, Arestas e Faces | **Concluído** |
| **M04** | **Ferramentas de Modelagem (Mesh Edit)** | Extrude, Inset, Bevel, Subdivide, Knife e Proportional Editing | Planejado |
| **M05** | **Modo Pintura 3D (Texture & Vertex Paint)** | Pincéis 3D, Raio, Força, Paleta de Cores e Estilo Pixel Art | Planejado |
| **M06** | **Modo Zoo (Gerenciador de Cena)** | Multi-objetos, Snap, Alinhamento, Duplicação e Bake de Escala | Planejado |
| **M07** | **Modo View & Shaders de Iluminação** | Iluminação Direcional, Sombras Leves, AO Viewport e Retro Filter | Planejado |
| **M08** | **Auto-UV & Empacotamento de Texturas** | Desdobramento UV Automático e Atlas de Textura Único | Planejado |
| **M09** | **Sistema de Arquivos & Import/Export** | Salvar/Carregar Projeto, Exportador/Importador OBJ, GLTF/GLB e STL | Planejado |
| **M10** | **Interface UI Estilo Blender/ImGui & Atalhos** | Painéis docking, Header, Barra de Ferramentas, Histórico Undo/Redo | Planejado |
| **M11** | **Empacotamento Desktop Offline (.EXE / Tauri)** | Executável Windows autônomo, offline, sem dependências externas | Planejado |

---

## 📋 Detalhamento dos Milestones

### Milestone 01 — Fundação & Viewport 3D Ultraleve
* **Objetivo:** Criar o motor base da aplicação com foco em consumo ultrabaixo de memória e CPU.
* **Tarefas:**
  1. Estruturar o projeto (HTML5 / Vanilla JS / TypeScript ou Three.js otimizado).
  2. Implementar Viewport 3D com resolução adaptativa (`devicePixelRatio <= 1.5`).
  3. Câmera orbital estilo Blender (Botão do meio do mouse = Orbitar, Shift+MMB = Pan, Scroll = Zoom).
  4. Grid de chão responsivo e eixos X/Y/Z coloridos.
  5. Monitor de FPS / Memória integrado para controle de desempenho.
* **Validação:** Rodar em 60 FPS estáveis com menos de 80 MB de memória RAM em repouso.

---

### Milestone 02 — Primitivas 3D & Sistema de Transformação
* **Objetivo:** Permitir a criação e manipulação básica de geometrias no espaço 3D.
* **Tarefas:**
  1. Gerador de geometrias básicas:
     * Cubo, Esfera, Cilindro, Cone, Plano, Torus.
  2. Gizmos 3D de transformação na tela:
     * Translação (setas X, Y, Z).
     * Rotação (anéis X, Y, Z).
     * Escala (cubos X, Y, Z e uniforme).
  3. Transformação via atalhos do teclado (`G`, `R`, `S` + eixo `X`/`Y`/`Z` + valor numérico).
  4. Painel de propriedades de Objeto (Posição, Rotação, Escala, Visibilidade).
* **Validação:** Criar, mover, rotacionar e escalar múltiplos objetos na viewport sem atraso de renderização.

---

### Milestone 03 — Modo Modelagem: Seleção de Malha
* **Objetivo:** Implementar a estrutura de edição interna de geometrias poligonais (Model Mode).
* **Tarefas:**
  1. Alternância de modos: `Tab` alterna entre **Object Mode** e **Edit/Model Mode**.
  2. Submodos de seleção com teclas numéricas:
     * `1` — Vértices (exibição de pontos selecionáveis).
     * `2` — Arestas (linhas conectando vértices).
     * `3` — Faces (polígonos planos).
  3. Algoritmo de Raycasting acelerado para detecção precisa do clique do mouse sobre vértices/arestas/faces.
  4. Seleção múltipla (`Shift + Clique`), Seleção por Retângulo (Box Select `B`) e Inversão (`Ctrl + I`).
* **Validação:** Selecionar individualmente ou em grupo qualquer vértice, aresta ou face de um modelo complexo.

---

### Milestone 04 — Ferramentas de Modelagem (Mesh Edit)
* **Objetivo:** Adicionar as ferramentas clássicas de modelagem poligonal 3D do Stove / Blender.
* **Tarefas:**
  1. **Extrude (`E`):** Extrusão de faces normais ou ao longo de eixos.
  2. **Inset (`I`):** Criação de faces internas proporcionais.
  3. **Bevel (`Ctrl + B`):** Chanfro e arredondamento de arestas e vértices.
  4. **Subdivide:** Subdivisão uniforme de malhas e polígonos.
  5. **Loop Cut (`Ctrl + R`):** Inserção de anéis de corte ao longo da malha.
  6. **Knife Tool (`K`):** Corte interativo ponto a ponto com finalização em `Enter`.
  7. **Proportional Editing (`O`):** Edição proporcional suave com ajuste de raio pelo scroll do mouse.
* **Validação:** Modelar um objeto personalizado (ex.: cadeira, casa ou personagem low-poly) usando apenas as ferramentas.

---

### Milestone 05 — Modo Pintura 3D (Texture & Vertex Paint)
* **Objetivo:** Pintar cores e texturas diretamente sobre o modelo 3D em tempo real.
* **Tarefas:**
  1. Ativação do **Paint Mode** na interface.
  2. Sistema de projeção de pintura (Raycast de pincel sobre a textura / UV da malha).
  3. Configurações de pincel:
     * Raio / Tamanho (`Shift + Scroll`).
     * Força / Opacidade (`Ctrl + Scroll`).
     * Falloff / Dureza da borda (`Ctrl + Shift + Scroll`).
     * Variação de Ruído (*Noise*).
  4. Paleta de cores com Color Picker, seletor HEX/RGB e paletas pré-definidas (retrô/pixel-art).
  5. Modo de Textura Pixelada (amostragem Nearest Neighbor para visual retrô sem custo de GPU).
* **Validação:** Pintar texturas detalhadas em tempo real sobre a malha 3D sem perda de quadros.

---

### Milestone 06 — Modo Zoo (Gerenciador de Cena & Multi-Objetos)
* **Objetivo:** Organizar, alinhar e compor múltiplos modelos no mesmo projeto (igual ao Zoo Mode do Stove).
* **Tarefas:**
  1. Lista de Outliner / Cena com hierarquia de objetos.
  2. Ferramenta de **Snap** (aderência a grade e vértices de outros objetos).
  3. Visualizador de Caixas Delimitadoras (**Bounding Boxes**).
  4. **Bake de Escala:** Aplicar rotações e escalas diretamente na malha (*Apply Scale / Reset Rotation*).
  5. Duplicação rápida (`Shift + D`) e Junção de Objetos (`Ctrl + J`).
* **Validação:** Montar uma cena com múltiplos objetos posicionados com snap e escalas aplicadas.

---

### Milestone 07 — Modo View & Shaders de Iluminação Ultraleves
* **Objetivo:** Proporcionar uma visualização limpa e estilizada com sombras e oclusão sem pesar na GPU integrada.
* **Tarefas:**
  1. **View Mode:** Oculta gizmos, grades e alças, mostrando o modelo renderizado.
  2. Iluminação direcional configurável (Sol / Posição da Luz, Intensidade, Cor).
  3. Sombras dinâmicas de baixo custo (Shadow Maps com filtro suavizado e toggle on/off).
  4. Oclusão de Ambiente simulada no Viewport (escurecimento suave de dobras e cantos).
  5. Seletor de cor de fundo da viewport e alinhamento de câmera ortográfica/perspectiva (`Numpad 1, 3, 7`).
* **Validação:** Visualização fluida do render com sombras ativas em PC com placa integrada a 60 FPS.

---

### Milestone 08 — Auto-UV & Empacotamento de Texturas
* **Objetivo:** Gerar mapas UV e empacotar texturas automaticamente para exportação e jogos.
* **Tarefas:**
  1. Algoritmo de projeção e desdobramento UV automático (Smart UV Unwrap).
  2. Empacotador de ilhas UV (UV Atlas Packer) para otimizar espaço de textura.
  3. Exportação de textura unificada em imagem PNG de alta ou baixa resolução (128x128 até 2048x2048).
* **Validação:** Gerar UVs limpos e sem sobreposição em modelos criados pelo editor.

---

### Milestone 09 — Sistema de Arquivos & Import/Export
* **Objetivo:** Salvar projetos localmente e exportar para formatos padrão da indústria de jogos e 3D.
* **Tarefas:**
  1. Formato de projeto nativo (`.stove3d` ou `.json`) contendo geometria, materiais, texturas e histórico.
  2. Exportador **Wavefront OBJ (`.obj` + `.mtl` + textura)**:
     * Modo arquivo único com atlas de textura.
     * Modo arquivos separados por material.
  3. Exportador **glTF / GLB (`.gltf` / `.glb`)** para Unity, Unreal, Godot e Web.
  4. Exportador **STL (`.stl`)** para impressão 3D.
  5. Importador de arquivos `.obj` e `.gltf` existentes para edição.
* **Validação:** Salvar um projeto, recarregar sem perda de dados e abrir o `.obj`/`.glb` exportado no Blender/Godot perfeitamente.

---

### Milestone 10 — Interface do Usuário (UI Blender/ImGui) & Atalhos
* **Objetivo:** Interface profissional, escura, moderna, minimalista e altamente responsiva.
* **Tarefas:**
  1. Barra Superior (Header): Arquivo, Editar, Adicionar, Modos (Model, Paint, Zoo, View).
  2. Barra Lateral de Ferramentas (Toolbar com ícones e dicas de atalhos).
  3. Painel Lateral Direito (Inspetor de Propriedades, Outliner, Modificadores, Cores).
  4. Sistema de Histórico Completo de Ações: **Desfazer (`Ctrl+Z`)** e **Refazer (`Ctrl+Y`)**.
  5. Barra de Status inferior com contagem de vértices, faces e instrução da ferramenta atual.
* **Validação:** Navegar em toda a interface exclusivamente por atalhos ou cliques sem bugs visuais.

---

### Milestone 11 — Empacotamento Desktop Offline (.EXE via Tauri)
* **Objetivo:** Gerar o executável nativo do Windows leve (~5 MB) para rodar 100% offline.
* **Tarefas:**
  1. Configurar o ambiente **Tauri** (Rust backend + WebView2 nativa do Windows).
  2. Integrar acesso a arquivos nativo (diálogos Abrir/Salvar do Windows).
  3. Compilar o `.exe` final autocontido.
  4. Testar funcionamento com o computador totalmente desconectado da internet.
* **Validação:** Executar o arquivo `.exe` gerado diretamente com duplo clique no Windows sem internet.

---

## 🔄 Fluxo de Execução Contínua

Ao iniciar qualquer Milestone:
1. Ler os objetivos e tarefas daquele Milestone específico.
2. Implementar o código e as interfaces correspondentes.
3. Executar o teste prático de validação.
4. Registrar os resultados e atualizar o status para **Concluído**.
5. Avançar para o Milestone seguinte.
