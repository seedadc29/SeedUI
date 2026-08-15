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
      if (this.engine.gridHelper) this.engine.gridHelper.visible = e.target.checked;
      if (this.engine.axesHelper) this.engine.axesHelper.visible = e.target.checked;
    });

    document.getElementById('chk-n-shadows')?.addEventListener('change', (e) => {
      this.engine.renderer.shadowMap.enabled = e.target.checked;
    });
  }

  toggleNPanel() {
    const rightSidebar = document.getElementById('right-sidebar');
    const handle = document.getElementById('btn-toggle-npanel');
    if (!rightSidebar) return;

    this.isNPanelOpen = !this.isNPanelOpen;
    rightSidebar.classList.toggle('hidden', !this.isNPanelOpen);
    if (handle) handle.innerHTML = this.isNPanelOpen ? '<span>‹</span>' : '<span>›</span>';
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

  // 10. Raycasting / 3D Selection
  initRaycasting() {
    const canvas = this.engine.canvas;

    canvas.addEventListener('pointerdown', (e) => {
      this.lastScreenMouse = { x: e.clientX, y: e.clientY };
    });

    canvas.addEventListener('click', (e) => {
      if (this.transformManager.isTransforming) return;

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
      // Delete: X or Delete
      else if (e.key === 'Delete' || e.key === 'x' || e.key === 'X') {
        const obj = this.sceneManager.getSelectedObject();
        if (obj && this.currentMode === 'object') {
          this.sceneManager.removeObject(obj);
        }
      }
    });
  }
}
