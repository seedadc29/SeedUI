import * as THREE from 'three';
import { SelectionTools } from '../tools/SelectionTools.js';

export class UIManager {
  constructor(engine, sceneManager, transformManager, meshEditor, historyManager, paintManager, exportManager) {
    this.engine = engine;
    this.sceneManager = sceneManager;
    this.transformManager = transformManager;
    this.meshEditor = meshEditor;
    this.historyManager = historyManager;
    this.paintManager = paintManager;
    this.exportManager = exportManager;
    
    this.currentMode = 'model'; // 'model' | 'paint' | 'zoo' | 'view'
    this.currentSubmode = 'object'; // 'object' | 'vertex' | 'edge' | 'face'
    this.currentTool = 'select';

    this.raycaster = new THREE.Raycaster();
    this.mouse = new THREE.Vector2();
    this.lastScreenMouse = { x: window.innerWidth / 2, y: window.innerHeight / 2 };

    // Track mouse on window for Shift+A exact cursor popup
    window.addEventListener('mousemove', (e) => {
      this.lastScreenMouse.x = e.clientX;
      this.lastScreenMouse.y = e.clientY;
    });

    // Link History Manager to Scene & Transform Managers
    this.sceneManager.setHistoryManager(this.historyManager);
    this.transformManager.setHistoryManager(this.historyManager);

    // Selection Tools (Box, Lasso, Circle, Brush)
    this.selectionTools = new SelectionTools(this.engine, this.sceneManager, this.meshEditor, this);

    // Callback when a primitive is created to open Operator Adjust Panel
    this.sceneManager.onPrimitiveCreated = (mesh, type, params) => {
      this.openOperatorPanel(mesh, type, params);
    };

    // Link selection callback
    this.sceneManager.onSelectionChange = (obj) => {
      if (this.currentSubmode !== 'object') {
        this.meshEditor.setSubmode(this.currentSubmode);
      } else if (obj && obj.visible && this.currentMode !== 'view' && this.currentMode !== 'paint') {
        this.transformManager.attach(obj);
      } else {
        this.transformManager.detach();
      }
    };

    // Attach to initial object
    if (this.sceneManager.getSelectedObject()) {
      this.transformManager.attach(this.sceneManager.getSelectedObject());
    }

    // Link camera change (persp/ortho)
    this.engine.onCameraChange = (camera) => {
      this.transformManager.updateCamera(camera);
    };

    this.initFileMenu();
    this.initModeTabs();
    this.initToolbar();
    this.initSubmodes();
    this.initSelectionToolsUI();
    this.initShadingControls();
    this.initCameraGizmos();
    this.initInspector();
    this.initPaintUI();
    this.initShiftAContextMenu();
    this.initOperatorPanelEvents();
    this.initRaycasting();
    this.initShortcuts();
  }

