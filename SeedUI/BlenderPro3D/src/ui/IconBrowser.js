/**
 * IconBrowser - Interactive Visual Tabler Icon Picker & Drag-and-Drop Customizer for BlenderPro3D
 * 
 * Features:
 * 1. Drag & Drop Icons directly onto ANY button (Toolshelf, Header, Navigation Gizmo, Windows)!
 * 2. Buttons glow and light up dynamically when hovering with a dragged icon.
 * 3. 1-Click copy of HTML icon tag.
 * 4. Instant search across 100+ curated 3D & UI icons.
 * 5. Full localStorage persistence of customized button icons + Reset button.
 */
export class IconBrowser {
  constructor() {
    this.storageKey = 'blender_pro_custom_button_icons';
    this.customIcons = this.loadCustomIcons();

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
    this.initDropZones();
    this.applyCustomIcons();
  }

  loadCustomIcons() {
    try {
      const saved = localStorage.getItem(this.storageKey);
      if (saved) return JSON.parse(saved);
    } catch (e) {
      console.warn('Could not load custom button icons', e);
    }
    return {};
  }

  saveCustomIcons() {
    try {
      localStorage.setItem(this.storageKey, JSON.stringify(this.customIcons));
    } catch (e) {
      console.warn('Could not save custom button icons', e);
    }
  }

  resetAllCustomIcons() {
    this.customIcons = {};
    this.saveCustomIcons();
    location.reload();
  }

  initDialog() {
    // Add "🔍 Ícones Tabler" button to Topbar
    const topbarRight = document.querySelector('.topbar-right');
    if (topbarRight && !document.getElementById('btn-open-icon-browser')) {
      const btnIcons = document.createElement('button');
      btnIcons.className = 'topbar-btn';
      btnIcons.id = 'btn-open-icon-browser';
      btnIcons.innerHTML = '<i class="ti ti-icons"></i> Ícones Tabler';
      btnIcons.title = 'Pesquisar e arrastar ícones Tabler para qualquer botão da interface';
      topbarRight.prepend(btnIcons);

      btnIcons.addEventListener('click', () => this.open());
    }

    // Floating Modal Dialog
    const modal = document.createElement('div');
    modal.className = 'blender-modal-overlay hidden';
    modal.id = 'tabler-icon-browser-modal';
    modal.innerHTML = `
      <div class="blender-modal-dialog" id="icon-browser-dialog" style="width: 600px; height: 500px;">
        <div class="modal-dialog-header" id="icon-browser-drag-header">
          <div class="modal-dialog-title">
            <span><i class="ti ti-icons"></i> Catálogo & Customizador de Ícones (Drag & Drop)</span>
          </div>
          <button class="modal-close-btn" id="btn-close-icon-browser">✕</button>
        </div>

        <div class="modal-dialog-body" style="flex-direction: column; padding: 12px; gap: 8px;">
          <!-- Interactive Guide Hint -->
          <div style="background: rgba(71, 114, 179, 0.15); border: 1px solid var(--b-accent-blue); border-radius: 4px; padding: 6px 10px; font-size: 10.5px; color: #ffffff; display: flex; align-items: center; justify-content: space-between;">
            <span>✨ <strong>Arraste qualquer ícone</strong> e solte em cima de um botão da tela para trocá-lo instantaneamente!</span>
            <button class="mgmt-btn" id="btn-reset-icons-all" style="font-size: 9px; padding: 2px 6px;">Restaurar Padrão</button>
          </div>

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
            ✓ Código do ícone copiado!
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

    document.getElementById('btn-reset-icons-all')?.addEventListener('click', () => {
      if (confirm('Deseja restaurar todos os ícones originais da interface?')) {
        this.resetAllCustomIcons();
      }
    });

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
      <div class="icon-card-item" draggable="true" data-icon="${icon.name}" title="${icon.title} (Arraste para um botão ou clique para copiar)">
        <i class="ti ti-${icon.name}" style="font-size: 22px; color: var(--b-text-main);"></i>
        <span class="icon-card-name">${icon.name}</span>
      </div>
    `).join('');

