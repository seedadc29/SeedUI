/**
 * UIEditor - Comprehensive Workspace & Interface Customization Engine
 * 
 * Features:
 * 1. Live Theme & CSS Variable Customizer (Colors, Sizes, Borders, Fonts, Accents)
 * 2. Container Window Controls:
 *    - 📌 Pin / Unpin
 *    - 🗗 Undock / Detach into Floating Window (Draggable)
 *    - 📥 Redock back to sidebar
 *    - ➖ Retract / Collapse container
 * 3. Toolbar & Navigation Widget Undocking (Floatable Toolshelf & Navigation Cluster)
 * 4. Theme Presets (Blender Dark, Classic Dark, Light Pro, Midnight, Obsidian)
 * 5. Full localStorage persistence and Export/Import Theme JSON
 */
export class UIEditor {
  constructor(engine) {
    this.engine = engine;
    this.storageKey = 'blender_pro_ui_theme';
    this.layoutKey = 'blender_pro_ui_layout';

    this.defaultTheme = {
      bgDarkest: '#161616',
      bgDark: '#222222',
      bgPanel: '#282828',
      bgViewport: '#484848',
      bgInput: '#171717',
      border: '#383838',
      borderSubtle: '#2d2d2d',
      accentOrange: '#ea7600',
      accentBlue: '#4772b3',
      textMain: '#d4d4d4',
      textMuted: '#8c8c8c',
      borderRadius: 4,
      fontSize: 11,
      topbarHeight: 28,
      sidebarWidth: 280,
      toolshelfWidth: 38
    };

    this.theme = { ...this.defaultTheme };
    this.floatingPanels = new Map();

    this.loadSettings();
    this.applyTheme();
    this.injectContainerControls();
    this.initPreferencesModal();
    this.initFloatingWidgets();
  }

  // 1. Theme and CSS Variables
  applyTheme() {
    const root = document.documentElement;
    root.style.setProperty('--b-bg-darkest', this.theme.bgDarkest);
    root.style.setProperty('--b-bg-dark', this.theme.bgDark);
    root.style.setProperty('--b-bg-panel', this.theme.bgPanel);
    root.style.setProperty('--b-bg-viewport', this.theme.bgViewport);
    root.style.setProperty('--b-bg-input', this.theme.bgInput);
    root.style.setProperty('--b-border', this.theme.border);
    root.style.setProperty('--b-border-subtle', this.theme.borderSubtle);
    root.style.setProperty('--b-accent-orange', this.theme.accentOrange);
    root.style.setProperty('--b-accent-blue', this.theme.accentBlue);
    root.style.setProperty('--b-text-main', this.theme.textMain);
    root.style.setProperty('--b-text-muted', this.theme.textMuted);
    root.style.setProperty('--b-radius', `${this.theme.borderRadius}px`);
    root.style.setProperty('--b-font-size', `${this.theme.fontSize}px`);

    const topbar = document.querySelector('.blender-topbar');
    if (topbar) topbar.style.height = `${this.theme.topbarHeight}px`;

    const sidebar = document.getElementById('right-sidebar');
    if (sidebar) sidebar.style.width = `${this.theme.sidebarWidth}px`;

    const toolshelf = document.getElementById('toolshelf');
    if (toolshelf) toolshelf.style.width = `${this.theme.toolshelfWidth}px`;

    if (this.engine && this.engine.scene) {
      this.engine.scene.background.set(this.theme.bgViewport);
    }
  }

  setThemeProperty(key, value, save = true) {
    this.theme[key] = value;
    this.applyTheme();
    if (save) this.saveSettings();
  }

  saveSettings() {
    try {
      localStorage.setItem(this.storageKey, JSON.stringify(this.theme));
    } catch (e) {
      console.warn('Could not save theme to localStorage', e);
    }
  }

  loadSettings() {
    try {
      const saved = localStorage.getItem(this.storageKey);
      if (saved) {
        this.theme = { ...this.defaultTheme, ...JSON.parse(saved) };
      }
    } catch (e) {
      console.warn('Could not load theme from localStorage', e);
    }
  }

  resetTheme() {
    this.theme = { ...this.defaultTheme };
    this.applyTheme();
    this.saveSettings();
    this.syncModalInputs();
  }

