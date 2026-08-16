/**
 * IconBrowser - Interactive Visual Tabler Icon Picker & Drag-and-Drop Customizer for BlenderPro3D
 * 
 * Features:
 * 1. Clean Icon REPLACEMENT: Replaces the existing button icon directly without adding alongside.
 * 2. Drag to Remove: When the Icon Catalog is open, clicking & dragging a custom icon OUT of a button
 *    (onto the viewport, into the trash dropzone, or anywhere outside) removes the custom icon and
 *    instantly restores the original default icon!
 * 3. Glow & Lighting Feedback: Buttons light up with glowing neon border when hovering with a dragged icon.
 * 4. Full localStorage persistence of customized button icons + 1-Click Reset button.
 */
export class IconBrowser {
  constructor() {
    this.storageKey = 'blender_pro_custom_button_icons';
    this.customIcons = this.loadCustomIcons();
    this.isOpen = false;

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
      btnIcons.title = 'Pesquisar e arrastar ícones para substituir os botões da tela';
      topbarRight.prepend(btnIcons);

      btnIcons.addEventListener('click', () => this.open());
    }

    // Floating Modal Dialog
    const modal = document.createElement('div');
    modal.className = 'blender-modal-overlay hidden';
    modal.id = 'tabler-icon-browser-modal';
    modal.innerHTML = `
      <div class="blender-modal-dialog" id="icon-browser-dialog" style="width: 620px; height: 520px;">
        <div class="modal-dialog-header" id="icon-browser-drag-header">
          <div class="modal-dialog-title">
            <span><i class="ti ti-icons"></i> Catálogo & Editor de Ícones (Drag & Drop)</span>
          </div>
          <button class="modal-close-btn" id="btn-close-icon-browser">✕</button>
        </div>

        <div class="modal-dialog-body" style="flex-direction: column; padding: 12px; gap: 8px;">
          <!-- Dual-Action Interactive Guide Hint -->
          <div style="background: rgba(71, 114, 179, 0.12); border: 1px solid var(--b-accent-blue); border-radius: 4px; padding: 8px 10px; font-size: 10px; color: #ffffff; display: flex; flex-direction: column; gap: 4px;">
            <div style="display: flex; justify-content: space-between; align-items: center;">
              <span>🎯 <strong>Trocar Ícone:</strong> Arraste do catálogo e solte em cima do botão desejado.</span>
              <button class="mgmt-btn" id="btn-reset-icons-all" style="font-size: 9px; padding: 2px 8px; background: rgba(234, 118, 0, 0.3); border: 1px solid var(--b-accent-orange);">Restaurar Tudo</button>
            </div>
            <div style="color: var(--b-text-muted);">
              ↩️ <strong>Remover / Voltar ao Padrão:</strong> Clique no botão que você modificou e arraste-o para fora (solte aqui ou na tela) para remover o ícone e restaurar o original!
            </div>
          </div>

          <!-- Trash / Drop Zone to Revert -->
          <div id="icon-trash-dropzone" class="icon-trash-revert-zone">
            <i class="ti ti-arrow-back-up"></i> Solte aqui ou fora do botão para remover a troca e restaurar o ícone original
          </div>

          <!-- Search & Filter Header -->
          <div style="display: flex; gap: 8px; align-items: center;">
            <div style="position: relative; flex: 1;">
              <input type="text" id="input-icon-search" class="blender-text-input" placeholder="🔍 Buscar ícone (ex: box, camera, rotate, cut, pin, sun)..." style="width: 100%; height: 26px; padding-left: 8px;">
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

          <!-- Toast Alert -->
          <div id="icon-copied-toast" class="hidden" style="background: var(--b-accent-orange); color: #fff; padding: 5px 12px; border-radius: 3px; font-size: 10px; font-weight: 700; text-align: center; transition: all 0.2s;">
            ✓ Notificação
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

    // Trash Dropzone inside Dialog
    const trashZone = document.getElementById('icon-trash-dropzone');
    if (trashZone) {
      trashZone.addEventListener('dragover', (e) => {
        e.preventDefault();
        trashZone.classList.add('trash-active');
      });
      trashZone.addEventListener('dragleave', () => {
        trashZone.classList.remove('trash-active');
      });
      trashZone.addEventListener('drop', (e) => {
        e.preventDefault();
        trashZone.classList.remove('trash-active');
        const buttonKey = e.dataTransfer.getData('text/remove-button-icon');
        if (buttonKey) {
          this.removeButtonIcon(buttonKey);
        }
      });
    }

    // Global dragover & drop on document body to allow dragging away from button to remove
    document.body.addEventListener('dragover', (e) => {
      const isRemoving = e.dataTransfer.types.includes('text/remove-button-icon');
      if (isRemoving) {
        e.preventDefault();
      }
    });

    document.body.addEventListener('drop', (e) => {
      const buttonKey = e.dataTransfer.getData('text/remove-button-icon');
      if (buttonKey && !e.target.closest('.icon-drop-target-active')) {
        e.preventDefault();
        this.removeButtonIcon(buttonKey);
      }
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
        '.shelf-tool-btn, .tool-btn, .topbar-btn, .win-ctrl-btn, .nav-circle-btn, .shading-sphere-btn, .submode-btn, .prop-edit-btn, .outliner-add-btn, .n-panel-tab-btn, .blender-mode-dropdown-btn'
      );
    };

    const attachDropListeners = () => {
      getValidButtons().forEach((btn) => {
        // Cache original default HTML once
        if (!btn.dataset.defaultHtml) {
          btn.dataset.defaultHtml = btn.innerHTML;
        }

        if (btn.dataset.hasIconDrop) return;
        btn.dataset.hasIconDrop = 'true';

        const buttonKey = this.getButtonKey(btn);

        // DRAG OVER (Incoming new icon from catalog): Light up brightly!
        btn.addEventListener('dragover', (e) => {
          if (e.dataTransfer.types.includes('text/plain')) {
            e.preventDefault();
            e.dataTransfer.dropEffect = 'copy';
            btn.classList.add('icon-drop-target-active');
          }
        });

        btn.addEventListener('dragenter', (e) => {
          if (e.dataTransfer.types.includes('text/plain')) {
            e.preventDefault();
            btn.classList.add('icon-drop-target-active');
          }
        });

        btn.addEventListener('dragleave', () => {
          btn.classList.remove('icon-drop-target-active');
        });

        // DROP: Replace existing icon cleanly!
        btn.addEventListener('drop', (e) => {
          btn.classList.remove('icon-drop-target-active');
          const iconName = e.dataTransfer.getData('text/plain');
          if (!iconName) return;

          e.preventDefault();
          e.stopPropagation();
          this.setButtonIcon(btn, iconName, buttonKey);
        });

        // DRAG OUT (Drag existing custom icon AWAY from button to remove & restore original)
        btn.addEventListener('dragstart', (e) => {
          if (this.isOpen || btn.dataset.hasCustomIcon === 'true') {
            e.dataTransfer.setData('text/remove-button-icon', buttonKey);
            e.dataTransfer.effectAllowed = 'move';
            btn.classList.add('is-dragging-out');
            document.body.classList.add('is-removing-icon');
          }
        });

        btn.addEventListener('dragend', () => {
          btn.classList.remove('is-dragging-out');
          document.body.classList.remove('is-removing-icon');
        });
      });
    };

    attachDropListeners();
    const observer = new MutationObserver(() => attachDropListeners());
    observer.observe(document.body, { childList: true, subtree: true });
  }

  getButtonKey(btn) {
    return btn.id || btn.dataset.tool || btn.dataset.submode || btn.dataset.shading || btn.className.split(' ').find(c => c.startsWith('tool-') || c.startsWith('nav-') || c.startsWith('btn-')) || btn.innerText.trim();
  }

  findButton(buttonKey) {
    let btn = document.getElementById(buttonKey);
    if (!btn) btn = document.querySelector(`[data-tool="${buttonKey}"]`);
    if (!btn) btn = document.querySelector(`[data-submode="${buttonKey}"]`);
    if (!btn) btn = document.querySelector(`[data-shading="${buttonKey}"]`);
    if (!btn) btn = document.querySelector(`.${buttonKey}`);
    return btn;
  }

  setButtonIcon(btn, iconName, buttonKey) {
    if (!btn.dataset.defaultHtml) {
      btn.dataset.defaultHtml = btn.innerHTML;
    }

    // Determine if button has text or is icon-only
    const hasText = btn.textContent.trim().length > 0 && !btn.classList.contains('shelf-tool-btn') && !btn.classList.contains('nav-circle-btn') && !btn.classList.contains('shading-sphere-btn') && !btn.classList.contains('win-ctrl-btn') && !btn.classList.contains('submode-btn') && !btn.classList.contains('prop-edit-btn');

    if (hasText) {
      // Preserve label text, replace icon prefix cleanly
      const labelText = btn.textContent.replace(/^[^\w\s\u00C0-\u00FF]+/, '').trim();
      btn.innerHTML = `<i class="ti ti-${iconName}"></i> <span>${labelText}</span>`;
    } else {
      // 100% pure icon replacement (no leftover SVGs or duplicate characters)
      btn.innerHTML = `<i class="ti ti-${iconName}"></i>`;
    }

    btn.dataset.hasCustomIcon = 'true';
    btn.setAttribute('draggable', 'true');
    btn.classList.add('custom-icon-applied');

    // Success burst flash
    btn.classList.add('icon-drop-success');
    setTimeout(() => btn.classList.remove('icon-drop-success'), 600);

    // Save in localStorage
    if (buttonKey) {
      this.customIcons[buttonKey] = iconName;
      this.saveCustomIcons();
    }

    this.showToast(`Ícone substituído por ti-${iconName}!`);
  }

  removeButtonIcon(buttonKey) {
    if (!buttonKey || !this.customIcons[buttonKey]) return;

    delete this.customIcons[buttonKey];
    this.saveCustomIcons();

    const btn = this.findButton(buttonKey);
    if (btn && btn.dataset.defaultHtml) {
      btn.innerHTML = btn.dataset.defaultHtml;
      delete btn.dataset.hasCustomIcon;
      btn.removeAttribute('draggable');
      btn.classList.remove('custom-icon-applied');

      // Feedback animation
      btn.classList.add('icon-drop-success');
      setTimeout(() => btn.classList.remove('icon-drop-success'), 500);
    }

    this.showToast('Ícone removido! Ícone original restaurado com sucesso.');
  }

  applyCustomIcons() {
    Object.keys(this.customIcons).forEach((buttonKey) => {
      const iconName = this.customIcons[buttonKey];
      const btn = this.findButton(buttonKey);
      if (btn) {
        if (!btn.dataset.defaultHtml) {
          btn.dataset.defaultHtml = btn.innerHTML;
        }

        const hasText = btn.textContent.trim().length > 0 && !btn.classList.contains('shelf-tool-btn') && !btn.classList.contains('nav-circle-btn') && !btn.classList.contains('shading-sphere-btn') && !btn.classList.contains('win-ctrl-btn') && !btn.classList.contains('submode-btn') && !btn.classList.contains('prop-edit-btn');

        if (hasText) {
          const labelText = btn.textContent.replace(/^[^\w\s\u00C0-\u00FF]+/, '').trim();
          btn.innerHTML = `<i class="ti ti-${iconName}"></i> <span>${labelText}</span>`;
        } else {
          btn.innerHTML = `<i class="ti ti-${iconName}"></i>`;
        }

        btn.dataset.hasCustomIcon = 'true';
        btn.setAttribute('draggable', 'true');
        btn.classList.add('custom-icon-applied');
      }
    });
  }

  showToast(msg) {
    const toast = document.getElementById('icon-copied-toast');
    if (toast) {
      toast.textContent = `✓ ${msg}`;
      toast.classList.remove('hidden');
      setTimeout(() => toast.classList.add('hidden'), 2400);
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
    this.isOpen = true;
    const modal = document.getElementById('tabler-icon-browser-modal');
    if (modal) modal.classList.remove('hidden');
    document.body.classList.add('icon-browser-active');
  }

  close() {
    this.isOpen = false;
    const modal = document.getElementById('tabler-icon-browser-modal');
    if (modal) modal.classList.add('hidden');
    document.body.classList.remove('icon-browser-active');
  }
}
