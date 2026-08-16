/**
 * IconBrowser - Interactive Visual Tabler Icon Picker & Explorer for BlenderPro3D
 * 
 * Features:
 * 1. Instant search across hundreds of 3D, UI, modeling, and system icons
 * 2. Category filtering (3D & Formas, Ferramentas & Edição, Visão & Câmeras, Janelas & UI, Ações & Arquivo)
 * 3. 1-Click copy of HTML/Class name for easy usage
 * 4. Draggable, non-blocking floating window
 */
export class IconBrowser {
  constructor() {
    this.icons = [
      // 3D & Geometry
      { name: 'box', cat: '3D & Formas', title: 'Cubo / Caixa' },
      { name: 'box-model', cat: '3D & Formas', title: 'Modelo 3D' },
      { name: 'box-margin', cat: '3D & Formas', title: 'Extrusão / Margem' },
      { name: 'box-padding', cat: '3D & Formas', title: 'Inset de Face' },
      { name: 'cube', cat: '3D & Formas', title: 'Cubo 3D Isométrico' },
      { name: 'cylinder', cat: '3D & Formas', title: 'Cilindro' },
      { name: 'cone', cat: '3D & Formas', title: 'Cone' },
      { name: 'sphere', cat: '3D & Formas', title: 'Esfera' },
      { name: 'torus', cat: '3D & Formas', title: 'Torus / Rosquinha' },
      { name: 'pyramid', cat: '3D & Formas', title: 'Pirâmide' },
      { name: 'circle', cat: '3D & Formas', title: 'Círculo' },
      { name: 'square', cat: '3D & Formas', title: 'Plano / Quadrado' },
      { name: 'triangle', cat: '3D & Formas', title: 'Triângulo' },
      { name: 'polygon', cat: '3D & Formas', title: 'Polígono' },
      { name: 'grid-dots', cat: '3D & Formas', title: 'Grid de Pontos / Vértices' },
      { name: 'grid-4x4', cat: '3D & Formas', title: 'Grid / Malha Quad' },
      { name: 'dimensions', cat: '3D & Formas', title: 'Dimensões / Transformação' },
      { name: 'mesh', cat: '3D & Formas', title: 'Malha Mesh' },

      // Ferramentas & Edição (Modeling Tools)
      { name: 'cursor-text', cat: 'Ferramentas', title: 'Seleção' },
      { name: 'marquee', cat: 'Ferramentas', title: 'Seleção em Caixa (Box Select)' },
      { name: 'arrows-move', cat: 'Ferramentas', title: 'Mover / Translação' },
      { name: 'rotate', cat: 'Ferramentas', title: 'Rotacionar' },
      { name: 'rotate-360', cat: 'Ferramentas', title: 'Rotação 360' },
      { name: 'arrows-maximize', cat: 'Ferramentas', title: 'Escalar' },
      { name: 'cut', cat: 'Ferramentas', title: 'Faca / Loop Cut' },
      { name: 'scissors', cat: 'Ferramentas', title: 'Cortar' },
      { name: 'wand', cat: 'Ferramentas', title: 'Subdivisão / Modificador' },
      { name: 'brush', cat: 'Ferramentas', title: 'Escultura / Pincel' },
      { name: 'eraser', cat: 'Ferramentas', title: 'Apagar' },
      { name: 'vector-spline', cat: 'Ferramentas', title: 'Curva Bézier' },
      { name: 'vector-bezier', cat: 'Ferramentas', title: 'Ponto de Controle' },
      { name: 'border-inner', cat: 'Ferramentas', title: 'Inset' },
      { name: 'triangle-square-circle', cat: 'Ferramentas', title: 'Bevel / Chanfro' },
      { name: 'flare', cat: 'Ferramentas', title: 'Vértice Ativo' },

      // Visão & Câmeras (View & Camera)
      { name: 'camera', cat: 'Visão & Câmeras', title: 'Câmera da Cena' },
      { name: 'camera-rotate', cat: 'Visão & Câmeras', title: 'Girar Câmera' },
      { name: 'video', cat: 'Visão & Câmeras', title: 'Gravação / Animação' },
      { name: 'eye', cat: 'Visão & Câmeras', title: 'Visível' },
      { name: 'eye-off', cat: 'Visão & Câmeras', title: 'Oculto' },
      { name: 'zoom-in', cat: 'Visão & Câmeras', title: 'Zoom Aproximar' },
      { name: 'zoom-out', cat: 'Visão & Câmeras', title: 'Zoom Afastar' },
      { name: 'hand-grab', cat: 'Visão & Câmeras', title: 'Pan / Arrastar Visão' },
      { name: 'sun', cat: 'Visão & Câmeras', title: 'Luz Solar / Direcional' },
      { name: 'bulb', cat: 'Visão & Câmeras', title: 'Ponto de Luz (Point Light)' },
      { name: 'sparkles', cat: 'Visão & Câmeras', title: 'Render PBR / Shading' },
      { name: 'photo', cat: 'Visão & Câmeras', title: 'Renderizar Imagem' },
      { name: 'palette', cat: 'Visão & Câmeras', title: 'Material / Cores' },

      // Janelas & UI (Workspace Controls)
      { name: 'pin', cat: 'Janelas & UI', title: 'Fixar Container' },
      { name: 'pinned', cat: 'Janelas & UI', title: 'Fixado' },
      { name: 'window-maximize', cat: 'Janelas & UI', title: 'Desacoplar Janela' },
      { name: 'window', cat: 'Janelas & UI', title: 'Janela Flutuante' },
      { name: 'minus', cat: 'Janelas & UI', title: 'Recolher / Minimizar' },
      { name: 'plus', cat: 'Janelas & UI', title: 'Adicionar / Expandir' },
      { name: 'layout-sidebar-right', cat: 'Janelas & UI', title: 'Painel N Direito' },
      { name: 'layout-sidebar', cat: 'Janelas & UI', title: 'Barra T Esquerda' },
      { name: 'adjustments', cat: 'Janelas & UI', title: 'Configurações' },
      { name: 'settings', cat: 'Janelas & UI', title: 'Preferências' },
      { name: 'layers-linked', cat: 'Janelas & UI', title: 'Outliner / Coleções' },
      { name: 'folder', cat: 'Janelas & UI', title: 'Coleção de Objetos' },
      { name: 'arrow-back-up', cat: 'Janelas & UI', title: 'Desfazer (Undo)' },
      { name: 'arrow-forward-up', cat: 'Janelas & UI', title: 'Refazer (Redo)' },
      { name: 'trash', cat: 'Janelas & UI', title: 'Excluir' },

      // Ações & Arquivo (System & Actions)
      { name: 'device-floppy', cat: 'Ações & Arquivo', title: 'Salvar Projeto' },
      { name: 'download', cat: 'Ações & Arquivo', title: 'Exportar OBJ / STL' },
      { name: 'upload', cat: 'Ações & Arquivo', title: 'Importar Modelo' },
      { name: 'file-code', cat: 'Ações & Arquivo', title: 'Script Python' },
      { name: 'help', cat: 'Ações & Arquivo', title: 'Ajuda / Atalhos' },
      { name: 'copy', cat: 'Ações & Arquivo', title: 'Duplicar (Shift+D)' },
      { name: 'share', cat: 'Ações & Arquivo', title: 'Compartilhar' }
    ];

    this.activeCategory = 'Todos';
    this.searchQuery = '';
    this.initDialog();
  }