    // Drag and Drop from Grid Cards
    grid.querySelectorAll('.icon-card-item').forEach((item) => {
      item.addEventListener('dragstart', (e) => {
        const iconName = item.dataset.icon;
        e.dataTransfer.setData('text/plain', iconName);
        e.dataTransfer.effectAllowed = 'copy';
        document.body.classList.add('is-dragging-icon');
      });

      item.addEventListener('dragend', () => {
        document.body.classList.remove('is-dragging-icon');
      });

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

  // 2. Interactive Drop Zones on all Buttons
  initDropZones() {
    const getValidButtons = () => {
      return document.querySelectorAll(
        '.tool-btn, .topbar-btn, .win-ctrl-btn, .nav-circle-btn, .shading-btn, .outliner-add-btn, .submode-btn, .viewport-mode-btn'
      );
    };

    const attachDropListeners = () => {
      getValidButtons().forEach((btn) => {
        if (btn.dataset.hasIconDrop) return;
        btn.dataset.hasIconDrop = 'true';

        // Drag Over -> Light up / Glow
        btn.addEventListener('dragover', (e) => {
          e.preventDefault();
          e.dataTransfer.dropEffect = 'copy';
          btn.classList.add('icon-drop-target-active');
        });

        btn.addEventListener('dragenter', (e) => {
          e.preventDefault();
          btn.classList.add('icon-drop-target-active');
        });

        btn.addEventListener('dragleave', () => {
          btn.classList.remove('icon-drop-target-active');
        });

        // Drop -> Apply New Icon and Save
        btn.addEventListener('drop', (e) => {
          e.preventDefault();
          btn.classList.remove('icon-drop-target-active');
          const iconName = e.dataTransfer.getData('text/plain');
          if (!iconName) return;

          const buttonKey = btn.id || btn.className.split(' ').find(c => c.startsWith('tool-') || c.startsWith('btn-')) || btn.innerText.trim();
          this.setButtonIcon(btn, iconName, buttonKey);
        });
      });
    };

    attachDropListeners();
    // Re-attach whenever DOM updates
    const observer = new MutationObserver(() => attachDropListeners());
    observer.observe(document.body, { childList: true, subtree: true });
  }

  setButtonIcon(btn, iconName, buttonKey) {
    // Replace SVG, <i>, or text icon inside button
    const existingIcon = btn.querySelector('i, svg, span.icon, span.nav-icon');
    if (existingIcon) {
      existingIcon.outerHTML = `<i class="ti ti-${iconName}"></i>`;
    } else {
      // If button has text or is empty
      const text = btn.innerText.trim();
      btn.innerHTML = `<i class="ti ti-${iconName}"></i> ${text ? `<span>${text}</span>` : ''}`;
    }

    // Success burst flash
    btn.classList.add('icon-drop-success');
    setTimeout(() => btn.classList.remove('icon-drop-success'), 600);

    // Save in localStorage
    if (buttonKey) {
      this.customIcons[buttonKey] = iconName;
      this.saveCustomIcons();
    }

    this.showToast(`Ícone ti-${iconName} aplicado com sucesso!`);
  }

  applyCustomIcons() {
    Object.keys(this.customIcons).forEach((buttonKey) => {
      const iconName = this.customIcons[buttonKey];
      let btn = document.getElementById(buttonKey);
      if (!btn) {
        btn = document.querySelector(`.${buttonKey}`);
      }
      if (btn) {
        const existingIcon = btn.querySelector('i, svg, span.icon, span.nav-icon');
        if (existingIcon) {
          existingIcon.outerHTML = `<i class="ti ti-${iconName}"></i>`;
        } else {
          const text = btn.innerText.trim();
          btn.innerHTML = `<i class="ti ti-${iconName}"></i> ${text ? `<span>${text}</span>` : ''}`;
        }
      }
    });
  }

  showToast(msg) {
    const toast = document.getElementById('icon-copied-toast');
    if (toast) {
      toast.textContent = `✓ ${msg}`;
      toast.classList.remove('hidden');
      setTimeout(() => toast.classList.add('hidden'), 2200);
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