  // 2. Container Window Controls (Pin, Undock, Collapse)
  injectContainerControls() {
    // 2.1 Outliner Controls
    const outlinerHeader = document.querySelector('.outliner-header');
    if (outlinerHeader && !outlinerHeader.querySelector('.container-ctrls')) {
      const ctrls = document.createElement('div');
      ctrls.className = 'container-ctrls';
      ctrls.innerHTML = `
        <button class="win-ctrl-btn btn-pin" title="Fixar / Desafixar Container" data-target="outliner">📌</button>
        <button class="win-ctrl-btn btn-undock" title="Desacoplar para Janela Flutuante" data-target="outliner">🗗</button>
        <button class="win-ctrl-btn btn-collapse" title="Recolher / Expandir" data-target="outliner">➖</button>
      `;
      outlinerHeader.querySelector('.outliner-actions')?.prepend(ctrls);
    }

    // 2.2 N-Panel Controls
    const nPanelHeader = document.querySelector('.n-panel-header');
    if (nPanelHeader && !nPanelHeader.querySelector('.container-ctrls')) {
      const ctrls = document.createElement('div');
      ctrls.className = 'container-ctrls';
      ctrls.style.marginLeft = 'auto';
      ctrls.innerHTML = `
        <button class="win-ctrl-btn btn-pin" title="Fixar / Desafixar Container" data-target="npanel">📌</button>
        <button class="win-ctrl-btn btn-undock" title="Desacoplar para Janela Flutuante" data-target="npanel">🗗</button>
        <button class="win-ctrl-btn btn-collapse" title="Recolher / Expandir" data-target="npanel">➖</button>
      `;
      nPanelHeader.appendChild(ctrls);
    }

    // Bind Button Actions
    document.querySelectorAll('.win-ctrl-btn').forEach((btn) => {
      btn.addEventListener('click', (e) => {
        e.stopPropagation();
        const target = btn.dataset.target;
        if (btn.classList.contains('btn-pin')) this.togglePin(target, btn);
        else if (btn.classList.contains('btn-undock')) this.toggleUndock(target, btn);
        else if (btn.classList.contains('btn-collapse')) this.toggleCollapse(target, btn);
      });
    });
  }

  toggleCollapse(targetId, btn) {
    if (targetId === 'outliner') {
      const body = document.getElementById('outliner-tree');
      const panel = document.querySelector('.blender-outliner-panel');
      if (body && panel) {
        body.classList.toggle('hidden');
        panel.style.height = body.classList.contains('hidden') ? '26px' : '180px';
        if (btn) btn.textContent = body.classList.contains('hidden') ? '➕' : '➖';
      }
    } else if (targetId === 'npanel') {
      const body = document.querySelector('.n-panel-body');
      if (body) {
        body.classList.toggle('hidden');
        if (btn) btn.textContent = body.classList.contains('hidden') ? '➕' : '➖';
      }
    }
  }

  togglePin(targetId, btn) {
    const el = targetId === 'outliner' ? document.querySelector('.blender-outliner-panel') : document.querySelector('.blender-n-panel-container');
    if (!el) return;
    el.classList.toggle('is-pinned');
    const isPinned = el.classList.contains('is-pinned');
    if (btn) btn.style.color = isPinned ? 'var(--b-accent-orange)' : 'inherit';
  }

  toggleUndock(targetId, btn) {
    const targetMap = {
      outliner: { el: document.querySelector('.blender-outliner-panel'), title: '📁 Scene Collection', defaultPos: { x: 120, y: 80, w: 260, h: 280 } },
      npanel: { el: document.querySelector('.blender-n-panel-container'), title: '📐 Properties & Transform', defaultPos: { x: 400, y: 80, w: 280, h: 420 } }
    };

    const config = targetMap[targetId];
    if (!config || !config.el) return;

    if (this.floatingPanels.has(targetId)) {
      // Redock
      this.redockPanel(targetId);
      if (btn) {
        btn.textContent = '🗗';
        btn.title = 'Desacoplar para Janela Flutuante';
      }
    } else {
      // Undock to Floating Window
      this.undockPanel(targetId, config);
      if (btn) {
        btn.textContent = '📥';
        btn.title = 'Acoplar de volta na barra lateral';
      }
    }
    this.engine.onResize();
  }

