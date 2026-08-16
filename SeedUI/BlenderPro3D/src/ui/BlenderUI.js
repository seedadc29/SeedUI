import * as THREE from 'three';

export class BlenderUI {
  constructor(engine, sceneManager, transformManager, meshEditor, historyManager) {
    this.engine = engine;
    this.sceneManager = sceneManager;
    this.transformManager = transformManager;
    this.meshEditor = meshEditor;
    this.historyManager = historyManager;

    this.currentMode = 'object'; // 'object' | 'edit'
    this.currentSubmode = 'vertex'; // 'vertex' | 'edge' | 'face'
    this.currentTool = 'select'; // 'select' | 'cursor' | 'move' | 'rotate' | 'scale' | 'transform'
    this.currentNTab = 'item'; // 'item' | 'tool' | 'view'
    this.isNPanelOpen = true;

    this.mouse = new THREE.Vector2();
    this.raycaster = new THREE.Raycaster();
    this.lastScreenMouse = { x: 200, y: 200 };

    this.initModeSwitcher();
    this.initSubmodes();
    this.initToolshelf();
    this.initShadingControls();
    this.initWorkspaceTabs();
    this.initTopMenus();
    this.initNPanel();
    this.initScrubbers();
    this.initOutliner();
    this.initCameraGizmos();
    this.initShiftAMenu();
    this.initRaycasting();
    this.initShortcuts();

    // Link callbacks
    this.sceneManager.onSelectionChange = (obj) => {
      this.updateOutlinerSelection();
      this.updateTransformUI();
      if (obj) {
        this.transformManager.attach(obj);
      } else {
        this.transformManager.detach();
      }
    };

    this.sceneManager.onSceneChange = () => {
      this.renderOutlinerTree();
      this.updateSceneStats();
    };

    this.engine.onCameraMoved = (cameraObj) => {
      if (this.sceneManager.getSelectedObject() === cameraObj) {
        this.updateTransformUI();
      }
    };

    // Initial render
    this.renderOutlinerTree();
    this.updateSceneStats();
  }

  // 1. Mode Switcher (Object vs Edit Mode)
  initModeSwitcher() {
    const btn = document.getElementById('btn-mode-dropdown');
    const menu = document.getElementById('mode-dropdown-menu');
    const label = document.getElementById('current-mode-label');
    const submodeContainer = document.getElementById('submode-selectors');
    const editTools = document.querySelectorAll('.edit-only');

    if (btn && menu) {
      btn.addEventListener('click', (e) => {
        e.stopPropagation();
        menu.classList.toggle('hidden');
      });

      window.addEventListener('click', () => {
        menu.classList.add('hidden');
      });

      menu.querySelectorAll('.mode-menu-option').forEach((opt) => {
        opt.addEventListener('click', () => {
          const mode = opt.dataset.mode;
          this.setMode(mode);
        });
      });
    }
  }

  setMode(mode) {
    this.currentMode = mode;
    const label = document.getElementById('current-mode-label');
    const submodeContainer = document.getElementById('submode-selectors');
    const editTools = document.querySelectorAll('.edit-only');

    if (label) label.textContent = mode === 'object' ? 'Modo Objeto' : 'Modo Edição';

    if (mode === 'edit') {
      submodeContainer?.classList.remove('hidden');
      editTools.forEach(t => t.classList.remove('hidden'));
      const activeObj = this.sceneManager.getSelectedObject();
      if (activeObj && activeObj.userData?.quadMesh) {
        this.meshEditor.enterEditMode(activeObj);
        this.meshEditor.setSubmode(this.currentSubmode);
      }
      this.sceneManager.setEditModeView(true);
    } else {
      submodeContainer?.classList.add('hidden');
      editTools.forEach(t => t.classList.add('hidden'));
      this.meshEditor.exitEditMode();
      this.sceneManager.setEditModeView(false);
      const activeObj = this.sceneManager.getSelectedObject();
      if (activeObj) this.transformManager.attach(activeObj);
    }

    const options = document.querySelectorAll('.mode-menu-option');
    options.forEach(opt => opt.classList.toggle('active', opt.dataset.mode === mode));
  }

  toggleMode() {
    this.setMode(this.currentMode === 'object' ? 'edit' : 'object');
  }

  // 2. Submodes (Vertex, Edge, Face)
  initSubmodes() {
    const btns = document.querySelectorAll('.submode-btn');
    btns.forEach((btn) => {
      btn.addEventListener('click', () => {
        const sub = btn.dataset.submode;
        this.setSubmode(sub);
      });
    });
  }

  setSubmode(submode) {
    this.currentSubmode = submode;
    document.querySelectorAll('.submode-btn').forEach(b => {
      b.classList.toggle('active', b.dataset.submode === submode);
    });
    if (this.currentMode === 'edit') {
      this.meshEditor.setSubmode(submode);
    }
  }

