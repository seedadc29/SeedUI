import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { TransformControls } from 'three/examples/jsm/controls/TransformControls.js';

export class Scene3D {
  constructor(canvas, historyManager = null) {
    this.canvas = canvas;
    this.historyManager = historyManager;
    this.scene = null;
    this.perspectiveCamera = null;
    this.orthoCamera = null;
    this.gameCamera = null;
    this.activeCamera = null;
    this.renderer = null;
    this.controls = null;
    this.transformControls = null;
    this.grid = null;
    this.selectionBoxHelper = null;
    this.cameraHelper = null;
    this.cameraHelperMesh = null;
    this.raycaster = new THREE.Raycaster();
    this.mouse = new THREE.Vector2();

    // 3D World Instances: instanceId -> { id, name, familyId, familyName, mesh, shape, type, ... }
    this.entities = new Map();
    this.selectedEntity = null;
    this.isPlaying = false;
    this.isPaused = false;
    this.isOrthographic = false;
    this.isPilotingGameCamera = false;

    // Game Camera Configuration with Mouse Look and Movement Clamping
    this.cameraConfig = {
      mode: 'follow', // 'follow' | 'static'
      preset: 'platformer', // 'platformer' | 'third_person' | 'top_down' | 'first_person' | 'static'
      offset: new THREE.Vector3(0, 3.2, 13.0),
      lookAtOffset: new THREE.Vector3(0, 1.2, 0),
      fov: 48,
      smoothSpeed: 6.0,
      trackRotation: false,
      mouseLook: {
        enabled: true,
        lockMouse: false, // Set to true for fixed 2D / platformer games
        sensitivityX: 0.003,
        sensitivityY: 0.003,
        invertY: false,
        smoothDamping: 0.12,
        yaw: 0,
        pitch: 0.1,
        targetYaw: 0,
        targetPitch: 0.1,
        minPitchDeg: -60,
        maxPitchDeg: 75,
        minYawDeg: -180,
        maxYawDeg: 180,
        enableYawLimit: false
      },
      limits: {
        enabled: false,
        minX: -25.0,
        maxX: 25.0,
        minY: 0.5,
        maxY: 20.0,
        minZ: -25.0,
        maxZ: 25.0
      },
      distanceLimits: {
        minDistance: 2.0,
        maxDistance: 30.0
      }
    };

    // Input state during play
    this.keys = { w: false, a: false, s: false, d: false, space: false, shift: false };

    // Callbacks
    this.onEntitySelected = null;
    this.onTransformChange = null;
    this.onCollisionEvent = null;
    this.onCameraConfigChange = null;

    this.init();
    this.initControls();
    this.initTransformControls();
    this.initGameCameraEntity();
    this.initShortcuts();
    this.animate();
  }

