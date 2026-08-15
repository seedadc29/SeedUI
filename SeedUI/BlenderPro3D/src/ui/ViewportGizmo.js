import * as THREE from 'three';

/**
 * ViewportGizmo - Authentic Blender 4.x Navigation Gizmo System
 * 
 * Features:
 * 1. 3D Interactive Orientation Compass (Interactive X/Y/Z spheres with depth sorting)
 * 2. Click axis bubble to snap to orthogonal view (Top, Front, Right, Back, Left, Bottom)
 * 3. Drag compass to orbit view
 * 4. Zoom button (drag up/down to zoom)
 * 5. Pan button (drag to pan)
 * 6. Camera toggle button (enter/exit scene camera)
 * 7. Grid toggle button (Perspective / Orthographic)
 */
export class ViewportGizmo {
  constructor(engine) {
    this.engine = engine;
    this.container = document.getElementById('viewport-container');

    this.axes = [
      { name: 'x', label: 'X', dir: new THREE.Vector3(1, 0, 0), color: '#e03838', oppColor: 'rgba(224, 56, 56, 0.35)', view: 'right', oppView: 'left' },
      { name: 'y', label: 'Y', dir: new THREE.Vector3(0, 1, 0), color: '#44b544', oppColor: 'rgba(68, 181, 68, 0.35)', view: 'back', oppView: 'front' },
      { name: 'z', label: 'Z', dir: new THREE.Vector3(0, 0, 1), color: '#3882e0', oppColor: 'rgba(56, 130, 224, 0.35)', view: 'top', oppView: 'bottom' }
    ];

    this.initElements();
    this.bindEvents();
    this.startRenderLoop();
  }

  initElements() {
    // 1. Compass Canvas Overlay
    this.canvas = document.getElementById('nav-gizmo-canvas');
    if (!this.canvas) {
      this.canvas = document.createElement('canvas');
      this.canvas.id = 'nav-gizmo-canvas';
      this.canvas.width = 100;
      this.canvas.height = 100;
      this.canvas.className = 'blender-nav-compass-canvas';
      this.container.appendChild(this.canvas);
    }
    this.ctx = this.canvas.getContext('2d');
  }

  bindEvents() {
    let isDragging = false;
    let lastX = 0, lastY = 0;

    // Compass interactions
    this.canvas.addEventListener('mousedown', (e) => {
      e.stopPropagation();
      const rect = this.canvas.getBoundingClientRect();
      const clickX = e.clientX - rect.left;
      const clickY = e.clientY - rect.top;

      // Check if clicked an axis bubble
      const hitAxis = this.hitTest(clickX, clickY);
      if (hitAxis) {
        this.snapToView(hitAxis.viewName);
        return;
      }

      isDragging = true;
      lastX = e.clientX;
      lastY = e.clientY;
    });

    window.addEventListener('mousemove', (e) => {
      if (!isDragging) return;
      const deltaX = e.clientX - lastX;
      const deltaY = e.clientY - lastY;
      lastX = e.clientX;
      lastY = e.clientY;

      // Orbit camera with compass drag
      const rotSpeed = 0.005;
      const offset = new THREE.Vector3().subVectors(this.engine.activeCamera.position, this.engine.controls.target);
      
      const phi = Math.atan2(Math.hypot(offset.x, offset.y), offset.z);
      const theta = Math.atan2(offset.y, offset.x);

      const newTheta = theta - deltaX * rotSpeed;
      const newPhi = Math.max(0.01, Math.min(Math.PI - 0.01, phi + deltaY * rotSpeed));
      const radius = offset.length();

      offset.x = radius * Math.sin(newPhi) * Math.cos(newTheta);
      offset.y = radius * Math.sin(newPhi) * Math.sin(newTheta);
      offset.z = radius * Math.cos(newPhi);

      this.engine.activeCamera.position.copy(this.engine.controls.target).add(offset);
      this.engine.activeCamera.lookAt(this.engine.controls.target);
      this.engine.controls.update();
    });

    window.addEventListener('mouseup', () => {
      isDragging = false;
    });

    // 2. Zoom Button (Drag up/down)
    const btnZoom = document.getElementById('nav-btn-zoom');
    if (btnZoom) {
      let isZooming = false;
      let startY = 0;
      btnZoom.addEventListener('mousedown', (e) => {
        e.preventDefault();
        e.stopPropagation();
        isZooming = true;
        startY = e.clientY;
        document.body.style.cursor = 'ns-resize';
      });

      window.addEventListener('mousemove', (e) => {
        if (!isZooming) return;
        const delta = (e.clientY - startY) * 0.02;
        startY = e.clientY;

        const offset = new THREE.Vector3().subVectors(this.engine.activeCamera.position, this.engine.controls.target);
        offset.multiplyScalar(1 + delta);
        if (offset.length() > 0.5 && offset.length() < 300) {
          this.engine.activeCamera.position.copy(this.engine.controls.target).add(offset);
          this.engine.controls.update();
        }
      });

      window.addEventListener('mouseup', () => {
        if (isZooming) {
          isZooming = false;
          document.body.style.cursor = 'default';
        }
      });
    }

    // 3. Pan Button (Drag)
    const btnPan = document.getElementById('nav-btn-pan');
    if (btnPan) {
      let isPanning = false;
      let lastPanX = 0, lastPanY = 0;
      btnPan.addEventListener('mousedown', (e) => {
        e.preventDefault();
        e.stopPropagation();
        isPanning = true;
        lastPanX = e.clientX;
        lastPanY = e.clientY;
        document.body.style.cursor = 'move';
      });

      window.addEventListener('mousemove', (e) => {
        if (!isPanning) return;
        const deltaX = (e.clientX - lastPanX) * 0.015;
        const deltaY = (e.clientY - lastPanY) * 0.015;
        lastPanX = e.clientX;
        lastPanY = e.clientY;

        const cam = this.engine.activeCamera;
        const right = new THREE.Vector3(1, 0, 0).applyQuaternion(cam.quaternion);
        const up = new THREE.Vector3(0, 1, 0).applyQuaternion(cam.quaternion);

        const panOffset = right.multiplyScalar(-deltaX).add(up.multiplyScalar(deltaY));
        cam.position.add(panOffset);
        this.engine.controls.target.add(panOffset);
        this.engine.controls.update();
      });

      window.addEventListener('mouseup', () => {
        if (isPanning) {
          isPanning = false;
          document.body.style.cursor = 'default';
        }
      });
    }

    // 4. Camera View Toggle Button
    document.getElementById('nav-btn-camera')?.addEventListener('click', () => {
      this.engine.setCameraView('camera');
    });

    // 5. Perspective / Orthographic Switcher Button
    document.getElementById('nav-btn-ortho')?.addEventListener('click', () => {
      this.engine.toggleOrthographic();
    });
  }

