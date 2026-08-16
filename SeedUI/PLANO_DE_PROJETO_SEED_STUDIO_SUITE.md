# 🪐 SEED ENGINE STUDIO SUITE
## Documento de Especificação de Arquitetura, Design e Plano de Projeto (Master GDD / PDD)

---

## 1. Sumário Executivo e Visão Geral

### 1.1 Objetivo do Projeto
O **Seed Engine Studio Suite** é uma plataforma criativa completa "tudo-em-um" (*All-In-One Creative Studio & Game Engine*), concebida para democratizar o desenvolvimento de jogos, aplicações interativas e softwares 3D. 

O grande diferencial do projeto é **eliminar o abismo entre o design gráfico, a lógica de programação e a modelagem 3D**, permitindo que pessoas sem formação técnica em programação consigam criar sistemas complexos e funcionais através de **metáforas visuais naturais e intuitivas**.

### 1.2 Público-Alvo
- **Designers de UI/UX e Artistas Gráficos** que desejam dar vida aos seus layouts sem precisar pedir para programadores codificarem o backend.
- **Modeladores e Artistas 3D** que querem criar mundos interativos diretamente no ambiente de modelagem.
- **Desenvolvedores Independentes (Indies) e Educadores** que buscam agilidade extrema e clareza cognitiva no desenvolvimento.
- **Público Não-Técnico (Iniciantes/Entusiastas)** que se sentem intimidados pela sintaxe de linguagens de programação e pelo emaranhado visual de fios de nós (*spaghetti nodes*).

---

## 2. A Arquitetura das 4 Salas (Pipeline de Produção Integrado)

O fluxo de trabalho do Seed Studio é estruturado em **4 Salas de Criação**, onde cada etapa alimenta a próxima de forma contínua:

```
┌─────────────────────────┐     ┌─────────────────────────┐
│  🎨 SALA 1: DESIGN 2D   │ ──> │ 🪐 SALA 2: LÓGICA ORB.  │
│  (Estética / SeedUI)    │     │ (Backend Gravitacional) │
└─────────────────────────┘     └─────────────────────────┘
             │                               │
             ▼                               ▼
┌─────────────────────────┐     ┌─────────────────────────┐
│  🧊 SALA 3: 3D STUDIO   │ ──> │  ⚡ SALA 4: RUNTIME     │
│  (BlenderPro3D Malhas)  │     │  (Engine em Tempo Real) │
└─────────────────────────┘     └─────────────────────────┘
```

---

### 🎨 SALA 1: Design Gráfico & Layout Visual (O "CorelDRAW / SeedUI")

**Propósito**: Construir toda a identidade visual, interfaces (UI), painéis HUD, vetores, botões e tipografia do projeto.

#### Ferramentas e Recursos:
1. **Motor Vetorial 2D**:
   - Caneta Bézier (*Pen Tool*), formas primitivas (retângulos, círculos, polígonos, estrelas).
   - Operações Booleanas vetoriais (União, Interseção, Subtração, Exclusão).
   - Manipulação precisa de vértices e pontos de controle com alças de curvatura.
2. **Sistema de Layout e Componentes**:
   - Contêineres responsivos (*Flex/Grid visual*), alinhamento magnético (*snapping*) e guias inteligentes.
   - Criação de componentes reutilizáveis: Botões, Caixas de Diálogo, Menus Suspensos, Barras de Progresso, Sliders e Ícones.
3. **Estilização Avançada**:
   - Preenchimentos sólidos, gradientes lineares e radiais, sombras dinâmicas, desfoque de fundo (*glassmorphism*), bordas e cantos arredondados independentes.
4. **Catálogo de Ícones Integrado**:
   - Biblioteca de ícones vetoriais nativos com suporte universal a Drag & Drop.
5. **Saída da Sala 1**:
   - Todos os elementos criados recebem uma ID única e são exportados automaticamente como entidades de interface prontas para receberem comportamento na Sala 2.

---

### 🪐 SALA 2: Mecânica & Lógica Orbital (Backend Visual / Gravitational Logic Graph)

**Propósito**: Programar todo o comportamento, fluxo de dados e regras do sistema sem escrever uma única linha de código tradicional e sem a confusão de nós retangulares com fios emaranhados (*spaghetti nodes*).

#### A Metáfora do Sistema Solar:
Em vez de caixas e linhas retas, a lógica se organiza em **Sistemas Solares Gravitacionais e Mapas Mentais Orbitais**:

