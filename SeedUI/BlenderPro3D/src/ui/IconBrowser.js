/**
 * IconBrowser - Universal Interactive Tabler Icon Customizer for ALL Menus & Buttons in BlenderPro3D
 * 
 * Features:
 * 1. UNIVERSAL: Works on ALL menus, popups, Shift+A items, toolshelf, topbar, workspace tabs,
 *    dropdowns, outliner rows, accordion headers, and navigation buttons!
 * 2. Clean REPLACEMENT: Replaces the existing button/menu icon directly without adding alongside.
 * 3. Drag-Out to Remove: Clicking & dragging a modified icon OUT of any menu or button (onto the viewport,
 *    into the trash dropzone, or anywhere outside) removes the custom icon and restores the original default icon!
 * 4. Glow & Pulse Feedback: Menus and buttons light up with glowing neon border when hovering with a dragged icon.
 * 5. Full localStorage persistence + 1-Click Reset button.
 */
export class IconBrowser {
  constructor() {
    this.storageKey = 'blender_pro_custom_all_icons_v2';
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
      console.warn('Could not load custom icons', e);
    }
    return {};
  }

  saveCustomIcons() {
    try {
      localStorage.setItem(this.storageKey, JSON.stringify(this.customIcons));
    } catch (e) {
      console.warn('Could not save custom icons', e);
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
      btnIcons.title = 'Pesquisar e arrastar ícones para substituir em QUALQUER menu ou botão';
      topbarRight.prepend(btnIcons);

      btnIcons.addEventListener('click', () => this.open());
    }

    // Floating Modal Dialog
    const modal = document.createElement('div');
    modal.className = 'blender-modal-overlay hidden';
    modal.id = 'tabler-icon-browser-modal';
    modal.innerHTML = `
      <div class="blender-modal-dialog" id="icon-browser-dialog" style="width: 640px; height: 530px;">
        <div class="modal-dialog-header" id="icon-browser-drag-header">
          <div class="modal-dialog-title">
            <span><i class="ti ti-icons"></i> Catálogo de Ícones — Válido Para Todos os Menus e Botões</span>
          </div>
          <button class="modal-close-btn" id="btn-close-icon-browser">✕</button>
        </div>

        <div class="modal-dialog-body" style="flex-direction: column; padding: 12px; gap: 8px;">
          <!-- Universal Guide Hint -->
          <div style="background: rgba(71, 114, 179, 0.12); border: 1px solid var(--b-accent-blue); border-radius: 4px; padding: 8px 10px; font-size: 10px; color: #ffffff; display: flex; flex-direction: column; gap: 4px;">
            <div style="display: flex; justify-content: space-between; align-items: center;">
              <span>🎯 <strong>Trocar Ícone:</strong> Arraste do catálogo e solte em cima de <strong>qualquer menu ou botão</strong> (Shift+A, Mesh, Luz, Topo, Ferramentas).</span>
              <button class="mgmt-btn" id="btn-reset-icons-all" style="font-size: 9px; padding: 2px 8px; background: rgba(234, 118, 0, 0.3); border: 1px solid var(--b-accent-orange);">Restaurar Tudo</button>
            </div>
            <div style="color: var(--b-text-muted);">
              ↩️ <strong>Remover / Voltar ao Padrão:</strong> Clique no menu/botão modificado e arraste-o para fora para remover a troca e restaurar o ícone original!
            </div>
          </div>

          <!-- Trash / Revert Drop Zone -->
          <div id="icon-trash-dropzone" class="icon-trash-revert-zone">
            <i class="ti ti-arrow-back-up"></i> Solte aqui ou em qualquer lugar fora para remover o ícone e restaurar o padrão
          </div>

          <!-- Search & Filter Header -->
          <div style="display: flex; gap: 8px; align-items: center;">
            <div style="position: relative; flex: 1;">
              <input type="text" id="input-icon-search" class="blender-text-input" placeholder="🔍 Buscar ícone (ex: box, camera, bulb, sun, cylinder, rotate, cut)..." style="width: 100%; height: 26px; padding-left: 8px;">
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
      if (confirm('Deseja restaurar todos os ícones originais de todos os menus e botões?')) {
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
        const itemKey = e.dataTransfer.getData('text/remove-button-icon');
        if (itemKey) {
          this.removeButtonIcon(itemKey);
        }
      });
    }

    // Global dragover & drop on document body to allow dragging away from menu/button to remove
    document.body.addEventListener('dragover', (e) => {
      const isRemoving = e.dataTransfer.types.includes('text/remove-button-icon');
      if (isRemoving) {
        e.preventDefault();
      }
    });

    document.body.addEventListener('drop', (e) => {
      const itemKey = e.dataTransfer.getData('text/remove-button-icon');
      if (itemKey && !e.target.closest('.icon-drop-target-active')) {
        e.preventDefault();
        this.removeButtonIcon(itemKey);
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
      <div class="icon-card-item" draggable="true" data-icon="${icon.name}" title="${icon.title} (Arraste para qualquer menu/botão ou clique para copiar)">
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

  getValidElements() {
    return document.querySelectorAll(
      'button, .shelf-tool-btn, .tool-btn, .topbar-btn, .win-ctrl-btn, .nav-circle-btn, ' +
      '.shading-sphere-btn, .submode-btn, .prop-edit-btn, .outliner-add-btn, .n-strip-tab, ' +
      '.blender-mode-dropdown-btn, .top-menu-item, .vp-menu-item, .ws-tab, .mode-menu-option, ' +
      '.popup-menu-item, .sub-item, .outliner-item, .accordion-head, .key-hint, .blender-brand'
    );
  }

  // 2. Interactive Drop Zones on ALL Menus & Buttons
  initDropZones() {
    const attachDropListeners = () => {
      this.getValidElements().forEach((el) => {
        // Cache original default HTML once
        if (!el.dataset.defaultHtml) {
          el.dataset.defaultHtml = el.innerHTML;
        }

        if (el.dataset.hasIconDrop) return;
        el.dataset.hasIconDrop = 'true';

        const itemKey = this.getItemKey(el);

        // DRAG OVER (Incoming new icon from catalog): Light up brightly!
        el.addEventListener('dragover', (e) => {
          if (e.dataTransfer.types.includes('text/plain')) {
            e.preventDefault();
            e.dataTransfer.dropEffect = 'copy';
            el.classList.add('icon-drop-target-active');
          }
        });

        el.addEventListener('dragenter', (e) => {
          if (e.dataTransfer.types.includes('text/plain')) {
            e.preventDefault();
            el.classList.add('icon-drop-target-active');
          }
        });

        el.addEventListener('dragleave', () => {
          el.classList.remove('icon-drop-target-active');
        });

        // DROP: Replace existing icon cleanly!
        el.addEventListener('drop', (e) => {
          el.classList.remove('icon-drop-target-active');
          const iconName = e.dataTransfer.getData('text/plain');
          if (!iconName) return;

          e.preventDefault();
          e.stopPropagation();
          this.setButtonIcon(el, iconName, itemKey);
        });

        // DRAG OUT (Drag existing custom icon AWAY from menu/button to remove & restore original)
        el.addEventListener('dragstart', (e) => {
          if (this.isOpen || el.dataset.hasCustomIcon === 'true') {
            e.dataTransfer.setData('text/remove-button-icon', itemKey);
            e.dataTransfer.effectAllowed = 'move';
            el.classList.add('is-dragging-out');
            document.body.classList.add('is-removing-icon');
          }
        });

        el.addEventListener('dragend', () => {
          el.classList.remove('is-dragging-out');
          document.body.classList.remove('is-removing-icon');
        });
      });
    };

    attachDropListeners();
    const observer = new MutationObserver(() => attachDropListeners());
    observer.observe(document.body, { childList: true, subtree: true });
  }

  getItemKey(el) {
    return el.id ||
      (el.dataset.tool ? `tool_${el.dataset.tool}` : null) ||
      (el.dataset.prim ? `prim_${el.dataset.prim}` : null) ||
      (el.dataset.submode ? `submode_${el.dataset.submode}` : null) ||
      (el.dataset.shading ? `shading_${el.dataset.shading}` : null) ||
      (el.dataset.ws ? `ws_${el.dataset.ws}` : null) ||
      (el.dataset.tab ? `tab_${el.dataset.tab}` : null) ||
      (el.dataset.mode ? `mode_${el.dataset.mode}` : null) ||
      (el.dataset.name ? `obj_${el.dataset.name}` : null) ||
      (el.className.split(' ').find(c => c.startsWith('tool-') || c.startsWith('nav-') || c.startsWith('btn-') || c.startsWith('menu-'))) ||
      `menu_${el.textContent.trim().replace(/\s+/g, '_')}`;
  }

  findElement(key) {
    let el = document.getElementById(key);
    if (el) return el;

    if (key.startsWith('tool_')) return document.querySelector(`[data-tool="${key.replace('tool_', '')}"]`);
    if (key.startsWith('prim_')) return document.querySelector(`[data-prim="${key.replace('prim_', '')}"]`);
    if (key.startsWith('submode_')) return document.querySelector(`[data-submode="${key.replace('submode_', '')}"]`);
    if (key.startsWith('shading_')) return document.querySelector(`[data-shading="${key.replace('shading_', '')}"]`);
    if (key.startsWith('ws_')) return document.querySelector(`[data-ws="${key.replace('ws_', '')}"]`);
    if (key.startsWith('tab_')) return document.querySelector(`[data-tab="${key.replace('tab_', '')}"]`);
    if (key.startsWith('mode_')) return document.querySelector(`[data-mode="${key.replace('mode_', '')}"]`);
    if (key.startsWith('obj_')) return document.querySelector(`[data-name="${key.replace('obj_', '')}"]`);

    el = document.querySelector(`.${key}`);
    if (el) return el;

    const all = this.getValidElements();
    for (const item of all) {
      if (this.getItemKey(item) === key) return item;
    }
    return null;
  }

  setButtonIcon(el, iconName, key) {
    if (!el.dataset.defaultHtml) {
      el.dataset.defaultHtml = el.innerHTML;
    }

    // Check if element has an existing icon tag, dot, or SVG
    const existingIconEl = el.querySelector('i.ti, svg, span.icon, span.nav-icon, span.mode-dot, span.blender-icon');
    
    if (existingIconEl) {
      existingIconEl.outerHTML = `<i class="ti ti-${iconName}"></i>`;
    } else {
      // Check if it is an icon-only button
      const rawText = el.textContent.trim();
      const isIconOnly = el.classList.contains('shelf-tool-btn') || el.classList.contains('nav-circle-btn') || el.classList.contains('shading-sphere-btn') || el.classList.contains('win-ctrl-btn') || el.classList.contains('submode-btn') || el.classList.contains('prop-edit-btn') || !rawText;

      if (isIconOnly) {
        el.innerHTML = `<i class="ti ti-${iconName}"></i>`;
      } else {
        // Has text (like menus "Mesh", "Camera", "Light", "Arquivo", "Layout", etc.)
        const cleanText = rawText.replace(/^[\p{Emoji}\p{Symbol}\p{Punctuation}\s]+/u, '').trim();
        const subArrow = el.querySelector('span:last-child');
        const hasSubArrow = subArrow && (subArrow.textContent.includes('▶') || subArrow.textContent.includes('▾') || subArrow.classList.contains('key-tag'));

        if (hasSubArrow && subArrow !== el) {
          const arrowHtml = subArrow.outerHTML;
          el.innerHTML = `<span><i class="ti ti-${iconName}"></i> ${cleanText}</span> ${arrowHtml}`;
        } else {
          el.innerHTML = `<i class="ti ti-${iconName}"></i> <span>${cleanText || rawText}</span>`;
        }
      }
    }

    el.dataset.hasCustomIcon = 'true';
    el.setAttribute('draggable', 'true');
    el.classList.add('custom-icon-applied');

    // Success flash burst
    el.classList.add('icon-drop-success');
    setTimeout(() => el.classList.remove('icon-drop-success'), 600);

    // Save in localStorage
    if (key) {
      this.customIcons[key] = iconName;
      this.saveCustomIcons();
    }

    this.showToast(`Ícone substituído por ti-${iconName}!`);
  }

  removeButtonIcon(key) {
    if (!key || !this.customIcons[key]) return;

    delete this.customIcons[key];
    this.saveCustomIcons();

    const el = this.findElement(key);
    if (el && el.dataset.defaultHtml) {
      el.innerHTML = el.dataset.defaultHtml;
      delete el.dataset.hasCustomIcon;
      el.removeAttribute('draggable');
      el.classList.remove('custom-icon-applied');

      el.classList.add('icon-drop-success');
      setTimeout(() => el.classList.remove('icon-drop-success'), 500);
    }

    this.showToast('Ícone removido! Padrão original restaurado.');
  }

  applyCustomIcons() {
    Object.keys(this.customIcons).forEach((key) => {
      const iconName = this.customIcons[key];
      const el = this.findElement(key);
      if (el) {
        this.setButtonIcon(el, iconName, null);
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
