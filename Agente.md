# Jogo RPG Mundo Aberto
Vamos criar um jogo de RPG em terceira pessoa, em 3D, mundo aberto para computador, especificamente para o sistema operacional Windows, onde o jogador vai conseguir explorar o mundo, matar inimigos de diferentes níveis, cada inimigo vai ter níveis e, consequentemente, vida e defesa e ataques com variáveis diferentes. O jogador vai poder encontrar ou coletar itens que são equipamentos e armas para ele poder melhorar. Ele vai ter uma barra de experiência, uma barra de vida, ele vai poder encontrar potes de vida para a vida dele, ter um inventário, equipar armas, subir de nível e etc. Ele também vai poder encontrar NPCs que ele vai conversar através de uma interface, receber quests de ir buscar um item em determinado lugar, ou então matar uma quantidade X de inimigos, e essa quest vai dar recompensa para ele.

# Tecnologia e Estrutura

## Ferramentas e Plataforma
Plataforma windows( o jogo sera exclusivo para wiondows)
-Linguageem de programação: **C++**
 - Compilador Visual Estudio 2026(MSVC)
   -Build Modes:
   -Debug
   -Development
   -Realise( Para a Build)
-Bibliotecas Externas (Third Party):
  - Grafica: Raylib(https://github.com/raysan5/raylib)
  - Fisica: Jolt(https://github.com/bazaarvoice/jolt)
  - Matematica: Glm(https://github.com/g-truc/glm)
-Interface de DEBUG:Dear ImGui(https://github.com/ocornut/imgui)
-Linguagem para os Tools: Python

# Estrutur de arquivos do projeto
## Estrutura do Projeto
* Game/Source/
* Game/ThirdParty/
* Game/Assets/
* Game/Game.vcxproj
* Tools/AssetCooking.py
* Game.sln
* Build/Debug/
* Build/Development/
* Build/Release/
* Build/Intermediate/Debug/
* Build/Intermediate/Development/
* Build/Intermediate/Release/
# Convençoes de codigo

Nomenclatura:

* Sempre use **PascalCase** para o nome de classes, structs e funções ou métodos.
* Sempre use **camelCase** para nome de variáveis.
* Namespaces devem ser curtos, de uma única palavra (ou abreviação) e **all lower case**.
* Macros devem ser **ALL_UPPER_CASE**.
* Nome de arquivos devem ser sempre em **PascalCase**.
* Use include guards (ifndef define) para os headers.

Estilo de programação:

* Mantenha o código simples e direto ao ponto.
* Não crie funções só porque você acha que deve criar. Se algo pode ser feito inline e só será usado em um único lugar, deixe-o inline.
* Não faça variáveis privadas com getter e setters a não ser que tenha um motivo claro. Prefira estruturas simples, com variáveis públicas.
* Antes de implementar um recurso, verifique se ele já existe. Foque em reusar o que já tem.
* Não precisa comentar tudo, apenas o que não é imediatamente claro apenas lendo o código.
* Você pode usar STL (`std::string`, `std::vector`, `std::unordered_map`, ...).
-Se algo é possivel ser feito usando as bibliotecas padões da liguagem, prefira fazer dessa forma.
- Não use smart pointers.

## Build Modes
* **Debug** e **Development** são para desenvolvimento, eles contêm símbolos para debug e também ferramentas extras de level design, etc.
* **Release** é para o jogo final, que será shipado. Não contém símbolos, foca em velocidade, não tem ferramentas de level design, etc. A diferença do Release e Development é basicamente que o Development tem essas ferramentas extras.
* **Debug** pode ser mais lento, por conter os símbolos, etc.
* **Development** deve ter uma performance melhor.

# Estrutura do codigo
Descricao de todo o codigo dentro de Game/Source:
 -Main.cpp: Ponto de inicio de jogo do programa 
 **App.h/cpp88: App  sera a classe principal do jogo responsavel por cuidar da janela dos imputes gerenciar a cena ativa e executar gameloope do jogo.
 **Scene.h/cpp**: Classe para as cenas do jogo. Cada cena tem informacões da camera que sera usada , uma lista de game objets interface etc. É responsavel por gerenciar esses dados, e executar a logica de cada objeto
 **GameObject.h/cpp** : Classe GameObject, cada um representa um objeto no mundo 3d e pode ter formas jogicas e fisicas diferentes.Feito para ser uma classe base que é especiaçlizada depois em classes filho dentro da pasta Game/Source/GameObjects/. Exemplo: TerrainObject para terrenos , PlayerObject para o player, EnemyObject para inimigos, NpcObject para Npc, ProptObject para props no cenario e etc EdictObject para objetos editaveis(arestas, vertices e faces(primitivas)) e etc.

# Tools
Nós teremos ferramentas para ajudar no desenvolvimento do jogo.Essas ferramentas podem ser escritas em Python. E um exemplo de ferramenta é uma ferramenta de asset cooker, que vai basicamente pegar todos os assets, modelos 3D e texturas que vão estar na pasta game/assets, e ela vai transformar, por exemplo, o FBX em um formato próprio binário que nosso motor consegue ler, transformar as imagens em PNG em um formato que o motor consegue ler e também transformar os sons em um formato que o motor consegue ler também. O Asset cooker é um build event.
Na versão final do jogo, ou seja, junto ao executável de cada build, seja de debug, seja development, seja de release, vai ter sim uma pasta assets, e dentro dessa pasta vão ter arquivos binários que só nosso motor de jogos vai conseguir ler também. É importante deixar claro que o modo de debug e o modo de development vão ter ferramentas de desenvolvimento, só que essas vão ser desenvolvidas dentro do próprio motor em C++. E é importante especificar já aqui de uma vez que essas ferramentas, elas vão alterar a estrutura de arquivos do jogo.

Esses arquivos dessas cenas, principalmente porque, por exemplo, o editor de level do jogo vai editar a cena, ele vai ser salvo num arquivo binário e esse arquivo, ele pode ficar dentro da própria pasta do arquivo de source do jogo, ou seja, game/assets, e aí é copiado na hora de compilar, na hora da build, pra juntar o executável.

# Assets 3d do jogo
** Muito importante, esse assets em formato fbx nao sao para serem importados diretamente em fbx no jogo, ao invez disso use o 'Tools/AssetCooker.py' para extrair esses arquivos e escrever um arquivo final em binario num formato em que o jogo consiga ler, e importar de forma rapida e facil. Use assim para importar os fbx. Crie um. '.venv' do python e um requirement.txt pra instalar  essas coisas. 
## Assets dos personagens:
Nas pastas E:\1Game 2\Game\Assets\Kit e Universal Animation Library 2, vc vais encontrar fbx contendo inumeras animacoes, UAL2_Standard.fbx, UAL2_Standard_RM.fbx, Mannequin_F.fbx
Em \Game\Assets\Kit assets\Universal Base Characters[Standard]\Base Characters\Unity, vc vai encontrar personagem base, ja rigado(Superhero_Female_FullBody.fbx) e (Superhero_Male_FullBody) para serem os personagens com opcao de escolha mas culino ou feminino. Eles setaram so com cueca e calcinha, sem roupas .
Em E:\1Game 2\Game\Assets\Kit assets\Modular Character Outfits - Fantasy[Standard]\Exports\FBX (Unity)\Outfits vc vai encontra 4 modelos diferentes de Outfitys 2 para home (Male_Peasant.fbx,Male_Ranger.fbx), e 2 para mulher(Female_Peasant.fbx,Female_Ranger.fbx), tambe ja rigados. Vamos fazer as partes individuais para fazer os equipamentos.

## Milestones:
 -Malestone 1:
 Estrutura do projeto:
  Criar um "Hello Word" para este projeto, que compila, tem todas as bibliotecas externas clonadas para game/ThirdParty/ e lincadas no game. O projeto compilas abre mas nao tem absolutamente nada. As tools (AssetCooker.py) executam no build eventt, mas tambem nao fazem nada.

## Milestone 2: Jogo Base
  Ter uma base jogavel, ou seja o cubo andando com W, A, S, D, pulo  tecla de espaço,, correr com precionado shift, e manter correndeo clicando 2 veses emm shift,( essas acoes ocorrerao em conjunto com a acao de tecla W ) camera em primeira, segunda e terceira pessoa, e topo( parecido com o que acontece nos jogos como lol ou league of legendes) controlavel com o meuse, e possivel de ser  alternado a visao e camera com a acao de tecla a Alt+P. Um sistema de terreno  e sombras do sol 
## Milestone 3: Menu inicial e de pausa
## Milestone 04:Personagempricipal com animacoes
Agora nos vamos melhorar o sitema de AssetsCooker para ler o fbx e criar um formato proprio , junto ao executavel do jogo dentro da pasta assets que vai junto com o executavel, de que a gente consiga importar os nosso modelos e animacoes e executar ele dentro do jogo. Implemente a secao assets 3d do jogo, desse documento, especificamente os asssets desse personagens que seram os personagem princiapaism tendo no menis de configuracoes a opcao de escolha de home ou mulher, a forma visual dos personagens bases stend caracter e devem ter animacoes, parado, andando, correndo, pulado e etc.
## Milestone 05:Sistema de vida, xp do personagem + ataques, etc 
Eu adicionei varios itens em 'Ultimate RPG Intens Pack'. Itens como armas  personagem machado etc e porcoes , agora eu quero implemente ataques ao clicar com o botao esquerdo do mouse, o personagem consiga atacar com a atimacao de ataque, e tambem  tem o sitema de vida dee experiencia dos personagens e etc, entao ja sera feita a interface do jogo aqui bem bonita, bem polida e sofisticada com tudo o que precisa pra esse jogo, e coloque no chao espalahdos para serem testados itens que se o player chegar perto vai aparecer na interface para apertar 'E' e coletar e se eu apertar 'E' e coletar vai aparecer um inventario, existira um sitema de inventario que o jogador podera abrir e fechar na interface e esse inventario tera os slotes e poderei arrastar os slotes de um lugar para o outro, dropar o item ou equipar o item ou tambem utilizar o item no caso da porção. Eu tambem quero itens no chao com armadura para que eu possa coletar e equiar e isso alterar o visual para os dois FBX de genero sexuais dos  personagem existentes. Lembre-se de usar o sistema de Asset Cooking.  
## Milestone 06:Inimigos (Logica + animcao)
Agora, nós vamos implementar os inimigos. Eles precisam ser um GameObject específico. Não acho que vale a pena criar um GameObject para cada inimigo, porque você vai ver que são dezenas deles. Em vez disso, crie um único GameObject que possamos configurar para selecionar diferentes inimigos.

Dentro da pasta Assets, você encontrará uma pasta chamada Ultimate Monsters. Nela, haverá três categorias de inimigos: Flying, Blob e Big. Todos terão a mesma mecânica. Eles poderão andar e realizar as mesmas ações, mudando apenas as animações, os modelos 3D e as texturas.

Obviamente, também quero que seja possível modificar individualmente o nível, a vida, o ataque e outros atributos de cada inimigo. Cada uma dessas pastas contém vários arquivos FBX, um para cada inimigo, e cada inimigo possui várias animações. As animações disponíveis são: Death, Duck, HitReact, Idle, Jump, Jump_Idle, Jump_Land, NO, Punch, Run, Walk, Wave, Weapon e Yes.

A lógica dos inimigos será a seguinte: quando forem gerados, eles deverão armazenar a posição inicial de spawn. Por padrão, permanecerão no estado de patrulha. Nesse estado, ficarão parados por alguns segundos e, em seguida, caminharão até uma direção aleatória. Ao chegarem ao destino, ficarão parados novamente por aproximadamente um segundo e repetirão o comportamento.

A direção escolhida deverá estar sempre dentro de um raio próximo ao ponto de spawn. Portanto, deverá ser possível definir individualmente o raio de patrulha de cada inimigo.

Se o personagem se aproximar de um inimigo, ele deverá mudar imediatamente para o estado de perseguição. Nesse estado, o inimigo correrá em direção ao player em vez de andar. Quando estiver perto o suficiente, ficará parado e atacará. Depois, aguardará por um intervalo aleatório e atacará novamente, repetindo esse comportamento enquanto o player estiver ao alcance.

Cada inimigo poderá ter uma probabilidade percentual de ser agressivo. Essa característica deverá ser definida aleatoriamente para cada instância gerada. Um inimigo agressivo permanecerá parado por menos tempo entre os ataques e, consequentemente, atacará com maior frequência.

Se o player morrer, os inimigos deverão abandonar a perseguição e retornar ao estado de patrulha.

Além disso, se o player estiver cinco ou mais níveis acima de determinado inimigo, esse inimigo não deverá atacá-lo por iniciativa própria. Ele continuará patrulhando, pois reconhecerá que o personagem é muito mais forte. Entretanto, se o player atacar primeiro, o inimigo deverá reagir e entrar em combate.

Acima da cabeça de cada inimigo, deverão ser exibidos seu nome, seu nível e sua barra de vida. Essas informações aparecerão quando pelo menos uma das seguintes condições for atendida:

O jogador estiver próximo do inimigo, permitindo que ele veja seu nome e nível.
A vida do inimigo estiver abaixo de 100%, indicando que ele sofreu algum dano. Nesse caso, a barra de vida deverá permanecer visível.

Também deverão ser implementados os sistemas de ataque e morte, tanto para o player quanto para os inimigos. Quando um inimigo morrer, ele deverá conceder dinheiro e experiência ao jogador.

Para testar o sistema e avaliar o desempenho, popule a cena de gameplay com centenas de inimigos aleatórios.
## Milestone 07:Cenario
Importe todos os packes de natureza da pasta Stylized Nature MegaKit[Standard] para o jogo. Pra fins de teste, espalne aleatoriamente eles pelo mapa. Arvore precisam de colisao , o restante nao.

## Milestone 08:Level Editor
No modo debug e development da build, nós vamos ter um level editor. A qualquer momento, durante o gameplay, o jogador, que no caso vai ser o desenvolvedor, vai poder apertar um botão para abrir ou ocultar o level editor. A gente vai usar o Dear ImGui para fazer isso. Quando aparecer esse editor, vai funcionar mais ou menos o seguinte. A gente tem algumas ferramentas. A primeira ferramenta é a ferramenta de terraformagem. Ah, e quando ele abre o editor de level, o mapa, o jogo vai ser pausado, ou seja, os inimigos, o personagem, etc, vai parar de ter os controles, e a câmera vai entrar num modo de câmera livre, onde se o personagem clicar e segurar o botão direito, ele consegue controlar a câmera com o mouse look e voar livremente com W, A, S, D, como se fosse em um motor de jogos, que aí ele vai conseguir posicionar ela onde ele quer. Alem disso podera tambem navegar pelos atalho de teclado em conjunto com o clic do mouse esquerdo, acontecendo na seguinte combinacao "clic+Alt"(Rotacao da camera),"Clic+Shift +Altl( Pan) movimento de deslocamento lateral ou vertical da câmera (lados, cima e baixo), mantendo a posição de origem, enquanto a rotação (órbita ou look around) muda o ângulo de visão para onde a câmera aponta., e "Clic+Alt+Shit" (zoom in zoom out), lembarndo que todos esse comandos é ativado em conjunto com o clic esquerdo do mouse  . A primeira ferramenta do editor de level vai ser a ferramenta de terraformagem, que ele vai ter um brush para mudar a escala, a geometria do terreno. Então ele pode clicar para subir o terreno, segurar Ctrl para deletar o terreno, e por aí vai. Ele pode usar a rodinha do mouse para aumentar ou diminuir o tamanho do brush.  A segunda ferramenta vai ser a ferramenta de adicionar as props do cenário, que a gente importou na milestone anterior, alem de todos os outros  props encontrados na pasta, cada um com sua textura e tamanho correto . Quando o personagem clicar, vai ter uma lista com a thumbnail de todos, contendo uma imagem que você vai renderizar para cada uma das props, que aí o personagem pode simplesmente clicar e selecionar a que ele quer, e aí ele vai, quando ele colocar o mouse em cima da superfície, vai aparecer um preview dessas props e ele pode clicar para adicionar. Ele também pode segurar Ctrl e clicar numa prop já adicionada para deletar essa prop. A terceira ferramenta é para adicionar inimigos, que vai funcionar exatamente igual à ferramenta anterior, a diferença é que vai ter uma lista de inimigos, não uma lista de props, também com as thumbnails e imagens. E aí, no caso, na lista de inimigos é importante ter o nível de cada inimigo exibindo ali. E a última ferramenta é exatamente igual às anteriores, só que de itens coletáveis que o jogador pode adicionar. Toda vez que o desenvolvedor sair do modo de edição, isso deve ser salvo nos assets do jogo, e aí isso vai persistir, obviamente, se o desenvolvedor compilar o jogo de novo ou compilar num modo diferente, essas mudanças vão persistir, que ele fez, as adições e coisas que ele deletou.Uma coisa importante é que quando o editor level estiver ativo, é claro que como o jogo vai estar rolando, os inimigos vão ter mudado de posição e tudo mais. Então é por isso que no editor level, você vai colocar no próprio level alguns ícones representando o spawn dos inimigos. E vai ter um botão lá para resetar todos os inimigos, todas as posições. Porque aí, obviamente, você só vai salvar o spawn dos inimigos e qual inimigo é, numa posição tal de cada um, porque isso vai mudar. Isso é bem importante.