  undockPanel(targetId, config) {
    const originalEl = config.el;
    const parent = originalEl.parentElement;
    const placeholder = document.createElement('div');
    placeholder.className = `dock-placeholder dock-${targetId}`;
    placeholder.style.display = 'none';
    parent.insertBefore(placeholder, originalEl);

    // Create Floating Window Wrapper
    const floatWin = document.createElement('div');
    floatWin.className = 'blender-floating-window';
    floatWin.id = `floating-win-${targetId}`;
    floatWin.style.left = `${config.defaultPos.x}px`;
    floatWin.style.top = `${config.defaultPos.y}px`;
    floatWin.style.width = `${config.defaultPos.w}px`;
    floatWin.style.height = `${config.defaultPos.h}px`;

    // Floating Window Titlebar
    const titlebar = document.createElement('div');
    titlebar.className = 'floating-win-header';
    titlebar.innerHTML = `
      <span class="float-win-drag-title">⋮⋮ ${config.title}</span>
      <div class="float-win-actions">
        <button class="win-ctrl-btn" title="Acoplar de volta na Barra Lateral" id="redock-${targetId}">📥</button>
      </div>
    `;

    floatWin.appendChild(titlebar);
    floatWin.appendChild(originalEl);
    document.body.appendChild(floatWin);

    this.makeDraggable(floatWin, titlebar);

    document.getElementById(`redock-${targetId}`)?.addEventListener('click', () => {
      this.toggleUndock(targetId);
    });

    this.floatingPanels.set(targetId, {
      originalEl,
      placeholder,
      parent,
      floatWin
    });
  }

  redockPanel(targetId) {
    const data = this.floatingPanels.get(targetId);
    if (!data) return;

    data.parent.insertBefore(data.originalEl, data.placeholder);
    data.placeholder.remove();
    data.floatWin.remove();
    this.floatingPanels.delete(targetId);
  }