  snapToView(viewName) {
    switch (viewName) {
      case 'top': this.engine.setCameraView('top'); break;
      case 'bottom':
        this.engine.activeCamera.up.set(0, 1, 0);
        this.engine.activeCamera.position.set(this.engine.controls.target.x, this.engine.controls.target.y, this.engine.controls.target.z - 8);
        this.engine.controls.update();
        break;
      case 'front': this.engine.setCameraView('front'); break;
      case 'back':
        this.engine.activeCamera.up.set(0, 0, 1);
        this.engine.activeCamera.position.set(this.engine.controls.target.x, this.engine.controls.target.y + 8, this.engine.controls.target.z);
        this.engine.controls.update();
        break;
      case 'right': this.engine.setCameraView('right'); break;
      case 'left':
        this.engine.activeCamera.up.set(0, 0, 1);
        this.engine.activeCamera.position.set(this.engine.controls.target.x - 8, this.engine.controls.target.y, this.engine.controls.target.z);
        this.engine.controls.update();
        break;
    }
  }

  hitTest(clickX, clickY) {
    if (!this.screenPoles) return null;
    for (const pole of this.screenPoles) {
      const dist = Math.hypot(clickX - pole.x, clickY - pole.y);
      if (dist <= pole.radius + 3) {
        return pole;
      }
    }
    return null;
  }

  startRenderLoop() {
    const render = () => {
      this.drawCompass();
      requestAnimationFrame(render);
    };
    requestAnimationFrame(render);
  }

  drawCompass() {
    const ctx = this.ctx;
    const w = this.canvas.width;
    const h = this.canvas.height;
    const cx = w / 2;
    const cy = h / 2;
    const radius = 32;

    ctx.clearRect(0, 0, w, h);

    // Get camera inverse rotation matrix
    const camera = this.engine.activeCamera;
    const rotMatrix = new THREE.Matrix4().extractRotation(camera.matrixWorldInverse);

    const poles = [];

    // Calculate projected 2D coordinates for all 6 poles
    this.axes.forEach((axis) => {
      // Positive pole
      const posVec = axis.dir.clone().applyMatrix4(rotMatrix);
      poles.push({
        x: cx + posVec.x * radius,
        y: cy - posVec.y * radius,
        z: posVec.z,
        label: axis.label,
        color: axis.color,
        isPositive: true,
        viewName: axis.view,
        radius: 8.5
      });

      // Negative pole
      const negVec = axis.dir.clone().negate().applyMatrix4(rotMatrix);
      poles.push({
        x: cx + negVec.x * radius,
        y: cy - negVec.y * radius,
        z: negVec.z,
        label: `-${axis.label}`,
        color: axis.oppColor,
        isPositive: false,
        viewName: axis.oppView,
        radius: 6.5
      });
    });

    // Depth sort poles from back (lowest z) to front (highest z)
    poles.sort((a, b) => a.z - b.z);
    this.screenPoles = poles;

    // 1. Draw back poles and center connection lines
    poles.forEach((pole) => {
      if (!pole.isPositive) {
        // Draw negative hollow/dim ring
        ctx.beginPath();
        ctx.arc(pole.x, pole.y, pole.radius, 0, Math.PI * 2);
        ctx.fillStyle = 'rgba(40, 40, 40, 0.6)';
        ctx.fill();
        ctx.strokeStyle = pole.color;
        ctx.lineWidth = 1.5;
        ctx.stroke();
      } else {
        // Draw axis line from center
        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.lineTo(pole.x, pole.y);
        ctx.strokeStyle = pole.color;
        ctx.lineWidth = 2.5;
        ctx.stroke();

        // Draw solid positive sphere with letter
        ctx.beginPath();
        ctx.arc(pole.x, pole.y, pole.radius, 0, Math.PI * 2);
        ctx.fillStyle = pole.color;
        ctx.fill();
        ctx.strokeStyle = '#ffffff';
        ctx.lineWidth = 0.8;
        ctx.stroke();

        // Letter label
        ctx.fillStyle = '#ffffff';
        ctx.font = 'bold 9px -apple-system, Inter, sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(pole.label, pole.x, pole.y);
      }
    });
  }
}
