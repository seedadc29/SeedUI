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

    // Blender Action Mapping:
    // Left Button = Rotate (0)
    // Middle Button = Rotate (0)
    // Right Button = Pan (2)
    this.controls.mouseButtons = {
      LEFT: THREE.MOUSE.ROTATE,
      MIDDLE: THREE.MOUSE.ROTATE,
      RIGHT: THREE.MOUSE.PAN
    };

    // Emulate 3 Button Mouse (default enabled like Blender Emulate 3-Button)
    this.emulate3Button = localStorage.getItem('stove3d_emulate_3_button') !== 'false';

    // State for MMB + RMB Pan & Context Menu Suppression
    this.isMMBRMBPanning = false;
    this.lastMMBRMBPanTime = 0;
    let isMMBDown = false;
    let isRMBDown = false;
    let lastPanPos = { x: 0, y: 0 };

    // Dedicated smooth Ctrl+Alt+LMB Zoom drag state
    let isCtrlAltZooming = false;
    let lastZoomY = 0;

    // Gate OrbitControls activation:
    // - With Alt: Left click triggers Rotate, Shift+Alt triggers Pan, Ctrl+Alt triggers Zoom
    // - MMB + RMB: Triggers Pan and suppresses context menu
    // - Without Alt: Left click is reserved for Selection
    // - Middle button alone: Always rotates
    const onPointerDownGate = (e) => {
      if (e.button === 1) isMMBDown = true;
      if (e.button === 2) isRMBDown = true;

      // Check MMB + RMB simultaneous press for Pan
      if ((isMMBDown && isRMBDown) || ((e.buttons & 4) && (e.buttons & 2))) {
        this.isMMBRMBPanning = true;
        this.lastMMBRMBPanTime = Date.now();
        lastPanPos.x = e.clientX;
        lastPanPos.y = e.clientY;
        this.controls.enabled = false;
        return;
      }

      if (e.button === 0) { // Left Mouse Button
        if (this.emulate3Button && e.altKey) {
          if (e.ctrlKey) {
            // Dedicated Ctrl+Alt Zoom / Dolly
            isCtrlAltZooming = true;
            lastZoomY = e.clientY;
            this.controls.enabled = false;
          } else {
            // Alt = Rotate, Shift+Alt = Pan
            isCtrlAltZooming = false;
            this.controls.enabled = true;
          }
        } else {
          // Without Alt -> Left click is reserved for Selection
          isCtrlAltZooming = false;
          this.controls.enabled = false;
        }
      } else if (e.button === 1) { // Middle Mouse Button
        isCtrlAltZooming = false;
        if (!isRMBDown) {
          this.controls.enabled = true;
        }
      }
    };

    const onPointerMove = (e) => {
      const isBothDown = (isMMBDown && isRMBDown) || ((e.buttons & 4) && (e.buttons & 2));

      // 1. MMB + RMB Pan (Smooth and Jump-Free)
      if (isBothDown) {
        if (!this.isMMBRMBPanning) {
          this.isMMBRMBPanning = true;
          this.lastMMBRMBPanTime = Date.now();
          lastPanPos.x = e.clientX;
          lastPanPos.y = e.clientY;
          this.controls.enabled = false;
          return;
        }

        this.lastMMBRMBPanTime = Date.now();
        const deltaX = e.clientX - lastPanPos.x;
        const deltaY = e.clientY - lastPanPos.y;
        lastPanPos.x = e.clientX;
        lastPanPos.y = e.clientY;

        // Prevent jump spikes
        if (Math.abs(deltaX) > 80 || Math.abs(deltaY) > 80) {
          return;
        }

        if (deltaX !== 0 || deltaY !== 0) {
          const cam = this.activeCamera;
          const target = this.controls.target;
          const dist = cam.position.distanceTo(target);
          
          let panSpeed = 0.0015 * dist;
          if (cam.isOrthographicCamera) {
            panSpeed = 0.005 / cam.zoom;
          }

          const right = new THREE.Vector3();
          const up = new THREE.Vector3();
          cam.matrix.extractBasis(right, up, new THREE.Vector3());

          const panOffset = new THREE.Vector3()
            .addScaledVector(right, -deltaX * panSpeed)
            .addScaledVector(up, deltaY * panSpeed);

          cam.position.add(panOffset);
          target.add(panOffset);
          this.controls.update();
        }
        return;
      }

      // 2. Ctrl + Alt + LMB Zoom
      if (isCtrlAltZooming) {
        const deltaY = e.clientY - lastZoomY;
        lastZoomY = e.clientY;

        if (deltaY !== 0) {
          const factor = Math.pow(0.992, -deltaY);
          const cam = this.activeCamera;
          const offset = new THREE.Vector3().subVectors(cam.position, this.controls.target);
          
          if (cam.isPerspectiveCamera) {
            offset.multiplyScalar(factor);
            if (offset.length() > 0.2 && offset.length() < 1000) {
              cam.position.copy(this.controls.target).add(offset);
            }
          } else if (cam.isOrthographicCamera) {
            cam.zoom = Math.max(0.1, Math.min(50, cam.zoom / factor));
            cam.updateProjectionMatrix();
          }
          this.controls.update();
        }
      }
    };

    const onPointerUpGate = (e) => {
      if (e.button === 1) isMMBDown = false;
      if (e.button === 2) isRMBDown = false;

      if (this.isMMBRMBPanning) {
        this.isMMBRMBPanning = false;
        this.lastMMBRMBPanTime = Date.now();
      }

      if (isCtrlAltZooming) {
        isCtrlAltZooming = false;
      }

      if (!isMMBDown && !isRMBDown) {
        this.controls.enabled = true;
      }
    };

    this.canvas.addEventListener('pointerdown', onPointerDownGate, { capture: true });
    window.addEventListener('pointermove', onPointerMove, { capture: true });
    window.addEventListener('pointerup', onPointerUpGate, { capture: true });
  }

  setEmulate3Button(enabled) {
    this.emulate3Button = enabled;
    localStorage.setItem('stove3d_emulate_3_button', enabled ? 'true' : 'false');
  }

  updateNavControls() {
    this.controls.mouseButtons = {
      LEFT: THREE.MOUSE.ROTATE,
      MIDDLE: THREE.MOUSE.ROTATE,
      RIGHT: THREE.MOUSE.PAN
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
        mesh.material.flatShading = true;
      } else if (mode === 'solid') {
        // Modo Sólido / Modelagem: Facetas planas com contraste nítido em todas as faces
        mesh.material.wireframe = false;
        mesh.material.flatShading = true;
      } else if (mode === 'material' || mode === 'pixel') {
        // Previsão de Material: Cor pura e textura do material sem render/sombras escuras
        mesh.material.wireframe = false;
        mesh.material.flatShading = (mesh.userData.shading === 'flat');
        if (mode === 'pixel' && mesh.material.map) {
          mesh.material.map.minFilter = THREE.NearestFilter;
          mesh.material.map.magFilter = THREE.NearestFilter;
        }
      } else if (mode === 'rendered') {
        // Render: Iluminação realista e sombras
        mesh.material.wireframe = false;
        mesh.material.flatShading = (mesh.userData.shading === 'flat');
      }
      mesh.material.needsUpdate = true;
    });

    // Control Scene Lights & Shadows according to mode
    if (mode === 'solid') {
      this.renderer.shadowMap.enabled = false;
      this.sunLight.castShadow = false;
      this.ambientLight.intensity = 1.1;
      this.sunLight.intensity = 1.0;
      this.fillLight.intensity = 0.8;
      this.rimLight.intensity = 0.6;
    } else if (mode === 'material' || mode === 'pixel') {
      // Visão clara do material sem sombras escuras que escondam faces
      this.renderer.shadowMap.enabled = false;
      this.sunLight.castShadow = false;
      this.ambientLight.intensity = 1.6;
      this.sunLight.intensity = 0.8;
      this.fillLight.intensity = 0.8;
      this.rimLight.intensity = 0.5;
    } else if (mode === 'rendered') {
      this.renderer.shadowMap.enabled = true;
      this.sunLight.castShadow = true;
      this.ambientLight.intensity = 0.6;
      this.sunLight.intensity = 1.4;
      this.fillLight.intensity = 0.6;
      this.rimLight.intensity = 0.4;
    } else if (mode === 'wireframe') {
      this.renderer.shadowMap.enabled = false;
      this.sunLight.castShadow = false;
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