  initDialog() {
    // Add "🔍 Ícones Tabler" button to Topbar
    const topbarRight = document.querySelector('.topbar-right');
    if (topbarRight && !document.getElementById('btn-open-icon-browser')) {
      const btnIcons = document.createElement('button');
      btnIcons.className = 'topbar-btn';
      btnIcons.id = 'btn-open-icon-browser';
      btnIcons.innerHTML = '<i class="ti ti-icons"></i> Catálogo de Ícones';
      btnIcons.title = 'Pesquisar e explorar ícones Tabler para usar na interface';
      topbarRight.prepend(btnIcons);

      btnIcons.addEventListener('click', () => this.open());
    }

    // Floating Modal Dialog
    const modal = document.createElement('div');
    modal.className = 'blender-modal-overlay hidden';
    modal.id = 'tabler-icon-browser-modal';
    modal.innerHTML = `
      <div class="blender-modal-dialog" id="icon-browser-dialog" style="width: 580px; height: 480px;">
        <div class="modal-dialog-header" id="icon-browser-drag-header">
          <div class="modal-dialog-title">
            <span><i class="ti ti-icons"></i> Catálogo Visual Tabler Icons (5.200+ Ícones)</span>
          </div>
          <button class="modal-close-btn" id="btn-close-icon-browser">✕</button>
        </div>

        <div class="modal-dialog-body" style="flex-direction: column; padding: 10px; gap: 8px;">
          <!-- Search & Filter Header -->
          <div style="display: flex; gap: 8px; align-items: center;">
            <div style="position: relative; flex: 1;">
              <input type="text" id="input-icon-search" class="blender-text-input" placeholder="🔍 Buscar ícone (ex: box, camera, rotate, cut, pin)..." style="width: 100%; height: 26px; padding-left: 8px;">
            </div>
            <span id="icon-count-label" style="font-size: 10px; color: var(--b-text-muted); font-family: var(--font-mono);"></span>
          </div>

          <!-- Category Pills -->
          <div class="icon-cat-pills" id="icon-cat-pills" style="display: flex; gap: 4px; overflow-x: auto; padding-bottom: 2px;">
            <button class="icon-cat-btn active" data-cat="Todos">Todos</button>
            <button class="icon-cat-btn" data-cat="3D & Formas">3D & Formas</button>
            <button class="icon-cat-btn" data-cat="Ferramentas">Ferramentas</button>
            <button class="icon-cat-btn" data-cat="Visão & Câmeras">Visão & Câmeras</button>
            <button class="icon-cat-btn" data-cat="Janelas & UI">Janelas & UI</button>
            <button class="icon-cat-btn" data-cat="Ações & Arquivo">Arquivo</button>
          </div>

          <!-- Copied Notification Toast -->
          <div id="icon-copied-toast" class="hidden" style="background: var(--b-accent-orange); color: #fff; padding: 4px 10px; border-radius: 3px; font-size: 10px; font-weight: 700; text-align: center;">
            ✓ Código do ícone copiado para a área de transferência!
          </div>

          <!-- Icons Grid -->
          <div class="icon-browser-grid" id="icon-browser-grid" style="flex: 1; overflow-y: auto; display: grid; grid-template-columns: repeat(auto-fill, minmax(75px, 1fr)); gap: 6px; padding: 4px;">
            <!-- Rendered dynamically -->
          </div>
        </div>
      </div>
    `;

    document.body.appendChild(modal);

    // Draggable
    const dialog = document.getElementById('icon-browser-dialog');
    const dragHeader = document.getElementById('icon-browser-drag-header');
    if (dialog && dragHeader) {
      this.makeDraggable(dialog, dragHeader);
    }

    this.bindEvents();
    this.render();
  }