  init() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x12141a);

    const rect = this.canvas.parentElement.getBoundingClientRect();
    const aspect = rect.width / rect.height;

    // 1. Perspective Camera (Editor Orbit)
    this.perspectiveCamera = new THREE.PerspectiveCamera(45, aspect, 0.1, 1000);
    this.perspectiveCamera.position.set(12, 10, 16);

    // 2. Orthographic Camera (Editor 2D Projection)
    const frustumSize = 18;
    this.orthoCamera = new THREE.OrthographicCamera(
      (-frustumSize * aspect) / 2,
      (frustumSize * aspect) / 2,
      frustumSize / 2,
      -frustumSize / 2,
      0.1,
      1000
    );
    this.orthoCamera.position.set(12, 10, 16);

    // 3. Dedicated Game / Player View Camera
    this.gameCamera = new THREE.PerspectiveCamera(this.cameraConfig.fov, aspect, 0.1, 1000);
    this.gameCamera.position.set(0, 3.5, 14.0);
    this.gameCamera.lookAt(0, 1.2, 0);

    this.activeCamera = this.perspectiveCamera;

    // Renderer
    this.renderer = new THREE.WebGLRenderer({ canvas: this.canvas, antialias: true });
    this.renderer.setSize(rect.width, rect.height);
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;

    // Lighting
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.7);
    this.scene.add(ambientLight);

    const dirLight = new THREE.DirectionalLight(0xffffff, 0.9);
    dirLight.position.set(20, 30, 20);
    dirLight.castShadow = true;
    dirLight.shadow.mapSize.width = 1024;
    dirLight.shadow.mapSize.height = 1024;
    this.scene.add(dirLight);

    const rimLight = new THREE.DirectionalLight(0x38bdf8, 0.4);
    rimLight.position.set(-15, 10, -15);
    this.scene.add(rimLight);

    // Floor Grid
    this.grid = new THREE.GridHelper(60, 60, 0x38bdf8, 0x222631);
    this.grid.position.y = 0.005;
    this.scene.add(this.grid);

    window.addEventListener('resize', () => this.onResize());
  }

  // --- Visual 3D Cinema Camera in World ---
  initGameCameraEntity() {
    const camGroup = new THREE.Group();
    camGroup.name = 'Câmera de Jogo';

    // Camera Body
    const bodyGeo = new THREE.BoxGeometry(1.0, 0.7, 1.4);
    const bodyMat = new THREE.MeshStandardMaterial({ color: 0x1f293d, metalness: 0.8, roughness: 0.2 });
    const bodyMesh = new THREE.Mesh(bodyGeo, bodyMat);
    bodyMesh.castShadow = true;
    camGroup.add(bodyMesh);

    // Camera Lens
    const lensGeo = new THREE.CylinderGeometry(0.35, 0.42, 0.6, 24);
    const lensMat = new THREE.MeshStandardMaterial({ color: 0x0284c7, metalness: 0.9, roughness: 0.1 });
    const lensMesh = new THREE.Mesh(lensGeo, lensMat);
    lensMesh.rotation.x = Math.PI / 2;
    lensMesh.position.z = -0.9;
    camGroup.add(lensMesh);

    // Top Viewfinder / Handle
    const handleGeo = new THREE.BoxGeometry(0.2, 0.3, 0.7);
    const handleMat = new THREE.MeshStandardMaterial({ color: 0x38bdf8, metalness: 0.5, roughness: 0.3 });
    const handleMesh = new THREE.Mesh(handleGeo, handleMat);
    handleMesh.position.set(0, 0.45, 0);
    camGroup.add(handleMesh);

    // Frustum Visual Wireframe
    this.cameraHelper = new THREE.CameraHelper(this.gameCamera);
    this.cameraHelper.material.opacity = 0.4;
    this.cameraHelper.material.transparent = true;

    camGroup.position.copy(this.gameCamera.position);
    this.cameraHelperMesh = camGroup;
    this.hasCameraInScene = false;
  }

  initControls() {
    this.controls = new OrbitControls(this.perspectiveCamera, this.canvas);
    this.controls.enableDamping = true;
    this.controls.dampingFactor = 0.08;
    this.controls.screenSpacePanning = true;
    this.controls.target.set(0, 0.8, 0);

    this.controls.mouseButtons = {
      LEFT: THREE.MOUSE.ROTATE,
      MIDDLE: THREE.MOUSE.ROTATE,
      RIGHT: THREE.MOUSE.PAN
    };

    let downPos = { x: 0, y: 0 };
    let prevMouse = { x: 0, y: 0 };
    let isPointerDown = false;

    this.canvas.addEventListener('pointerdown', (e) => {
      isPointerDown = true;
      downPos = { x: e.clientX, y: e.clientY };
      prevMouse = { x: e.clientX, y: e.clientY };

      if (this.isPlaying) {
        if (e.button === 0 && !e.altKey) this.controls.enabled = false;
        else this.controls.enabled = true;

        if (this.cameraConfig.mouseLook?.enabled && !this.cameraConfig.mouseLook?.lockMouse) {
          try { this.canvas.requestPointerLock?.(); } catch (err) {}
        }
      } else if (this.isPilotingGameCamera) {
        this.controls.enabled = false;
      } else {
        if (this.transformControls && this.transformControls.dragging) {
          this.controls.enabled = false;
        } else {
          this.controls.enabled = true;
        }
      }
    });

    window.addEventListener('pointerup', () => {
      isPointerDown = false;
    });

    // Player Mouse Look: Free look without clicking when playing, or on drag in editor camera
    window.addEventListener('pointermove', (e) => {
      const isCameraView = this.isPilotingGameCamera || this.isPlaying;
      const mLook = this.cameraConfig.mouseLook;
      if (!isCameraView || !mLook?.enabled || mLook?.lockMouse) {
        prevMouse = { x: e.clientX, y: e.clientY };
        return;
      }

      const isLocked = document.pointerLockElement === this.canvas;
      let dx = 0;
      let dy = 0;

      if (isLocked) {
        dx = e.movementX || 0;
        dy = e.movementY || 0;
      } else if (this.isPlaying || isPointerDown) {
        // Free mouse look during play mode without holding buttons!
        dx = e.movementX !== undefined && Math.abs(e.movementX) < 150 ? e.movementX : (e.clientX - prevMouse.x);
        dy = e.movementY !== undefined && Math.abs(e.movementY) < 150 ? e.movementY : (e.clientY - prevMouse.y);
      } else {
        prevMouse = { x: e.clientX, y: e.clientY };
        return;
      }

      prevMouse = { x: e.clientX, y: e.clientY };

      if (dx === 0 && dy === 0) return;

      mLook.targetYaw -= dx * mLook.sensitivityX;
      if (mLook.invertY) {
        mLook.targetPitch -= dy * mLook.sensitivityY;
      } else {
        mLook.targetPitch += dy * mLook.sensitivityY;
      }

      // Clamp Pitch (Vertical)
      const minPitchRad = (mLook.minPitchDeg * Math.PI) / 180;
      const maxPitchRad = (mLook.maxPitchDeg * Math.PI) / 180;
      mLook.targetPitch = Math.max(minPitchRad, Math.min(maxPitchRad, mLook.targetPitch));

      // Clamp Yaw (Horizontal) if enabled
      if (mLook.enableYawLimit) {
        const minYawRad = (mLook.minYawDeg * Math.PI) / 180;
        const maxYawRad = (mLook.maxYawDeg * Math.PI) / 180;
        mLook.targetYaw = Math.max(minYawRad, Math.min(maxYawRad, mLook.targetYaw));
      }
    });

    // Hover feedback over 3D objects (Works in Editor and Camera View)
    this.canvas.addEventListener('pointermove', (e) => {
      if (this.isPlaying) return;
      if (this.transformControls && this.transformControls.dragging) return;

      const rect = this.canvas.getBoundingClientRect();
      this.mouse.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      this.mouse.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;

      this.raycaster.setFromCamera(this.mouse, this.activeCamera);
      const meshes = Array.from(this.entities.values())
        .filter(e => e.type !== 'camera' || !this.isPilotingGameCamera)
        .map(e => e.mesh)
        .filter(Boolean);
      const intersects = this.raycaster.intersectObjects(meshes, true);

      if (intersects.length > 0) {
        this.canvas.style.cursor = 'pointer';
      } else {
        this.canvas.style.cursor = 'default';
      }
    });

    // Raycast selection on click (Works in Editor and Camera View)
    this.canvas.addEventListener('click', (e) => {
      if (this.isPlaying) return;
      if (Math.hypot(e.clientX - downPos.x, e.clientY - downPos.y) > 10) return;
      if (this.transformControls && this.transformControls.dragging) return;

      const rect = this.canvas.getBoundingClientRect();
      this.mouse.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      this.mouse.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;

      this.raycaster.setFromCamera(this.mouse, this.activeCamera);
      const meshes = Array.from(this.entities.values())
        .filter(e => e.type !== 'camera' || !this.isPilotingGameCamera)
        .map(e => e.mesh)
        .filter(Boolean);
      const intersects = this.raycaster.intersectObjects(meshes, true);

      if (intersects.length > 0) {
        let foundEnt = null;
        for (const hit of intersects) {
          let curr = hit.object;
          while (curr) {
            for (const ent of this.entities.values()) {
              if (ent.mesh === curr) {
                foundEnt = ent;
                break;
              }
            }
            if (foundEnt) break;
            curr = curr.parent;
          }
          if (foundEnt) break;
        }

        if (foundEnt) {
          this.selectEntity(foundEnt);
        } else {
          this.selectEntity(null);
        }
      } else {
        this.selectEntity(null);
      }
    });
  }

  // --- Interactive Transform Gizmo (Translate, Rotate, Scale) ---
  initTransformControls() {
    this.transformControls = new TransformControls(this.perspectiveCamera, this.canvas);
    this.transformControls.size = 0.85;
    this.transformControls.setSpace('world');
    this.transformControls.setMode('translate');

    this.transformControls.addEventListener('dragging-changed', (e) => {
      this.controls.enabled = !e.value;
      if (e.value && this.historyManager) {
        this.historyManager.saveSnapshot();
      }
      if (!e.value && this.selectedEntity) {
        this.selectedEntity.initialPos.copy(this.selectedEntity.mesh.position);
        if (this.selectedEntity.scale) {
          this.selectedEntity.scale.x = this.selectedEntity.mesh.scale.x;
          this.selectedEntity.scale.y = this.selectedEntity.mesh.scale.y;
          this.selectedEntity.scale.z = this.selectedEntity.mesh.scale.z;
        }

        // Sync game camera position with helper mesh
        if (this.selectedEntity.type === 'camera') {
          this.gameCamera.position.copy(this.selectedEntity.mesh.position);
          this.gameCamera.quaternion.copy(this.selectedEntity.mesh.quaternion);
          const p = this.getPlayerEntity();
          if (p && p.mesh) {
            this.cameraConfig.offset.copy(this.gameCamera.position).sub(p.mesh.position);
          }
          this.cameraHelper.update();
        }
      }
    });

    this.transformControls.addEventListener('change', () => {
      if (this.selectedEntity) {
        if (this.selectionBoxHelper) this.selectionBoxHelper.update();
        if (this.selectedEntity.type === 'camera') {
          this.gameCamera.position.copy(this.selectedEntity.mesh.position);
          this.gameCamera.quaternion.copy(this.selectedEntity.mesh.quaternion);
          this.cameraHelper.update();
        }
        if (this.onTransformChange) {
          this.onTransformChange(this.selectedEntity);
        }
      }
    });

    const helper = this.transformControls.getHelper ? this.transformControls.getHelper() : this.transformControls;
    this.scene.add(helper);
  }

  setGizmoMode(mode) {
    if (this.transformControls) {
      this.transformControls.setMode(mode);
    }
  }

  selectEntity(entity) {
    this.selectedEntity = entity;

    if (this.selectionBoxHelper) {
      this.scene.remove(this.selectionBoxHelper);
      this.selectionBoxHelper = null;
    }

    const isCameraView = this.isPilotingGameCamera || this.isPlaying;

    if (entity && entity.mesh) {
      if (!isCameraView) {
        this.transformControls.attach(entity.mesh);

        this.selectionBoxHelper = new THREE.BoxHelper(entity.mesh, entity.type === 'camera' ? 0x38bdf8 : 0xa855f7);
        this.selectionBoxHelper.material.linewidth = 2;
        this.selectionBoxHelper.material.depthTest = false;
        this.selectionBoxHelper.renderOrder = 999;
        this.scene.add(this.selectionBoxHelper);
      } else {
        this.transformControls.detach();
      }

      if (this.onEntitySelected) {
        this.onEntitySelected(entity);
      }
    } else {
      this.transformControls.detach();
      if (this.onEntitySelected) {
        this.onEntitySelected(null);
      }
    }
  }

  selectEntityById(entityId) {
    if (!entityId) {
      this.selectEntity(null);
      return;
    }
    const ent = this.entities.get(entityId);
    if (ent) this.selectEntity(ent);
  }

  selectEntityBySunId(sunId) {
    if (!sunId) {
      this.selectEntity(null);
      return;
    }
    for (const ent of this.entities.values()) {
      if (ent.familyId === sunId || ent.id === sunId) {
        this.selectEntity(ent);
        return;
      }
    }
  }

  // --- Game Camera Modes & Presets ---
  setGameCameraMode(mode) { // 'follow' | 'static'
    this.cameraConfig.mode = mode;
    if (this.onCameraConfigChange) this.onCameraConfigChange(this.cameraConfig);
  }

  setGameCameraPreset(preset) {
    this.cameraConfig.preset = preset;
    const player = this.getPlayerEntity();
    const pPos = player && player.mesh ? player.mesh.position : new THREE.Vector3(0, 1.1, 0);

    if (preset === 'platformer') {
      this.cameraConfig.mode = 'follow';
      this.cameraConfig.offset.set(0, 3.0, 14.0);
      this.cameraConfig.lookAtOffset.set(0, 1.2, 0);
      this.cameraConfig.fov = 46;
      if (this.cameraConfig.mouseLook) this.cameraConfig.mouseLook.lockMouse = true;
    } else if (preset === 'third_person') {
      this.cameraConfig.mode = 'follow';
      this.cameraConfig.offset.set(0, 4.5, 9.5);
      this.cameraConfig.lookAtOffset.set(0, 1.2, 0);
      this.cameraConfig.fov = 52;
      if (this.cameraConfig.mouseLook) this.cameraConfig.mouseLook.lockMouse = false;
    } else if (preset === 'top_down') {
      this.cameraConfig.mode = 'follow';
      this.cameraConfig.offset.set(0, 16.0, 0.1);
      this.cameraConfig.lookAtOffset.set(0, 0, 0);
      this.cameraConfig.fov = 48;
      if (this.cameraConfig.mouseLook) this.cameraConfig.mouseLook.lockMouse = true;
    } else if (preset === 'first_person') {
      this.cameraConfig.mode = 'follow';
      this.cameraConfig.offset.set(0, 1.7, 0.3);
      this.cameraConfig.lookAtOffset.set(0, 1.7, -5.0);
      this.cameraConfig.fov = 65;
      if (this.cameraConfig.mouseLook) this.cameraConfig.mouseLook.lockMouse = false;
    } else if (preset === 'static') {
      this.cameraConfig.mode = 'static';
      if (this.cameraConfig.mouseLook) this.cameraConfig.mouseLook.lockMouse = true;
    }

    this.gameCamera.fov = this.cameraConfig.fov;
    this.gameCamera.updateProjectionMatrix();

    if (this.cameraHelperMesh) {
      this.cameraHelperMesh.position.copy(pPos).add(this.cameraConfig.offset);
      this.gameCamera.position.copy(this.cameraHelperMesh.position);
      this.gameCamera.lookAt(pPos.clone().add(this.cameraConfig.lookAtOffset));
      this.cameraHelperMesh.quaternion.copy(this.gameCamera.quaternion);
      this.cameraHelper.update();
    }

    if (this.selectionBoxHelper && this.selectedEntity?.type === 'camera') {
      this.selectionBoxHelper.update();
    }

    if (this.onCameraConfigChange) this.onCameraConfigChange(this.cameraConfig);
  }

  setGameCameraMouseLook(params = {}) {
    if (!this.cameraConfig.mouseLook) this.cameraConfig.mouseLook = {};
    Object.assign(this.cameraConfig.mouseLook, params);
    if (this.onCameraConfigChange) this.onCameraConfigChange(this.cameraConfig);
  }

  setGameCameraLimits(params = {}) {
    if (!this.cameraConfig.limits) this.cameraConfig.limits = {};
    Object.assign(this.cameraConfig.limits, params);
    if (this.onCameraConfigChange) this.onCameraConfigChange(this.cameraConfig);
  }

  setGameCameraDistanceLimits(params = {}) {
    if (!this.cameraConfig.distanceLimits) this.cameraConfig.distanceLimits = {};
    Object.assign(this.cameraConfig.distanceLimits, params);
    if (this.onCameraConfigChange) this.onCameraConfigChange(this.cameraConfig);
  }

  toggleGameCameraView() {
    this.isPilotingGameCamera = !this.isPilotingGameCamera;

    if (this.isPilotingGameCamera) {
      this.activeCamera = this.gameCamera;
      this.controls.enabled = false;
      this.transformControls.detach();
      if (this.selectionBoxHelper) {
        this.scene.remove(this.selectionBoxHelper);
        this.selectionBoxHelper = null;
      }
      if (this.cameraHelperMesh) this.cameraHelperMesh.visible = false;
      if (this.cameraHelper) this.cameraHelper.visible = false;
    } else {
      this.activeCamera = this.perspectiveCamera;
      this.controls.enabled = true;
      this.controls.object = this.perspectiveCamera;
      this.transformControls.camera = this.perspectiveCamera;
      if (this.cameraHelperMesh) this.cameraHelperMesh.visible = true;
      if (this.cameraHelper) this.cameraHelper.visible = true;

      if (this.selectedEntity && this.selectedEntity.mesh) {
        this.selectEntity(this.selectedEntity);
      }
    }

    this.onResize();
    return this.isPilotingGameCamera;
  }

  // --- Duplication of 3D Objects in World ---
  duplicateSelectedEntity() {
    if (!this.selectedEntity || !this.selectedEntity.mesh || this.selectedEntity.type === 'camera') return null;

    if (this.historyManager) {
      this.historyManager.saveSnapshot();
    }

    const src = this.selectedEntity;
    const sameFamilyCount = Array.from(this.entities.values()).filter(e => e.familyId === src.familyId).length;
    const nextIdx = sameFamilyCount + 1;
    const newId = `inst-${src.familyId}-${Date.now()}-${Math.floor(Math.random() * 1000)}`;
    const newName = `${src.familyName || src.name} #${nextIdx}`;

    const { mesh, baseSize } = this.createMeshForShape(src.shape, src.type, nextIdx);
    mesh.position.copy(src.mesh.position);
    mesh.position.x += (src.dimensions?.x || 2.0) + 1.2;
    mesh.rotation.copy(src.mesh.rotation);
    mesh.scale.copy(src.mesh.scale);
    mesh.name = newName;
    this.scene.add(mesh);

    const newEnt = {
      ...src,
      id: newId,
      name: newName,
      mesh,
      baseSize,
      scale: { x: src.scale.x, y: src.scale.y, z: src.scale.z },
      dimensions: { x: src.dimensions.x, y: src.dimensions.y, z: src.dimensions.z },
      initialPos: mesh.position.clone(),
      velocity: new THREE.Vector3(),
      isGrounded: true,
      currentGroundY: 1.1,
      animTime: Math.random() * 10
    };

    this.entities.set(newId, newEnt);
    this.selectEntity(newEnt);
    return newEnt;
  }

  deleteSelectedEntity() {
    if (!this.selectedEntity || this.selectedEntity.type === 'camera') return;

    if (this.historyManager) {
      this.historyManager.saveSnapshot();
    }

    const ent = this.selectedEntity;
    if (ent.mesh) {
      this.scene.remove(ent.mesh);
      ent.mesh.geometry?.dispose();
    }

    this.entities.delete(ent.id);
    this.selectEntity(null);
  }

  // --- Dimension, Scale, Rotation & Position Modifiers ---
  setEntityDimensions(entityId, widthX, heightY, depthZ) {
    let ent = this.entities.get(entityId);
    if (!ent) {
      for (const e of this.entities.values()) {
        if (e.familyId === entityId) { ent = e; break; }
      }
    }
    if (!ent || !ent.mesh || ent.type === 'camera') return;

    const baseW = ent.baseSize?.x || 1.8;
    const baseH = ent.baseSize?.y || 2.2;
    const baseD = ent.baseSize?.z || 1.8;

    const sx = Math.max(0.05, widthX / baseW);
    const sy = Math.max(0.05, heightY / baseH);
    const sz = Math.max(0.05, depthZ / baseD);

    ent.mesh.scale.set(sx, sy, sz);
    ent.scale = { x: sx, y: sy, z: sz };
    ent.dimensions = { x: widthX, y: heightY, z: depthZ };

    if (this.selectionBoxHelper) this.selectionBoxHelper.update();
    if (this.onTransformChange) this.onTransformChange(ent);
  }

  setEntityScale(entityId, sx, sy, sz) {
    let ent = this.entities.get(entityId);
    if (!ent) {
      for (const e of this.entities.values()) {
        if (e.familyId === entityId) { ent = e; break; }
      }
    }
    if (!ent || !ent.mesh || ent.type === 'camera') return;

    ent.mesh.scale.set(Math.max(0.05, sx), Math.max(0.05, sy), Math.max(0.05, sz));
    ent.scale = { x: ent.mesh.scale.x, y: ent.mesh.scale.y, z: ent.mesh.scale.z };

    const baseW = ent.baseSize?.x || 1.8;
    const baseH = ent.baseSize?.y || 2.2;
    const baseD = ent.baseSize?.z || 1.8;
    ent.dimensions = {
      x: baseW * ent.scale.x,
      y: baseH * ent.scale.y,
      z: baseD * ent.scale.z
    };

    if (this.selectionBoxHelper) this.selectionBoxHelper.update();
    if (this.onTransformChange) this.onTransformChange(ent);
  }

  setEntityRotation(entityId, rxDeg, ryDeg, rzDeg) {
    let ent = this.entities.get(entityId);
    if (!ent) {
      for (const e of this.entities.values()) {
        if (e.familyId === entityId) { ent = e; break; }
      }
    }
    if (!ent || !ent.mesh) return;

    ent.mesh.rotation.set(
      (rxDeg * Math.PI) / 180,
      (ryDeg * Math.PI) / 180,
      (rzDeg * Math.PI) / 180
    );

    if (ent.type === 'camera') {
      this.gameCamera.quaternion.copy(ent.mesh.quaternion);
      if (this.cameraHelper) this.cameraHelper.update();
    }

    if (this.selectionBoxHelper) this.selectionBoxHelper.update();
    if (this.onTransformChange) this.onTransformChange(ent);
  }

  setEntityPosition(entityId, px, py, pz) {
    let ent = this.entities.get(entityId);
    if (!ent) {
      for (const e of this.entities.values()) {
        if (e.familyId === entityId) { ent = e; break; }
      }
    }
    if (!ent || !ent.mesh) return;

    ent.mesh.position.set(px, py, pz);
    ent.initialPos.copy(ent.mesh.position);

    if (ent.type === 'camera') {
      this.gameCamera.position.copy(ent.mesh.position);
      const player = this.getPlayerEntity();
      if (player && player.mesh) {
        this.cameraConfig.offset.copy(this.gameCamera.position).sub(player.mesh.position);
      }
      if (this.cameraHelper) this.cameraHelper.update();
    }

    if (this.selectionBoxHelper) this.selectionBoxHelper.update();
    if (this.onTransformChange) this.onTransformChange(ent);
  }

  setEntityCollisionConfig(entityId, { padding = 0, walkableTop = true, isSolid = true }) {
    let ent = this.entities.get(entityId);
    if (!ent) {
      for (const e of this.entities.values()) {
        if (e.familyId === entityId) { ent = e; break; }
      }
    }
    if (!ent) return;
    ent.collisionPadding = padding;
    ent.walkableTop = walkableTop;
    ent.isSolid = isSolid;
  }

  // --- Complete Viewport Shortcuts ---
  initShortcuts() {
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;

      // Camera toggle view shortcut
      if (e.code === 'KeyC' && !e.ctrlKey) {
        e.preventDefault();
        this.toggleGameCameraView();
        const btn = document.getElementById('btn-toggle-game-camera');
        const icon = document.getElementById('icon-game-camera');
        const lbl = document.getElementById('lbl-game-camera');
        if (btn && icon && lbl) {
          if (this.isPilotingGameCamera) {
            btn.style.background = '#0284c7';
            btn.style.color = '#ffffff';
            icon.className = 'ti ti-eye';
            lbl.textContent = 'Visão de Jogo (Ativa)';
          } else {
            btn.style.background = '';
            btn.style.color = '#38bdf8';
            icon.className = 'ti ti-video';
            lbl.textContent = 'Câmera de Jogo (C)';
          }
        }
        return;
      }

      if (this.isPlaying) return;

      // Duplicate shortcut
      if ((e.ctrlKey || e.metaKey) && e.code === 'KeyD') {
        e.preventDefault();
        this.duplicateSelectedEntity();
        return;
      }

      // Delete shortcut
      if (e.code === 'Delete' || e.code === 'Backspace') {
        if (this.selectedEntity && this.selectedEntity.type !== 'camera') {
          e.preventDefault();
          this.deleteSelectedEntity();
          return;
        }
      }

      // Transform Gizmo shortcuts
      if (e.code === 'KeyG') {
        e.preventDefault();
        this.setGizmoMode('translate');
      } else if (e.code === 'KeyR') {
        e.preventDefault();
        this.setGizmoMode('rotate');
      } else if (e.code === 'KeyS' && !e.ctrlKey) {
        e.preventDefault();
        this.setGizmoMode('scale');
      }

      // Camera view shortcuts
      else if (e.code === 'Numpad1' || e.code === 'Digit1') {
        e.preventDefault();
        if (e.ctrlKey) this.setCameraView('back');
        else this.setCameraView('front');
      } else if (e.code === 'Numpad3' || e.code === 'Digit3') {
        e.preventDefault();
        if (e.ctrlKey) this.setCameraView('left');
        else this.setCameraView('right');
      } else if (e.code === 'Numpad7' || e.code === 'Digit7') {
        e.preventDefault();
        if (e.ctrlKey) this.setCameraView('bottom');
        else this.setCameraView('top');
      } else if (e.code === 'Numpad5' || e.code === 'Digit5') {
        e.preventDefault();
        this.toggleProjection();
      } else if (e.code === 'NumpadDecimal' || e.code === 'KeyF' || e.code === 'Home') {
        e.preventDefault();
        this.frameSelectedEntity();
      }
    });
  }

  setCameraView(viewName) {
    const target = this.controls.target.clone();
    const dist = this.perspectiveCamera.position.distanceTo(target);
    const newPos = target.clone();

    switch (viewName) {
      case 'front': newPos.z += dist; break;
      case 'back': newPos.z -= dist; break;
      case 'right': newPos.x += dist; break;
      case 'left': newPos.x -= dist; break;
      case 'top': newPos.y += dist; break;
      case 'bottom': newPos.y -= dist; break;
    }

    this.animateCameraTo(newPos, target);
  }

  toggleProjection() {
    this.isOrthographic = !this.isOrthographic;
    const oldCam = this.activeCamera;
    const newCam = this.isOrthographic ? this.orthoCamera : this.perspectiveCamera;

    newCam.position.copy(oldCam.position);
    newCam.quaternion.copy(oldCam.quaternion);

    this.activeCamera = newCam;
    this.controls.object = newCam;
    this.transformControls.camera = newCam;
    this.controls.update();
    this.onResize();
  }

  frameSelectedEntity() {
    const ent = this.selectedEntity || this.getPlayerEntity();
    const target = ent && ent.mesh ? ent.mesh.position.clone() : new THREE.Vector3(0, 0.8, 0);
    const newPos = new THREE.Vector3(target.x + 6, target.y + 5, target.z + 8);
    this.animateCameraTo(newPos, target);
  }

  animateCameraTo(newPos, newTarget) {
    const startPos = this.activeCamera.position.clone();
    const startTarget = this.controls.target.clone();
    let progress = 0;

    const step = () => {
      progress += 0.08;
      if (progress >= 1.0) {
        this.activeCamera.position.copy(newPos);
        this.controls.target.copy(newTarget);
        this.controls.update();
      } else {
        const ease = 0.5 - 0.5 * Math.cos(progress * Math.PI);
        this.activeCamera.position.lerpVectors(startPos, newPos, ease);
        this.controls.target.lerpVectors(startTarget, newTarget, ease);
        this.controls.update();
        requestAnimationFrame(step);
      }
    };
    step();
  }

  // --- Dynamic Synchronization with Orbital Families (Suns) ---
  syncWithOrbitalSuns(suns) {
    if (!suns || suns.length === 0) {
      this.entities.forEach((ent, id) => {
        if (ent.type === 'camera') return;
        if (ent.mesh) {
          this.scene.remove(ent.mesh);
          ent.mesh.geometry?.dispose();
        }
        this.entities.delete(id);
      });
      this.selectEntity(null);
      return;
    }

    const activeFamilyIds = new Set(suns.map(s => s.id));

    // 0. Synchronize Game Camera Sun
    const cameraSun = suns.find(s => s.name.toUpperCase().includes('CAMERA') || s.name.toUpperCase().includes('CÂMERA'));

    if (cameraSun) {
      this.hasCameraInScene = true;
      if (this.cameraHelperMesh && !this.scene.children.includes(this.cameraHelperMesh)) {
        this.scene.add(this.cameraHelperMesh);
      }
      if (this.cameraHelper && !this.scene.children.includes(this.cameraHelper)) {
        this.scene.add(this.cameraHelper);
      }

      let camEnt = this.entities.get('game-camera-1');
      if (!camEnt) {
        camEnt = {
          id: 'game-camera-1',
          name: cameraSun.name || 'Câmera de Jogo',
          familyId: cameraSun.id,
          familyName: cameraSun.name || 'Câmera de Jogo',
          type: 'camera',
          shape: 'box',
          mesh: this.cameraHelperMesh,
          baseSize: { x: 1.0, y: 0.7, z: 1.4 },
          scale: { x: 1, y: 1, z: 1 },
          dimensions: { x: 1.0, y: 0.7, z: 1.4 },
          initialPos: this.cameraHelperMesh.position.clone(),
          velocity: new THREE.Vector3(),
          isGrounded: true,
          currentGroundY: 1.1,
          animTime: 0,
          hasCollision: false,
          isSolid: false
        };
        this.entities.set('game-camera-1', camEnt);
      } else {
        camEnt.familyId = cameraSun.id;
        camEnt.name = cameraSun.name;
        camEnt.familyName = cameraSun.name;
      }

      // Read planets and moons of the Camera Sun
      cameraSun.planets.forEach(planet => {
        const pName = planet.name.toLowerCase();
        if (pName.includes('seguir')) {
          this.cameraConfig.mode = 'follow';
        } else if (pName.includes('estatica') || pName.includes('estática') || pName.includes('fixa')) {
          this.cameraConfig.mode = 'static';
        }

        planet.moons?.forEach(moon => {
          const mName = moon.name.toLowerCase();
          if (mName.includes('preset')) {
            const valLower = (moon.val || '').toString().toLowerCase();
            if (valLower.includes('plataforma') || valLower.includes('2.5d')) this.cameraConfig.preset = 'platformer';
            else if (valLower.includes('3') || valLower.includes('terceira')) this.cameraConfig.preset = 'third_person';
            else if (valLower.includes('top') || valLower.includes('aerea') || valLower.includes('aérea')) this.cameraConfig.preset = 'top_down';
            else if (valLower.includes('1') || valLower.includes('primeira')) this.cameraConfig.preset = 'first_person';
            else if (valLower.includes('fixa') || valLower.includes('estatica')) this.cameraConfig.preset = 'static';
          } else if (mName.includes('fov')) {
            const fov = parseFloat(moon.val) || 48;
            this.cameraConfig.fov = fov;
            this.gameCamera.fov = fov;
            this.gameCamera.updateProjectionMatrix();
          } else if (mName.includes('distancia') || mName.includes('distância')) {
            this.cameraConfig.offset.z = parseFloat(moon.val) || 14.0;
          } else if (mName.includes('altura')) {
            this.cameraConfig.offset.y = parseFloat(moon.val) || 3.2;
          }
        });
      });

      if (this.cameraHelper) this.cameraHelper.update();
    } else {
      this.hasCameraInScene = false;
      if (this.cameraHelperMesh && this.scene.children.includes(this.cameraHelperMesh)) {
        this.scene.remove(this.cameraHelperMesh);
      }
      if (this.cameraHelper && this.scene.children.includes(this.cameraHelper)) {
        this.scene.remove(this.cameraHelper);
      }
      if (this.entities.has('game-camera-1')) {
        this.entities.delete('game-camera-1');
      }
      if (this.isPilotingGameCamera) {
        this.toggleGameCameraView();
      }
    }

    // 1. Remove deleted families' instances (except camera)
    this.entities.forEach((ent, instanceId) => {
      if (ent.type === 'camera') return;
      if (!activeFamilyIds.has(ent.familyId) || (cameraSun && ent.familyId === cameraSun.id)) {
        if (ent.mesh) {
          this.scene.remove(ent.mesh);
          ent.mesh.geometry?.dispose();
        }
        if (this.selectedEntity === ent) this.selectEntity(null);
        this.entities.delete(instanceId);
      }
    });

    // 2. Build / Update family capabilities on all instances
    suns.forEach((sun, index) => {
      const sunNameUpper = sun.name.toUpperCase();
      if (sunNameUpper.includes('CAMERA') || sunNameUpper.includes('CÂMERA')) return; // Handled exclusively by camera system

      let entityType = 'object';
      if (sunNameUpper.includes('PLAYER')) entityType = 'player';
      else if (sunNameUpper.includes('INIMIGO') || sunNameUpper.includes('ENEMY')) entityType = 'enemy';
      else if (sunNameUpper.includes('NPC')) entityType = 'npc';
      else if (sunNameUpper.includes('BLOCO') || sunNameUpper.includes('COLISAO') || sunNameUpper.includes('COLISÃO') || sunNameUpper.includes('WALL') || sunNameUpper.includes('PAREDE')) entityType = 'block';
      else if (sunNameUpper.includes('GATILHO') || sunNameUpper.includes('TRIGGER')) entityType = 'trigger';
      else if (sunNameUpper.includes('PLATAFORMA') || sunNameUpper.includes('PLATFORM')) entityType = 'platform';

      let modelShape = null;
      let hasMove = false;
      let walkSpeed = 0;
      let hasRun = false;
      let runMultiplier = 1.0;
      let hasJump = false;
      let jumpForce = 0;
      let hasPhysics = false;
      let mass = 1.0;
      let gravity = 0;
      let hasCollision = false;
      let isSolid = false;
      let hasAnimation = false;
      let hasAttract = false;
      let attractForce = 12.0;
      let attractRadius = 15.0;

      sun.planets.forEach(planet => {
        const pName = planet.name.toLowerCase();

        if (pName.includes('cubo') || pName.includes('cube')) modelShape = 'cube';
        else if (pName.includes('esfera') || pName.includes('sphere')) modelShape = 'sphere';
        else if (pName.includes('cilindro') || pName.includes('cylinder')) modelShape = 'cylinder';
        else if (pName.includes('triangulo') || pName.includes('cone')) modelShape = 'cone';
        else if (pName.includes('plano') || pName.includes('plane') || pName.includes('chao')) {
          modelShape = 'plane';
        }

        if (pName === 'andar') {
          hasMove = true;
          const speedMoon = planet.moons?.find(m => m.name.toLowerCase().includes('velocidade'));
          walkSpeed = speedMoon && speedMoon.val !== undefined ? Math.max(0, parseFloat(speedMoon.val) || 6.0) : 6.0;
        } else if (pName === 'correr') {
          hasRun = true;
          const multMoon = planet.moons?.find(m => m.name.toLowerCase().includes('multiplicador'));
          runMultiplier = multMoon && multMoon.val !== undefined ? Math.max(1.0, parseFloat(multMoon.val) || 1.8) : 1.8;
        } else if (pName === 'pular') {
          hasJump = true;
          const jumpMoon = planet.moons?.find(m => m.name.toLowerCase().includes('força') || m.name.toLowerCase().includes('forca'));
          jumpForce = jumpMoon && jumpMoon.val !== undefined ? Math.max(0, parseFloat(jumpMoon.val) || 8.5) : 8.5;
        } else if (pName === 'fisica') {
          hasPhysics = true;
          const massMoon = planet.moons?.find(m => m.name.toLowerCase().includes('massa'));
          mass = massMoon && massMoon.val !== undefined ? Math.max(0.1, parseFloat(massMoon.val) || 1.0) : 1.0;
          gravity = 20.0 * mass;
        } else if (pName === 'colisao') {
          hasCollision = true;
          isSolid = true;
        } else if (pName === 'gatilho') {
          hasCollision = true;
          isSolid = false;
        } else if (pName === 'animacao') {
          hasAnimation = true;
        } else if (pName === 'atrair' || pName.includes('attract')) {
          hasAttract = true;
          const forceMoon = planet.moons?.find(m => m.name.toLowerCase().includes('força') || m.name.toLowerCase().includes('forca'));
          attractForce = forceMoon && forceMoon.val !== undefined ? Math.max(1, parseFloat(forceMoon.val) || 12.0) : 12.0;
          const radiusMoon = planet.moons?.find(m => m.name.toLowerCase().includes('raio') || m.name.toLowerCase().includes('alcance'));
          attractRadius = radiusMoon && radiusMoon.val !== undefined ? Math.max(2, parseFloat(radiusMoon.val) || 15.0) : 15.0;
        }
      });

      if (!modelShape) {
        if (entityType === 'player') modelShape = 'cube';
        else if (entityType === 'enemy') modelShape = 'cylinder';
        else if (entityType === 'npc') modelShape = 'sphere';
        else if (entityType === 'block') modelShape = 'cube';
        else if (entityType === 'trigger') modelShape = 'cube';
        else if (entityType === 'platform') modelShape = 'plane';
        else modelShape = 'plane';
      }

      const familyInstances = Array.from(this.entities.values()).filter(e => e.familyId === sun.id);

      if (familyInstances.length === 0) {
        const instanceId = `inst-${sun.id}-1`;
        const { mesh, baseSize } = this.createMeshForShape(modelShape, entityType, index);
        mesh.name = `${sun.name} #1`;
        this.scene.add(mesh);

        const ent = {
          id: instanceId,
          name: `${sun.name} #1`,
          familyId: sun.id,
          familyName: sun.name,
          mesh,
          shape: modelShape,
          type: entityType,
          baseSize,
          scale: { x: 1, y: 1, z: 1 },
          dimensions: { x: baseSize.x, y: baseSize.y, z: baseSize.z },
          collisionPadding: 0.0,
          walkableTop: true,
          hasMove, walkSpeed,
          hasRun, runMultiplier,
          hasJump, jumpForce,
          hasPhysics, mass, gravity,
          hasCollision, isSolid,
          hasAnimation,
          hasAttract, attractForce, attractRadius,
          velocity: new THREE.Vector3(),
          isGrounded: true,
          currentGroundY: 1.1,
          animTime: Math.random() * 10,
          initialPos: mesh.position.clone()
        };
        this.entities.set(instanceId, ent);

        if (entityType === 'player' && !this.selectedEntity) {
          this.selectEntity(ent);
        }
      } else {
        familyInstances.forEach((ent) => {
          if (ent.shape !== modelShape || ent.type !== entityType) {
            const oldPos = ent.mesh.position.clone();
            const oldRot = ent.mesh.rotation.clone();
            const oldScale = ent.mesh.scale.clone();

            this.scene.remove(ent.mesh);
            ent.mesh.geometry?.dispose();

            const { mesh, baseSize } = this.createMeshForShape(modelShape, entityType, index);
            mesh.position.copy(oldPos);
            mesh.rotation.copy(oldRot);
            mesh.scale.copy(oldScale);
            mesh.name = ent.name;
            this.scene.add(mesh);

            ent.mesh = mesh;
            ent.shape = modelShape;
            ent.baseSize = baseSize;
            if (this.selectedEntity === ent) this.transformControls.attach(mesh);
          }

          ent.familyName = sun.name;
          ent.type = entityType;
          ent.hasMove = hasMove;
          ent.walkSpeed = walkSpeed;
          ent.hasRun = hasRun;
          ent.runMultiplier = runMultiplier;
          ent.hasJump = hasJump;
          ent.jumpForce = jumpForce;
          ent.hasPhysics = hasPhysics;
          ent.mass = mass;
          ent.gravity = gravity;
          ent.hasCollision = hasCollision;
          ent.isSolid = isSolid;
          ent.hasAnimation = hasAnimation;
          ent.hasAttract = hasAttract;
          ent.attractForce = attractForce;
          ent.attractRadius = attractRadius;
        });
      }
    });
  }

  createMeshForShape(shape, type, index) {
    let geo;
    let mat;
    let baseSize = { x: 1.8, y: 2.2, z: 1.8 };

    if (type === 'platform' || (shape === 'plane' && type !== 'player')) {
      if (type === 'platform') {
        baseSize = { x: 5.0, y: 0.4, z: 5.0 };
        geo = new THREE.BoxGeometry(5.0, 0.4, 5.0);
        mat = new THREE.MeshStandardMaterial({ color: 0x1e3a5f, roughness: 0.4, metalness: 0.3 });
        const mesh = new THREE.Mesh(geo, mat);
        mesh.position.set(index * 4.0 - 4, 0.2, -1.0);
        mesh.receiveShadow = true;
        mesh.castShadow = true;
        const edgeGeo = new THREE.EdgesGeometry(geo);
        const edgeMat = new THREE.LineBasicMaterial({ color: 0x38bdf8, linewidth: 2 });
        mesh.add(new THREE.LineSegments(edgeGeo, edgeMat));
        return { mesh, baseSize };
      }
      baseSize = { x: 28, y: 0.1, z: 28 };
      geo = new THREE.PlaneGeometry(28, 28);
      mat = new THREE.MeshStandardMaterial({ color: 0x1c1c20, roughness: 0.85, metalness: 0.1 });
      const mesh = new THREE.Mesh(geo, mat);
      mesh.rotation.x = -Math.PI / 2;
      mesh.receiveShadow = true;
      return { mesh, baseSize };
    }

    if (shape === 'sphere') {
      baseSize = { x: 2.0, y: 2.0, z: 2.0 };
      geo = new THREE.SphereGeometry(1.0, 32, 24);
    } else if (shape === 'cylinder') {
      baseSize = { x: 1.6, y: 2.0, z: 1.6 };
      geo = new THREE.CylinderGeometry(0.8, 0.8, 2.0, 24);
    } else if (shape === 'cone') {
      baseSize = { x: 2.0, y: 2.2, z: 2.0 };
      geo = new THREE.ConeGeometry(1.0, 2.2, 24);
    } else {
      baseSize = { x: 1.8, y: 2.2, z: 1.8 };
      geo = new THREE.BoxGeometry(1.8, 2.2, 1.8);
    }

    let color = 0x1f1f22;
    let edgeColor = 0x555560;
    let isTransparent = false;
    let opacity = 1.0;

    if (type === 'player') {
      color = 0x1e2430;
      edgeColor = 0x60a5fa;
    } else if (type === 'enemy') {
      color = 0x451212;
      edgeColor = 0xf87171;
    } else if (type === 'npc') {
      color = 0x143820;
      edgeColor = 0x4ade80;
    } else if (type === 'block') {
      color = 0x312e81;
      edgeColor = 0xa855f7;
    } else if (type === 'trigger') {
      color = 0x7c2d12;
      edgeColor = 0xfb923c;
      isTransparent = true;
      opacity = 0.45;
    } else {
      color = index % 2 === 1 ? 0x2a2540 : 0x203242;
      edgeColor = 0x94a3b8;
    }

    mat = new THREE.MeshStandardMaterial({
      color,
      roughness: 0.4,
      metalness: 0.2,
      transparent: isTransparent,
      opacity
    });

    const mesh = new THREE.Mesh(geo, mat);
    mesh.position.set(index * 3.5 - 3.5, 1.1, (index % 2 === 0 ? -1 : 1) * 2.5);
    mesh.castShadow = true;
    mesh.receiveShadow = true;

    const edgeGeo = new THREE.EdgesGeometry(geo);
    const edgeMat = new THREE.LineBasicMaterial({ color: edgeColor, linewidth: 2 });
    mesh.add(new THREE.LineSegments(edgeGeo, edgeMat));

    return { mesh, baseSize };
  }

  getPlayerEntity() {
    for (const ent of this.entities.values()) {
      if (ent.type === 'player') return ent;
    }
    return null;
  }

  // --- Play Simulation & Live Game Loop ---
  startPlay() {
    this.isPlaying = true;
    this.isPaused = false;
    this.selectEntity(null);

    // Switch to Game Camera
    this.activeCamera = this.gameCamera;
    if (this.cameraHelperMesh) this.cameraHelperMesh.visible = false;
    if (this.cameraHelper) this.cameraHelper.visible = false;

    this.entities.forEach(ent => {
      if (ent.mesh) ent.initialPos.copy(ent.mesh.position);
      if (!ent.velocity) ent.velocity = new THREE.Vector3();
      else ent.velocity.set(0, 0, 0);
      ent.isGrounded = true;
      ent.currentGroundY = 1.1;
    });

    this.initKeyboardControls();
  }

  stopPlay() {
    this.isPlaying = false;
    this.isPaused = false;

    if (!this.isPilotingGameCamera) {
      this.activeCamera = this.perspectiveCamera;
      if (this.cameraHelperMesh) this.cameraHelperMesh.visible = true;
      if (this.cameraHelper) this.cameraHelper.visible = true;
    }

    this.entities.forEach(ent => {
      if (ent.mesh) {
        ent.mesh.position.copy(ent.initialPos);
        if (ent.velocity) ent.velocity.set(0, 0, 0);
        if (ent.scale) {
          ent.mesh.scale.set(ent.scale.x, ent.scale.y, ent.scale.z);
        }
      }
    });

    this.removeKeyboardControls();
    this.controls.enabled = true;
  }

  pausePlay() {
    this.isPaused = true;
  }

  resumePlay() {
    this.isPaused = false;
  }

  initKeyboardControls() {
    this.keydownHandler = (e) => {
      if (e.code === 'KeyW' || e.code === 'ArrowUp') this.keys.w = true;
      if (e.code === 'KeyS' || e.code === 'ArrowDown') this.keys.s = true;
      if (e.code === 'KeyA' || e.code === 'ArrowLeft') this.keys.a = true;
      if (e.code === 'KeyD' || e.code === 'ArrowRight') this.keys.d = true;
      if (e.code === 'Space') {
        this.keys.space = true;
        const p = this.getPlayerEntity();
        if (p && p.hasJump && p.isGrounded) {
          p.velocity.y = p.jumpForce || 8.5;
          p.isGrounded = false;
        }
      }
      if (e.code === 'ShiftLeft' || e.code === 'ShiftRight') this.keys.shift = true;
    };

    this.keyupHandler = (e) => {
      if (e.code === 'KeyW' || e.code === 'ArrowUp') this.keys.w = false;
      if (e.code === 'KeyS' || e.code === 'ArrowDown') this.keys.s = false;
      if (e.code === 'KeyA' || e.code === 'ArrowLeft') this.keys.a = false;
      if (e.code === 'KeyD' || e.code === 'ArrowRight') this.keys.d = false;
      if (e.code === 'Space') this.keys.space = false;
      if (e.code === 'ShiftLeft' || e.code === 'ShiftRight') this.keys.shift = false;
    };

    window.addEventListener('keydown', this.keydownHandler);
    window.addEventListener('keyup', this.keyupHandler);
  }

  removeKeyboardControls() {
    if (this.keydownHandler) window.removeEventListener('keydown', this.keydownHandler);
    if (this.keyupHandler) window.removeEventListener('keyup', this.keyupHandler);
    this.keys = { w: false, a: false, s: false, d: false, space: false, shift: false };
  }

  onResize() {
    if (!this.canvas.parentElement) return;
    const rect = this.canvas.parentElement.getBoundingClientRect();
    const aspect = rect.width / rect.height;

    this.perspectiveCamera.aspect = aspect;
    this.perspectiveCamera.updateProjectionMatrix();

    this.gameCamera.aspect = aspect;
    this.gameCamera.updateProjectionMatrix();

    const frustumSize = 18;
    this.orthoCamera.left = (-frustumSize * aspect) / 2;
    this.orthoCamera.right = (frustumSize * aspect) / 2;
    this.orthoCamera.top = frustumSize / 2;
    this.orthoCamera.bottom = -frustumSize / 2;
    this.orthoCamera.updateProjectionMatrix();

    this.renderer.setSize(rect.width, rect.height);
  }

  update(delta) {
    if (this.activeCamera === this.perspectiveCamera || this.activeCamera === this.orthoCamera) {
      this.controls.update();
    }

    const playerEnt = this.getPlayerEntity();

    // --- GAME CAMERA TRACKING LOGIC (Follow vs Static) ---
    if (playerEnt && playerEnt.mesh) {
      const pPos = playerEnt.mesh.position;

      if (this.cameraConfig.mode === 'follow') {
        const lerpFactor = Math.min(1.0, (this.cameraConfig.smoothSpeed || 6.0) * delta);
        const mLook = this.cameraConfig.mouseLook;

        // Smooth mouseLook damping
        if (mLook && mLook.enabled) {
          mLook.yaw += (mLook.targetYaw - mLook.yaw) * Math.min(1.0, 14.0 * delta);
          mLook.pitch += (mLook.targetPitch - mLook.pitch) * Math.min(1.0, 14.0 * delta);
        }

        if (this.cameraConfig.preset === 'platformer') {
          // 2.5D Lateral platformer tracking: slides horizontally (X) and vertically (Y), fixed Z distance
          const targetCamX = pPos.x + this.cameraConfig.offset.x;
          const targetCamY = pPos.y + this.cameraConfig.offset.y;
          const targetCamZ = this.cameraConfig.offset.z;

          this.gameCamera.position.x += (targetCamX - this.gameCamera.position.x) * lerpFactor;
          this.gameCamera.position.y += (targetCamY - this.gameCamera.position.y) * lerpFactor;
          this.gameCamera.position.z += (targetCamZ - this.gameCamera.position.z) * lerpFactor;

          const lookTarget = new THREE.Vector3(pPos.x, pPos.y + this.cameraConfig.lookAtOffset.y, pPos.z);
          this.gameCamera.lookAt(lookTarget);
        } else if (this.cameraConfig.preset === 'top_down') {
          const targetCamX = pPos.x + this.cameraConfig.offset.x;
          const targetCamY = pPos.y + this.cameraConfig.offset.y;
          const targetCamZ = pPos.z + this.cameraConfig.offset.z;

          this.gameCamera.position.x += (targetCamX - this.gameCamera.position.x) * lerpFactor;
          this.gameCamera.position.y += (targetCamY - this.gameCamera.position.y) * lerpFactor;
          this.gameCamera.position.z += (targetCamZ - this.gameCamera.position.z) * lerpFactor;

          this.gameCamera.lookAt(pPos.x, pPos.y, pPos.z);
        } else if (this.cameraConfig.preset === 'first_person') {
          // 1st Person: at player head level looking with mouse
          const eyePos = pPos.clone().add(this.cameraConfig.offset);
          this.gameCamera.position.copy(eyePos);

          const lookDir = new THREE.Vector3(
            -Math.sin(mLook.yaw) * Math.cos(mLook.pitch),
            Math.sin(mLook.pitch),
            -Math.cos(mLook.yaw) * Math.cos(mLook.pitch)
          );
          this.gameCamera.lookAt(eyePos.clone().add(lookDir));
        } else {
          // Third-person / Custom 3D follow with spherical Mouse Look & Distance Clamping
          const baseDist = Math.max(
            this.cameraConfig.distanceLimits.minDistance,
            Math.min(this.cameraConfig.distanceLimits.maxDistance, this.cameraConfig.offset.length())
          );

          const camOffset = new THREE.Vector3(
            baseDist * Math.sin(mLook.yaw) * Math.cos(mLook.pitch),
            baseDist * Math.sin(mLook.pitch) + this.cameraConfig.offset.y,
            baseDist * Math.cos(mLook.yaw) * Math.cos(mLook.pitch)
          );

          const targetPos = pPos.clone().add(camOffset);
          this.gameCamera.position.lerp(targetPos, lerpFactor);

          const lookTarget = pPos.clone().add(this.cameraConfig.lookAtOffset);
          this.gameCamera.lookAt(lookTarget);
        }

        // Apply World Position / Boundary Clamps if enabled
        if (this.cameraConfig.limits && this.cameraConfig.limits.enabled) {
          const lim = this.cameraConfig.limits;
          this.gameCamera.position.x = Math.max(lim.minX, Math.min(lim.maxX, this.gameCamera.position.x));
          this.gameCamera.position.y = Math.max(lim.minY, Math.min(lim.maxY, this.gameCamera.position.y));
          this.gameCamera.position.z = Math.max(lim.minZ, Math.min(lim.maxZ, this.gameCamera.position.z));
        }

        if (this.cameraHelperMesh && !this.isPlaying && !this.isPilotingGameCamera) {
          this.cameraHelperMesh.position.copy(this.gameCamera.position);
          this.cameraHelperMesh.quaternion.copy(this.gameCamera.quaternion);
        }
      } else if (this.cameraConfig.mode === 'static') {
        // Static Camera: stays fixed at its placed world position
        if (this.isPlaying && this.cameraConfig.trackRotation) {
          const lookTarget = pPos.clone().add(this.cameraConfig.lookAtOffset);
          this.gameCamera.lookAt(lookTarget);
        }
      }

      if (this.cameraHelper) this.cameraHelper.update();
    }

    if (!this.isPlaying || this.isPaused) return;

    // 1. Calculate dynamic ground height for entities (Standing on obstacles/platforms)
    this.entities.forEach((ent) => {
      if (!ent.mesh || ent.shape === 'plane' || ent.type === 'camera') return;

      ent.animTime += delta;

      // 1.1 KINEMATICS & HORIZONTAL MOVEMENT
      if (ent.type === 'player') {
        if (ent.hasMove && ent.walkSpeed > 0) {
          const moveDir = new THREE.Vector3();
          const isPlatformer2D = this.cameraConfig.preset === 'platformer' || this.cameraConfig.mouseLook?.lockMouse;

          if (isPlatformer2D) {
            // 2D Lateral movement (A = Left, D = Right)
            if (this.keys.a) moveDir.x -= 1;
            if (this.keys.d) moveDir.x += 1;

            if (moveDir.lengthSq() > 0) {
              moveDir.normalize();
              const activeSpeed = ent.walkSpeed * (this.keys.shift && ent.hasRun ? ent.runMultiplier : 1.0);
              ent.velocity.x = moveDir.x * activeSpeed;
              ent.velocity.z = 0;

              ent.mesh.rotation.y = moveDir.x > 0 ? Math.PI / 2 : -Math.PI / 2;
            } else {
              ent.velocity.x *= 0.75;
              ent.velocity.z = 0;
            }
          } else {
            // 3D Camera-Relative Movement & Mouse Heading
            const camYaw = (this.cameraConfig.mouseLook && this.cameraConfig.mouseLook.enabled)
              ? this.cameraConfig.mouseLook.yaw
              : 0;

            const fwd = new THREE.Vector3(-Math.sin(camYaw), 0, -Math.cos(camYaw));
            const right = new THREE.Vector3(Math.cos(camYaw), 0, -Math.sin(camYaw));

            if (this.keys.w) moveDir.add(fwd);
            if (this.keys.s) moveDir.sub(fwd);
            if (this.keys.d) moveDir.add(right);
            if (this.keys.a) moveDir.sub(right);

            if (moveDir.lengthSq() > 0) {
              moveDir.normalize();
              const activeSpeed = ent.walkSpeed * (this.keys.shift && ent.hasRun ? ent.runMultiplier : 1.0);
              ent.velocity.x = moveDir.x * activeSpeed;
              ent.velocity.z = moveDir.z * activeSpeed;

              // Smoothly rotate character to face composite movement direction
              const targetRot = Math.atan2(-moveDir.z, moveDir.x) + Math.PI / 2;
              ent.mesh.rotation.y = THREE.MathUtils.lerp(ent.mesh.rotation.y, targetRot, Math.min(1.0, 18.0 * delta));
            } else {
              ent.velocity.x *= 0.75;
              ent.velocity.z *= 0.75;

              // When standing, player smoothly follows camera / mouse orientation!
              const idleTargetRot = -camYaw + Math.PI;
              ent.mesh.rotation.y = THREE.MathUtils.lerp(ent.mesh.rotation.y, idleTargetRot, Math.min(1.0, 10.0 * delta));
            }
          }
        } else {
          ent.velocity.x = 0;
          ent.velocity.z = 0;
        }
      } else if (ent.type === 'enemy') {
        if (ent.hasMove && playerEnt && playerEnt.mesh) {
          const toPlayer = new THREE.Vector3().subVectors(playerEnt.mesh.position, ent.mesh.position);
          toPlayer.y = 0;
          const dist = toPlayer.length();

          if (dist > 1.8 && dist < 14) {
            toPlayer.normalize();
            ent.velocity.x = toPlayer.x * (ent.walkSpeed || 3.0);
            ent.velocity.z = toPlayer.z * (ent.walkSpeed || 3.0);
            ent.mesh.rotation.y = Math.atan2(-toPlayer.z, toPlayer.x) + Math.PI / 2;
          } else {
            ent.velocity.x = 0;
            ent.velocity.z = 0;
          }
        }

        if (ent.hasJump && ent.isGrounded && Math.sin(ent.animTime * 2) > 0.95) {
          ent.velocity.y = ent.jumpForce;
          ent.isGrounded = false;
        }
      } else if (ent.type === 'npc') {
        if (ent.hasMove) {
          ent.velocity.x = Math.sin(ent.animTime * 1.5) * (ent.walkSpeed || 2.5);
          ent.mesh.rotation.y = ent.velocity.x > 0 ? Math.PI / 2 : -Math.PI / 2;
        }
      }

      // Horizontal displacement
      ent.mesh.position.x += ent.velocity.x * delta;
      ent.mesh.position.z += ent.velocity.z * delta;

      // 1.2 DYNAMIC PLATFORM & OBSTACLE GROUND LEVEL (Stand on top of obstacles)
      let targetFloorY = 1.1;

      this.entities.forEach((otherEnt) => {
        if (otherEnt === ent || !otherEnt.mesh || otherEnt.shape === 'plane' || otherEnt.type === 'camera' || !otherEnt.hasCollision || !otherEnt.isSolid) return;

        const oPos = otherEnt.mesh.position;
        const sx = otherEnt.mesh.scale.x;
        const sy = otherEnt.mesh.scale.y;
        const sz = otherEnt.mesh.scale.z;
        const baseW = otherEnt.baseSize?.x || 1.8;
        const baseH = otherEnt.baseSize?.y || 2.2;
        const baseD = otherEnt.baseSize?.z || 1.8;

        const padding = otherEnt.collisionPadding || 0;
        const halfW = ((baseW * sx) / 2) + padding;
        const halfH = (baseH * sy) / 2;
        const halfD = ((baseD * sz) / 2) + padding;
        const rotY = otherEnt.mesh.rotation.y;

        const topY = oPos.y + halfH;
        const entFeetY = ent.mesh.position.y - 1.1;

        const dx = ent.mesh.position.x - oPos.x;
        const dz = ent.mesh.position.z - oPos.z;
        const cosR = Math.cos(-rotY);
        const sinR = Math.sin(-rotY);
        const localX = dx * cosR - dz * sinR;
        const localZ = dx * sinR + dz * cosR;

        const entRadius = 0.7;
        const isInsideFootprint = Math.abs(localX) <= (halfW + entRadius * 0.4) && Math.abs(localZ) <= (halfD + entRadius * 0.4);

        if (isInsideFootprint && otherEnt.walkableTop !== false) {
          if (entFeetY >= topY - 0.4) {
            const platformFloorY = topY + 1.1;
            if (platformFloorY > targetFloorY) {
              targetFloorY = platformFloorY;
            }
          }
        }
      });

      ent.currentGroundY = targetFloorY;

      // 1.3 GRAVITY & VERTICAL PHYSICS
      if (ent.hasPhysics && ent.gravity > 0) {
        ent.velocity.y -= ent.gravity * delta;
        ent.mesh.position.y += ent.velocity.y * delta;

        if (ent.mesh.position.y <= targetFloorY) {
          ent.mesh.position.y = targetFloorY;
          ent.velocity.y = 0;
          ent.isGrounded = true;
        } else if (ent.mesh.position.y > targetFloorY + 0.08) {
          ent.isGrounded = false;
        }
      }

      // 1.4 PROCEDURAL ANIMATION
      if (ent.hasAnimation) {
        const speed2D = Math.hypot(ent.velocity.x, ent.velocity.z);
        const baseSx = ent.scale?.x || 1.0;
        const baseSy = ent.scale?.y || 1.0;
        const baseSz = ent.scale?.z || 1.0;

        if (speed2D > 0.1 && ent.isGrounded) {
          const bob = Math.abs(Math.sin(ent.animTime * (speed2D * 1.5))) * 0.15;
          ent.mesh.position.y = ent.currentGroundY + bob;
          ent.mesh.scale.set(baseSx * (1.0 + bob * 0.5), baseSy * (1.0 - bob * 0.5), baseSz * (1.0 + bob * 0.5));
        } else {
          const breath = Math.sin(ent.animTime * 3) * 0.03;
          ent.mesh.scale.set(baseSx * (1.0 - breath), baseSy * (1.0 + breath), baseSz * (1.0 - breath));
        }
      }
    });

    // 2. GRAVITATIONAL ATTRACTION (ATRAIR MECÂNICA)
    this.entities.forEach((attractorEnt) => {
      if (!attractorEnt.hasAttract || !attractorEnt.mesh || attractorEnt.shape === 'plane' || attractorEnt.type === 'camera') return;

      this.entities.forEach((targetEnt) => {
        if (targetEnt === attractorEnt || !targetEnt.mesh || targetEnt.shape === 'plane' || targetEnt.type === 'camera') return;

        const aPos = attractorEnt.mesh.position;
        const tPos = targetEnt.mesh.position;

        const toAttractor = new THREE.Vector3().subVectors(aPos, tPos);
        toAttractor.y = 0;
        const dist = toAttractor.length();

        if (dist > 1.2 && dist <= (attractorEnt.attractRadius || 15.0)) {
          toAttractor.normalize();
          const pullForce = (attractorEnt.attractForce || 12.0) / Math.max(1.0, Math.sqrt(dist));
          targetEnt.mesh.position.x += toAttractor.x * pullForce * delta;
          targetEnt.mesh.position.z += toAttractor.z * pullForce * delta;

          if (this.onCollisionEvent && Math.random() < 0.1) {
            this.onCollisionEvent(attractorEnt.familyName || attractorEnt.name, targetEnt.familyName || targetEnt.name, '#ff3b30');
          }
        }
      });
    });

    // 3. RIGID BODY LATERAL COLLISION & TRIGGER DETECTION
    if (playerEnt && playerEnt.mesh) {
      this.entities.forEach((otherEnt) => {
        if (otherEnt === playerEnt || !otherEnt.mesh || otherEnt.shape === 'plane' || otherEnt.type === 'camera') return;

        const pPos = playerEnt.mesh.position;
        const oPos = otherEnt.mesh.position;

        // 3.1 TRIGGER DETECTION (GATILHO)
        if (otherEnt.type === 'trigger') {
          const dx = pPos.x - oPos.x;
          const dz = pPos.z - oPos.z;
          const dist = Math.hypot(dx, dz);

          if (dist < 2.8) {
            if (otherEnt.mesh.material) {
              otherEnt.mesh.material.opacity = 0.85;
            }
            if (this.onCollisionEvent && Math.random() < 0.12) {
              this.onCollisionEvent(playerEnt.familyName, otherEnt.familyName);
            }
          } else {
            if (otherEnt.mesh.material) {
              otherEnt.mesh.material.opacity = 0.45;
            }
          }
          return;
        }

        // 3.2 SOLID LATERAL COLLISION
        if (playerEnt.hasCollision && otherEnt.hasCollision && otherEnt.isSolid) {
          const sx = otherEnt.mesh.scale.x;
          const sy = otherEnt.mesh.scale.y;
          const sz = otherEnt.mesh.scale.z;
          const baseW = otherEnt.baseSize?.x || 1.8;
          const baseH = otherEnt.baseSize?.y || 2.2;
          const baseD = otherEnt.baseSize?.z || 1.8;

          const padding = otherEnt.collisionPadding || 0;
          const halfW = ((baseW * sx) / 2) + padding;
          const halfH = (baseH * sy) / 2;
          const halfD = ((baseD * sz) / 2) + padding;
          const rotY = otherEnt.mesh.rotation.y;

          const topY = oPos.y + halfH;
          const bottomY = oPos.y - halfH;
          const playerFeetY = pPos.y - 1.1;
          const playerHeadY = pPos.y + 1.1;

          if (playerFeetY >= topY - 0.25) return;
          if (playerHeadY <= bottomY + 0.1) return;

          const dx = pPos.x - oPos.x;
          const dz = pPos.z - oPos.z;
          const cosR = Math.cos(-rotY);
          const sinR = Math.sin(-rotY);
          const localX = dx * cosR - dz * sinR;
          const localZ = dx * sinR + dz * cosR;

          const playerRadius = 0.7;

          const closestX = Math.max(-halfW, Math.min(halfW, localX));
          const closestZ = Math.max(-halfD, Math.min(halfD, localZ));

          const diffX = localX - closestX;
          const diffZ = localZ - closestZ;
          const distSq = diffX * diffX + diffZ * diffZ;

          if (distSq < playerRadius * playerRadius) {
            const dist = Math.sqrt(distSq);
            let pushLocalX = 0;
            let pushLocalZ = 0;

            if (dist > 0.0001) {
              const overlap = playerRadius - dist;
              pushLocalX = (diffX / dist) * overlap;
              pushLocalZ = (diffZ / dist) * overlap;
            } else {
              const penLeft = localX - (-halfW) + playerRadius;
              const penRight = halfW - localX + playerRadius;
              const penBack = localZ - (-halfD) + playerRadius;
              const penFront = halfD - localZ + playerRadius;
              const minPen = Math.min(penLeft, penRight, penBack, penFront);

              if (minPen === penLeft) pushLocalX = -penLeft;
              else if (minPen === penRight) pushLocalX = penRight;
              else if (minPen === penBack) pushLocalZ = -penBack;
              else pushLocalZ = penFront;
            }

            const worldPushX = pushLocalX * Math.cos(rotY) - pushLocalZ * Math.sin(rotY);
            const worldPushZ = pushLocalX * Math.sin(rotY) + pushLocalZ * Math.cos(rotY);

            pPos.x += worldPushX;
            pPos.z += worldPushZ;

            if (this.onCollisionEvent) {
              this.onCollisionEvent(playerEnt.familyName, otherEnt.familyName);
            }
          }
        }
      });
    }

    if (this.selectionBoxHelper && this.selectedEntity && this.selectedEntity.mesh) {
      this.selectionBoxHelper.update();
    }
  }

  animate() {
    requestAnimationFrame(() => this.animate());
    const delta = 0.016;
    this.update(delta);
    this.renderer.render(this.scene, this.activeCamera);
  }
}