| Elemento Cósmico | Correspondente na Computação | Descrição Prática |
| :--- | :--- | :--- |
| **☀️ Sol (Núcleo / Estrela)** | **Objeto / Entidade Central** | O elemento que detém a lógica (ex: o botão `IniciarJogo`, o personagem `Player_Cube`, ou a `Câmera`). |
| **⭕ Órbitas (Trajetórias)** | **Canais / Categorias de Execução** | Linhas finas concêntricas e pontilhadas que delimitam o raio de influência e ordem das operações. |
| **🪐 Planetas** | **Funções Principais** | Blocos de ação fundamentais (ex: `Movimento`, `Física & Gravidade`, `Ao Clicar`, `Áudio`, `Transição de Tela`). |
| **🌕 Luas / Satélites** | **Subfunções e Parâmetros** | Valores, variáveis e modificadores orbitando o planeta (ex: `Velocidade = 10`, `Direção = +X`, `Massa = 2kg`). |
| **⚡ Feixes Gravitacionais** | **Sinais e Fluxo de Dados** | Raios de luz ou ondas de energia disparados quando um evento é acionado, transmitindo dados entre planetas e luas. |
| **🌌 Galáxias** | **Múltiplos Sistemas Solares** | A visão macro onde vários sistemas solares (objetos) interagem entre si em um ecossistema completo. |

#### Tipos de Módulos Orbitais:
1. **Planetas de Gatilho / Eventos (Inputs)**:
   - `Ao Clicar (On Click)`, `Ao Passar o Mouse (On Hover)`, `Ao Pressionar Tecla`, `Ao Iniciar Cena`, `Ao Colidir`.
2. **Planetas de Ação e Transformação**:
   - `Mover Objeto`, `Girar`, `Escalar`, `Alterar Cor/Opacidade`, `Trocar de Sala/Cena`, `Reproduzir Som`.
3. **Planetas de Controle e Condição**:
   - `Se/Então (If/Else)`, `Repetidor (Loop Orbital)`, `Temporizador (Timer)`, `Comparador (> , < , ==)`.
4. **Luas de Variáveis e Dados**:
   - Números, Textos, Vetores 3D `(X, Y, Z)`, Booleanos `(Verdadeiro/Falso)`, Estados de Jogo (Pontuação, Vidas, Nível).

---

### 🧊 SALA 3: 3D Creation & Sculpting Studio (O "BlenderPro3D")

**Propósito**: Modelar malhas tridimensionais, criar personagens, construir cenários e configurar materiais PBR em tempo real.

#### Ferramentas e Recursos:
1. **Modelagem de Primitivas e Malhas Quads**:
   - Geração de Cubo, Esfera UV, Cilindro, Cone, Plano, Torus, Icosfera.
2. **Modo Edição Completo (<kbd>Tab</kbd>)**:
   - Seleção de Vértices (<kbd>1</kbd>), Arestas (<kbd>2</kbd>) e Faces (<kbd>3</kbd>).
   - **Extrusão (<kbd>E</kbd>)**: Criação de novas paredes e volumes na malha.
   - **Inset (<kbd>I</kbd>)**: Margem concêntrica interna em faces.
   - **Chanfro / Bevel (<kbd>Ctrl+B</kbd>)**: Arestas e vértices biselados.
   - **Loop Cut & Slide (<kbd>Ctrl+R</kbd>)**: Anéis de corte interativos com suporte a scroll do mouse.
   - **Subdividir Malha (<kbd>W</kbd>)**: Refinamento de topologia quad.
   - **Criar Face (<kbd>F</kbd>)**: Criação de polígonos a partir de vértices selecionados.
   - **Mesclar Vértices (<kbd>M</kbd>)**: Fusão no ponto central ou no cursor.
   - **Suavizar Malha (Smooth)**: Relaxamento laplaciano de vértices.
   - **Encolher / Engordar (<kbd>Alt+S</kbd>)**: Escala ao longo das normais.
   - **Seleção em Caixa (Box Select / <kbd>W</kbd>)**: Seleção por retângulo de arraste.
3. **Shading e Materiais**:
   - Wireframe, Sólido com Matcap, Prévia de Material PBR e Renderizador em Tempo Real.
4. **Câmeras e Iluminação**:
   - Câmeras ortográficas e perspectivas com Passepartout 16:9, enquadramento live (*Lock Camera to View*).
5. **Exportação / Importação**:
   - Suporte a `.obj`, `.stl`, `.json` e malhas poligonais.

---

### ⚡ SALA 4: A Engine em Execução (Runtime Integrado)

**Propósito**: O palco final onde o design 2D da Sala 1, a lógica gravitacional da Sala 2 e o mundo 3D da Sala 3 rodam unificados em tempo real.