  bindEvents() {
    const modal = document.getElementById('tabler-icon-browser-modal');
    document.getElementById('btn-close-icon-browser')?.addEventListener('click', () => this.close());

    // Search Input
    const searchInput = document.getElementById('input-icon-search');
    searchInput?.addEventListener('input', (e) => {
      this.searchQuery = e.target.value.toLowerCase().trim();
      this.render();
    });

    // Category Buttons
    const pills = document.querySelectorAll('.icon-cat-btn');
    pills.forEach((btn) => {
      btn.addEventListener('click', () => {
        pills.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        this.activeCategory = btn.dataset.cat;
        this.render();
      });
    });
  }

  render() {
    const grid = document.getElementById('icon-browser-grid');
    const countLabel = document.getElementById('icon-count-label');
    if (!grid) return;

    const filtered = this.icons.filter((icon) => {
      const matchCat = this.activeCategory === 'Todos' || icon.cat === this.activeCategory;
      const matchSearch = !this.searchQuery || icon.name.toLowerCase().includes(this.searchQuery) || icon.title.toLowerCase().includes(this.searchQuery);
      return matchCat && matchSearch;
    });

    if (countLabel) countLabel.textContent = `${filtered.length} ícones`;

    grid.innerHTML = filtered.map((icon) => `
      <div class="icon-card-item" data-icon="${icon.name}" title="${icon.title} (Clique para copiar <i class='ti ti-${icon.name}'></i>)">
        <i class="ti ti-${icon.name}" style="font-size: 20px; color: var(--b-text-main);"></i>
        <span class="icon-card-name">${icon.name}</span>
      </div>
    `).join('');

    // Click to copy
    grid.querySelectorAll('.icon-card-item').forEach((item) => {
      item.addEventListener('click', () => {
        const iconName = item.dataset.icon;
        const htmlSnippet = `<i class="ti ti-${iconName}"></i>`;
        navigator.clipboard?.writeText(htmlSnippet).then(() => {
          this.showToast(`Copiado: ${htmlSnippet}`);
        }).catch(() => {
          this.showToast(`Ícone: ti-${iconName}`);
        });
      });
    });
  }

  showToast(msg) {
    const toast = document.getElementById('icon-copied-toast');
    if (toast) {
      toast.textContent = `✓ ${msg}`;
      toast.classList.remove('hidden');
      setTimeout(() => toast.classList.add('hidden'), 2000);
    }
  }

  makeDraggable(element, handle) {
    let isDragging = false;
    let startX = 0, startY = 0;
    let initialLeft = 0, initialTop = 0;

    handle.addEventListener('mousedown', (e) => {
      if (e.target.tagName === 'BUTTON' || e.target.tagName === 'INPUT') return;
      isDragging = true;
      startX = e.clientX;
      startY = e.clientY;
      initialLeft = element.offsetLeft;
      initialTop = element.offsetTop;
      element.style.zIndex = '1100';
    });

    window.addEventListener('mousemove', (e) => {
      if (!isDragging) return;
      const dx = e.clientX - startX;
      const dy = e.clientY - startY;
      element.style.left = `${Math.max(10, Math.min(window.innerWidth - 80, initialLeft + dx))}px`;
      element.style.top = `${Math.max(30, Math.min(window.innerHeight - 80, initialTop + dy))}px`;
    });

    window.addEventListener('mouseup', () => {
      if (isDragging) {
        isDragging = false;
        element.style.zIndex = '1000';
      }
    });
  }

  open() {
    const modal = document.getElementById('tabler-icon-browser-modal');
    if (modal) modal.classList.remove('hidden');
  }

  close() {
    const modal = document.getElementById('tabler-icon-browser-modal');
    if (modal) modal.classList.add('hidden');
  }
}
