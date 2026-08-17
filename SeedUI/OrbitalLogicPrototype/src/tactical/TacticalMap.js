export class TacticalMap {
  constructor(canvas, scene3D = null, orbitalGraph = null, onSelectEntity = null, onSelectRoom = null) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.scene3D = scene3D;
    this.orbitalGraph = orbitalGraph;
    this.onSelectEntity = onSelectEntity;
    this.onSelectRoom = onSelectRoom;

    // Viewport transform
    this.panX = 0;
    this.panY = 0;
    this.zoom = 1.0;
    this.dpr = 1;
    this.isPanning = false;
    this.lastMouse = { x: 0, y: 0 };

    // Active Tool: 'select' | 'move' | 'pan'
    this.currentTool = 'select';

    // Interactive selection & manipulation
    this.selectedRoom = null;
    this.hoveredRoom = null;
    this.selectedEntity = null;
    this.hoveredHandle = null;
    this.activeDragHandle = null;
    this.isDraggingRoom = false;
    this.startRoomPos = { x: 0, z: 0 };
    this.startMousePos = { x: 0, z: 0 };
    this.roomInitialState = null;
    this.animTime = 0;

    // Diagnostic logging system
    this.diagnosticLogs = [];
    this.isDiagnosticOpen = false;

    // Scale conversion: 1 meter in 3D = 22 pixels on Tactical Map
    this.meterToPx = 22;

    // Blueprint Rooms / Zones (Rectangles, Circles, Triangles)
    this.rooms = [
      { id: 'room-armory-1', shape: 'rect', name: 'Armory A', x: -11, z: -7, w: 6.0, h: 4.5, color: '#1a1d24' },
      { id: 'room-armory-2', shape: 'rect', name: 'Armory B', x: -11, z: 2.5, w: 6.0, h: 4.5, color: '#1a1d24' },
      { id: 'corridor-main', shape: 'rect', name: 'Corredor Central', x: -3.5, z: -2.0, w: 8.0, h: 4.0, color: '#14161c' },
      { id: 'room-main-hall', shape: 'rect', name: 'Main Hall', x: 6.5, z: -5.5, w: 8.5, h: 11.0, color: '#1a1d24' }
    ];

    // Blueprint Routes (Dashed arrows showing navigation paths)
    this.routes = [
      { from: { x: -8, z: 0 }, to: { x: 0.5, z: 0 } },
      { from: { x: 0.5, z: 0 }, to: { x: 0.5, z: -4.5 } },
      { from: { x: 0.5, z: 0 }, to: { x: 0.5, z: 4.5 } },
      { from: { x: 0.5, z: 0 }, to: { x: 10.5, z: 0 } }
    ];

    this.initCanvas();
    this.initEvents();
    this.initInlineEditor();
    this.initDiagnosticUI();
    this.animate();

    this.logDiag('INIT', 'TacticalMap inicializado com sucesso.');
  }

  initCanvas() {
    this.resize();
    window.addEventListener('resize', () => this.resize());
    if (this.canvas.parentElement && window.ResizeObserver) {
      new ResizeObserver(() => this.resize()).observe(this.canvas.parentElement);
    }
  }

  resize() {
    if (!this.canvas.parentElement) return;
    const parent = this.canvas.parentElement;
    this.dpr = Math.min(window.devicePixelRatio || 1, 2);
    const width = parent.clientWidth;
    const height = parent.clientHeight;
    if (width === 0 || height === 0) return;

    this.canvas.width = width * this.dpr;
    this.canvas.height = height * this.dpr;

    if (this.panX === 0 && this.panY === 0) {
      this.panX = width / 2;
      this.panY = height / 2;
    }
    this.updateDiagnosticHUD();
  }

  setTool(toolName) {
    this.currentTool = toolName;
    document.querySelectorAll('.tactical-tool-btn').forEach((btn) => {
      btn.classList.toggle('active', btn.dataset.ttool === toolName);
    });

    if (toolName === 'pan') {
      this.selectRoom(null);
      this.activeDragHandle = null;
      this.isDraggingRoom = false;
      this.canvas.style.cursor = 'grab';
    } else if (toolName === 'move') {
      this.canvas.style.cursor = 'move';
    } else {
      this.canvas.style.cursor = 'default';
    }
    this.logDiag('FERRAMENTA', `Ferramenta ativa alterada para: [${toolName.toUpperCase()}]`);
  }

  selectRoom(room) {
    if (!room) {
      this.selectedRoom = null;
      if (this.onSelectRoom) this.onSelectRoom(null);
      this.updateDiagnosticHUD();
      return;
    }
    // Elevate selected room to end of rooms array so it draws on top
    this.rooms = this.rooms.filter(r => r.id !== room.id);
    this.rooms.push(room);
    this.selectedRoom = room;
    this.selectedEntity = null;
    if (this.onSelectRoom) this.onSelectRoom(room);
    this.updateDiagnosticHUD();
    this.logDiag('SELEÇÃO_SALA', `Sala selecionada: "${room.name}" (${room.shape}) em X=${room.x}m, Z=${room.z}m`);
  }

  initInlineEditor() {
    this.inlineInput = document.getElementById('tactical-inline-name-input');
    if (!this.inlineInput) return;

    this.inlineInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        this.saveInlineRename();
      } else if (e.key === 'Escape') {
        this.cancelInlineRename();
      }
    });

    this.inlineInput.addEventListener('blur', () => {
      this.saveInlineRename();
    });
  }

  startInlineRename(room) {
    if (!this.inlineInput || !room) return;
    this.editingRoom = room;

    const centerPx = this.getRoomCenterCanvasPx(room);
    this.inlineInput.value = room.name;
    this.inlineInput.style.left = `${centerPx.x - 70}px`;
    this.inlineInput.style.top = `${centerPx.y - 12}px`;
    this.inlineInput.classList.remove('hidden');
    this.inlineInput.focus();
    this.inlineInput.select();
  }

  saveInlineRename() {
    if (this.editingRoom && this.inlineInput) {
      this.editingRoom.name = this.inlineInput.value.trim() || 'Nova Sala';
      if (this.onSelectRoom) this.onSelectRoom(this.editingRoom);
      this.cancelInlineRename();
    }
  }

  cancelInlineRename() {
    this.editingRoom = null;
    if (this.inlineInput) {
      this.inlineInput.classList.add('hidden');
    }
  }

  getRoomCenterCanvasPx(room) {
    let wx = room.x;
    let wz = room.z;
    if (room.shape === 'rect' || room.shape === 'triangle') {
      wx += room.w / 2;
      wz += room.h / 2;
    }
    return {
      x: this.panX + wx * this.meterToPx * this.zoom,
      y: this.panY + wz * this.meterToPx * this.zoom
    };
  }

  addShape(shapeType, worldX = null, worldZ = null) {
    if (worldX === null || worldZ === null) {
      const parent = this.canvas.parentElement;
      const w = parent ? parent.clientWidth : 600;
      const h = parent ? parent.clientHeight : 400;
      worldX = ((w / 2 - this.panX) / this.zoom) / this.meterToPx;
      worldZ = ((h / 2 - this.panY) / this.zoom) / this.meterToPx;
    }

    worldX = Math.round(worldX * 2) / 2;
    worldZ = Math.round(worldZ * 2) / 2;

    let newRoom;
    if (shapeType === 'circle') {
      newRoom = {
        id: `room-circle-${Date.now()}`,
        shape: 'circle',
        name: 'Zona Circular',
        x: worldX,
        z: worldZ,
        radius: 3.5,
        color: '#1a1d24'
      };
    } else if (shapeType === 'triangle') {
      newRoom = {
        id: `room-tri-${Date.now()}`,
        shape: 'triangle',
        name: 'Zona Triangular',
        x: worldX - 3,
        z: worldZ - 2.5,
        w: 6.0,
        h: 5.5,
        color: '#1a1d24'
      };
    } else {
      newRoom = {
        id: `room-rect-${Date.now()}`,
        shape: 'rect',
        name: 'Nova Sala',
        x: worldX - 3,
        z: worldZ - 2.5,
        w: 6.0,
        h: 5.0,
        color: '#1a1d24'
      };
    }

    this.selectRoom(newRoom);
    this.setTool('select');
    this.logDiag('CRIAR_FORMA', `Forma criada: [${shapeType.toUpperCase()}] em X=${newRoom.x}m, Z=${newRoom.z}m`);
    return newRoom;
  }

  initEvents() {
    let isMouseDown = false;
    let downPos = { x: 0, y: 0 };
    let isDragging = false;
    let lastClickTime = 0;

    this.canvas.addEventListener('mousedown', (e) => {
      if (e.button !== 0 && e.button !== 1) return;
      isMouseDown = true;
      isDragging = false;
      downPos = { x: e.clientX, y: e.clientY };
      this.lastMouse = { x: e.clientX, y: e.clientY };

      const mouse = this.getMapPos(e);
      this.logDiag('MOUSEDOWN', `Clique no Canvas: Tela(${e.clientX}, ${e.clientY}) -> Mundo(X=${mouse.x.toFixed(2)}m, Z=${mouse.z.toFixed(2)}m), Tool=${this.currentTool}`);

      // 1. Pan Tool or Middle Click -> Pan only
      if (this.currentTool === 'pan' || e.button === 1 || e.spaceKey) {
        this.isPanning = true;
        this.canvas.parentElement?.classList.add('is-panning');
        return;
      }

      // 2. In Select mode: Check Resize Handles on currently Selected Room (20px tolerance)
      if (this.currentTool === 'select' && this.selectedRoom) {
        const handle = this.hitTestHandles(this.selectedRoom, mouse);
        if (handle) {
          this.activeDragHandle = handle;
          this.roomInitialState = { ...this.selectedRoom };
          this.isPanning = false;
          this.logDiag('HIT_ALÇA', `Alça de redimensionamento [${handle}] clicada na sala "${this.selectedRoom.name}"`);
          return;
        }
      }

      // 3. Check Blueprint Rooms (Smallest Area First)
      const hitRoom = this.hitTestRooms(mouse.x, mouse.z);
      if (hitRoom) {
        const now = Date.now();
        if (now - lastClickTime < 350 && this.selectedRoom === hitRoom) {
          this.startInlineRename(hitRoom);
          return;
        }
        lastClickTime = now;

        this.selectRoom(hitRoom);
        this.isDraggingRoom = true;
        this.isPanning = false;
        this.startRoomPos = { x: hitRoom.x, z: hitRoom.z };
        this.startMousePos = { x: mouse.x, z: mouse.z };
        this.logDiag('HIT_SALA', `Sala atingida e selecionada com sucesso: "${hitRoom.name}"`);
        return;
      }

      // 4. Check 3D Entities on Map
      const hitEntity = this.hitTestEntities(mouse.x, mouse.z);
      if (hitEntity) {
        this.selectedEntity = hitEntity;
        this.selectRoom(null);
        this.isPanning = false;
        if (this.onSelectEntity) this.onSelectEntity(hitEntity);
        this.logDiag('HIT_ENTIDADE_3D', `Entidade 3D atingida: "${hitEntity.sunName || hitEntity.type}"`);
        return;
      }

      // 5. Clicked on empty space -> Deselect & pan
      this.selectRoom(null);
      this.isPanning = true;
      this.cancelInlineRename();
      this.logDiag('CLIQUE_VAZIO', `Clique em espaço vazio -> Nenhuma forma selecionada.`);
    });

    window.addEventListener('mousemove', (e) => {
      const mouse = this.getMapPos(e);

      if (!isMouseDown) {
        if (this.currentTool === 'select') {
          if (this.selectedRoom) {
            this.hoveredHandle = this.hitTestHandles(this.selectedRoom, mouse);
            if (this.hoveredHandle) {
              this.updateCursor(this.hoveredHandle);
              return;
            }
          }
          this.hoveredHandle = null;
          this.hoveredRoom = this.hitTestRooms(mouse.x, mouse.z);
          this.canvas.style.cursor = this.hoveredRoom ? 'pointer' : 'default';
        } else if (this.currentTool === 'move') {
          this.hoveredRoom = this.hitTestRooms(mouse.x, mouse.z);
          this.canvas.style.cursor = 'move';
        } else if (this.currentTool === 'pan') {
          this.canvas.style.cursor = 'grab';
        }
        this.lastMouse = { x: e.clientX, y: e.clientY };
        return;
      }

      const moveDist = Math.hypot(e.clientX - downPos.x, e.clientY - downPos.y);
      if (moveDist > 3) isDragging = true;

      // 1. Dragging Resize Handles (Stretching sides in Select mode)
      if (this.activeDragHandle && this.selectedRoom && this.roomInitialState) {
        const room = this.selectedRoom;
        const init = this.roomInitialState;
        const h = this.activeDragHandle;

        if (room.shape === 'circle') {
          const dist = Math.hypot(mouse.x - init.x, mouse.z - init.z);
          room.radius = Math.max(1.0, Math.round(dist * 2) / 2);
        } else {
          if (h.includes('e')) {
            const newW = mouse.x - init.x;
            room.w = Math.max(1.5, Math.round(newW * 2) / 2);
          }
          if (h.includes('w')) {
            const newRight = init.x + init.w;
            const newX = Math.round(mouse.x * 2) / 2;
            if (newRight - newX >= 1.5) {
              room.x = newX;
              room.w = newRight - newX;
            }
          }
          if (h.includes('s')) {
            const newH = mouse.z - init.z;
            room.h = Math.max(1.5, Math.round(newH * 2) / 2);
          }
          if (h.includes('n')) {
            const newBottom = init.z + init.h;
            const newZ = Math.round(mouse.z * 2) / 2;
            if (newBottom - newZ >= 1.5) {
              room.z = newZ;
              room.h = newBottom - newZ;
            }
          }
        }
      }
      // 2. Dragging Room Position on Grid (Robust Delta Translation)
      else if (this.isDraggingRoom && this.selectedRoom) {
        const dx = mouse.x - this.startMousePos.x;
        const dz = mouse.z - this.startMousePos.z;
        const newX = Math.round((this.startRoomPos.x + dx) * 2) / 2;
        const newZ = Math.round((this.startRoomPos.z + dz) * 2) / 2;
        this.selectedRoom.x = newX;
        this.selectedRoom.z = newZ;
      }
      // 3. Panning Viewport
      else if (this.isPanning) {
        this.panX += e.clientX - this.lastMouse.x;
        this.panY += e.clientY - this.lastMouse.y;
        this.canvas.parentElement?.classList.add('is-panning');
      }

      this.lastMouse = { x: e.clientX, y: e.clientY };
    });

    window.addEventListener('mouseup', () => {
      if (this.isDraggingRoom || this.activeDragHandle) {
        if (this.selectedRoom && this.onSelectRoom) {
          this.onSelectRoom(this.selectedRoom);
          this.logDiag('MOUSEUP', `Operação concluída na sala "${this.selectedRoom.name}". Nova Pos: (${this.selectedRoom.x}m, ${this.selectedRoom.z}m), Dim: W=${this.selectedRoom.w || this.selectedRoom.radius*2}, H=${this.selectedRoom.h || this.selectedRoom.radius*2}`);
        }
      }
      isMouseDown = false;
      isDragging = false;
      this.isPanning = false;
      this.isDraggingRoom = false;
      this.activeDragHandle = null;
      this.canvas.parentElement?.classList.remove('is-panning');
      this.updateCursor(null);
      this.updateDiagnosticHUD();
    });

    // Zoom on wheel
    this.canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      const zoomFactor = e.deltaY < 0 ? 1.08 : 0.92;
      this.zoom = Math.max(0.4, Math.min(2.5, this.zoom * zoomFactor));
      this.updateDiagnosticHUD();
    }, { passive: false });

    // Keyboard Shortcuts
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;
      if (e.key === 'Delete' || e.key === 'Backspace') {
        if (this.selectedRoom) {
          const name = this.selectedRoom.name;
          this.rooms = this.rooms.filter(r => r.id !== this.selectedRoom.id);
          this.selectRoom(null);
          this.logDiag('EXCLUIR', `Sala "${name}" excluída.`);
        }
      } else if (e.code === 'KeyV') this.setTool('select');
      else if (e.code === 'KeyG' || e.code === 'KeyW') this.setTool('move');
      else if (e.code === 'KeyH') this.setTool('pan');
    });
  }

  updateCursor(handle) {
    if (!handle) {
      if (this.currentTool === 'pan') this.canvas.style.cursor = 'grab';
      else if (this.currentTool === 'move') this.canvas.style.cursor = 'move';
      else this.canvas.style.cursor = this.hoveredRoom ? 'pointer' : 'default';
      return;
    }
    if (handle === 'n' || handle === 's') this.canvas.style.cursor = 'ns-resize';
    else if (handle === 'e' || handle === 'w') this.canvas.style.cursor = 'ew-resize';
    else if (handle === 'nw' || handle === 'se') this.canvas.style.cursor = 'nwse-resize';
    else if (handle === 'ne' || handle === 'sw') this.canvas.style.cursor = 'nesw-resize';
    else if (handle === 'radius') this.canvas.style.cursor = 'crosshair';
  }

  getMapPos(e) {
    const rect = this.canvas.getBoundingClientRect();
    const clientX = e.clientX - rect.left;
    const clientY = e.clientY - rect.top;
    return {
      x: ((clientX - this.panX) / this.zoom) / this.meterToPx,
      z: ((clientY - this.panY) / this.zoom) / this.meterToPx,
      clientX,
      clientY
    };
  }

  isPointInsideRoom(r, worldX, worldZ) {
    const pad = 0.5;
    if (r.shape === 'circle') {
      return Math.hypot(worldX - r.x, worldZ - r.z) <= (r.radius + pad);
    } else if (r.shape === 'triangle') {
      const ax = r.x + r.w / 2, az = r.z;
      const bx = r.x + r.w,     bz = r.z + r.h;
      const cx = r.x,           cz = r.z + r.h;
      
      const v0x = cx - ax, v0z = cz - az;
      const v1x = bx - ax, v1z = bz - az;
      const v2x = worldX - ax, v2z = worldZ - az;

      const dot00 = v0x * v0x + v0z * v0z;
      const dot01 = v0x * v1x + v0z * v1z;
      const dot02 = v0x * v2x + v0z * v2z;
      const dot11 = v1x * v1x + v1z * v1z;
      const dot12 = v1x * v2x + v1z * v2z;

      const invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
      const u = (dot11 * dot02 - dot01 * dot12) * invDenom;
      const v = (dot00 * dot12 - dot01 * dot02) * invDenom;

      return (u >= -0.15) && (v >= -0.15) && (u + v <= 1.15);
    } else {
      return worldX >= r.x - pad && worldX <= r.x + r.w + pad && worldZ >= r.z - pad && worldZ <= r.z + r.h + pad;
    }
  }

  hitTestRooms(worldX, worldZ) {
    // 1. If currently selected room is hit, prioritize it
    if (this.selectedRoom && this.isPointInsideRoom(this.selectedRoom, worldX, worldZ)) {
      return this.selectedRoom;
    }

    // 2. Collect all rooms that contain the clicked point
    const hits = [];
    for (const r of this.rooms) {
      if (this.isPointInsideRoom(r, worldX, worldZ)) {
        let area = r.w * r.h;
        if (r.shape === 'circle') area = Math.PI * r.radius * r.radius;
        else if (r.shape === 'triangle') area = (r.w * r.h) / 2;
        hits.push({ room: r, area });
      }
    }

    // 3. Sort by SMALLEST AREA FIRST!
    if (hits.length > 0) {
      hits.sort((a, b) => a.area - b.area);
      return hits[0].room;
    }

    return null;
  }

  hitTestEntities(worldX, worldZ) {
    if (!this.scene3D) return null;
    let closest = null;
    let minDist = 1.6;

    this.scene3D.entities.forEach((ent) => {
      if (!ent.mesh || ent.shape === 'plane') return;
      const d = Math.hypot(worldX - ent.mesh.position.x, worldZ - ent.mesh.position.z);
      if (d < minDist) {
        minDist = d;
        closest = ent;
      }
    });

    return closest;
  }

  hitTestHandles(room, mouse) {
    const hitTolerancePx = 20;

    const toScreen = (wx, wz) => ({
      x: this.panX + (wx * this.meterToPx) * this.zoom,
      y: this.panY + (wz * this.meterToPx) * this.zoom
    });

    if (room.shape === 'circle') {
      const sp = toScreen(room.x + room.radius, room.z);
      if (Math.hypot(mouse.clientX - sp.x, mouse.clientY - sp.y) <= hitTolerancePx) {
        return 'radius';
      }
      return null;
    }

    const x1 = room.x;
    const x2 = room.x + room.w;
    const xm = room.x + room.w / 2;
    const z1 = room.z;
    const z2 = room.z + room.h;
    const zm = room.z + room.h / 2;

    const handles = [
      { id: 'nw', ...toScreen(x1, z1) },
      { id: 'n',  ...toScreen(xm, z1) },
      { id: 'ne', ...toScreen(x2, z1) },
      { id: 'e',  ...toScreen(x2, zm) },
      { id: 'se', ...toScreen(x2, z2) },
      { id: 's',  ...toScreen(xm, z2) },
      { id: 'sw', ...toScreen(x1, z2) },
      { id: 'w',  ...toScreen(x1, zm) }
    ];

    for (const h of handles) {
      if (Math.hypot(mouse.clientX - h.x, mouse.clientY - h.y) <= hitTolerancePx) {
        return h.id;
      }
    }

    return null;
  }

  // --- Diagnostic System Implementation ---
  initDiagnosticUI() {
    const btnToggle = document.getElementById('btn-toggle-diagnostic');
    const btnClose = document.getElementById('btn-close-diagnostic');
    const btnCopy = document.getElementById('btn-copy-diagnostic');
    const btnClear = document.getElementById('btn-clear-diagnostic');
    const drawer = document.getElementById('tactical-diagnostic-drawer');

    btnToggle?.addEventListener('click', () => {
      this.isDiagnosticOpen = !this.isDiagnosticOpen;
      drawer?.classList.toggle('hidden', !this.isDiagnosticOpen);
    });

    btnClose?.addEventListener('click', () => {
      this.isDiagnosticOpen = false;
      drawer?.classList.add('hidden');
    });

    btnClear?.addEventListener('click', () => {
      this.diagnosticLogs = [];
      const logEl = document.getElementById('diagnostic-log-text');
      if (logEl) logEl.textContent = 'Logs limpos. Clique nas formas para registrar novos eventos...';
    });

    btnCopy?.addEventListener('click', () => {
      const fullReport = this.getDiagnosticReport();
      navigator.clipboard.writeText(fullReport).then(() => {
        const origText = btnCopy.innerHTML;
        btnCopy.innerHTML = '<i class="ti ti-check"></i> <span>Copiado com Sucesso!</span>';
        btnCopy.style.background = '#34c759';
        btnCopy.style.color = '#ffffff';
        setTimeout(() => {
          btnCopy.innerHTML = origText;
          btnCopy.style.background = '#ffcc00';
          btnCopy.style.color = '#0b0f19';
        }, 2500);
      });
    });

    this.updateDiagnosticHUD();
  }

  logDiag(category, message) {
    const time = new Date().toTimeString().split(' ')[0];
    const logLine = `[${time}] [${category}] ${message}`;
    this.diagnosticLogs.push(logLine);
    if (this.diagnosticLogs.length > 50) this.diagnosticLogs.shift();

    const logEl = document.getElementById('diagnostic-log-text');
    if (logEl) {
      logEl.textContent = this.diagnosticLogs.slice(-15).join('\n');
      logEl.scrollTop = logEl.scrollHeight;
    }
    this.updateDiagnosticHUD();
  }

  updateDiagnosticHUD() {
    const rect = this.canvas.getBoundingClientRect();
    const canvasInfo = document.getElementById('diag-canvas-info');
    const camInfo = document.getElementById('diag-cam-info');
    const mouseInfo = document.getElementById('diag-mouse-info');
    const selectedInfo = document.getElementById('diag-selected-info');

    if (canvasInfo) {
      canvasInfo.textContent = `W:${Math.round(rect.width)}px, H:${Math.round(rect.height)}px (DPR:${this.dpr})`;
    }
    if (camInfo) {
      camInfo.textContent = `Pan:(${Math.round(this.panX)}, ${Math.round(this.panY)}), Zoom:${this.zoom.toFixed(2)}x`;
    }
    if (mouseInfo) {
      const wx = ((this.lastMouse.x - rect.left - this.panX) / this.zoom) / this.meterToPx;
      const wz = ((this.lastMouse.y - rect.top - this.panY) / this.zoom) / this.meterToPx;
      mouseInfo.textContent = `Tela:(${Math.round(this.lastMouse.x)}, ${Math.round(this.lastMouse.y)}) | Mundo:(${wx.toFixed(1)}m, ${wz.toFixed(1)}m)`;
    }
    if (selectedInfo) {
      selectedInfo.textContent = this.selectedRoom ? `"${this.selectedRoom.name}" (${this.selectedRoom.shape})` : 'NENHUMA';
    }
  }

  getDiagnosticReport() {
    const rect = this.canvas.getBoundingClientRect();
    return JSON.stringify({
      timestamp: new Date().toISOString(),
      canvas: {
        widthCss: rect.width,
        heightCss: rect.height,
        widthInternal: this.canvas.width,
        heightInternal: this.canvas.height,
        dpr: this.dpr,
        devicePixelRatio: window.devicePixelRatio
      },
      viewport: {
        panX: this.panX,
        panY: this.panY,
        zoom: this.zoom,
        meterToPx: this.meterToPx
      },
      state: {
        activeTool: this.currentTool,
        selectedRoom: this.selectedRoom,
        roomsCount: this.rooms.length,
        roomsList: this.rooms.map(r => ({
          id: r.id,
          name: r.name,
          shape: r.shape,
          x: r.x,
          z: r.z,
          w: r.w,
          h: r.h,
          radius: r.radius
        }))
      },
      recentLogs: this.diagnosticLogs
    }, null, 2);
  }

  update(delta) {
    this.animTime += delta;
  }

  draw() {
    if (!this.canvas.parentElement) return;
    const w = this.canvas.parentElement.clientWidth;
    const h = this.canvas.parentElement.clientHeight;
    if (w === 0 || h === 0) return;

    if (this.panX === 0 && this.panY === 0) {
      this.panX = w / 2;
      this.panY = h / 2;
    }

    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

    this.ctx.save();
    // 1. Correct DPR Scale Matrix
    this.ctx.scale(this.dpr, this.dpr);
    this.ctx.translate(this.panX, this.panY);
    this.ctx.scale(this.zoom, this.zoom);

    // 2. Blueprint Grid
    this.drawGrid();

    // 3. Blueprint Rooms & Shapes
    this.drawRooms();

    // 4. Navigation Routes
    this.drawRoutes();

    // 5. Trigger Action Lines
    this.drawTriggerActionLinks();

    // 6. 3D Entities on Map
    this.drawEntities();

    // 7. Selected Room Transform Handles & Borders
    if (this.selectedRoom) {
      this.drawRoomHandles(this.selectedRoom);
    }

    this.ctx.restore();

    // 8. Map Legend (in Screen CSS coordinates)
    this.drawLegend(w, h);
  }

  drawGrid() {
    const gridSize = this.meterToPx;
    this.ctx.save();
    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.04)';
    this.ctx.lineWidth = 1;

    const bound = 800;
    for (let x = -bound; x <= bound; x += gridSize) {
      this.ctx.beginPath();
      this.ctx.moveTo(x, -bound);
      this.ctx.lineTo(x, bound);
      this.ctx.stroke();
    }
    for (let y = -bound; y <= bound; y += gridSize) {
      this.ctx.beginPath();
      this.ctx.moveTo(-bound, y);
      this.ctx.lineTo(bound, y);
      this.ctx.stroke();
    }

    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.14)';
    this.ctx.beginPath();
    this.ctx.moveTo(-30, 0); this.ctx.lineTo(30, 0);
    this.ctx.moveTo(0, -30); this.ctx.lineTo(0, 30);
    this.ctx.stroke();
    this.ctx.restore();
  }

  drawRooms() {
    this.rooms.forEach((room) => {
      const isSel = this.selectedRoom === room;
      const isHov = this.hoveredRoom === room && !isSel;
      const rx = room.x * this.meterToPx;
      const rz = room.z * this.meterToPx;

      this.ctx.save();

      // CIRCULAR ROOM
      if (room.shape === 'circle') {
        const rad = room.radius * this.meterToPx;
        this.ctx.beginPath();
        this.ctx.arc(rx, rz, rad, 0, Math.PI * 2);
        this.ctx.fillStyle = isSel ? 'rgba(56, 189, 248, 0.18)' : room.color;
        this.ctx.fill();

        this.ctx.strokeStyle = isSel ? '#38bdf8' : (isHov ? '#0284c7' : '#333742');
        this.ctx.lineWidth = isSel ? 3 : (isHov ? 2.5 : 2);
        if (isSel) {
          this.ctx.shadowColor = '#38bdf8';
          this.ctx.shadowBlur = 14;
        }
        this.ctx.stroke();

        this.drawRoomLabel(room.name, rx, rz, isSel);
      }
      // TRIANGULAR ROOM
      else if (room.shape === 'triangle') {
        const rw = room.w * this.meterToPx;
        const rh = room.h * this.meterToPx;

        this.ctx.beginPath();
        this.ctx.moveTo(rx + rw / 2, rz);
        this.ctx.lineTo(rx + rw, rz + rh);
        this.ctx.lineTo(rx, rz + rh);
        this.ctx.closePath();

        this.ctx.fillStyle = isSel ? 'rgba(56, 189, 248, 0.18)' : room.color;
        this.ctx.fill();

        this.ctx.strokeStyle = isSel ? '#38bdf8' : (isHov ? '#0284c7' : '#333742');
        this.ctx.lineWidth = isSel ? 3 : (isHov ? 2.5 : 2);
        if (isSel) {
          this.ctx.shadowColor = '#38bdf8';
          this.ctx.shadowBlur = 14;
        }
        this.ctx.stroke();

        this.drawRoomLabel(room.name, rx + rw / 2, rz + rh * 0.65, isSel);
      }
      // RECTANGULAR ROOM (DEFAULT)
      else {
        const rw = room.w * this.meterToPx;
        const rh = room.h * this.meterToPx;

        this.ctx.fillStyle = isSel ? 'rgba(56, 189, 248, 0.18)' : room.color;
        this.ctx.fillRect(rx, rz, rw, rh);

        // Internal blueprint crosshatch lines
        this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.04)';
        this.ctx.lineWidth = 1;
        for (let tx = rx; tx <= rx + rw; tx += this.meterToPx / 2) {
          this.ctx.beginPath();
          this.ctx.moveTo(tx, rz);
          this.ctx.lineTo(tx, rz + rh);
          this.ctx.stroke();
        }

        this.ctx.strokeStyle = isSel ? '#38bdf8' : (isHov ? '#0284c7' : '#333742');
        this.ctx.lineWidth = isSel ? 3 : (isHov ? 2.5 : 2);
        if (isSel) {
          this.ctx.shadowColor = '#38bdf8';
          this.ctx.shadowBlur = 14;
        }
        this.ctx.strokeRect(rx, rz, rw, rh);

        this.drawRoomLabel(room.name, rx + rw / 2, rz + rh / 2, isSel);
      }

      this.ctx.restore();
    });
  }

  drawRoomLabel(text, cx, cy, isSel) {
    this.ctx.font = 'bold 11px sans-serif';
    const txtWidth = this.ctx.measureText(text).width;

    this.ctx.fillStyle = 'rgba(15, 17, 23, 0.88)';
    this.ctx.fillRect(cx - txtWidth / 2 - 6, cy - 8, txtWidth + 12, 16);

    this.ctx.fillStyle = isSel ? '#38bdf8' : 'rgba(255, 255, 255, 0.65)';
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText(text, cx, cy);
  }

  drawRoomHandles(room) {
    this.ctx.save();

    let cx = room.x * this.meterToPx;
    let cz = room.z * this.meterToPx;

    if (room.shape === 'circle') {
      const rx = (room.x + room.radius) * this.meterToPx;
      const rz = room.z * this.meterToPx;
      this.drawHandleBox(rx, rz);
    } else {
      const x1 = room.x * this.meterToPx;
      const x2 = (room.x + room.w) * this.meterToPx;
      const xm = (room.x + room.w / 2) * this.meterToPx;
      const z1 = room.z * this.meterToPx;
      const z2 = (room.z + room.h) * this.meterToPx;
      const zm = (room.z + room.h / 2) * this.meterToPx;
      cx = xm;
      cz = zm;

      if (this.currentTool === 'select') {
        // 4 Corners
        this.drawHandleBox(x1, z1);
        this.drawHandleBox(x2, z1);
        this.drawHandleBox(x1, z2);
        this.drawHandleBox(x2, z2);

        // 4 Edge Midpoints (for isolated side stretching)
        this.drawHandleCircle(xm, z1);
        this.drawHandleCircle(x2, zm);
        this.drawHandleCircle(xm, z2);
        this.drawHandleCircle(x1, zm);
      }
    }

    // Move Gizmo if in Move mode
    if (this.currentTool === 'move') {
      this.drawMoveGizmo(cx, cz);
    }

    this.ctx.restore();
  }

  drawMoveGizmo(cx, cy) {
    this.ctx.save();
    this.ctx.strokeStyle = '#38bdf8';
    this.ctx.fillStyle = '#38bdf8';
    this.ctx.lineWidth = 2.5;

    const size = 20;
    // Cross lines
    this.ctx.beginPath();
    this.ctx.moveTo(cx - size, cy); this.ctx.lineTo(cx + size, cy);
    this.ctx.moveTo(cx, cy - size); this.ctx.lineTo(cx + size, cy);
    this.ctx.stroke();

    // 4 Arrow Heads
    const arrow = (x, y, angle) => {
      this.ctx.save();
      this.ctx.translate(x, y);
      this.ctx.rotate(angle);
      this.ctx.beginPath();
      this.ctx.moveTo(0, 0);
      this.ctx.lineTo(-6, -4);
      this.ctx.lineTo(-6, 4);
      this.ctx.closePath();
      this.ctx.fill();
      this.ctx.restore();
    };

    arrow(cx + size, cy, 0);
    arrow(cx - size, cy, Math.PI);
    arrow(cx, cy + size, Math.PI / 2);
    arrow(cx, cy - size, -Math.PI / 2);

    // Center handle disc
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, 5, 0, Math.PI * 2);
    this.ctx.fillStyle = '#ffffff';
    this.ctx.fill();
    this.ctx.stroke();

    this.ctx.restore();
  }

  drawHandleBox(x, y) {
    this.ctx.fillStyle = '#38bdf8';
    this.ctx.strokeStyle = '#ffffff';
    this.ctx.lineWidth = 2;
    this.ctx.fillRect(x - 6, y - 6, 12, 12);
    this.ctx.strokeRect(x - 6, y - 6, 12, 12);
  }

  drawHandleCircle(x, y) {
    this.ctx.beginPath();
    this.ctx.arc(x, y, 6, 0, Math.PI * 2);
    this.ctx.fillStyle = '#ffffff';
    this.ctx.strokeStyle = '#38bdf8';
    this.ctx.lineWidth = 2.5;
    this.ctx.fill();
    this.ctx.stroke();
  }

  drawRoutes() {
    this.ctx.save();
    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.35)';
    this.ctx.setLineDash([4, 4]);
    this.ctx.lineWidth = 1.5;

    this.routes.forEach((route) => {
      const x1 = route.from.x * this.meterToPx;
      const y1 = route.from.z * this.meterToPx;
      const x2 = route.to.x * this.meterToPx;
      const y2 = route.to.z * this.meterToPx;

      this.ctx.beginPath();
      this.ctx.moveTo(x1, y1);
      this.ctx.lineTo(x2, y2);
      this.ctx.stroke();

      const angle = Math.atan2(y2 - y1, x2 - x1);
      const midX = (x1 + x2) / 2;
      const midY = (y1 + y2) / 2;
      const headLen = 6;

      this.ctx.save();
      this.ctx.setLineDash([]);
      this.ctx.beginPath();
      this.ctx.moveTo(midX, midY);
      this.ctx.lineTo(midX - headLen * Math.cos(angle - Math.PI / 6), midY - headLen * Math.sin(angle - Math.PI / 6));
      this.ctx.moveTo(midX, midY);
      this.ctx.lineTo(midX - headLen * Math.cos(angle + Math.PI / 6), midY - headLen * Math.sin(angle + Math.PI / 6));
      this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.6)';
      this.ctx.lineWidth = 1.5;
      this.ctx.stroke();
      this.ctx.restore();
    });

    this.ctx.restore();
  }

  drawTriggerActionLinks() {
    if (!this.scene3D) return;

    const triggers = [];
    const blocks = [];

    this.scene3D.entities.forEach((ent) => {
      if (!ent.mesh) return;
      if (ent.type === 'trigger') triggers.push(ent);
      else if (ent.type === 'block' || ent.type === 'platform') blocks.push(ent);
    });

    triggers.forEach((trig) => {
      blocks.forEach((blk) => {
        const tx = trig.mesh.position.x * this.meterToPx;
        const tz = trig.mesh.position.z * this.meterToPx;
        const bx = blk.mesh.position.x * this.meterToPx;
        const bz = blk.mesh.position.z * this.meterToPx;

        this.ctx.save();
        this.ctx.beginPath();
        this.ctx.moveTo(tx, tz);
        this.ctx.lineTo(bx, bz);
        this.ctx.strokeStyle = '#ff9500';
        this.ctx.setLineDash([3, 4]);
        this.ctx.lineWidth = 1.8;
        this.ctx.shadowColor = '#ff9500';
        this.ctx.shadowBlur = 8;
        this.ctx.stroke();

        const progress = (this.animTime * 1.2) % 1.0;
        const px = tx + (bx - tx) * progress;
        const pz = tz + (bz - tz) * progress;

        this.ctx.beginPath();
        this.ctx.arc(px, pz, 3.5, 0, Math.PI * 2);
        this.ctx.fillStyle = '#ffcc00';
        this.ctx.shadowColor = '#ffcc00';
        this.ctx.shadowBlur = 10;
        this.ctx.fill();

        this.ctx.restore();
      });
    });
  }

  drawEntities() {
    if (!this.scene3D) return;

    this.scene3D.entities.forEach((ent) => {
      if (!ent.mesh || ent.shape === 'plane') return;

      const px = ent.mesh.position.x * this.meterToPx;
      const pz = ent.mesh.position.z * this.meterToPx;
      const isSelected = this.scene3D.selectedEntity === ent || this.selectedEntity === ent;

      // 1. TRIGGER ZONE
      if (ent.type === 'trigger') {
        const boxSize = 2.8 * this.meterToPx;
        this.ctx.save();
        this.ctx.fillStyle = 'rgba(255, 149, 0, 0.18)';
        this.ctx.fillRect(px - boxSize / 2, pz - boxSize / 2, boxSize, boxSize);

        this.ctx.strokeStyle = isSelected ? '#ffffff' : '#ff9500';
        this.ctx.setLineDash([4, 3]);
        this.ctx.lineWidth = 2;
        this.ctx.shadowColor = '#ff9500';
        this.ctx.shadowBlur = isSelected ? 16 : 8;
        this.ctx.strokeRect(px - boxSize / 2, pz - boxSize / 2, boxSize, boxSize);

        this.ctx.fillStyle = '#ff9500';
        this.ctx.font = 'bold 10px sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.fillText(ent.sunName || 'Gatilho A', px, pz - boxSize / 2 - 4);
        this.ctx.restore();
        return;
      }

      // 2. BLOCK / WALL / PLATFORM / STATIC SCENERY (Render exact scaled width, depth and rotation)
      if (ent.type === 'block' || ent.type === 'platform' || (ent.type === 'object' && ent.hasCollision)) {
        const sx = ent.mesh ? ent.mesh.scale.x : 1.0;
        const sz = ent.mesh ? ent.mesh.scale.z : 1.0;
        const baseW = ent.baseSize?.x || 1.8;
        const baseD = ent.baseSize?.z || 1.8;
        const w = (baseW * sx) * this.meterToPx;
        const d = (baseD * sz) * this.meterToPx;
        const rotY = ent.mesh ? ent.mesh.rotation.y : 0;

        this.ctx.save();
        this.ctx.translate(px, pz);
        this.ctx.rotate(rotY);

        this.ctx.fillStyle = ent.type === 'platform' ? 'rgba(30, 58, 95, 0.75)' : 'rgba(49, 46, 129, 0.85)';
        this.ctx.fillRect(-w / 2, -d / 2, w, d);

        // Internal blueprint crosshatch lines for walls/blocks
        this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
        this.ctx.lineWidth = 1;
        for (let tx = -w / 2; tx <= w / 2; tx += this.meterToPx / 2) {
          this.ctx.beginPath();
          this.ctx.moveTo(tx, -d / 2);
          this.ctx.lineTo(tx, d / 2);
          this.ctx.stroke();
        }

        this.ctx.strokeStyle = isSelected ? '#38bdf8' : (ent.type === 'platform' ? '#38bdf8' : '#a855f7');
        this.ctx.lineWidth = isSelected ? 3 : 1.8;
        if (isSelected) {
          this.ctx.shadowColor = '#38bdf8';
          this.ctx.shadowBlur = 12;
        }
        this.ctx.strokeRect(-w / 2, -d / 2, w, d);

        this.ctx.fillStyle = '#e0e7ff';
        this.ctx.font = 'bold 9.5px sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.textBaseline = 'middle';
        this.ctx.fillText(ent.sunName || 'Parede/Bloco', 0, 0);

        this.ctx.restore();
        return;
      }

      // 3. ENEMY
      if (ent.type === 'enemy') {
        this.ctx.save();
        this.ctx.beginPath();
        this.ctx.arc(px, pz, 10, 0, Math.PI * 2);
        this.ctx.fillStyle = '#ff3b30';
        this.ctx.shadowColor = '#ff3b30';
        this.ctx.shadowBlur = 12;
        this.ctx.fill();

        this.ctx.strokeStyle = isSelected ? '#ffffff' : '#881111';
        this.ctx.lineWidth = 2;
        this.ctx.stroke();

        this.ctx.fillStyle = '#ffffff';
        this.ctx.font = 'bold 9px sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.fillText('INIMIGO', px, pz + 18);
        this.ctx.restore();
        return;
      }

      // 4. PLAYER
      if (ent.type === 'player') {
        this.ctx.save();
        const pulse = (this.animTime * 2) % 1.0;
        this.ctx.beginPath();
        this.ctx.arc(px, pz, 12 + pulse * 14, 0, Math.PI * 2);
        this.ctx.strokeStyle = `rgba(50, 173, 230, ${1 - pulse})`;
        this.ctx.lineWidth = 1.5;
        this.ctx.stroke();

        this.ctx.beginPath();
        this.ctx.arc(px, pz, 11, 0, Math.PI * 2);
        this.ctx.fillStyle = '#32ade6';
        this.ctx.shadowColor = '#32ade6';
        this.ctx.shadowBlur = 15;
        this.ctx.fill();

        this.ctx.strokeStyle = '#ffffff';
        this.ctx.lineWidth = 2;
        this.ctx.stroke();

        const rot = ent.mesh.rotation.y;
        const dirX = Math.sin(rot) * 18;
        const dirZ = Math.cos(rot) * 18;

        this.ctx.beginPath();
        this.ctx.moveTo(px, pz);
        this.ctx.lineTo(px + dirX, pz + dirZ);
        this.ctx.strokeStyle = '#ffffff';
        this.ctx.lineWidth = 2.5;
        this.ctx.stroke();

        this.ctx.fillStyle = '#ffffff';
        this.ctx.font = 'bold 10px sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.fillText('PLAYER', px, pz - 16);
        this.ctx.restore();
        return;
      }

      // 5. GENERIC NPC / OBJECT
      this.ctx.save();
      this.ctx.beginPath();
      this.ctx.arc(px, pz, 8, 0, Math.PI * 2);
      this.ctx.fillStyle = '#34c759';
      this.ctx.fill();
      this.ctx.restore();
    });
  }

  drawLegend(w, h) {
    this.ctx.save();
    const lx = w - 145;
    const ly = h - 105;

    this.ctx.fillStyle = 'rgba(18, 20, 26, 0.88)';
    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.12)';
    this.ctx.lineWidth = 1;
    this.ctx.fillRect(lx, ly, 135, 95);
    this.ctx.strokeRect(lx, ly, 135, 95);

    this.ctx.fillStyle = '#94a3b8';
    this.ctx.font = 'bold 10px sans-serif';
    this.ctx.fillText('LEGENDA TÁTICA', lx + 10, ly + 16);

    this.ctx.beginPath();
    this.ctx.arc(lx + 16, ly + 34, 5, 0, Math.PI * 2);
    this.ctx.fillStyle = '#32ade6';
    this.ctx.fill();
    this.ctx.fillStyle = '#ffffff';
    this.ctx.font = '10px sans-serif';
    this.ctx.fillText('Player (Posição)', lx + 30, ly + 37);

    this.ctx.fillStyle = '#a855f7';
    this.ctx.fillRect(lx + 11, ly + 47, 10, 10);
    this.ctx.fillStyle = '#ffffff';
    this.ctx.fillText('Bloco / Parede', lx + 30, ly + 56);

    this.ctx.strokeStyle = '#ff9500';
    this.ctx.lineWidth = 1.5;
    this.ctx.strokeRect(lx + 11, ly + 66, 10, 10);
    this.ctx.fillStyle = '#ffffff';
    this.ctx.fillText('Gatilho (Trigger)', lx + 30, ly + 75);

    this.ctx.restore();
  }

  animate() {
    requestAnimationFrame(() => this.animate());
    const delta = 0.016;
    this.update(delta);
    this.draw();
  }
}