  makeDraggable(element, handle) {
    let isDragging = false;
    let startX = 0, startY = 0;
    let initialLeft = 0, initialTop = 0;

    handle.addEventListener('mousedown', (e) => {
      if (e.target.tagName === 'BUTTON') return;
      isDragging = true;
      startX = e.clientX;
      startY = e.clientY;
      initialLeft = element.offsetLeft;
      initialTop = element.offsetTop;
      element.style.zIndex = '1000';
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
        element.style.zIndex = '100';
      }
    });
  }

  // 3. Floating Navigation & Toolshelf Detach
  initFloatingWidgets() {
    const navCluster = document.getElementById('blender-nav-cluster');
    if (navCluster) {
      // Add subtle drag grip on navigation cluster
      const grip = document.createElement('div');
      grip.className = 'widget-drag-grip';
      grip.title = 'Arraste para reposicionar na tela';
      grip.innerHTML = '⋮⋮';
      navCluster.prepend(grip);
      this.makeDraggable(navCluster, grip);
    }
  }

  // 4. Preferences & Theme Modal
  initPreferencesModal() {
    // Add "🎨 Customizar UI" button in Topbar
    const topbarRight = document.querySelector('.topbar-right');
    if (topbarRight) {
      const btnCustom = document.createElement('button');
      btnCustom.className = 'topbar-btn custom-ui-btn';
      btnCustom.id = 'btn-open-ui-editor';
      btnCustom.innerHTML = '🎨 Customizar UI';
      btnCustom.title = 'Personalizar toda a interface (Cores, Tamanhos, Bordas, Desacoplamento)';
      topbarRight.prepend(btnCustom);

      btnCustom.addEventListener('click', () => {
        this.openModal();
      });
    }

    // Add Menu item in "Editar ➔ Preferências de Interface"
    const editMenu = document.querySelector('.top-menu-dropdowns .top-menu-item:nth-child(2)');
    if (editMenu) {
      editMenu.addEventListener('click', () => {
        this.openModal();
      });
    }

    // Modal Structure
    const modal = document.createElement('div');
    modal.className = 'blender-modal-overlay hidden';
    modal.id = 'ui-customizer-modal';
    modal.innerHTML = `
      <div class="blender-modal-dialog">
        <div class="modal-dialog-header">
          <div class="modal-dialog-title">
            <span>🎨 Editor & Customizador de Interface (Blender Pro)</span>
          </div>
          <button class="modal-close-btn" id="btn-close-ui-editor">✕</button>
        </div>

        <div class="modal-dialog-body">
          <!-- Left Tabs in Modal -->
          <div class="modal-nav-sidebar">
            <button class="modal-nav-item active" data-tab="theme-colors">🎨 Cores & Temas</button>
            <button class="modal-nav-item" data-tab="theme-sizes">📐 Tamanhos & Bordas</button>
            <button class="modal-nav-item" data-tab="theme-panels">🗂️ Painéis & Fixação</button>
            <button class="modal-nav-item" data-tab="theme-export">💾 Presets & Backup</button>
          </div>

          <!-- Content Panes -->
          <div class="modal-content-area">
            <!-- TAB 1: CORES -->
            <div class="modal-tab-pane" id="pane-theme-colors">
              <div class="modal-section-title">Presets de Tema Rápidos</div>
              <div class="theme-preset-grid">
                <button class="theme-preset-card" data-preset="blender-dark">
                  <div class="preset-preview" style="background:#222; border-color:#ea7600;"></div>
                  <span>Blender 4.1 Dark</span>
                </button>
                <button class="theme-preset-card" data-preset="classic-dark">
                  <div class="preset-preview" style="background:#181818; border-color:#4772b3;"></div>
                  <span>Classic Charcoal</span>
                </button>
                <button class="theme-preset-card" data-preset="midnight-blue">
                  <div class="preset-preview" style="background:#131c26; border-color:#38bdf8;"></div>
                  <span>Midnight Blue</span>
                </button>
                <button class="theme-preset-card" data-preset="obsidian">
                  <div class="preset-preview" style="background:#0a0a0a; border-color:#10b981;"></div>
                  <span>Obsidian Emerald</span>
                </button>
                <button class="theme-preset-card" data-preset="cyber-amber">
                  <div class="preset-preview" style="background:#1c1917; border-color:#f59e0b;"></div>
                  <span>Cyber Amber</span>
                </button>
              </div>

              <div class="modal-section-title" style="margin-top:14px;">Paleta de Cores Customizada</div>
              <div class="color-picker-grid">
                <div class="color-prop-row">
                  <label>Fundo da Viewport 3D:</label>
                  <input type="color" id="clr-bgViewport" value="${this.theme.bgViewport}">
                </div>
                <div class="color-prop-row">
                  <label>Fundo dos Painéis:</label>
                  <input type="color" id="clr-bgPanel" value="${this.theme.bgPanel}">
                </div>
                <div class="color-prop-row">
                  <label>Barra Superior (Topbar):</label>
                  <input type="color" id="clr-bgDarkest" value="${this.theme.bgDarkest}">
                </div>
                <div class="color-prop-row">
                  <label>Barra Lateral (Sidebar):</label>
                  <input type="color" id="clr-bgDark" value="${this.theme.bgDark}">
                </div>
                <div class="color-prop-row">
                  <label>Cor de Realce (Seleção Laranja):</label>
                  <input type="color" id="clr-accentOrange" value="${this.theme.accentOrange}">
                </div>
                <div class="color-prop-row">
                  <label>Cor de Ação Ativa (Azul):</label>
                  <input type="color" id="clr-accentBlue" value="${this.theme.accentBlue}">
                </div>
                <div class="color-prop-row">
                  <label>Bordas e Divisórias:</label>
                  <input type="color" id="clr-border" value="${this.theme.border}">
                </div>
                <div class="color-prop-row">
                  <label>Texto Principal:</label>
                  <input type="color" id="clr-textMain" value="${this.theme.textMain}">
                </div>
              </div>
            </div>

            <!-- TAB 2: TAMANHOS & BORDAS -->
            <div class="modal-tab-pane hidden" id="pane-theme-sizes">
              <div class="modal-section-title">Dimensões & Espaçamento</div>
              <div class="slider-prop-group">
                <div class="slider-prop-row">
                  <div class="slider-prop-header">
                    <label>Largura da Barra Lateral (N-Panel):</label>
                    <span id="val-sidebarWidth">${this.theme.sidebarWidth}px</span>
                  </div>
                  <input type="range" min="200" max="450" step="10" id="rng-sidebarWidth" value="${this.theme.sidebarWidth}">
                </div>

                <div class="slider-prop-row">
                  <div class="slider-prop-header">
                    <label>Largura da Barra de Ferramentas (T-Panel):</label>
                    <span id="val-toolshelfWidth">${this.theme.toolshelfWidth}px</span>
                  </div>
                  <input type="range" min="30" max="80" step="2" id="rng-toolshelfWidth" value="${this.theme.toolshelfWidth}">
                </div>

                <div class="slider-prop-row">
                  <div class="slider-prop-header">
                    <label>Altura do Header Topbar:</label>
                    <span id="val-topbarHeight">${this.theme.topbarHeight}px</span>
                  </div>
                  <input type="range" min="24" max="48" step="2" id="rng-topbarHeight" value="${this.theme.topbarHeight}">
                </div>

                <div class="slider-prop-row">
                  <div class="slider-prop-header">
                    <label>Arredondamento das Bordas (Radius):</label>
                    <span id="val-borderRadius">${this.theme.borderRadius}px</span>
                  </div>
                  <input type="range" min="0" max="16" step="1" id="rng-borderRadius" value="${this.theme.borderRadius}">
                </div>

                <div class="slider-prop-row">
                  <div class="slider-prop-header">
                    <label>Tamanho da Fonte da Interface:</label>
                    <span id="val-fontSize">${this.theme.fontSize}px</span>
                  </div>
                  <input type="range" min="9" max="16" step="1" id="rng-fontSize" value="${this.theme.fontSize}">
                </div>
              </div>
            </div>

            <!-- TAB 3: PAINÉIS & FIXAÇÃO -->
            <div class="modal-tab-pane hidden" id="pane-theme-panels">
              <div class="modal-section-title">Gerenciamento de Janelas e Containers</div>
              <div class="panels-management-list">
                <div class="panel-mgmt-row">
                  <div class="panel-mgmt-info">
                    <strong>📁 Scene Collection (Outliner)</strong>
                    <span>Árvore de objetos da cena</span>
                  </div>
                  <div class="panel-mgmt-actions">
                    <button class="mgmt-btn" id="mgmt-undock-outliner">Desacoplar (Janela Flutuante)</button>
                    <button class="mgmt-btn" id="mgmt-collapse-outliner">Recolher</button>
                  </div>
                </div>

                <div class="panel-mgmt-row">
                  <div class="panel-mgmt-info">
                    <strong>📐 N-Panel (Propriedades & Transform)</strong>
                    <span>Campos numéricos XYZ, ferramentas e visão</span>
                  </div>
                  <div class="panel-mgmt-actions">
                    <button class="mgmt-btn" id="mgmt-undock-npanel">Desacoplar (Janela Flutuante)</button>
                    <button class="mgmt-btn" id="mgmt-collapse-npanel">Recolher</button>
                  </div>
                </div>

                <div class="panel-mgmt-row">
                  <div class="panel-mgmt-info">
                    <strong>🧭 Bússola & Botões de Navegação</strong>
                    <span>Gizmo de eixos e controles de zoom/pan</span>
                  </div>
                  <div class="panel-mgmt-actions">
                    <button class="mgmt-btn" id="mgmt-reset-nav">Centralizar Gizmo</button>
                  </div>
                </div>
              </div>
            </div>

            <!-- TAB 4: PRESETS & BACKUP -->
            <div class="modal-tab-pane hidden" id="pane-theme-export">
              <div class="modal-section-title">Exportar / Importar Configurações</div>
              <div class="backup-actions-box">
                <button class="topbar-btn" id="btn-export-theme">💾 Exportar Tema (JSON)</button>
                <button class="topbar-btn" id="btn-import-theme">📂 Importar Tema</button>
                <input type="file" id="file-import-theme" accept=".json" class="hidden">
              </div>

              <div class="modal-section-title" style="margin-top:20px;">Restaurar Padrões</div>
              <button class="topbar-btn" id="btn-reset-default-theme" style="background:#5a2323; border-color:#833;">Restaurar Padrão Oficial do Blender 4.1</button>
            </div>
          </div>
        </div>
      </div>
    `;

    document.body.appendChild(modal);
    this.bindModalEvents();
  }

  bindModalEvents() {
    const modal = document.getElementById('ui-customizer-modal');
    document.getElementById('btn-close-ui-editor')?.addEventListener('click', () => this.closeModal());

    modal.addEventListener('click', (e) => {
      if (e.target === modal) this.closeModal();
    });

    // Tab Navigation inside Modal
    const tabBtns = modal.querySelectorAll('.modal-nav-item');
    const panes = {
      'theme-colors': document.getElementById('pane-theme-colors'),
      'theme-sizes': document.getElementById('pane-theme-sizes'),
      'theme-panels': document.getElementById('pane-theme-panels'),
      'theme-export': document.getElementById('pane-theme-export')
    };

    tabBtns.forEach((btn) => {
      btn.addEventListener('click', () => {
        tabBtns.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        const tab = btn.dataset.tab;
        Object.keys(panes).forEach(k => {
          if (panes[k]) panes[k].classList.toggle('hidden', k !== tab);
        });
      });
    });

    // Live Color Pickers
    const colorProps = ['bgViewport', 'bgPanel', 'bgDarkest', 'bgDark', 'accentOrange', 'accentBlue', 'border', 'textMain'];
    colorProps.forEach((prop) => {
      const input = document.getElementById(`clr-${prop}`);
      if (input) {
        input.addEventListener('input', (e) => {
          this.setThemeProperty(prop, e.target.value);
        });
      }
    });

    // Sliders
    const sliderProps = [
      { id: 'sidebarWidth', unit: 'px' },
      { id: 'toolshelfWidth', unit: 'px' },
      { id: 'topbarHeight', unit: 'px' },
      { id: 'borderRadius', unit: 'px' },
      { id: 'fontSize', unit: 'px' }
    ];

    sliderProps.forEach(({ id, unit }) => {
      const rng = document.getElementById(`rng-${id}`);
      const valEl = document.getElementById(`val-${id}`);
      if (rng) {
        rng.addEventListener('input', (e) => {
          const val = parseInt(e.target.value);
          if (valEl) valEl.textContent = `${val}${unit}`;
          this.setThemeProperty(id, val);
        });
      }
    });

    // Preset Cards
    modal.querySelectorAll('.theme-preset-card').forEach((card) => {
      card.addEventListener('click', () => {
        const preset = card.dataset.preset;
        this.applyPreset(preset);
      });
    });

    // Panel Management tab buttons
    document.getElementById('mgmt-undock-outliner')?.addEventListener('click', () => {
      this.toggleUndock('outliner');
      this.closeModal();
    });

    document.getElementById('mgmt-collapse-outliner')?.addEventListener('click', () => {
      this.toggleCollapse('outliner');
    });

    document.getElementById('mgmt-undock-npanel')?.addEventListener('click', () => {
      this.toggleUndock('npanel');
      this.closeModal();
    });

    document.getElementById('mgmt-collapse-npanel')?.addEventListener('click', () => {
      this.toggleCollapse('npanel');
    });

    document.getElementById('mgmt-reset-nav')?.addEventListener('click', () => {
      const cluster = document.getElementById('blender-nav-cluster');
      if (cluster) {
        cluster.style.left = '';
        cluster.style.top = '36px';
        cluster.style.right = '14px';
      }
    });

    // Reset default
    document.getElementById('btn-reset-default-theme')?.addEventListener('click', () => {
      this.resetTheme();
    });

    // Export / Import
    document.getElementById('btn-export-theme')?.addEventListener('click', () => {
      const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(this.theme, null, 2));
      const downloadAnchor = document.createElement('a');
      downloadAnchor.setAttribute("href", dataStr);
      downloadAnchor.setAttribute("download", "blender_pro_theme.json");
      document.body.appendChild(downloadAnchor);
      downloadAnchor.click();
      downloadAnchor.remove();
    });

    const fileInput = document.getElementById('file-import-theme');
    document.getElementById('btn-import-theme')?.addEventListener('click', () => {
      fileInput?.click();
    });

    fileInput?.addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (file) {
        const reader = new FileReader();
        reader.onload = (evt) => {
          try {
            const imported = JSON.parse(evt.target.result);
            this.theme = { ...this.defaultTheme, ...imported };
            this.applyTheme();
            this.saveSettings();
            this.syncModalInputs();
          } catch (err) {
            alert('Arquivo de tema inválido!');
          }
        };
        reader.readAsText(file);
      }
    });
  }

  applyPreset(presetName) {
    const presets = {
      'blender-dark': {
        bgDarkest: '#161616', bgDark: '#222222', bgPanel: '#282828', bgViewport: '#484848',
        bgInput: '#171717', border: '#383838', borderSubtle: '#2d2d2d',
        accentOrange: '#ea7600', accentBlue: '#4772b3', textMain: '#d4d4d4', textMuted: '#8c8c8c',
        borderRadius: 4, fontSize: 11
      },
      'classic-dark': {
        bgDarkest: '#121212', bgDark: '#1a1a1a', bgPanel: '#202020', bgViewport: '#333333',
        bgInput: '#141414', border: '#303030', borderSubtle: '#242424',
        accentOrange: '#f97316', accentBlue: '#3b82f6', textMain: '#e5e7eb', textMuted: '#9ca3af',
        borderRadius: 2, fontSize: 11
      },
      'midnight-blue': {
        bgDarkest: '#0c1218', bgDark: '#131c26', bgPanel: '#1a2634', bgViewport: '#1e293b',
        bgInput: '#0f172a', border: '#25384d', borderSubtle: '#1b2a3a',
        accentOrange: '#0284c7', accentBlue: '#38bdf8', textMain: '#f1f5f9', textMuted: '#94a3b8',
        borderRadius: 6, fontSize: 11
      },
      'obsidian': {
        bgDarkest: '#050505', bgDark: '#0e0e0e', bgPanel: '#161616', bgViewport: '#1c1c1c',
        bgInput: '#080808', border: '#282828', borderSubtle: '#1c1c1c',
        accentOrange: '#10b981', accentBlue: '#059669', textMain: '#f3f4f6', textMuted: '#6b7280',
        borderRadius: 4, fontSize: 11
      },
      'cyber-amber': {
        bgDarkest: '#12100e', bgDark: '#1c1917', bgPanel: '#292524', bgViewport: '#2d2825',
        bgInput: '#171412', border: '#44403c', borderSubtle: '#2e2a27',
        accentOrange: '#f59e0b', accentBlue: '#d97706', textMain: '#fef3c7', textMuted: '#a8a29e',
        borderRadius: 4, fontSize: 11
      }
    };

    if (presets[presetName]) {
      this.theme = { ...this.theme, ...presets[presetName] };
      this.applyTheme();
      this.saveSettings();
      this.syncModalInputs();
    }
  }

  syncModalInputs() {
    const colorProps = ['bgViewport', 'bgPanel', 'bgDarkest', 'bgDark', 'accentOrange', 'accentBlue', 'border', 'textMain'];
    colorProps.forEach((prop) => {
      const input = document.getElementById(`clr-${prop}`);
      if (input) input.value = this.theme[prop];
    });

    const sliderProps = ['sidebarWidth', 'toolshelfWidth', 'topbarHeight', 'borderRadius', 'fontSize'];
    sliderProps.forEach((prop) => {
      const rng = document.getElementById(`rng-${prop}`);
      const valEl = document.getElementById(`val-${prop}`);
      if (rng) rng.value = this.theme[prop];
      if (valEl) valEl.textContent = `${this.theme[prop]}px`;
    });
  }

  openModal() {
    const modal = document.getElementById('ui-customizer-modal');
    if (modal) {
      this.syncModalInputs();
      modal.classList.remove('hidden');
    }
  }

  closeModal() {
    const modal = document.getElementById('ui-customizer-modal');
    if (modal) modal.classList.add('hidden');
  }
}