#### Como Funciona a Execução:
1. **Camada de Renderização Composta**:
   - O viewport 3D renderiza o cenário e personagens no fundo.
   - A camada 2D do SeedUI desenha a interface e os botões sobrepostos com precisão de pixels.
2. **Ciclo de Atualização (Tick Loop a 60 FPS)**:
   - A cada quadro (*frame*), o motor de física e o grafo orbital processam as variáveis, os sinais e as entradas do usuário.
3. **Modo Teste / Play Instantâneo**:
   - Um botão **Play (<kbd>F5</kbd> ou ⏵)** permite testar a aplicação instantaneamente sem tempos de compilação demorados.

---

## 3. Matriz de Riscos Técnicos e Estratégias de Mitigação

| Risco / Desafio Técnico | Impacto | Causa Raiz | Estratégia de Solução & Mitigação |
| :--- | :--- | :--- | :--- |
| **Complexidade Visual em Grafos Grandes** | Médio | Muitas órbitas e luas na tela podem poluir a visão do usuário. | **Níveis de Zoom Semântico (LOD de UI)**: Em zoom distante, luas viram pontos luminosos; em zoom próximo, detalhes se expandem. Opção de "colapsar órbitas". |
| **Loops Infinitos na Lógica Orbital** | Alto | O usuário pode conectar um planeta a outro em círculo fechado. | **Limite Seguro de Ciclos por Tick (Watchdog)**: Se um sinal circular executar mais de 1000 vezes no mesmo frame, a engine interrompe e destaca o planeta em vermelho com alerta amigável. |
| **Sobrecarga de Renderização (Queda de FPS)** | Médio | Cenas com muitos polígonos e efeitos simultâneos. | **Frustum Culling, Instanced Meshes e Occlusion**: Apenas o que está no campo de visão é renderizado. Shaders otimizados em GLSL. |
| **Sincronização de Estado entre as 4 Salas** | Alto | Modificar um botão na Sala 1 e perder a conexão lógica na Sala 2. | **Arquitetura de Dados Orientada a Entidades (ECS com UUIDs)**: Cada elemento tem uma ID imutável. Alterar a estética na Sala 1 não rompe a lógica na Sala 2. |

---

## 4. Roteiro de Implementação Passo a Passo (Roadmap)

### 📌 FASE 1: Protótipo Isolado da Lógica Orbital (Sandbox Teórica)
- [ ] Criar um ambiente isolado (sem interferir no BlenderPro3D).
- [ ] Desenvolver o motor 2D em Canvas/SVG dos Sistemas Solares interativos.
- [ ] Implementar a física orbital visual (planetas girando suavemente ao redor do Sol).
- [ ] Permitir adicionar Sol, Planetas e Luas com Drag & Drop.
- [ ] Implementar o disparo de feixes de energia entre planetas ao clicar.
- [ ] Validar a intuição e a usabilidade com testes práticos.

### 📌 FASE 2: Sala 1 - Vetor & Design Gráfico (SeedUI Vector Studio)
- [ ] Desenvolver a prancheta vetorial com ferramentas de forma, caneta e curvas.
- [ ] Criar o editor de propriedades de estilo (cores, gradientes, bordas, sombras).
- [ ] Implementar o gerador de componentes reutilizáveis de interface.

### 📌 FASE 3: Integração das Salas 1, 2 e 3
- [ ] Conectar os componentes da Sala 1 como nós-Sol na Sala 2.
- [ ] Integrar os objetos 3D do BlenderPro3D (Sala 3) como entidades controláveis na Sala 2.
- [ ] Criar a barra de navegação superior universal entre as salas.

### 📌 FASE 4: Sala 4 - Engine Runtime & Modo Play
- [ ] Integrar o loop de execução em tempo real a 60 FPS.
- [ ] Adicionar botão de Play/Pause com depuração visual de feixes em execução.
- [ ] Implementar exportação final da aplicação executável web/desktop.

---

## 5. Critérios de Teste, Validação e Aprovação

1. **Teste do "Usuário de 10 Minutos"**:
   - Um usuário sem conhecimento de programação deve conseguir criar um botão na Sala 1, conectar uma rotação orbital na Sala 2, e fazer o Cubo 3D da Sala 3 girar ao clicar em menos de 10 minutos, sem ler manuais.
2. **Performance e Fluidez**:
   - 60 quadros por segundo constantes no canvas orbital com até 50 sistemas solares simultâneos.
3. **Integridade de Dados**:
   - Salvar e carregar projetos `.seed` sem perda de parâmetros, posições ou conexões.
4. **Aprovação do Designer**:
   - Interface visualmente premium, limpa, sem clichês visuais saturados, com tipografia legível e contraste confortável.