  // 3. Toolshelf (T-Panel)
  initToolshelf() {
    const btns = document.querySelectorAll('.shelf-tool-btn');
    btns.forEach((btn) => {
      btn.addEventListener('click', () => {
        const tool = btn.dataset.tool;
        this.setTool(tool);
      });
    });
  }

  setTool(tool) {
    this.currentTool = tool;
    document.querySelectorAll('.shelf-tool-btn').forEach(b => {
      b.classList.toggle('active', b.dataset.tool === tool);
    });

    switch (tool) {
      case 'move':
        this.transformManager.setMode('translate');
        break;
      case 'rotate':
        this.transformManager.setMode('rotate');
        break;
      case 'scale':
        this.transformManager.setMode('scale');
        break;
      case 'extrude':
        if (this.currentMode === 'edit') this.meshEditor.extrude();
        break;
      case 'inset':
        if (this.currentMode === 'edit') this.meshEditor.inset();
        break;
      case 'bevel':
        if (this.currentMode === 'edit') this.meshEditor.bevel();
        break;
      case 'loopcut':
        if (this.currentMode === 'edit') this.meshEditor.startLoopCut();
        break;
      case 'subdivide':
        if (this.currentMode === 'edit') this.meshEditor.subdivide();
        break;
      case 'fill':
        if (this.currentMode === 'edit') this.meshEditor.fillFace();
        break;
      case 'merge':
        if (this.currentMode === 'edit') this.meshEditor.mergeVertices('center');
        break;
      case 'smooth':
        if (this.currentMode === 'edit') this.meshEditor.smoothVertices(0.5);
        break;
      case 'shrink':
        if (this.currentMode === 'edit') this.meshEditor.shrinkFlatten(0.2);
        break;
      case 'delete':
        if (this.currentMode === 'edit') this.meshEditor.deleteSelection();
        else {
          const obj = this.sceneManager.getSelectedObject();
          if (obj) this.sceneManager.removeObject(obj);
        }
        break;
    }
  }

  // 4. Shading Controls (Wireframe, Solid, Material, Rendered)
  initShadingControls() {
    const btns = document.querySelectorAll('.shading-sphere-btn');
    btns.forEach((btn) => {
      btn.addEventListener('click', () => {
        const shading = btn.dataset.shading;
        this.setShading(shading);
      });
    });
  }

  setShading(mode) {
    this.engine.setShadingMode(mode, this.sceneManager.getAllMeshes());
    document.querySelectorAll('.shading-sphere-btn').forEach(b => {
      b.classList.toggle('active', b.dataset.shading === mode);
    });
  }

  // 5. Authentic Blender N-Panel
  initNPanel() {
    const handle = document.getElementById('btn-toggle-npanel');
    const rightSidebar = document.getElementById('right-sidebar');
    const tabs = document.querySelectorAll('.n-strip-tab');
    const titleEl = document.getElementById('n-panel-title');

    const panes = {
      item: document.getElementById('n-pane-item'),
      tool: document.getElementById('n-pane-tool'),
      view: document.getElementById('n-pane-view')
    };

    if (handle && rightSidebar) {
      handle.addEventListener('click', () => {
        this.toggleNPanel();
      });
    }

    tabs.forEach((tabBtn) => {
      tabBtn.addEventListener('click', () => {
        const tab = tabBtn.dataset.tab;
        tabs.forEach(t => t.classList.remove('active'));
        tabBtn.classList.add('active');

        Object.keys(panes).forEach((k) => {
          if (panes[k]) {
            if (k === tab) panes[k].classList.remove('hidden');
            else panes[k].classList.add('hidden');
          }
        });

        const titles = { item: 'Transform', tool: 'Tool Settings', view: 'View Settings' };
        if (titleEl) titleEl.textContent = titles[tab] || 'Properties';
      });
    });

    // View Lock Checkboxes
    document.getElementById('chk-n-cam-lock')?.addEventListener('change', (e) => {
      this.engine.lockCameraToView = e.target.checked;
    });

    document.getElementById('chk-n-grid')?.addEventListener('change', (e) => {
      const vis = e.target.checked;
      if (this.engine.gridHelper) this.engine.gridHelper.visible = vis;
      if (this.engine.xAxisLine) this.engine.xAxisLine.visible = vis;
      if (this.engine.yAxisLine) this.engine.yAxisLine.visible = vis;
      if (this.engine.axesHelper) this.engine.axesHelper.visible = vis;
    });

    document.getElementById('chk-n-shadows')?.addEventListener('change', (e) => {
      this.engine.renderer.shadowMap.enabled = e.target.checked;
    });

    // World Background Color Picker & Presets
    const worldPicker = document.getElementById('picker-world-bg');
    const worldHexLabel = document.getElementById('label-world-hex');
    const worldPresetBtns = document.querySelectorAll('.world-preset-btn');

    const updateWorldBg = (colorHex) => {
      this.engine.scene.background = new THREE.Color(colorHex);
      if (worldPicker) worldPicker.value = colorHex;
      if (worldHexLabel) worldHexLabel.textContent = colorHex.toUpperCase();
      worldPresetBtns.forEach(btn => {
        btn.classList.toggle('active', btn.dataset.color.toLowerCase() === colorHex.toLowerCase());
      });
    };

    if (worldPicker) {
      worldPicker.addEventListener('input', (e) => {
        updateWorldBg(e.target.value);
      });
    }

    worldPresetBtns.forEach((btn) => {
      btn.addEventListener('click', () => {
        const color = btn.dataset.color;
        updateWorldBg(color);
      });
    });
  }

