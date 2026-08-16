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

    // Modes: isEditMode = true (Edit / Move / Stretch shapes) vs false (Pan / Navigate)
    this.isEditMode = true;

    // Interactive selection & manipulation
    this.selectedRoom = null;
    this.selectedEntity = null;
    this.hoveredHandle = null;
    this.activeDragHandle = null;
    this.isDraggingRoom = false;
    this.roomDragOffset = { x: 0, z: 0 };
    this.roomInitialState = null;
    this.animTime = 0;

    // Scale conversion: 1 meter in 3D = 22 pixels on Tactical Map
    this.meterToPx = 22;

    // Blueprint Rooms / Zones (Rectangles, Circles, Triangles)
    this.rooms = [
      { id: 'room-armory-1', shape: 'rect', name: 'Armory A', x: -9, z: -7, w: 6.5, h: 5, color: '#1a1d24' },
      { id: 'room-armory-2', shape: 'rect', name: 'Armory B', x: -9, z: 2, w: 6.5, h: 5, color: '#1a1d24' },
      { id: 'corridor-main', shape: 'rect', name: 'Corredor Central', x: -2.5, z: -2.5, w: 10, h: 5, color: '#15171d' },
      { id: 'room-main-hall', shape: 'rect', name: 'Main Hall', x: 7.5, z: -6, w: 9, h: 12, color: '#1a1d24' }
    ];

    // Blueprint Routes (Dashed arrows showing navigation paths)
    this.routes = [
      { from: { x: -6, z: 0 }, to: { x: 2.5, z: 0 } },
      { from: { x: 2.5, z: 0 }, to: { x: 2.5, z: -4.5 } },
      { from: { x: 2.5, z: 0 }, to: { x: 2.5, z: 4.5 } },
      { from: { x: 2.5, z: 0 }, to: { x: 12, z: 0 } }
    ];

    this.initCanvas();
    this.initEvents();
    this.initInlineEditor();
    this.animate();
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
  }

  setEditMode(enabled) {
    this.isEditMode = enabled;
    if (!enabled) {
      this.selectedRoom = null;
      this.activeDragHandle = null;
      this.isDraggingRoom = false;
    }
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
    if (room.shape === 'rect') {
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

    this.rooms.push(newRoom);
    this.selectedRoom = newRoom;
    this.selectedEntity = null;
    this.isEditMode = true;

    const btnEdit = document.getElementById('btn-toggle-edit-mode');
    const labelEdit = document.getElementById('label-edit-mode');
    const iconEdit = document.getElementById('icon-edit-mode');
    if (btnEdit) btnEdit.classList.add('active');
    if (labelEdit) labelEdit.textContent = 'Editar Mapa';
    if (iconEdit) iconEdit.className = 'ti ti-edit';

    if (this.onSelectRoom) this.onSelectRoom(newRoom);
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

      // Explicit Navigation Mode or Middle Click -> Pan only
      if (!this.isEditMode || e.button === 1 || e.spaceKey) {
        this.isPanning = true;
        this.canvas.parentElement?.classList.add('is-panning');
        return;
      }

      // 1. Check Resize Handles on Selected Room (using 16px screen tolerance)
      if (this.selectedRoom) {
        const handle = this.hitTestHandles(this.selectedRoom, mouse);
        if (handle) {
          this.activeDragHandle = handle;
          this.roomInitialState = { ...this.selectedRoom };
          return;
        }
      }

      // 2. Check 3D Entities on Map
      const hitEntity = this.hitTestEntities(mouse.x, mouse.y);
      if (hitEntity) {
        this.selectedEntity = hitEntity;
        this.selectedRoom = null;
        if (this.onSelectEntity) this.onSelectEntity(hitEntity);
        if (this.onSelectRoom) this.onSelectRoom(null);
        return;
      }

      // 3. Check Blueprint Rooms (Direct Click on Room)
      const hitRoom = this.hitTestRooms(mouse.x, mouse.y);
      if (hitRoom) {
        const now = Date.now();
        if (now - lastClickTime < 350 && this.selectedRoom === hitRoom) {
          this.startInlineRename(hitRoom);
          return;
        }
        lastClickTime = now;

        this.selectedRoom = hitRoom;
        this.selectedEntity = null;
        this.isDraggingRoom = true;
        this.roomDragOffset = { x: mouse.x - hitRoom.x, z: mouse.z - hitRoom.z };
        if (this.onSelectRoom) this.onSelectRoom(hitRoom);
        return;
      }

      // Clicked on empty space -> Deselect & pan
      this.selectedRoom = null;
      this.selectedEntity = null;
      if (this.onSelectRoom) this.onSelectRoom(null);
      this.isPanning = true;
      this.canvas.parentElement?.classList.add('is-panning');
      this.cancelInlineRename();
    });

    window.addEventListener('mousemove', (e) => {
      const mouse = this.getMapPos(e);

      if (!isMouseDown) {
        if (this.isEditMode && this.selectedRoom) {
          this.hoveredHandle = this.hitTestHandles(this.selectedRoom, mouse);
          this.updateCursor(this.hoveredHandle);
        } else if (this.isEditMode) {
          const hoveredRoom = this.hitTestRooms(mouse.x, mouse.y);
          this.canvas.style.cursor = hoveredRoom ? 'pointer' : 'default';
        } else {
          this.canvas.style.cursor = 'grab';
        }
        this.lastMouse = { x: e.clientX, y: e.clientY };
        return;
      }

      const moveDist = Math.hypot(e.clientX - downPos.x, e.clientY - downPos.y);
      if (moveDist > 3) isDragging = true;

      // 1. Dragging Resize Handles
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
        if (this.onSelectRoom) this.onSelectRoom(room);
      }
      // 2. Dragging Room Position on Grid
      else if (this.isDraggingRoom && this.selectedRoom) {
        const newX = Math.round((mouse.x - this.roomDragOffset.x) * 2) / 2;
        const newZ = Math.round((mouse.z - this.roomDragOffset.z) * 2) / 2;
        this.selectedRoom.x = newX;
        this.selectedRoom.z = newZ;
        if (this.onSelectRoom) this.onSelectRoom(this.selectedRoom);
      }
      // 3. Panning Viewport
      else if (this.isPanning) {
        this.panX += e.clientX - this.lastMouse.x;
        this.panY += e.clientY - this.lastMouse.y;
      }

      this.lastMouse = { x: e.clientX, y: e.clientY };
    });

    window.addEventListener('mouseup', () => {
      isMouseDown = false;
      isDragging = false;
      this.isPanning = false;
      this.isDraggingRoom = false;
      this.activeDragHandle = null;
      this.canvas.parentElement?.classList.remove('is-panning');
      this.updateCursor(null);
    });

    // Zoom on wheel
    this.canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      const zoomFactor = e.deltaY < 0 ? 1.08 : 0.92;
      this.zoom = Math.max(0.4, Math.min(2.5, this.zoom * zoomFactor));
    }, { passive: false });

    // Keyboard Shortcuts (Delete room)
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;
      if (e.key === 'Delete' || e.key === 'Backspace') {
        if (this.selectedRoom) {
          this.rooms = this.rooms.filter(r => r.id !== this.selectedRoom.id);
          this.selectedRoom = null;
          if (this.onSelectRoom) this.onSelectRoom(null);
        }
      }
    });
  }

  updateCursor(handle) {
    if (!handle) {
      this.canvas.style.cursor = this.isEditMode ? 'default' : 'grab';
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

  hitTestRooms(worldX, worldZ) {
    for (let i = this.rooms.length - 1; i >= 0; i--) {
      const r = this.rooms[i];
      if (r.shape === 'circle') {
        if (Math.hypot(worldX - r.x, worldZ - r.z) <= r.radius + 0.3) return r;
      } else {
        const pad = 0.3;
        if (worldX >= r.x - pad && worldX <= r.x + r.w + pad && worldZ >= r.z - pad && worldZ <= r.z + r.h + pad) {
          return r;
        }
      }
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
    const hitTolerancePx = 16; // 16 CSS pixels radius for effortless grabbing!

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
      const rx = room.x * this.meterToPx;
      const rz = room.z * this.meterToPx;

      this.ctx.save();

      // CIRCULAR ROOM
      if (room.shape === 'circle') {
        const rad = room.radius * this.meterToPx;
        this.ctx.beginPath();
        this.ctx.arc(rx, rz, rad, 0, Math.PI * 2);
        this.ctx.fillStyle = room.color;
        this.ctx.fill();

        this.ctx.strokeStyle = isSel ? '#38bdf8' : '#333742';
        this.ctx.lineWidth = isSel ? 3 : 2;
        if (isSel) {
          this.ctx.shadowColor = '#38bdf8';
          this.ctx.shadowBlur = 12;
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

        this.ctx.fillStyle = room.color;
        this.ctx.fill();

        this.ctx.strokeStyle = isSel ? '#38bdf8' : '#333742';
        this.ctx.lineWidth = isSel ? 3 : 2;
        if (isSel) {
          this.ctx.shadowColor = '#38bdf8';
          this.ctx.shadowBlur = 12;
        }
        this.ctx.stroke();

        this.drawRoomLabel(room.name, rx + rw / 2, rz + rh * 0.65, isSel);
      }
      // RECTANGULAR ROOM (DEFAULT)
      else {
        const rw = room.w * this.meterToPx;
        const rh = room.h * this.meterToPx;

        this.ctx.fillStyle = room.color;
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

        this.ctx.strokeStyle = isSel ? '#38bdf8' : '#333742';
        this.ctx.lineWidth = isSel ? 3 : 2;
        if (isSel) {
          this.ctx.shadowColor = '#38bdf8';
          this.ctx.shadowBlur = 12;
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

    this.ctx.fillStyle = 'rgba(15, 17, 23, 0.85)';
    this.ctx.fillRect(cx - txtWidth / 2 - 6, cy - 8, txtWidth + 12, 16);

    this.ctx.fillStyle = isSel ? '#38bdf8' : 'rgba(255, 255, 255, 0.65)';
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText(text, cx, cy);
  }

  drawRoomHandles(room) {
    this.ctx.save();

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

    this.ctx.restore();
  }

  drawHandleBox(x, y) {
    this.ctx.fillStyle = '#38bdf8';
    this.ctx.strokeStyle = '#ffffff';
    this.ctx.lineWidth = 1.5;
    this.ctx.fillRect(x - 5, y - 5, 10, 10);
    this.ctx.strokeRect(x - 5, y - 5, 10, 10);
  }

  drawHandleCircle(x, y) {
    this.ctx.beginPath();
    this.ctx.arc(x, y, 5, 0, Math.PI * 2);
    this.ctx.fillStyle = '#ffffff';
    this.ctx.strokeStyle = '#38bdf8';
    this.ctx.lineWidth = 2;
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

      // 2. BLOCK / SOLID OBSTACLE
      if (ent.type === 'block') {
        const size = 1.8 * this.meterToPx;
        this.ctx.save();
        this.ctx.fillStyle = '#312e81';
        this.ctx.fillRect(px - size / 2, pz - size / 2, size, size);

        this.ctx.strokeStyle = isSelected ? '#ffffff' : '#a855f7';
        this.ctx.lineWidth = 2;
        this.ctx.strokeRect(px - size / 2, pz - size / 2, size, size);

        this.ctx.fillStyle = '#e0e7ff';
        this.ctx.font = 'bold 9px sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.textBaseline = 'middle';
        this.ctx.fillText(ent.sunName || 'Bloco', px, pz);
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
