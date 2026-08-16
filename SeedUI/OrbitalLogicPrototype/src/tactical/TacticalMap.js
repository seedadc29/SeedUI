export class TacticalMap {
  constructor(canvas, scene3D = null, orbitalGraph = null, onSelectEntity = null) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.scene3D = scene3D;
    this.orbitalGraph = orbitalGraph;
    this.onSelectEntity = onSelectEntity;

    // Viewport transform
    this.panX = 0;
    this.panY = 0;
    this.zoom = 1.0;
    this.isPanning = false;
    this.lastMouse = { x: 0, y: 0 };

    // Interactive selection
    this.selectedItem = null;
    this.hoveredItem = null;
    this.animTime = 0;

    // Scale conversion: 1 meter in 3D = 20 pixels on Tactical Map
    this.meterToPx = 22;

    // Predefined Tactical Level Blueprint Layout (Corridors, Rooms)
    this.rooms = [
      { id: 'room-armory-1', name: 'Armory A', x: -8, z: -7, w: 6, h: 5, color: '#1a1d24' },
      { id: 'room-armory-2', name: 'Armory B', x: -8, z: 2, w: 6, h: 5, color: '#1a1d24' },
      { id: 'corridor-main', name: 'Corredor Central', x: -2, z: -2.5, w: 10, h: 5, color: '#15171d' },
      { id: 'room-main-hall', name: 'Main Hall', x: 8, z: -6, w: 9, h: 12, color: '#1a1d24' }
    ];

    // Blueprint Routes (Dashed arrows showing player navigation paths)
    this.routes = [
      { from: { x: -6, z: 0 }, to: { x: 3, z: 0 } },
      { from: { x: 3, z: 0 }, to: { x: 3, z: -4.5 } },
      { from: { x: 3, z: 0 }, to: { x: 3, z: 4.5 } },
      { from: { x: 3, z: 0 }, to: { x: 12, z: 0 } }
    ];

    this.initCanvas();
    this.initEvents();
    this.animate();
  }

  initCanvas() {
    this.resize();
    window.addEventListener('resize', () => this.resize());
  }

  resize() {
    if (!this.canvas.parentElement) return;
    const parent = this.canvas.parentElement;
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    const width = parent.clientWidth;
    const height = parent.clientHeight;
    if (width === 0 || height === 0) return;

    this.canvas.width = width * dpr;
    this.canvas.height = height * dpr;
    this.ctx.scale(dpr, dpr);

    if (this.panX === 0 && this.panY === 0) {
      this.panX = width / 2;
      this.panY = height / 2;
    }
  }

  initEvents() {
    let isMouseDown = false;
    let downPos = { x: 0, y: 0 };
    let isDragging = false;

    this.canvas.addEventListener('mousedown', (e) => {
      if (e.button !== 0 && e.button !== 1) return;
      isMouseDown = true;
      isDragging = false;
      downPos = { x: e.clientX, y: e.clientY };
      this.lastMouse = { x: e.clientX, y: e.clientY };

      const mouse = this.getMapPos(e);
      const hit = this.hitTest(mouse.x, mouse.y);
      if (hit) {
        this.selectedItem = hit;
        if (this.onSelectEntity) this.onSelectEntity(hit);
      } else {
        this.selectedItem = null;
        this.isPanning = true;
        this.canvas.parentElement?.classList.add('is-panning');
      }
    });

    window.addEventListener('mousemove', (e) => {
      if (!isMouseDown) {
        const mouse = this.getMapPos(e);
        this.hoveredItem = this.hitTest(mouse.x, mouse.y);
        this.lastMouse = { x: e.clientX, y: e.clientY };
        return;
      }

      const moveDist = Math.hypot(e.clientX - downPos.x, e.clientY - downPos.y);
      if (moveDist > 4) isDragging = true;

      if (this.isPanning) {
        this.panX += e.clientX - this.lastMouse.x;
        this.panY += e.clientY - this.lastMouse.y;
      }

      this.lastMouse = { x: e.clientX, y: e.clientY };
    });

    window.addEventListener('mouseup', () => {
      isMouseDown = false;
      isDragging = false;
      this.isPanning = false;
      this.canvas.parentElement?.classList.remove('is-panning');
    });

    // Zoom
    this.canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      const zoomFactor = e.deltaY < 0 ? 1.08 : 0.92;
      this.zoom = Math.max(0.5, Math.min(2.5, this.zoom * zoomFactor));
    }, { passive: false });
  }

  getMapPos(e) {
    const rect = this.canvas.getBoundingClientRect();
    const clientX = e.clientX - rect.left;
    const clientY = e.clientY - rect.top;
    return {
      x: ((clientX - this.panX) / this.zoom) / this.meterToPx,
      z: ((clientY - this.panY) / this.zoom) / this.meterToPx
    };
  }

  hitTest(worldX, worldZ) {
    if (!this.scene3D) return null;

    let closest = null;
    let minDist = 1.5;

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

  update(delta) {
    this.animTime += delta;
  }

  draw() {
    if (!this.canvas.parentElement) return;
    const w = this.canvas.parentElement.clientWidth;
    const h = this.canvas.parentElement.clientHeight;
    if (w === 0 || h === 0) return;

    this.ctx.clearRect(0, 0, w, h);

    this.ctx.save();
    this.ctx.translate(this.panX, this.panY);
    this.ctx.scale(this.zoom, this.zoom);

    // 1. Draw Blueprint Background Grid
    this.drawGrid();

    // 2. Draw Blueprint Rooms and Corridors
    this.drawRooms();

    // 3. Draw Route Directional Paths
    this.drawRoutes();

    // 4. Draw Trigger Action Link Lines (Trigger -> Target Door/Block)
    this.drawTriggerActionLinks();

    // 5. Draw 3D Entities on Map (Player, Enemies, Blocks, Triggers)
    this.drawEntities();

    this.ctx.restore();

    // 6. Draw Map Legend & HUD Overlay (in screen coordinates)
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

    // Origin cross
    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.15)';
    this.ctx.beginPath();
    this.ctx.moveTo(-40, 0); this.ctx.lineTo(40, 0);
    this.ctx.moveTo(0, -40); this.ctx.lineTo(0, 40);
    this.ctx.stroke();
    this.ctx.restore();
  }

  drawRooms() {
    this.rooms.forEach((room) => {
      const rx = room.x * this.meterToPx;
      const rz = room.z * this.meterToPx;
      const rw = room.w * this.meterToPx;
      const rh = room.h * this.meterToPx;

      // Floor
      this.ctx.fillStyle = room.color;
      this.ctx.fillRect(rx, rz, rw, rh);

      // Floor Grid Pattern
      this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
      this.ctx.lineWidth = 1;
      for (let tx = rx; tx <= rx + rw; tx += this.meterToPx / 2) {
        this.ctx.beginPath();
        this.ctx.moveTo(tx, rz);
        this.ctx.lineTo(tx, rz + rh);
        this.ctx.stroke();
      }

      // Walls Outer Border
      this.ctx.strokeStyle = '#333742';
      this.ctx.lineWidth = 3;
      this.ctx.strokeRect(rx, rz, rw, rh);

      // Room Title
      this.ctx.fillStyle = 'rgba(255, 255, 255, 0.35)';
      this.ctx.font = 'bold 11px sans-serif';
      this.ctx.textAlign = 'center';
      this.ctx.textBaseline = 'middle';
      this.ctx.fillText(room.name, rx + rw / 2, rz + rh / 2);
    });
  }

  drawRoutes() {
    this.ctx.save();
    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.4)';
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

      // Draw arrowhead
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

    // Draw connecting action flow line from trigger to target blocks
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

        // Traveling energy pulse dot
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
      const isSelected = this.scene3D.selectedEntity === ent;

      // 1. TRIGGER ZONE (Orange Glowing Box)
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

      // 2. BLOCK / SOLID OBSTACLE (Indigo Block)
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

      // 3. ENEMY (Red Dot)
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

      // 4. PLAYER (Cyan Dot with Heading Indicator)
      if (ent.type === 'player') {
        this.ctx.save();
        // Pulsing radar ring
        const pulse = (this.animTime * 2) % 1.0;
        this.ctx.beginPath();
        this.ctx.arc(px, pz, 12 + pulse * 14, 0, Math.PI * 2);
        this.ctx.strokeStyle = `rgba(50, 173, 230, ${1 - pulse})`;
        this.ctx.lineWidth = 1.5;
        this.ctx.stroke();

        // Player Core
        this.ctx.beginPath();
        this.ctx.arc(px, pz, 11, 0, Math.PI * 2);
        this.ctx.fillStyle = '#32ade6';
        this.ctx.shadowColor = '#32ade6';
        this.ctx.shadowBlur = 15;
        this.ctx.fill();

        this.ctx.strokeStyle = '#ffffff';
        this.ctx.lineWidth = 2;
        this.ctx.stroke();

        // Heading direction arrow
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

    // Background card
    this.ctx.fillStyle = 'rgba(18, 20, 26, 0.88)';
    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.12)';
    this.ctx.lineWidth = 1;
    this.ctx.fillRect(lx, ly, 135, 95);
    this.ctx.strokeRect(lx, ly, 135, 95);

    this.ctx.fillStyle = '#94a3b8';
    this.ctx.font = 'bold 10px sans-serif';
    this.ctx.fillText('LEGENDA TÁTICA', lx + 10, ly + 16);

    // Player item
    this.ctx.beginPath();
    this.ctx.arc(lx + 16, ly + 34, 5, 0, Math.PI * 2);
    this.ctx.fillStyle = '#32ade6';
    this.ctx.fill();
    this.ctx.fillStyle = '#ffffff';
    this.ctx.font = '10px sans-serif';
    this.ctx.fillText('Player (Posição)', lx + 30, ly + 37);

    // Obstacle item
    this.ctx.fillStyle = '#a855f7';
    this.ctx.fillRect(lx + 11, ly + 47, 10, 10);
    this.ctx.fillStyle = '#ffffff';
    this.ctx.fillText('Bloco / Parede', lx + 30, ly + 56);

    // Trigger item
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
