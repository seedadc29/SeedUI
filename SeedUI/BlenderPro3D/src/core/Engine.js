import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

// Blender Coordinate System standard: Z is Up, Y is Depth, X is Width
THREE.Object3D.DEFAULT_UP.set(0, 0, 1);

export class Engine {
  constructor(canvasOrId = 'viewport-canvas') {
    this.canvas = typeof canvasOrId === 'string' ? document.getElementById(canvasOrId) : canvasOrId;
    this.container = this.canvas.parentElement;
    
    // Performance settings (Low-Spec PC Optimization)
    this.isLowSpecMode = true;
    this.devicePixelRatio = Math.min(window.devicePixelRatio, 1.25);
    this.currentShading = 'solid'; // 'solid' | 'wireframe' | 'pixel' | 'rendered'

    // FPS Counter variables
    this.frameCount = 0;
    this.lastFpsUpdate = performance.now();
    this.currentFps = 60;

    // Blender Camera to View Navigation State
    this.isCameraViewActive = false;
    this.lockCameraToView = true;
    this.onCameraMoved = null;

    this.initScene();
    this.initRenderer();
    this.initCamera();
    this.initControls();
    this.initLights();
    this.initHelpers();
    this.setShadingMode('solid', []);

    this.bindEvents();
    this.startLoop();
  }

