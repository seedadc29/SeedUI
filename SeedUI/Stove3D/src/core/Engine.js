import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

export class Engine {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.container = this.canvas.parentElement;
    
    // Performance settings (Low-Spec PC Optimization)
    this.isLowSpecMode = true;
    this.devicePixelRatio = Math.min(window.devicePixelRatio, 1.25);
    this.currentShading = 'solid'; // 'solid' | 'wireframe' | 'pixel' | 'rendered'

    // FPS Counter variables
    this.frameCount = 0;
    this.lastFpsUpdate = performance.now();
    this.currentFps = 60;

    this.initScene();
    this.initRenderer();
    this.initCamera();
    this.initControls();
    this.initLights();
    this.initHelpers();

    this.bindEvents();
    this.startLoop();
  }

  initScene() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x282830); // Blender 4.1 Viewport Gray
  }

  initRenderer() {
    this.renderer = new THREE.WebGLRenderer({
      canvas: this.canvas,
      antialias: true,
      powerPreference: 'high-performance',
      precision: 'mediump'
    });

    this.renderer.setPixelRatio(this.devicePixelRatio);
    this.renderer.setSize(this.container.clientWidth, this.container.clientHeight);
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.05;
  }

  initCamera() {
    const aspect = this.container.clientWidth / this.container.clientHeight;
    
    // Perspective Camera
    this.cameraPersp = new THREE.PerspectiveCamera(45, aspect, 0.1, 500);
    this.cameraPersp.position.set(4.5, 3.5, 5.5);
    
    // Orthographic Camera
    const frustumSize = 8;
    this.cameraOrtho = new THREE.OrthographicCamera(
      (frustumSize * aspect) / -2,
      (frustumSize * aspect) / 2,
      frustumSize / 2,
      frustumSize / -2,
      0.1,
      500
    );
    this.cameraOrtho.position.set(4.5, 3.5, 5.5);

    this.activeCamera = this.cameraPersp;
    this.isOrthographic = false;
  }

  initControls() {
    this.controls = new OrbitControls(this.activeCamera, this.canvas);
    this.controls.enableDamping = true;
    this.controls.dampingFactor = 0.08;
    this.controls.screenSpacePanning = true;
    this.controls.target.set(0, 0, 0);

    // Emulate 3 Button Mouse (default enabled like Blender Emulate 3-Button)
    this.emulate3Button = localStorage.getItem('stove3d_emulate_3_button') !== 'false';
    this.updateNavControls();

    // CRITICAL: Intercept on POINTERDOWN with CAPTURE phase BEFORE OrbitControls runs
    const updateModifiers = (e) => {
      const isAlt = e.altKey || (e.key && e.key === 'Alt');
      const isShift = e.shiftKey || (e.key && e.key === 'Shift');
      const isCtrl = e.ctrlKey || (e.key && e.key === 'Control');

      if (this.emulate3Button && isAlt) {
        if (isCtrl) {
          this.controls.mouseButtons.LEFT = THREE.MOUSE.DOLLY;
        } else if (isShift) {
          this.controls.mouseButtons.LEFT = THREE.MOUSE.PAN;
        } else {
          this.controls.mouseButtons.LEFT = THREE.MOUSE.ROTATE;
        }
      } else if (e.button === 1) {
        if (isCtrl) {
          this.controls.mouseButtons.MIDDLE = THREE.MOUSE.DOLLY;
        } else if (isShift) {
          this.controls.mouseButtons.MIDDLE = THREE.MOUSE.PAN;
        } else {
          this.controls.mouseButtons.MIDDLE = THREE.MOUSE.ROTATE;
        }
      } else if (e.button === 0 && !isAlt) {
        this.controls.mouseButtons.LEFT = THREE.MOUSE.NONE;
      }
    };

    this.canvas.addEventListener('pointerdown', updateModifiers, { capture: true });
    this.canvas.addEventListener('mousedown', updateModifiers, { capture: true });

    // Dynamic modifier updates while Alt is held
    window.addEventListener('keydown', (e) => {
      updateModifiers(e);
    });

    window.addEventListener('keyup', (e) => {
      if (e.key === 'Alt' || !e.altKey) {
        this.controls.mouseButtons.LEFT = THREE.MOUSE.NONE;
      } else if (e.altKey) {
        updateModifiers(e);
      }
    });
  }

  setEmulate3Button(enabled) {
    this.emulate3Button = enabled;
    localStorage.setItem('stove3d_emulate_3_button', enabled ? 'true' : 'false');
    this.updateNavControls();
  }

  updateNavControls() {
    this.controls.mouseButtons = {
      LEFT: this.emulate3Button ? THREE.MOUSE.ROTATE : THREE.MOUSE.NONE,
      MIDDLE: THREE.MOUSE.ROTATE,
      RIGHT: THREE.MOUSE.NONE
    };
  }

  initLights() {
    // 1. Ambient Light (Soft base)
    this.ambientLight = new THREE.AmbientLight(0x404048, 1.0);
    this.scene.add(this.ambientLight);

    // 2. Main Key Sun Light (Blender Top-Front-Right)
    this.sunLight = new THREE.DirectionalLight(0xffffff, 1.25);
    this.sunLight.position.set(5, 8, 7);
    this.sunLight.castShadow = true;
    this.sunLight.shadow.mapSize.width = 1024;
    this.sunLight.shadow.mapSize.height = 1024;
    this.sunLight.shadow.bias = -0.0005;
    this.scene.add(this.sunLight);

    // 3. Fill Light (Left-Back)
    this.fillLight = new THREE.DirectionalLight(0xa5b8d0, 0.6);
    this.fillLight.position.set(-6, 4, -4);
    this.scene.add(this.fillLight);

    // 4. Top Rim Light (Top-Left)
    this.rimLight = new THREE.DirectionalLight(0xffffff, 0.4);
    this.rimLight.position.set(-3, 8, 4);
    this.scene.add(this.rimLight);
  }

  initHelpers() {
    // Ground Grid (Blender 4.x subtle gray grid)
    this.grid = new THREE.GridHelper(24, 24, 0x484856, 0x363640);
    this.grid.position.y = -0.001;
    this.scene.add(this.grid);

    // Origin Axes (Subtle red & green axes lines like Blender)
    this.axes = new THREE.AxesHelper(1.5);
    this.axes.position.y = 0.001;
    this.scene.add(this.axes);
  }

  bindEvents() {
    window.addEventListener('resize', () => this.onResize());
  }

  onResize() {
    const width = this.container.clientWidth;
    const height = this.container.clientHeight;
    const aspect = width / height;

    // Update Perspective
    this.cameraPersp.aspect = aspect;
    this.cameraPersp.updateProjectionMatrix();

    // Update Ortho
    const frustumSize = 8;
    this.cameraOrtho.left = (-frustumSize * aspect) / 2;
    this.cameraOrtho.right = (frustumSize * aspect) / 2;
    this.cameraOrtho.top = frustumSize / 2;
    this.cameraOrtho.bottom = -frustumSize / 2;
    this.cameraOrtho.updateProjectionMatrix();

    this.renderer.setSize(width, height);
  }

  toggleOrthographic() {
    this.isOrthographic = !this.isOrthographic;
    const target = this.controls.target.clone();
    const position = this.activeCamera.position.clone();

    if (this.isOrthographic) {
      this.cameraOrtho.position.copy(position);
      this.cameraOrtho.lookAt(target);
      this.activeCamera = this.cameraOrtho;
    } else {
      this.cameraPersp.position.copy(position);
      this.cameraPersp.lookAt(target);
      this.activeCamera = this.cameraPersp;
    }

    this.controls.object = this.activeCamera;
    this.controls.update();
    if (this.onCameraChange) this.onCameraChange(this.activeCamera);
    return this.isOrthographic;
  }

  setCameraView(viewName) {
    const dist = 8;
    const target = this.controls.target;

    switch (viewName) {
      case 'top': // Numpad 7
        this.activeCamera.position.set(target.x, target.y + dist, target.z + 0.001);
        break;
      case 'front': // Numpad 1
        this.activeCamera.position.set(target.x, target.y, target.z + dist);
        break;
      case 'right': // Numpad 3
        this.activeCamera.position.set(target.x + dist, target.y, target.z);
        break;
    }
    this.activeCamera.lookAt(target);
    this.controls.update();
  }

  focusOnObject(obj) {
    if (!obj) return;
    const box = new THREE.Box3().setFromObject(obj);
    const center = box.getCenter(new THREE.Vector3());
    const size = box.getSize(new THREE.Vector3());
    const maxDim = Math.max(size.x, size.y, size.z);

    this.controls.target.copy(center);
    this.activeCamera.position.set(center.x + maxDim * 2, center.y + maxDim * 1.5, center.z + maxDim * 2.5);
    this.controls.update();
  }

  setShadingMode(mode, meshes) {
    this.currentShading = mode;
    meshes.forEach((mesh) => {
      if (!mesh.material) return;

      if (mode === 'wireframe') {
        mesh.material.wireframe = true;
        mesh.material.needsUpdate = true;
      } else {
        mesh.material.wireframe = false;
        if (mode === 'pixel') {
          // Nearest neighbor / pixelated style
          if (mesh.material.map) {
            mesh.material.map.minFilter = THREE.NearestFilter;
            mesh.material.map.magFilter = THREE.NearestFilter;
            mesh.material.map.needsUpdate = true;
          }
        }
        mesh.material.needsUpdate = true;
      }
    });

    if (mode === 'rendered') {
      this.renderer.shadowMap.enabled = true;
    } else {
      this.renderer.shadowMap.enabled = !this.isLowSpecMode;
    }
  }

  setLowSpecMode(enabled) {
    this.isLowSpecMode = enabled;
    this.devicePixelRatio = enabled ? 1.0 : Math.min(window.devicePixelRatio, 1.5);
    this.renderer.setPixelRatio(this.devicePixelRatio);
    this.renderer.shadowMap.enabled = !enabled;
    this.onResize();
  }

  startLoop() {
    const animate = (timestamp) => {
      requestAnimationFrame(animate);

      // Update FPS counter
      this.frameCount++;
      if (timestamp - this.lastFpsUpdate >= 500) {
        this.currentFps = Math.round((this.frameCount * 1000) / (timestamp - this.lastFpsUpdate));
        this.frameCount = 0;
        this.lastFpsUpdate = timestamp;
        
        const fpsEl = document.getElementById('stat-fps');
        if (fpsEl) fpsEl.textContent = `${this.currentFps} FPS`;
      }

      this.controls.update();
      this.renderer.render(this.scene, this.activeCamera);
    };

    requestAnimationFrame(animate);
  }
}
