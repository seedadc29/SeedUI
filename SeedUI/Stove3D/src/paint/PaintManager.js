import * as THREE from 'three';

/**
 * PaintManager implements direct 3D mesh surface painting using dynamic GPU CanvasTextures.
 * Optimized for Intel HD/integrated graphics with zero performance overhead.
 */
export class PaintManager {
  constructor(engine, sceneManager, historyManager) {
    this.engine = engine;
    this.sceneManager = sceneManager;
    this.historyManager = historyManager;

    this.isPainting = false;
    this.brushColor = '#ff3b30';
    this.brushSize = 16;
    this.brushOpacity = 1.0;
    this.isPixelMode = false;
    this.brushMode = 'draw'; // 'draw' | 'erase'

    this.raycaster = new THREE.Raycaster();
    this.mouse = new THREE.Vector2();
    this.lastUV = null;

    // Cache of dynamic canvases per mesh
    this.canvasMap = new Map(); // mesh -> { canvas, ctx, texture }

    this.initPaintListeners();
  }

  getOrCreateMeshCanvas(mesh, resolution = 512) {
    if (this.canvasMap.has(mesh)) {
      return this.canvasMap.get(mesh);
    }

    const canvas = document.createElement('canvas');
    canvas.width = resolution;
    canvas.height = resolution;
    const ctx = canvas.getContext('2d', { willReadFrequently: true });

    // Fill with default material base color
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, resolution, resolution);

    const texture = new THREE.CanvasTexture(canvas);
    texture.colorSpace = THREE.SRGBColorSpace;
    texture.generateMipmaps = false;
    texture.minFilter = this.isPixelMode ? THREE.NearestFilter : THREE.LinearFilter;
    texture.magFilter = this.isPixelMode ? THREE.NearestFilter : THREE.LinearFilter;

    // Assign texture to mesh material
    if (mesh.material) {
      mesh.material.map = texture;
      mesh.material.roughness = 0.7;
      mesh.material.metalness = 0.05;
      mesh.material.needsUpdate = true;
    }

    const entry = { canvas, ctx, texture, resolution };
    this.canvasMap.set(mesh, entry);
    return entry;
  }

  setBrushColor(color) {
    this.brushColor = color;
  }

  setBrushSize(size) {
    this.brushSize = Math.max(1, Math.min(128, size));
  }

  setBrushOpacity(opacity) {
    this.brushOpacity = Math.max(0.05, Math.min(1.0, opacity));
  }

  setPixelMode(enabled) {
    this.isPixelMode = enabled;
    this.canvasMap.forEach(({ texture }) => {
      texture.minFilter = enabled ? THREE.NearestFilter : THREE.LinearFilter;
      texture.magFilter = enabled ? THREE.NearestFilter : THREE.LinearFilter;
      texture.needsUpdate = true;
    });
  }

  setBrushMode(mode) {
    this.brushMode = mode; // 'draw' | 'erase'
  }

  initPaintListeners() {
    const canvas = this.engine.canvas;

    canvas.addEventListener('mousedown', (e) => {
      if (this.engine.currentMode !== 'paint' && this.mode !== 'paint') return;
      if (e.button !== 0) return; // Left click only

      this.isPainting = true;
      this.lastUV = null;
      this.paintAtMouse(e.clientX, e.clientY);
    });

    window.addEventListener('mousemove', (e) => {
      if (!this.isPainting) return;
      this.paintAtMouse(e.clientX, e.clientY);
    });

    window.addEventListener('mouseup', () => {
      if (this.isPainting) {
        this.isPainting = false;
        this.lastUV = null;
      }
    });
  }

  paintAtMouse(clientX, clientY) {
    const canvas = this.engine.canvas;
    const rect = canvas.getBoundingClientRect();
    this.mouse.x = ((clientX - rect.left) / rect.width) * 2 - 1;
    this.mouse.y = -((clientY - rect.top) / rect.height) * 2 + 1;
    this.raycaster.setFromCamera(this.mouse, this.engine.activeCamera);

    const meshes = this.sceneManager.getAllMeshes().filter(m => m.visible);
    const intersects = this.raycaster.intersectObjects(meshes, false);

    if (intersects.length > 0 && intersects[0].uv) {
      const hit = intersects[0];
      const mesh = hit.object;
      const uv = hit.uv;

      this.paintOnMeshUV(mesh, uv);
    }
  }

  paintOnMeshUV(mesh, uv) {
    const { canvas, ctx, texture, resolution } = this.getOrCreateMeshCanvas(mesh);

    const x = uv.x * resolution;
    const y = (1 - uv.y) * resolution; // Three.js UV Y is flipped

    ctx.save();
    if (this.brushMode === 'erase') {
      ctx.fillStyle = '#ffffff';
    } else {
      ctx.fillStyle = this.brushColor;
    }
    ctx.globalAlpha = this.brushOpacity;

    if (this.isPixelMode) {
      // Crisp pixel block
      const pSize = Math.max(2, Math.round(this.brushSize / 4));
      const px = Math.floor(x / pSize) * pSize;
      const py = Math.floor(y / pSize) * pSize;
      ctx.fillRect(px, py, pSize, pSize);
    } else {
      // Smooth round dab
      ctx.beginPath();
      ctx.arc(x, y, this.brushSize / 2, 0, Math.PI * 2);
      ctx.fill();

      // Stroke interpolation to prevent gaps when mouse moves fast
      if (this.lastUV) {
        const lx = this.lastUV.x * resolution;
        const ly = (1 - this.lastUV.y) * resolution;
        ctx.lineWidth = this.brushSize;
        ctx.lineCap = 'round';
        ctx.strokeStyle = this.brushMode === 'erase' ? '#ffffff' : this.brushColor;
        ctx.beginPath();
        ctx.moveTo(lx, ly);
        ctx.lineTo(x, y);
        ctx.stroke();
      }
    }

    ctx.restore();
    texture.needsUpdate = true;
    this.lastUV = uv.clone();
  }

  clearTexture(mesh) {
    if (!mesh) mesh = this.sceneManager.getSelectedObject();
    if (!mesh) return;

    const { canvas, ctx, texture, resolution } = this.getOrCreateMeshCanvas(mesh);
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, resolution, resolution);
    texture.needsUpdate = true;
  }
}