  initScene() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x484848); // Blender Studio Gray
  }

  initRenderer() {
    this.renderer = new THREE.WebGLRenderer({
      canvas: this.canvas,
      antialias: true,
      preserveDrawingBuffer: true,
      powerPreference: 'high-performance',
      precision: 'mediump'
    });

    this.renderer.setPixelRatio(this.devicePixelRatio);
    this.renderer.setSize(this.container.clientWidth, this.container.clientHeight);
    this.renderer.shadowMap.enabled = false;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.05;
  }

  initCamera() {
    const aspect = this.container.clientWidth / this.container.clientHeight;
    
    // Perspective Camera (Blender Z-Up: X=Right, Y=Depth, Z=Height)
    this.cameraPersp = new THREE.PerspectiveCamera(45, aspect, 0.1, 500);
    this.cameraPersp.up.set(0, 0, 1);
    this.cameraPersp.position.set(10.5, -12.5, 8.0);
    this.cameraPersp.lookAt(0, 0, 0);
    this.scene.add(this.cameraPersp);
    
    // Orthographic Camera (Blender Z-Up)
    const frustumSize = 8;
    this.cameraOrtho = new THREE.OrthographicCamera(
      (frustumSize * aspect) / -2,
      (frustumSize * aspect) / 2,
      frustumSize / 2,
      frustumSize / -2,
      0.1,
      500
    );
    this.cameraOrtho.up.set(0, 0, 1);
    this.cameraOrtho.position.set(10.5, -12.5, 8.0);
    this.cameraOrtho.lookAt(0, 0, 0);
    this.scene.add(this.cameraOrtho);

    // Blender Studio Directional Keylight (At fixed angle for clear 3D facet definition)
    this.headlightPersp = new THREE.DirectionalLight(0xffffff, 0.7);
    this.headlightPersp.position.set(2, 3, 4);
    this.cameraPersp.add(this.headlightPersp);

    this.headlightOrtho = new THREE.DirectionalLight(0xffffff, 0.7);
    this.headlightOrtho.position.set(2, 3, 4);
    this.cameraOrtho.add(this.headlightOrtho);

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

    // Live Camera to View Synchronization:
    // Moving the viewport while inside Camera View directly moves the 3D Camera Object!
    this.isSyncingCamera = false;
    this.controls.addEventListener('change', () => {
      if (this.isCameraViewActive && this.lockCameraToView && this.sceneCamera && !this.isSyncingCamera) {
        this.sceneCamera.position.copy(this.activeCamera.position);
        this.sceneCamera.quaternion.copy(this.activeCamera.quaternion);
        if (this.onCameraMoved) {
          this.onCameraMoved(this.sceneCamera);
        }
      }
    });

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
    // 1. Hemisphere Light (Blender Studio Sky/Ground balance)
    this.hemiLight = new THREE.HemisphereLight(0xffffff, 0x383838, 0.55);
    this.scene.add(this.hemiLight);

    // 2. Soft Ambient Light
    this.ambientLight = new THREE.AmbientLight(0xffffff, 0.35);
    this.scene.add(this.ambientLight);

    // 3. Directional Sun Light (Usado apenas no modo Rendered)
    this.sunLight = new THREE.DirectionalLight(0xffffff, 1.2);
    this.sunLight.position.set(5, 8, 7);
    this.sunLight.castShadow = true;
    this.sunLight.shadow.mapSize.width = 1024;
    this.sunLight.shadow.mapSize.height = 1024;
    this.sunLight.shadow.bias = -0.0005;
    this.sunLight.visible = false; // Desligado por padrão no modo Sólido/Material para eliminar sombras
    this.scene.add(this.sunLight);

    // 4. Fill e Rim Lights
    this.fillLight = new THREE.DirectionalLight(0xffffff, 0.4);
    this.fillLight.position.set(-6, 4, -4);
    this.fillLight.visible = false;
    this.scene.add(this.fillLight);

    this.rimLight = new THREE.DirectionalLight(0xffffff, 0.3);
    this.rimLight.position.set(-3, 8, 4);
    this.rimLight.visible = false;
    this.scene.add(this.rimLight);
  }

  initHelpers() {
    // 1. Blender 4.x Subtle Ground Grid on XY Plane (Z is Up)
    const gridSize = 32;
    const gridDivisions = 32;
    this.grid = new THREE.GridHelper(gridSize, gridDivisions, 0x363640, 0x2c2c34);
    this.grid.rotation.x = Math.PI / 2;
    this.grid.position.z = -0.001;
    this.gridHelper = this.grid;
    this.scene.add(this.grid);

    // 2. Blender Principal Central Axis Lines (Red for X, Green for Y running across the floor)
    const axisExtent = gridSize / 2;

    // Red X-Axis Center Line
    const xGeom = new THREE.BufferGeometry().setFromPoints([
      new THREE.Vector3(-axisExtent, 0, 0.001),
      new THREE.Vector3(axisExtent, 0, 0.001)
    ]);
    const xMat = new THREE.LineBasicMaterial({ color: 0xcc3d3d, linewidth: 2, depthTest: true });
    this.xAxisLine = new THREE.Line(xGeom, xMat);
    this.xAxisLine.name = 'blender_x_axis';
    this.scene.add(this.xAxisLine);

    // Green Y-Axis Center Line
    const yGeom = new THREE.BufferGeometry().setFromPoints([
      new THREE.Vector3(0, -axisExtent, 0.001),
      new THREE.Vector3(0, axisExtent, 0.001)
    ]);
    const yMat = new THREE.LineBasicMaterial({ color: 0x46a846, linewidth: 2, depthTest: true });
    this.yAxisLine = new THREE.Line(yGeom, yMat);
    this.yAxisLine.name = 'blender_y_axis';
    this.scene.add(this.yAxisLine);

    // 3. Subtle Origin Axes (1.2m)
    this.axes = new THREE.AxesHelper(1.2);
    this.axes.position.z = 0.002;
    this.axesHelper = this.axes;
    this.scene.add(this.axes);
  }

  bindEvents() {
    window.addEventListener('resize', () => this.onResize());

    if (window.ResizeObserver && this.container) {
      this.resizeObserver = new ResizeObserver(() => {
        this.onResize();
      });
      this.resizeObserver.observe(this.container);
    }
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
      this.cameraOrtho.up.set(0, 0, 1);
      this.cameraOrtho.position.copy(position);
      this.cameraOrtho.lookAt(target);
      this.activeCamera = this.cameraOrtho;
    } else {
      this.cameraPersp.up.set(0, 0, 1);
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
    const overlay = document.getElementById('camera-frame-overlay');
    const camBtn = document.getElementById('btn-view-cam');

    if (viewName === 'camera') {
      if (this.isCameraViewActive) {
        // Toggle OUT of camera view
        this.isCameraViewActive = false;
        if (overlay) overlay.classList.add('hidden');
        if (camBtn) camBtn.classList.remove('active');
        if (this.sceneCamera) this.sceneCamera.visible = true;
        return;
      }

      // Enter Camera View
      this.isCameraViewActive = true;
      if (overlay) overlay.classList.remove('hidden');
      if (camBtn) camBtn.classList.add('active');

      if (this.sceneCamera) {
        this.isSyncingCamera = true;
        this.sceneCamera.visible = false; // Hide camera wireframe while looking through it

        this.activeCamera.up.set(0, 0, 1);
        this.activeCamera.position.copy(this.sceneCamera.position);
        this.controls.target.set(0, 0, 0);
        this.activeCamera.lookAt(0, 0, 0);
        this.controls.update();

        setTimeout(() => {
          this.isSyncingCamera = false;
        }, 80);
      } else {
        this.activeCamera.up.set(0, 0, 1);
        this.activeCamera.position.set(7.3589, -6.9258, 4.9583);
        this.controls.target.set(0, 0, 0);
        this.activeCamera.lookAt(0, 0, 0);
        this.controls.update();
      }
      return;
    }

    // Exiting camera view if switching to other preset views
    if (this.isCameraViewActive) {
      this.isCameraViewActive = false;
      if (overlay) overlay.classList.add('hidden');
      if (camBtn) camBtn.classList.remove('active');
      if (this.sceneCamera) this.sceneCamera.visible = true;
    }

    switch (viewName) {
      case 'top': // Numpad 7 (Top view: looking down along -Z, Y points up on screen)
        this.activeCamera.up.set(0, 1, 0);
        this.activeCamera.position.set(target.x, target.y + 0.001, target.z + dist);
        break;
      case 'front': // Numpad 1 (Front view: looking along +Y, Z points up on screen)
        this.activeCamera.up.set(0, 0, 1);
        this.activeCamera.position.set(target.x, target.y - dist, target.z);
        break;
      case 'right': // Numpad 3 (Right view: looking along -X, Z points up on screen)
        this.activeCamera.up.set(0, 0, 1);
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
        // Wireframe Limpo em Quads: Oculta a triangulação interna da GPU, exibindo apenas as arestas reais dos polígonos
        mesh.material.wireframe = false;
        mesh.material.colorWrite = false;
        mesh.material.transparent = true;
        mesh.material.opacity = 0;

        mesh.children.forEach((child) => {
          if (child.name.endsWith('_edges')) {
            child.visible = true;
            if (child.material) {
              child.material.color.set(0xdddddd);
              child.material.opacity = 1.0;
            }
          }
        });
      } else {
        mesh.material.colorWrite = true;
        mesh.material.transparent = false;
        mesh.material.opacity = 1.0;
        mesh.material.wireframe = false;

        if (mode === 'solid') {
          // Modo Sólido / Modelagem: Facetas planas com contraste nítido em todas as faces
          mesh.material.flatShading = true;
        } else if (mode === 'material' || mode === 'pixel') {
          // Previsão de Material: Cor pura e textura do material sem render/sombras escuras
          mesh.material.flatShading = (mesh.userData.shading === 'flat');
          if (mode === 'pixel' && mesh.material.map) {
            mesh.material.map.minFilter = THREE.NearestFilter;
            mesh.material.map.magFilter = THREE.NearestFilter;
          }
        } else if (mode === 'rendered') {
          // Render: Iluminação realista e sombras
          mesh.material.flatShading = (mesh.userData.shading === 'flat');
        }

        mesh.children.forEach((child) => {
          if (child.name.endsWith('_edges')) {
            if (child.material) {
              child.material.color.set(0x383d47);
              child.material.opacity = 0.85;
            }
          }
        });
      }
      mesh.material.needsUpdate = true;
    });

    // Control Scene Lights & Shadows according to mode
    if (mode === 'solid') {
      // Blender Studio Clay Lighting
      this.renderer.shadowMap.enabled = false;
      if (this.sunLight) this.sunLight.visible = false;
      if (this.fillLight) this.fillLight.visible = false;
      if (this.rimLight) this.rimLight.visible = false;
      if (this.hemiLight) {
        this.hemiLight.visible = true;
        this.hemiLight.intensity = 0.55;
      }
      if (this.ambientLight) this.ambientLight.intensity = 0.35;
      if (this.headlightPersp) {
        this.headlightPersp.visible = true;
        this.headlightPersp.intensity = 0.7;
      }
      if (this.headlightOrtho) {
        this.headlightOrtho.visible = true;
        this.headlightOrtho.intensity = 0.7;
      }
    } else if (mode === 'material' || mode === 'pixel') {
      // Visão equilibrada de material
      this.renderer.shadowMap.enabled = false;
      if (this.sunLight) this.sunLight.visible = false;
      if (this.fillLight) this.fillLight.visible = false;
      if (this.rimLight) this.rimLight.visible = false;
      if (this.hemiLight) {
        this.hemiLight.visible = true;
        this.hemiLight.intensity = 0.7;
      }
      if (this.ambientLight) this.ambientLight.intensity = 0.5;
      if (this.headlightPersp) {
        this.headlightPersp.visible = true;
        this.headlightPersp.intensity = 0.8;
      }
      if (this.headlightOrtho) {
        this.headlightOrtho.visible = true;
        this.headlightOrtho.intensity = 0.8;
      }
    } else if (mode === 'rendered') {
      // Sombras e iluminação realista de cena
      this.renderer.shadowMap.enabled = true;
      if (this.sunLight) {
        this.sunLight.visible = true;
        this.sunLight.castShadow = true;
      }
      if (this.fillLight) this.fillLight.visible = true;
      if (this.rimLight) this.rimLight.visible = true;
      if (this.hemiLight) this.hemiLight.visible = false;
      if (this.ambientLight) this.ambientLight.intensity = 0.4;
      if (this.headlightPersp) this.headlightPersp.visible = false;
      if (this.headlightOrtho) this.headlightOrtho.visible = false;
    } else if (mode === 'wireframe') {
      this.renderer.shadowMap.enabled = false;
      if (this.sunLight) this.sunLight.visible = false;
      if (this.fillLight) this.fillLight.visible = false;
      if (this.rimLight) this.rimLight.visible = false;
      if (this.hemiLight) {
        this.hemiLight.visible = true;
        this.hemiLight.intensity = 1.0;
      }
    }
  }

  setLowSpecMode(enabled) {
    this.isLowSpecMode = enabled;
    this.devicePixelRatio = enabled ? 1.0 : Math.min(window.devicePixelRatio, 1.5);
    this.renderer.setPixelRatio(this.devicePixelRatio);
    this.renderer.shadowMap.enabled = !enabled;
    this.onResize();
  }

  /**
   * Focus on selected object (Blender Numpad . / Del / Home)
   * Centers the OrbitControls target on the object's 3D bounding box center
   * and positions the main viewport camera to frame it perfectly.
   */
  focusOnObject(targetObject) {
    const cam = this.activeCamera;
    if (!cam) return;

    // 1. Calculate Target 3D Center and Bounding Sphere Radius
    const center = new THREE.Vector3(0, 0, 0);
    let radius = 2.0;

    if (targetObject) {
      const box = new THREE.Box3().setFromObject(targetObject);
      if (!box.isEmpty()) {
        box.getCenter(center);
        const size = new THREE.Vector3();
        box.getSize(size);
        radius = Math.max(size.x, size.y, size.z, 0.5) * 0.7;
      } else {
        targetObject.getWorldPosition(center);
        radius = 1.5;
      }
    }

    // 2. Compute Viewing Distance from FOV
    let distance = 6.0;
    if (cam.isPerspectiveCamera) {
      const fovRad = (cam.fov * Math.PI) / 180;
      distance = Math.max(radius / Math.sin(fovRad / 2), 2.5);
    } else if (cam.isOrthographicCamera) {
      cam.zoom = Math.min(window.innerWidth, window.innerHeight) / (radius * 250);
      cam.updateProjectionMatrix();
      distance = 8.0;
    }

    // 3. Compute Viewing Direction from current camera orientation
    let dir = new THREE.Vector3().subVectors(cam.position, this.controls.target).normalize();
    if (dir.lengthSq() < 0.001) {
      dir.set(1.0, -1.0, 0.8).normalize();
    }

    const newCamPos = center.clone().addScaledVector(dir, distance);

    // 4. Smoothly animate or snap to framed position
    const startTarget = this.controls.target.clone();
    const startCamPos = cam.position.clone();
    const startTime = performance.now();
    const duration = 220; // 220ms smooth Blender transition

    const animateFocus = (now) => {
      const elapsed = now - startTime;
      const t = Math.min(1.0, elapsed / duration);
      const ease = t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t; // Ease in-out

      this.controls.target.lerpVectors(startTarget, center, ease);
      cam.position.lerpVectors(startCamPos, newCamPos, ease);
      this.controls.update();

      if (t < 1.0) {
        requestAnimationFrame(animateFocus);
      } else {
        this.controls.target.copy(center);
        cam.position.copy(newCamPos);
        this.controls.update();
      }
    };

    requestAnimationFrame(animateFocus);
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