  toggleNPanel() {
    const rightSidebar = document.getElementById('right-sidebar');
    const handle = document.getElementById('btn-toggle-npanel');
    if (!rightSidebar) return;

    this.isNPanelOpen = !this.isNPanelOpen;
    rightSidebar.classList.toggle('hidden', !this.isNPanelOpen);
    if (handle) handle.innerHTML = this.isNPanelOpen ? '<span>‹</span>' : '<span>›</span>';

    // Force canvas resize to fill 100% of the screen
    this.engine.onResize();
    requestAnimationFrame(() => this.engine.onResize());
    setTimeout(() => this.engine.onResize(), 50);
  }

  // 6. Interactive Drag-to-Scrub Inputs (Exact Blender Ergonomics)
  initScrubbers() {
    const fields = document.querySelectorAll('.scrubber-field');

    fields.forEach((field) => {
      const input = field.querySelector('.scrubber-input');
      const axis = field.dataset.axis;
      if (!input || !axis) return;

      let isDragging = false;
      let startX = 0;
      let startVal = 0;

      field.addEventListener('mousedown', (e) => {
        if (e.target === input && document.activeElement === input) return;
        isDragging = true;
        startX = e.clientX;
        startVal = parseFloat(input.value) || 0;
        document.body.style.cursor = 'ew-resize';
      });

      window.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const delta = e.clientX - startX;
        const speed = e.shiftKey ? 0.002 : (axis.startsWith('rot') ? 0.5 : 0.02);
        const newVal = startVal + delta * speed;

        if (axis.startsWith('rot')) {
          input.value = newVal.toFixed(1);
        } else {
          input.value = newVal.toFixed(3);
        }

        this.applyScrubberChange(axis, newVal);
      });

      window.addEventListener('mouseup', () => {
        if (isDragging) {
          isDragging = false;
          document.body.style.cursor = 'default';
        }
      });

      input.addEventListener('change', () => {
        const val = parseFloat(input.value) || 0;
        this.applyScrubberChange(axis, val);
      });
    });
  }

  applyScrubberChange(axis, val) {
    const obj = this.sceneManager.getSelectedObject();
    if (!obj) return;

    switch (axis) {
      case 'pos-x': obj.position.x = val; break;
      case 'pos-y': obj.position.y = val; break;
      case 'pos-z': obj.position.z = val; break;
      case 'rot-x': obj.rotation.x = THREE.MathUtils.degToRad(val); break;
      case 'rot-y': obj.rotation.y = THREE.MathUtils.degToRad(val); break;
      case 'rot-z': obj.rotation.z = THREE.MathUtils.degToRad(val); break;
      case 'scl-x': obj.scale.x = Math.max(0.001, val); break;
      case 'scl-y': obj.scale.y = Math.max(0.001, val); break;
      case 'scl-z': obj.scale.z = Math.max(0.001, val); break;
    }
  }

  updateTransformUI() {
    const obj = this.sceneManager.getSelectedObject();
    if (!obj) return;

    const setVal = (id, v, decimals = 3) => {
      const el = document.getElementById(id);
      if (el && document.activeElement !== el) el.value = v.toFixed(decimals);
    };

    setVal('num-pos-x', obj.position.x, 3);
    setVal('num-pos-y', obj.position.y, 3);
    setVal('num-pos-z', obj.position.z, 3);

    setVal('num-rot-x', THREE.MathUtils.radToDeg(obj.rotation.x), 1);
    setVal('num-rot-y', THREE.MathUtils.radToDeg(obj.rotation.y), 1);
    setVal('num-rot-z', THREE.MathUtils.radToDeg(obj.rotation.z), 1);

    setVal('num-scl-x', obj.scale.x, 3);
    setVal('num-scl-y', obj.scale.y, 3);
    setVal('num-scl-z', obj.scale.z, 3);
  }

  // 7. Outliner Tree (Scene Collection)
  initOutliner() {
    document.getElementById('btn-add-object')?.addEventListener('click', (e) => {
      this.toggleShiftAMenu(e.clientX - 160, e.clientY + 20);
    });
  }

  renderOutlinerTree() {
    const tree = document.getElementById('outliner-tree');
    if (!tree) return;

    const objects = this.sceneManager.getAllMeshes();
    const selected = this.sceneManager.getSelectedObject();

    let html = '';
    objects.forEach((obj) => {
      const isSel = (obj === selected);
      const icon = obj.userData?.isCamera ? '📷' : (obj.userData?.isLight ? '💡' : '🧊');
      html += `
        <div class="outliner-item ${isSel ? 'active' : ''}" data-name="${obj.name}">
          <div class="outliner-item-left">
            <span class="outliner-item-icon">${icon}</span>
            <span>${obj.name}</span>
          </div>
          <span class="outliner-eye">${obj.visible ? '👁' : 'Ø'}</span>
        </div>
      `;
    });

    tree.innerHTML = html;

    tree.querySelectorAll('.outliner-item').forEach((item) => {
      item.addEventListener('click', (e) => {
        const name = item.dataset.name;
        const target = objects.find(o => o.name === name);
        if (target) this.sceneManager.selectObject(target);
      });
    });
  }

  updateOutlinerSelection() {
    const selected = this.sceneManager.getSelectedObject();
    document.querySelectorAll('.outliner-item').forEach((item) => {
      item.classList.toggle('active', item.dataset.name === selected?.name);
    });
  }

  updateSceneStats() {
    const objects = this.sceneManager.getAllMeshes();
    let totalVerts = 0;
    let totalFaces = 0;

    objects.forEach((obj) => {
      if (obj.userData?.quadMesh) {
        totalVerts += obj.userData.quadMesh.vertices.length;
        totalFaces += obj.userData.quadMesh.quads.length;
      }
    });

    const topChip = document.getElementById('top-scene-stats');
    if (topChip) topChip.textContent = `Verts: ${totalVerts} • Faces: ${totalFaces}`;

    const botInfo = document.getElementById('status-selection-info');
    const selected = this.sceneManager.getSelectedObject();
    if (botInfo) {
      if (selected) {
        const vCount = selected.userData?.quadMesh?.vertices?.length || 0;
        const fCount = selected.userData?.quadMesh?.quads?.length || 0;
        botInfo.textContent = `${selected.name} | Verts: ${vCount} | Faces: ${fCount}`;
      } else {
        botInfo.textContent = `Nenhum selecionado | Total Verts: ${totalVerts}`;
      }
    }
  }

  // 8. Camera Compass & Gizmo Buttons
  initCameraGizmos() {
    document.getElementById('btn-cam-z')?.addEventListener('click', () => this.engine.setCameraView('top'));
    document.getElementById('btn-cam-y')?.addEventListener('click', () => this.engine.setCameraView('front'));
    document.getElementById('btn-cam-x')?.addEventListener('click', () => this.engine.setCameraView('right'));
    document.getElementById('btn-cam-view')?.addEventListener('click', () => this.engine.setCameraView('camera'));
    document.getElementById('btn-cam-persp')?.addEventListener('click', () => this.engine.toggleOrthographic());
    document.getElementById('btn-cam-focus')?.addEventListener('click', () => this.engine.focusOnObject(this.sceneManager.getSelectedObject()));
  }

  // 9. Floating Shift+A Context Menu
  initShiftAMenu() {
    const popup = document.getElementById('shift-a-popup');
    if (!popup) return;

    window.addEventListener('click', () => {
      popup.classList.add('hidden');
    });

    popup.addEventListener('click', (e) => {
      e.stopPropagation();
      const target = e.target.closest('[data-prim]');
      if (target) {
        const prim = target.dataset.prim;
        if (prim === 'camera') {
          this.sceneManager.createCameraObject();
        } else if (prim) {
          this.sceneManager.createPrimitive(prim);
        }
        popup.classList.add('hidden');
      }
    });

    document.getElementById('vp-menu-add')?.addEventListener('click', (e) => {
      e.stopPropagation();
      this.toggleShiftAMenu(e.clientX, e.clientY + 20);
    });
  }

  toggleShiftAMenu(x, y) {
    const popup = document.getElementById('shift-a-popup');
    if (!popup) return;

    popup.style.left = `${Math.min(window.innerWidth - 180, Math.max(10, x))}px`;
    popup.style.top = `${Math.min(window.innerHeight - 250, Math.max(10, y))}px`;
    popup.classList.toggle('hidden');
  }

  // 10. Workspace Tabs (Layout, Modeling, Sculpting, Shading, Rendering)
  initWorkspaceTabs() {
    const tabs = document.querySelectorAll('.ws-tab');
    tabs.forEach((tab) => {
      tab.addEventListener('click', () => {
        tabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');
        const ws = tab.dataset.ws;
        if (ws === 'modeling') {
          this.setMode('edit');
        } else if (ws === 'layout') {
          this.setMode('object');
        } else if (ws === 'shading') {
          this.setShading('material');
        } else if (ws === 'rendering') {
          this.setShading('rendered');
        }
      });
    });
  }

  // 11. Top Dropdown Menus (Arquivo, Editar, Render, Janela, Ajuda)
  initTopMenus() {
    const createMenu = (items, x, y) => {
      const existing = document.getElementById('floating-top-menu');
      if (existing) existing.remove();

      const menu = document.createElement('div');
      menu.id = 'floating-top-menu';
      menu.className = 'blender-popup-menu';
      menu.style.position = 'fixed';
      menu.style.left = `${x}px`;
      menu.style.top = `${y}px`;
      menu.style.zIndex = '99999';

      menu.innerHTML = items.map(item => `
        <div class="popup-menu-item" data-action="${item.action}">
          <span class="item-label">${item.icon ? `<i class="ti ti-${item.icon}"></i> ` : ''}${item.label}</span>
          ${item.shortcut ? `<span class="key-tag">${item.shortcut}</span>` : ''}
        </div>
      `).join('');

      document.body.appendChild(menu);

      const close = (e) => {
        if (!menu.contains(e.target)) {
          menu.remove();
          window.removeEventListener('pointerdown', close);
        }
      };
      setTimeout(() => window.addEventListener('pointerdown', close), 10);

      menu.querySelectorAll('.popup-menu-item').forEach(itemEl => {
        itemEl.addEventListener('click', () => {
          const action = itemEl.dataset.action;
          this.handleMenuAction(action);
          menu.remove();
        });
      });
    };

    // Top menu bar dropdowns
    document.querySelectorAll('.top-menu-item').forEach(item => {
      item.addEventListener('click', (e) => {
        e.stopPropagation();
        const rect = item.getBoundingClientRect();
        const text = item.textContent.trim().toLowerCase();

        if (text.includes('arquivo')) {
          createMenu([
            { label: 'Novo Projeto', icon: 'file', action: 'file_new', shortcut: 'Ctrl+N' },
            { label: 'Exportar OBJ (.obj)', icon: 'download', action: 'export_obj' },
            { label: 'Exportar STL (.stl)', icon: 'download', action: 'export_stl' },
          ], rect.left, rect.bottom + 4);
        } else if (text.includes('editar')) {
          createMenu([
            { label: 'Desfazer', icon: 'arrow-back-up', action: 'undo', shortcut: 'Ctrl+Z' },
            { label: 'Refazer', icon: 'arrow-forward-up', action: 'redo', shortcut: 'Ctrl+Shift+Z' },
            { label: 'Duplicar Objeto', icon: 'copy', action: 'duplicate', shortcut: 'Shift+D' },
            { label: 'Excluir Selecionado', icon: 'trash', action: 'delete', shortcut: 'X' },
          ], rect.left, rect.bottom + 4);
        } else if (text.includes('render')) {
          createMenu([
            { label: 'Renderizar Imagem (PBR)', icon: 'sparkles', action: 'render_pbr', shortcut: 'F12' },
          ], rect.left, rect.bottom + 4);
        } else if (text.includes('janela')) {
          createMenu([
            { label: 'Alternar Tela Cheia', icon: 'maximize', action: 'fullscreen', shortcut: 'F11' },
            { label: 'Catálogo de Ícones Tabler', icon: 'icons', action: 'icon_browser' },
          ], rect.left, rect.bottom + 4);
        } else if (text.includes('ajuda')) {
          createMenu([
            { label: 'Atalhos do Blender', icon: 'help', action: 'help_shortcuts' },
          ], rect.left, rect.bottom + 4);
        }
      });
    });

    // Viewport Header Menus (Visualizar, Selecionar)
    document.getElementById('vp-menu-view')?.addEventListener('click', (e) => {
      e.stopPropagation();
      const rect = e.currentTarget.getBoundingClientRect();
      createMenu([
        { label: 'Vista da Câmera', icon: 'camera', action: 'cam_view', shortcut: 'Numpad 0' },
        { label: 'Superior (Top)', icon: 'arrow-up', action: 'cam_top', shortcut: 'Numpad 7' },
        { label: 'Frontal (Front)', icon: 'arrow-right', action: 'cam_front', shortcut: 'Numpad 1' },
        { label: 'Direita (Right)', icon: 'arrow-narrow-right', action: 'cam_right', shortcut: 'Numpad 3' },
        { label: 'Perspectiva / Ortográfica', icon: 'grid-4x4', action: 'toggle_ortho', shortcut: 'Numpad 5' },
        { label: 'Enquadrar Selecionado', icon: 'focus-2', action: 'focus_selected', shortcut: 'Numpad .' },
      ], rect.left, rect.bottom + 4);
    });

    document.getElementById('vp-menu-select')?.addEventListener('click', (e) => {
      e.stopPropagation();
      const rect = e.currentTarget.getBoundingClientRect();
      createMenu([
        { label: 'Selecionar Tudo', icon: 'select-all', action: 'select_all', shortcut: 'A' },
        { label: 'Desmarcar Tudo', icon: 'square-x', action: 'deselect_all', shortcut: 'Alt+A' },
      ], rect.left, rect.bottom + 4);
    });
  }

  handleMenuAction(action) {
    switch (action) {
      case 'file_new':
        location.reload();
        break;
      case 'export_obj':
        this.sceneManager.exportOBJ();
        break;
      case 'export_stl':
        this.sceneManager.exportSTL();
        break;
      case 'undo':
        this.historyManager.undo();
        break;
      case 'redo':
        this.historyManager.redo();
        break;
      case 'duplicate':
        this.sceneManager.duplicateSelected();
        break;
      case 'delete':
        if (this.currentMode === 'edit') this.meshEditor.deleteSelection();
        else {
          const obj = this.sceneManager.getSelectedObject();
          if (obj) this.sceneManager.removeObject(obj);
        }
        break;
      case 'render_pbr':
        document.getElementById('btn-open-pbr-render')?.click();
        break;
      case 'fullscreen':
        if (!document.fullscreenElement) document.documentElement.requestFullscreen();
        else document.exitFullscreen();
        break;
      case 'icon_browser':
        document.getElementById('btn-toggle-icon-browser')?.click();
        break;
      case 'help_shortcuts':
        alert("Atalhos do Blender:\n\n• Tab: Alternar Modo Objeto / Edição\n• 1, 2, 3: Vértice, Aresta, Face\n• E: Extrusão\n• I: Inset\n• Ctrl+B: Bevel (Chanfro)\n• Ctrl+R: Loop Cut & Slide\n• F: Criar Face\n• M: Mesclar Vértices\n• Alt+S: Encolher/Engordar\n• G, R, S: Mover, Rotacionar, Escalar\n• X / Del: Excluir\n• Shift+A: Adicionar Primitiva\n• Numpad 0: Vista da Câmera\n• Numpad .: Enquadrar Seleção");
        break;
      case 'cam_view':
        this.engine.setCameraView('camera');
        break;
      case 'cam_top':
        this.engine.setCameraView('top');
        break;
      case 'cam_front':
        this.engine.setCameraView('front');
        break;
      case 'cam_right':
        this.engine.setCameraView('right');
        break;
      case 'toggle_ortho':
        this.engine.toggleOrthographic();
        break;
      case 'focus_selected':
        this.engine.focusOnObject(this.sceneManager.getSelectedObject());
        break;
      case 'select_all':
        if (this.currentMode === 'edit') {
          const qm = this.meshEditor.activeMesh?.userData?.quadMesh;
          if (qm) {
            qm.vertices.forEach((_, i) => this.meshEditor.selectedVertices.add(i));
            this.meshEditor.rebuildEditHelpers();
            this.meshEditor.updateTransformAnchor();
          }
        }
        break;
      case 'deselect_all':
        if (this.currentMode === 'edit') {
          this.meshEditor.selectedVertices.clear();
          this.meshEditor.selectedEdges.clear();
          this.meshEditor.selectedFaces.clear();
          this.meshEditor.rebuildEditHelpers();
          this.meshEditor.updateTransformAnchor();
        } else {
          this.sceneManager.deselectAll();
        }
        break;
    }
  }

  // 12. Raycasting & Live Box Selection (Marquee Select)
  initRaycasting() {
    const canvas = this.engine.canvas;
    const marquee = document.getElementById('selection-marquee-box');
    const container = document.getElementById('viewport-container');

    let isPointerDown = false;
    let isBoxSelecting = false;
    let startX = 0;
    let startY = 0;

    canvas.addEventListener('pointerdown', (e) => {
      if (e.button !== 0) return; // Left mouse only
      if (this.transformManager.isTransforming) return;

      this.lastScreenMouse = { x: e.clientX, y: e.clientY };
      isPointerDown = true;
      startX = e.clientX;
      startY = e.clientY;
    });

    window.addEventListener('pointermove', (e) => {
      if (!isPointerDown || this.transformManager.isTransforming) return;

      const dx = Math.abs(e.clientX - startX);
      const dy = Math.abs(e.clientY - startY);

      if (dx > 4 || dy > 4) {
        isBoxSelecting = true;
        this.engine.controls.enabled = false;

        if (marquee && container) {
          const rect = container.getBoundingClientRect();
          const left = Math.min(startX, e.clientX) - rect.left;
          const top = Math.min(startY, e.clientY) - rect.top;
          const width = Math.abs(e.clientX - startX);
          const height = Math.abs(e.clientY - startY);

          marquee.style.display = 'block';
          marquee.style.left = `${left}px`;
          marquee.style.top = `${top}px`;
          marquee.style.width = `${width}px`;
          marquee.style.height = `${height}px`;
        }
      }
    });

    window.addEventListener('pointerup', (e) => {
      if (!isPointerDown) return;
      isPointerDown = false;
      this.engine.controls.enabled = true;

      if (isBoxSelecting) {
        isBoxSelecting = false;
        if (marquee) marquee.style.display = 'none';

        const minX = Math.min(startX, e.clientX);
        const maxX = Math.max(startX, e.clientX);
        const minY = Math.min(startY, e.clientY);
        const maxY = Math.max(startY, e.clientY);
        const isSubtract = e.ctrlKey || e.altKey;
        const isAdd = e.shiftKey;

        if (this.currentMode === 'edit') {
          const activeMesh = this.meshEditor.activeMesh;
          const qm = activeMesh?.userData?.quadMesh;

          if (activeMesh && qm) {
            activeMesh.updateMatrixWorld(true);
            if (!isAdd && !isSubtract) {
              this.meshEditor.selectedVertices.clear();
              this.meshEditor.selectedEdges.clear();
              this.meshEditor.selectedFaces.clear();
            }

            const cam = this.engine.activeCamera;
            const matWorld = activeMesh.matrixWorld;

            // Box select vertices
            for (let i = 0; i < qm.vertices.length; i++) {
              const vWorld = qm.vertices[i].clone().applyMatrix4(matWorld);
              const vNDC = vWorld.project(cam);

              if (vNDC.z < 1.0) {
                const screenX = (vNDC.x * 0.5 + 0.5) * window.innerWidth;
                const screenY = (-vNDC.y * 0.5 + 0.5) * window.innerHeight;

                if (screenX >= minX && screenX <= maxX && screenY >= minY && screenY <= maxY) {
                  if (isSubtract) this.meshEditor.selectedVertices.delete(i);
                  else this.meshEditor.selectedVertices.add(i);
                }
              }
            }

            // Sync edges & faces
            if (this.currentSubmode === 'edge') {
              qm.edges.forEach((edge, eIdx) => {
                if (this.meshEditor.selectedVertices.has(edge[0]) && this.meshEditor.selectedVertices.has(edge[1])) {
                  this.meshEditor.selectedEdges.add(eIdx);
                }
              });
            } else if (this.currentSubmode === 'face') {
              qm.quads.forEach((quad, qIdx) => {
                const unique = Array.from(new Set(quad));
                if (unique.every(v => this.meshEditor.selectedVertices.has(v))) {
                  this.meshEditor.selectedFaces.add(qIdx);
                }
              });
            }

            this.meshEditor.rebuildEditHelpers();
            this.meshEditor.updateTransformAnchor();
          }
        } else {
          // Object Mode Box Select
          const objects = this.sceneManager.getAllMeshes().filter(m => m.visible);
          const cam = this.engine.activeCamera;

          if (!isAdd && !isSubtract) {
            this.sceneManager.deselectAll();
          }

          objects.forEach((obj) => {
            const pos = new THREE.Vector3();
            obj.getWorldPosition(pos);
            const vNDC = pos.project(cam);

            if (vNDC.z < 1.0) {
              const screenX = (vNDC.x * 0.5 + 0.5) * window.innerWidth;
              const screenY = (-vNDC.y * 0.5 + 0.5) * window.innerHeight;

              if (screenX >= minX && screenX <= maxX && screenY >= minY && screenY <= maxY) {
                if (isSubtract) this.sceneManager.deselectObject(obj);
                else this.sceneManager.selectObject(obj, true);
              }
            }
          });
        }
      }
    });

    // Single Click Raycast Selection Fallback
    canvas.addEventListener('click', (e) => {
      if (this.transformManager.isTransforming || isBoxSelecting) return;

      const rect = canvas.getBoundingClientRect();
      this.mouse.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      this.mouse.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;
      this.raycaster.setFromCamera(this.mouse, this.engine.activeCamera);

      if (this.currentMode === 'edit') {
        this.meshEditor.handleSelection(e.clientX, e.clientY, e.shiftKey, this.raycaster);
      } else {
        const objects = this.sceneManager.getAllMeshes().filter(m => m.visible);
        const intersects = this.raycaster.intersectObjects(objects, true);

        if (intersects.length > 0) {
          let hitObj = intersects[0].object;
          while (hitObj && hitObj.parent && hitObj.parent !== this.engine.scene && !objects.includes(hitObj)) {
            hitObj = hitObj.parent;
          }
          this.sceneManager.selectObject(hitObj, e.shiftKey);
        } else {
          this.sceneManager.deselectAll();
        }
      }
    });
  }

  // 11. Authentic Blender Shortcuts
  initShortcuts() {
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT') return;

      // Tab: Mode Switch
      if (e.key === 'Tab') {
        e.preventDefault();
        this.toggleMode();
      }
      // Shift + A: Add Menu
      else if (e.shiftKey && (e.key === 'a' || e.key === 'A')) {
        e.preventDefault();
        this.toggleShiftAMenu(this.lastScreenMouse.x, this.lastScreenMouse.y);
      }
      // N: Toggle N-Panel
      else if (!e.ctrlKey && !e.altKey && (e.key === 'n' || e.key === 'N')) {
        e.preventDefault();
        this.toggleNPanel();
      }
      // G: Move
      else if (!e.ctrlKey && !e.altKey && (e.key === 'g' || e.key === 'G')) {
        e.preventDefault();
        this.setTool('move');
      }
      // R: Rotate
      else if (!e.ctrlKey && !e.altKey && (e.key === 'r' || e.key === 'R')) {
        e.preventDefault();
        this.setTool('rotate');
      }
      // S: Scale
      else if (!e.ctrlKey && !e.altKey && (e.key === 's' || e.key === 'S')) {
        e.preventDefault();
        this.setTool('scale');
      }
      // Z / Shift+Z: Shading Switch
      else if (!e.ctrlKey && !e.altKey && (e.key === 'z' || e.key === 'Z')) {
        e.preventDefault();
        if (e.shiftKey) {
          const next = this.engine.currentShading === 'wireframe' ? 'solid' : 'wireframe';
          this.setShading(next);
        } else {
          const next = this.engine.currentShading === 'solid' ? 'material' : 'solid';
          this.setShading(next);
        }
      }
      // 0 / Numpad 0: Camera View
      else if (e.code === 'Numpad0' || (e.key === '0' && !e.ctrlKey && !e.altKey)) {
        e.preventDefault();
        this.engine.setCameraView('camera');
      }
      // 1: Front / Vertex
      else if (e.key === '1') {
        if (this.currentMode === 'edit') this.setSubmode('vertex');
        else this.engine.setCameraView('front');
      }
      // 2: Edge
      else if (e.key === '2' && this.currentMode === 'edit') {
        this.setSubmode('edge');
      }
      // 3: Right / Face
      else if (e.key === '3') {
        if (this.currentMode === 'edit') this.setSubmode('face');
        else this.engine.setCameraView('right');
      }
      // 7: Top View
      else if (e.key === '7') {
        this.engine.setCameraView('top');
      }
      // 5: Ortho / Persp Toggle
      else if (e.key === '5') {
        this.engine.toggleOrthographic();
      }
      // Numpad . / Del / Home: Focus View on Selected Object (Blender Frame Selected)
      else if (
        e.code === 'NumpadDecimal' || 
        e.code === 'NumpadDelete' || 
        e.key === 'Decimal' || 
        e.code === 'Home' ||
        (e.key === '.' && !e.ctrlKey && !e.altKey)
      ) {
        e.preventDefault();
        const selObj = this.sceneManager.getSelectedObject();
        this.engine.focusOnObject(selObj);
      }
      // Undo: Ctrl+Z
      else if (e.ctrlKey && !e.shiftKey && (e.key === 'z' || e.key === 'Z')) {
        e.preventDefault();
        this.historyManager.undo();
      }
      // Redo: Ctrl+Shift+Z / Ctrl+Y
      else if ((e.ctrlKey && e.shiftKey && (e.key === 'z' || e.key === 'Z')) || (e.ctrlKey && (e.key === 'y' || e.key === 'Y'))) {
        e.preventDefault();
        this.historyManager.redo();
      }
      // Fill Face: F
      else if (!e.ctrlKey && !e.altKey && (e.key === 'f' || e.key === 'F')) {
        if (this.currentMode === 'edit') {
          e.preventDefault();
          this.meshEditor.fillFace();
        }
      }
      // Merge: M
      else if (!e.ctrlKey && !e.altKey && (e.key === 'm' || e.key === 'M')) {
        if (this.currentMode === 'edit') {
          e.preventDefault();
          this.meshEditor.mergeVertices('center');
        }
      }
      // Alt+S: Shrink / Fatten
      else if (e.altKey && !e.ctrlKey && (e.key === 's' || e.key === 'S')) {
        if (this.currentMode === 'edit') {
          e.preventDefault();
          this.meshEditor.shrinkFlatten(0.2);
        }
      }
      // Delete: Main Delete key or X
      else if (e.code === 'Delete' || e.key === 'x' || e.key === 'X') {
        if (this.currentMode === 'edit') {
          this.meshEditor.deleteSelection();
        } else {
          const obj = this.sceneManager.getSelectedObject();
          if (obj) this.sceneManager.removeObject(obj);
        }
      }
    });
  }
}