  initFileMenu() {
    const fileBtn = document.getElementById('btn-menu-file');
    const dropdown = document.getElementById('file-dropdown');
    if (!fileBtn || !dropdown) return;

    fileBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      dropdown.classList.toggle('hidden');
    });

    window.addEventListener('click', (e) => {
      if (!dropdown.contains(e.target) && e.target !== fileBtn) {
        dropdown.classList.add('hidden');
      }
    });

    document.getElementById('btn-export-obj')?.addEventListener('click', () => {
      dropdown.classList.add('hidden');
      if (this.exportManager) this.exportManager.exportOBJ();
    });

    document.getElementById('btn-export-gltf')?.addEventListener('click', () => {
      dropdown.classList.add('hidden');
      if (this.exportManager) this.exportManager.exportGLTF(null, true);
    });

    document.getElementById('btn-export-stl')?.addEventListener('click', () => {
      dropdown.classList.add('hidden');
      if (this.exportManager) this.exportManager.exportSTL();
    });

    document.getElementById('btn-save-project')?.addEventListener('click', () => {
      dropdown.classList.add('hidden');
      if (this.exportManager) this.exportManager.saveProjectJSON();
    });
  }

  initModeTabs() {
    const tabs = document.querySelectorAll('.mode-tab');
    tabs.forEach((tab) => {
      tab.addEventListener('click', () => {
        tabs.forEach((t) => t.classList.remove('active'));
        tab.classList.add('active');
        this.setMode(tab.dataset.mode);
      });
    });
  }

  setMode(mode) {
    this.currentMode = mode;
    this.engine.currentMode = mode;
    const leftToolbar = document.getElementById('left-toolbar');
    const paintCategory = document.getElementById('paint-category-panel');

    if (mode === 'model') {
      if (leftToolbar) leftToolbar.style.display = 'flex';
      if (paintCategory) paintCategory.classList.add('hidden');
      if (this.currentSubmode === 'object' && this.sceneManager.getSelectedObject()) {
        this.transformManager.attach(this.sceneManager.getSelectedObject());
      }
    } else if (mode === 'paint') {
      if (leftToolbar) leftToolbar.style.display = 'flex';
      if (paintCategory) paintCategory.classList.remove('hidden');
      this.transformManager.detach();
      this.meshEditor.clearHelpers();
    } else if (mode === 'view') {
      if (leftToolbar) leftToolbar.style.display = 'none';
      if (paintCategory) paintCategory.classList.add('hidden');
      this.transformManager.detach();
      this.meshEditor.clearHelpers();
    } else {
      if (leftToolbar) leftToolbar.style.display = 'flex';
      if (paintCategory) paintCategory.classList.add('hidden');
    }
  }

  initPaintUI() {
    if (!this.paintManager) return;

    const brushColor = document.getElementById('paint-brush-color');
    if (brushColor) {
      brushColor.addEventListener('input', (e) => this.paintManager.setBrushColor(e.target.value));
    }

    const brushSize = document.getElementById('paint-brush-size');
    const brushSizeVal = document.getElementById('paint-brush-size-val');
    if (brushSize) {
      brushSize.addEventListener('input', (e) => {
        this.paintManager.setBrushSize(parseInt(e.target.value));
        if (brushSizeVal) brushSizeVal.textContent = `${e.target.value}px`;
      });
    }

    const brushOpacity = document.getElementById('paint-brush-opacity');
    const brushOpacityVal = document.getElementById('paint-brush-opacity-val');
    if (brushOpacity) {
      brushOpacity.addEventListener('input', (e) => {
        this.paintManager.setBrushOpacity(parseFloat(e.target.value));
        if (brushOpacityVal) brushOpacityVal.textContent = `${Math.round(parseFloat(e.target.value) * 100)}%`;
      });
    }

    const pixelMode = document.getElementById('paint-pixel-mode');
    if (pixelMode) {
      pixelMode.addEventListener('change', (e) => this.paintManager.setPixelMode(e.target.checked));
    }

    const drawBtn = document.getElementById('btn-paint-draw');
    const eraseBtn = document.getElementById('btn-paint-erase');
    if (drawBtn && eraseBtn) {
      drawBtn.addEventListener('click', () => {
        drawBtn.style.background = 'var(--accent)';
        eraseBtn.style.background = 'var(--bg-panel)';
        this.paintManager.setBrushMode('draw');
      });
      eraseBtn.addEventListener('click', () => {
        eraseBtn.style.background = 'var(--accent)';
        drawBtn.style.background = 'var(--bg-panel)';
        this.paintManager.setBrushMode('erase');
      });
    }

    const clearBtn = document.getElementById('btn-paint-clear');
    if (clearBtn) {
      clearBtn.addEventListener('click', () => {
        this.paintManager.clearTexture();
      });
    }
  }

  initToolbar() {
    const toolBtns = document.querySelectorAll('.tool-btn');
    toolBtns.forEach((btn) => {
      btn.addEventListener('click', () => {
        toolBtns.forEach((b) => b.classList.remove('active'));
        btn.classList.add('active');
        this.setTool(btn.dataset.tool);
      });
    });
  }

  setTool(tool) {
    this.currentTool = tool;
    if (tool === 'select') {
      this.transformManager.setMode('translate');
    } else if (tool === 'move') {
      this.transformManager.setMode('translate');
    } else if (tool === 'rotate') {
      this.transformManager.setMode('rotate');
    } else if (tool === 'scale') {
      this.transformManager.setMode('scale');
    } else if (tool === 'extrude') {
      if (this.currentSubmode === 'object') this.setSubmode('face');
      this.meshEditor.extrude();
    } else if (tool === 'inset') {
      if (this.currentSubmode === 'object') this.setSubmode('face');
      this.meshEditor.inset();
    } else if (tool === 'bevel') {
      this.meshEditor.subdivide();
    }
  }

  initSubmodes() {
    const modeToggleBtn = document.getElementById('btn-mode-toggle');
    if (modeToggleBtn) {
      modeToggleBtn.addEventListener('click', () => {
        this.setSubmode(this.currentSubmode === 'object' ? 'vertex' : 'object');
      });
    }

    const meshSubBtns = document.querySelectorAll('.mesh-sub-btn');
    meshSubBtns.forEach((btn) => {
      btn.addEventListener('click', () => {
        this.setSubmode(btn.dataset.submode);
      });
    });

    const vhAddBtn = document.getElementById('btn-vh-add');
    if (vhAddBtn) {
      vhAddBtn.addEventListener('click', (e) => {
        const rect = vhAddBtn.getBoundingClientRect();
        this.toggleShiftAMenu(rect.left, rect.bottom + 4);
      });
    }
  }

  initSelectionToolsUI() {
    const selBtns = document.querySelectorAll('.select-type-btn');
    selBtns.forEach((btn) => {
      btn.addEventListener('click', () => {
        selBtns.forEach((b) => b.classList.remove('active'));
        btn.classList.add('active');
        this.selectionTools.setTool(btn.dataset.selTool);
      });
    });
  }

  setSubmode(submode) {
    this.currentSubmode = submode;
    this.meshEditor.setSubmode(submode);

    const modeToggleBtn = document.getElementById('btn-mode-toggle');
    if (modeToggleBtn) {
      if (submode === 'object') {
        modeToggleBtn.innerHTML = '<span>Modo Objeto</span> <span class="dropdown-arrow">▾</span>';
      } else {
        modeToggleBtn.innerHTML = '<span>Modo Edição</span> <span class="dropdown-arrow">▾</span>';
      }
    }

    const meshSubBtns = document.querySelectorAll('.mesh-sub-btn');
    meshSubBtns.forEach((btn) => {
      if (btn.dataset.submode === submode) {
        btn.classList.add('active');
      } else {
        btn.classList.remove('active');
      }
    });

    const hintEl = document.getElementById('status-hint');
    if (hintEl) {
      if (submode === 'object') hintEl.innerHTML = '<span class="key-hint"><kbd>Shift+A</kbd> Adicionar</span> <span class="key-hint"><kbd>Tab</kbd> Modo Edição</span> <span class="key-hint"><kbd>G</kbd> Mover</span> <span class="key-hint"><kbd>R</kbd> Rotacionar</span> <span class="key-hint"><kbd>S</kbd> Escalar</span> <span class="key-hint"><kbd>Ctrl+Z</kbd> Desfazer</span>';
      else if (submode === 'vertex') hintEl.innerHTML = '<span class="key-hint"><kbd>1</kbd> Vértices</span> <span class="key-hint"><kbd>G</kbd> Puxar Vértice</span> <span class="key-hint"><kbd>Tab</kbd> Modo Objeto</span> <span class="key-hint"><kbd>Ctrl+Z</kbd> Desfazer</span>';
      else if (submode === 'edge') hintEl.innerHTML = '<span class="key-hint"><kbd>2</kbd> Arestas</span> <span class="key-hint"><kbd>G</kbd> Mover Aresta</span> <span class="key-hint"><kbd>Tab</kbd> Modo Objeto</span>';
      else if (submode === 'face') hintEl.innerHTML = '<span class="key-hint"><kbd>3</kbd> Faces</span> <span class="key-hint"><kbd>G</kbd> Mover Face</span> <span class="key-hint"><kbd>Tab</kbd> Modo Objeto</span>';
    }
  }

  initShadingControls() {
    const shadingBtns = document.querySelectorAll('.shading-icon-btn');
    shadingBtns.forEach((btn) => {
      btn.addEventListener('click', () => {
        shadingBtns.forEach((b) => b.classList.remove('active'));
        btn.classList.add('active');
        this.setShading(btn.dataset.shading);
      });
    });

    const chkGrid = document.getElementById('chk-grid');
    if (chkGrid) chkGrid.addEventListener('change', (e) => this.engine.toggleGrid(e.target.checked));

    const chkShadows = document.getElementById('chk-shadows');
    if (chkShadows) chkShadows.addEventListener('change', (e) => this.engine.toggleShadows(e.target.checked));

    const chkEdges = document.getElementById('chk-edges');
    if (chkEdges) chkEdges.addEventListener('change', (e) => this.sceneManager.toggleEdges(e.target.checked));

    const bgColor = document.getElementById('prop-bg-color');
    if (bgColor) bgColor.addEventListener('input', (e) => this.engine.setBackgroundColor(e.target.value));
  }

  setShading(shading) {
    const meshes = this.sceneManager.getAllMeshes();
    meshes.forEach((mesh) => {
      if (shading === 'wireframe') {
        mesh.material.wireframe = true;
      } else {
        mesh.material.wireframe = false;
      }
    });
  }

  initCameraGizmos() {
    document.getElementById('btn-view-top')?.addEventListener('click', () => this.engine.setView('top'));
    document.getElementById('btn-view-front')?.addEventListener('click', () => this.engine.setView('front'));
    document.getElementById('btn-view-side')?.addEventListener('click', () => this.engine.setView('side'));
    document.getElementById('btn-view-ortho')?.addEventListener('click', () => this.engine.toggleProjection());
    document.getElementById('btn-view-focus')?.addEventListener('click', () => {
      const selected = this.sceneManager.getSelectedObject();
      if (selected) this.engine.focusObject(selected);
    });
  }

  initInspector() {
    const inputs = [
      'prop-pos-x', 'prop-pos-y', 'prop-pos-z',
      'prop-rot-x', 'prop-rot-y', 'prop-rot-z',
      'prop-scl-x', 'prop-scl-y', 'prop-scl-z'
    ];

    inputs.forEach((id) => {
      const el = document.getElementById(id);
      if (el) {
        el.addEventListener('change', () => this.sceneManager.applyTransformFromUI());
      }
    });

    document.getElementById('btn-add-prim')?.addEventListener('click', (e) => {
      const rect = document.getElementById('btn-add-prim').getBoundingClientRect();
      this.toggleShiftAMenu(rect.left - 180, rect.bottom + 4);
    });

    document.getElementById('btn-close-sidebar')?.addEventListener('click', () => {
      this.toggleSidebar(false);
    });

    document.getElementById('btn-sidebar-reopen')?.addEventListener('click', () => {
      this.toggleSidebar(true);
    });
  }

  toggleSidebar(forceState = null) {
    const sidebar = document.getElementById('right-sidebar');
    const reopenBtn = document.getElementById('btn-sidebar-reopen');
    if (!sidebar) return;

    if (forceState !== null) {
      if (forceState) sidebar.classList.remove('collapsed');
      else sidebar.classList.add('collapsed');
    } else {
      sidebar.classList.toggle('collapsed');
    }

    const isCollapsed = sidebar.classList.contains('collapsed');

    if (reopenBtn) {
      if (isCollapsed) reopenBtn.classList.remove('hidden');
      else reopenBtn.classList.add('hidden');
    }

    window.dispatchEvent(new Event('resize'));
    setTimeout(() => {
      window.dispatchEvent(new Event('resize'));
    }, 100);
  }

  // --- FLOATING RETRACTABLE SHIFT+A MENU ---

  initShiftAContextMenu() {
    const menu = document.getElementById('shift-a-popup');
    if (!menu) return;

    // Action clicks on menu items
    const actionBtns = menu.querySelectorAll('.context-menu-action');
    actionBtns.forEach((btn) => {
      btn.addEventListener('click', (e) => {
        e.stopPropagation();
        const prim = btn.dataset.prim;
        this.sceneManager.createPrimitive(prim);
        this.closeShiftAMenu();
      });
    });

    // Close / retract when clicking anywhere outside
    window.addEventListener('click', (e) => {
      if (!menu.contains(e.target) && e.target.id !== 'btn-add-prim' && e.target.id !== 'btn-vh-add') {
        this.closeShiftAMenu();
      }
    });
  }

  openShiftAMenu(x = this.lastScreenMouse.x, y = this.lastScreenMouse.y) {
    const menu = document.getElementById('shift-a-popup');
    if (!menu) return;

    // Ensure menu stays within window boundaries
    const menuWidth = 190;
    const menuHeight = 220;
    const posX = Math.max(10, Math.min(x, window.innerWidth - menuWidth - 190));
    const posY = Math.max(35, Math.min(y, window.innerHeight - menuHeight));

    menu.style.left = `${posX}px`;
    menu.style.top = `${posY}px`;
    menu.classList.remove('hidden');
  }

  closeShiftAMenu() {
    const menu = document.getElementById('shift-a-popup');
    if (menu) menu.classList.add('hidden');
  }

  toggleShiftAMenu(x = this.lastScreenMouse.x, y = this.lastScreenMouse.y) {
    const menu = document.getElementById('shift-a-popup');
    if (!menu) return;
    if (menu.classList.contains('hidden')) {
      this.openShiftAMenu(x, y);
    } else {
      this.closeShiftAMenu();
    }
  }

  // --- OPERATOR PARAMETERS PANEL ---

  initOperatorPanelEvents() {
    const panel = document.getElementById('operator-panel');
    const header = document.getElementById('operator-header');
    const toggleBtn = document.getElementById('operator-toggle-btn');

    if (header && toggleBtn && panel) {
      header.addEventListener('click', () => {
        panel.classList.toggle('collapsed');
        toggleBtn.textContent = panel.classList.contains('collapsed') ? '▲' : '▼';
      });
    }
  }

  openOperatorPanel(mesh, type, params) {
    const panel = document.getElementById('operator-panel');
    const titleEl = document.getElementById('operator-title');
    const bodyEl = document.getElementById('operator-body');
    if (!panel || !titleEl || !bodyEl) return;

    panel.classList.remove('hidden', 'collapsed');
    const toggleBtn = document.getElementById('operator-toggle-btn');
    if (toggleBtn) toggleBtn.textContent = '▼';

    const titles = {
      cube: '🧊 Adicionar Cubo',
      uvsphere: '🌐 Adicionar Esfera UV',
      cylinder: '🥫 Adicionar Cilindro',
      cone: '🍦 Adicionar Cone',
      plane: '📄 Adicionar Plano',
      torus: '🍩 Adicionar Torus',
      icosphere: '⚽ Adicionar Esfera Triangulada'
    };
    titleEl.textContent = titles[type] || '⚙️ Ajustar Primitiva';

    let html = '';

    if (type === 'cylinder') {
      html = `
        <div class="operator-row">
          <label>Vértices / Lados</label>
          <input type="number" class="operator-input" id="op-segments" min="3" max="64" value="${params.segments || 12}">
        </div>
        <div class="operator-row">
          <label>Raio</label>
          <input type="number" class="operator-input" id="op-radius" step="0.1" min="0.1" value="${params.radius || 1}">
        </div>
        <div class="operator-row">
          <label>Profundidade</label>
          <input type="number" class="operator-input" id="op-height" step="0.1" min="0.1" value="${params.height || 2}">
        </div>
        <div class="operator-row">
          <label>Tampa Superior</label>
          <input type="checkbox" class="operator-checkbox" id="op-captop" ${params.capTop !== false ? 'checked' : ''}>
        </div>
        <div class="operator-row">
          <label>Tampa Inferior</label>
          <input type="checkbox" class="operator-checkbox" id="op-capbottom" ${params.capBottom !== false ? 'checked' : ''}>
        </div>
      `;
    } else if (type === 'uvsphere') {
      html = `
        <div class="operator-row">
          <label>Segmentos</label>
          <input type="number" class="operator-input" id="op-segments" min="4" max="64" value="${params.segments || 16}">
        </div>
        <div class="operator-row">
          <label>Anéis</label>
          <input type="number" class="operator-input" id="op-rings" min="3" max="32" value="${params.rings || 10}">
        </div>
        <div class="operator-row">
          <label>Raio</label>
          <input type="number" class="operator-input" id="op-radius" step="0.1" min="0.1" value="${params.radius || 1}">
        </div>
      `;
    } else if (type === 'icosphere') {
      html = `
        <div class="operator-row">
          <label>Subdivisões</label>
          <input type="number" class="operator-input" id="op-subdivisions" min="0" max="3" value="${params.subdivisions || 1}">
        </div>
        <div class="operator-row">
          <label>Raio</label>
          <input type="number" class="operator-input" id="op-radius" step="0.1" min="0.1" value="${params.radius || 1}">
        </div>
      `;
    } else if (type === 'cone') {
      html = `
        <div class="operator-row">
          <label>Vértices</label>
          <input type="number" class="operator-input" id="op-segments" min="3" max="64" value="${params.segments || 12}">
        </div>
        <div class="operator-row">
          <label>Raio</label>
          <input type="number" class="operator-input" id="op-radius" step="0.1" min="0.1" value="${params.radius || 1}">
        </div>
        <div class="operator-row">
          <label>Profundidade</label>
          <input type="number" class="operator-input" id="op-height" step="0.1" min="0.1" value="${params.height || 2}">
        </div>
      `;
    } else if (type === 'torus') {
      html = `
        <div class="operator-row">
          <label>Segmentos Tubo</label>
          <input type="number" class="operator-input" id="op-tubular" min="4" max="48" value="${params.tubularSegments || 16}">
        </div>
        <div class="operator-row">
          <label>Segmentos Raio</label>
          <input type="number" class="operator-input" id="op-radial" min="3" max="24" value="${params.radialSegments || 8}">
        </div>
        <div class="operator-row">
          <label>Raio Maior</label>
          <input type="number" class="operator-input" id="op-radius" step="0.1" min="0.1" value="${params.radius || 1.2}">
        </div>
        <div class="operator-row">
          <label>Raio do Tubo</label>
          <input type="number" class="operator-input" id="op-tube" step="0.05" min="0.05" value="${params.tube || 0.4}">
        </div>
      `;
    } else if (type === 'cube') {
      html = `
        <div class="operator-row">
          <label>Tamanho</label>
          <input type="number" class="operator-input" id="op-size" step="0.1" min="0.1" value="${params.size || 2}">
        </div>
      `;
    } else if (type === 'plane') {
      html = `
        <div class="operator-row">
          <label>Largura</label>
          <input type="number" class="operator-input" id="op-width" step="0.1" min="0.1" value="${params.width || 3}">
        </div>
        <div class="operator-row">
          <label>Comprimento</label>
          <input type="number" class="operator-input" id="op-height" step="0.1" min="0.1" value="${params.height || 3}">
        </div>
      `;
    }

    bodyEl.innerHTML = html;

    // Attach dynamic input listeners to recalculate mesh on the fly
    const inputs = bodyEl.querySelectorAll('input');
    inputs.forEach((input) => {
      input.addEventListener('input', () => {
        const newParams = {};
        if (document.getElementById('op-segments')) newParams.segments = parseInt(document.getElementById('op-segments').value);
        if (document.getElementById('op-rings')) newParams.rings = parseInt(document.getElementById('op-rings').value);
        if (document.getElementById('op-subdivisions')) newParams.subdivisions = parseInt(document.getElementById('op-subdivisions').value);
        if (document.getElementById('op-radius')) newParams.radius = parseFloat(document.getElementById('op-radius').value);
        if (document.getElementById('op-height')) newParams.height = parseFloat(document.getElementById('op-height').value);
        if (document.getElementById('op-size')) newParams.size = parseFloat(document.getElementById('op-size').value);
        if (document.getElementById('op-width')) newParams.width = parseFloat(document.getElementById('op-width').value);
        if (document.getElementById('op-tube')) newParams.tube = parseFloat(document.getElementById('op-tube').value);
        if (document.getElementById('op-radial')) newParams.radialSegments = parseInt(document.getElementById('op-radial').value);
        if (document.getElementById('op-tubular')) newParams.tubularSegments = parseInt(document.getElementById('op-tubular').value);
        if (document.getElementById('op-captop')) newParams.capTop = document.getElementById('op-captop').checked;
        if (document.getElementById('op-capbottom')) newParams.capBottom = document.getElementById('op-capbottom').checked;

        this.sceneManager.updatePrimitiveGeometry(mesh, newParams);
      });
    });
  }

  initRaycasting() {
    const canvas = this.engine.canvas;

    canvas.addEventListener('click', (e) => {
      if (this.transformManager.isTransforming || this.selectionTools.isSelecting || this.selectionTools.justFinishedDragSelection) {
        this.selectionTools.justFinishedDragSelection = false;
        return;
      }

      const rect = canvas.getBoundingClientRect();
      this.mouse.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      this.mouse.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;
      this.raycaster.setFromCamera(this.mouse, this.engine.activeCamera);

      if (this.currentSubmode !== 'object') {
        this.meshEditor.handleSelection(e.clientX, e.clientY, e.shiftKey, this.raycaster);
      } else {
        const meshes = this.sceneManager.getAllMeshes().filter((m) => m.visible);
        const intersects = this.raycaster.intersectObjects(meshes, false);

        if (intersects.length > 0) {
          this.sceneManager.selectObject(intersects[0].object, e.shiftKey);
        } else {
          this.sceneManager.deselectAll();
        }
      }
    });

    // Hover Cursor Feedback for easy picking
    canvas.addEventListener('mousemove', (e) => {
      if (this.transformManager.isTransforming || this.selectionTools.isSelecting) return;
      if (this.currentSubmode === 'vertex' && this.meshEditor.activeMesh && this.meshEditor.activeMesh.userData.quadMesh) {
        const qm = this.meshEditor.activeMesh.userData.quadMesh;
        const rect = canvas.getBoundingClientRect();
        const mouseX = e.clientX - rect.left;
        const mouseY = e.clientY - rect.top;
        let isNear = false;

        for (let i = 0; i < qm.vertices.length; i++) {
          const vWorld = qm.vertices[i].clone().applyMatrix4(this.meshEditor.activeMesh.matrixWorld);
          const vNDC = vWorld.project(this.engine.activeCamera);
          if (vNDC.z < 1.0) {
            const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
            const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
            if (Math.hypot(mouseX - sx, mouseY - sy) < 24) {
              isNear = true;
              break;
            }
          }
        }
        canvas.style.cursor = isNear ? 'pointer' : 'default';
      } else {
        canvas.style.cursor = 'default';
      }
    });
  }

  initShortcuts() {
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT') return;

      // Escape: Close Shift+A Menu or Deselect
      if (e.key === 'Escape') {
        this.closeShiftAMenu();
        return;
      }

      // Shift + A: Toggle Floating Retractable Context Menu
      if (e.shiftKey && (e.key === 'a' || e.key === 'A')) {
        e.preventDefault();
        this.toggleShiftAMenu();
      }
      // Undo: Ctrl + Z (without Shift)
      else if (e.ctrlKey && (e.key === 'z' || e.key === 'Z') && !e.shiftKey) {
        e.preventDefault();
        this.historyManager.undo();
      }
      // Redo: Ctrl + Shift + Z OR Ctrl + Y
      else if ((e.ctrlKey && e.shiftKey && (e.key === 'z' || e.key === 'Z')) || (e.ctrlKey && (e.key === 'y' || e.key === 'Y'))) {
        e.preventDefault();
        this.historyManager.redo();
      } else if (e.key === 'Tab') {
        e.preventDefault();
        this.setSubmode(this.currentSubmode === 'object' ? 'vertex' : 'object');
      } else if (e.key === '1') {
        this.setSubmode('vertex');
      } else if (e.key === '2') {
        this.setSubmode('edge');
      } else if (e.key === '3') {
        this.setSubmode('face');
      } else if (e.key === 'b' || e.key === 'B') {
        const boxBtn = document.querySelector('[data-sel-tool="box"]');
        if (boxBtn) boxBtn.click();
      } else if (e.key === 'c' || e.key === 'C') {
        const circleBtn = document.querySelector('[data-sel-tool="circle"]');
        if (circleBtn) circleBtn.click();
      } else if (e.key === 'g' || e.key === 'G') {
        this.setTool('move');
        const moveBtn = document.querySelector('[data-tool="move"]');
        if (moveBtn) {
          document.querySelectorAll('.tool-btn').forEach(b => b.classList.remove('active'));
          moveBtn.classList.add('active');
        }
      } else if (e.key === 'r' || e.key === 'R') {
        this.setTool('rotate');
        const rotBtn = document.querySelector('[data-tool="rotate"]');
        if (rotBtn) {
          document.querySelectorAll('.tool-btn').forEach(b => b.classList.remove('active'));
          rotBtn.classList.add('active');
        }
      } else if (e.key === 's' || e.key === 'S') {
        this.setTool('scale');
        const sclBtn = document.querySelector('[data-tool="scale"]');
        if (sclBtn) {
          document.querySelectorAll('.tool-btn').forEach(b => b.classList.remove('active'));
          sclBtn.classList.add('active');
        }
      } else if (e.key === 'a' || e.key === 'A') {
        if (e.altKey) {
          if (this.currentSubmode === 'object') this.sceneManager.deselectAll();
          else this.meshEditor.deselectAll();
        } else {
          if (this.currentSubmode !== 'object') this.meshEditor.selectAll();
        }
      } else if (e.key === 'Delete' || e.key === 'x' || e.key === 'X') {
        if (this.currentSubmode === 'object') {
          this.sceneManager.deleteObject();
        }
      } else if (e.shiftKey && (e.key === 'd' || e.key === 'D')) {
        e.preventDefault();
        this.sceneManager.duplicateObject();
      } else if (e.key === 'n' || e.key === 'N') {
        this.toggleSidebar();
      } else if (e.key === 'e' || e.key === 'E') {
        if (this.currentSubmode !== 'object') {
          e.preventDefault();
          this.meshEditor.extrude();
        }
      } else if (e.key === 'i' || e.key === 'I') {
        if (this.currentSubmode !== 'object') {
          e.preventDefault();
          this.meshEditor.inset();
        }
      } else if (e.key === 'o' || e.key === 'O') {
        const isProp = this.meshEditor.toggleProportionalEditing();
        const hintEl = document.getElementById('status-hint');
        if (hintEl) {
          const badge = isProp ? '<span class="key-hint" style="color:#60a5fa">● Edição Proporcional ON</span>' : '';
          hintEl.innerHTML = `<span class="key-hint"><kbd>E</kbd> Extrusão</span> <span class="key-hint"><kbd>I</kbd> Inset</span> <span class="key-hint"><kbd>G</kbd> Mover</span> <span class="key-hint"><kbd>O</kbd> Proporcional</span> ${badge}`;
        }
      }
    });
  }
}
