import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { TransformControls } from 'three/examples/jsm/controls/TransformControls.js';

export class Scene3D {
  constructor(canvas) {
    this.canvas = canvas;
    this.isPlaying = false;

    // Keyboard state
    this.keys = {
      w: false, a: false, s: false, d: false,
      space: false, shift: false
    };

    // Dynamic ECS Entities: Map(sunId -> EntityData)
    this.entities = new Map();
    this.selectedEntity = null;
    this.onCollisionEvent = null;
    this.onEntitySelected = null;

    // Raycaster for 3D selection
    this.raycaster = new THREE.Raycaster();
    this.mouse = new THREE.Vector2();

    this.initScene();
    this.initControls();
    this.initTransformControls();
    this.initShortcuts();
    this.initInputs();
    this.animate();
  }

  initScene() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x181818);

    const rect = this.canvas.getBoundingClientRect();
    const aspect = (rect.width || 400) / (rect.height || 400);

    // Perspective & Orthographic Cameras
    this.cameraPersp = new THREE.PerspectiveCamera(45, aspect, 0.1, 1000);
    this.cameraPersp.position.set(8, 7, 10);
    this.cameraPersp.lookAt(0, 0.8, 0);

    const frustumSize = 14;
    this.cameraOrtho = new THREE.OrthographicCamera(
      (-frustumSize * aspect) / 2,
      (frustumSize * aspect) / 2,
      frustumSize / 2,
      -frustumSize / 2,
      0.1,
      1000
    );
    this.cameraOrtho.position.copy(this.cameraPersp.position);
    this.cameraOrtho.lookAt(0, 0.8, 0);

    this.activeCamera = this.cameraPersp;
    this.isOrthographic = false;

    this.renderer = new THREE.WebGLRenderer({ canvas: this.canvas, antialias: true });
    this.renderer.setSize(rect.width || 400, rect.height || 400);
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;

    // Lighting
    const hemiLight = new THREE.HemisphereLight(0xffffff, 0x222222, 0.6);
    this.scene.add(hemiLight);

    const dirLight = new THREE.DirectionalLight(0xffffff, 0.9);
    dirLight.position.set(6, 12, 8);
    dirLight.castShadow = true;
    dirLight.shadow.mapSize.width = 1024;
    dirLight.shadow.mapSize.height = 1024;
    this.scene.add(dirLight);

    // Coordinate Ground Grid
    this.grid = new THREE.GridHelper(30, 30, 0x444444, 0x242424);
    this.grid.position.y = 0.005;
    this.scene.add(this.grid);

    window.addEventListener('resize', () => this.onResize());
  }

  initControls() {
    this.controls = new OrbitControls(this.activeCamera, this.canvas);
    this.controls.enableDamping = true;
    this.controls.dampingFactor = 0.08;
    this.controls.screenSpacePanning = true;
    this.controls.target.set(0, 0.8, 0);

    this.controls.mouseButtons = {
      LEFT: THREE.MOUSE.ROTATE,
      MIDDLE: THREE.MOUSE.ROTATE,
      RIGHT: THREE.MOUSE.PAN
    };

    // Smooth gating
    let downPos = { x: 0, y: 0 };
    this.canvas.addEventListener('pointerdown', (e) => {
      downPos = { x: e.clientX, y: e.clientY };
      if (this.isPlaying) {
        if (e.button === 0 && !e.altKey) this.controls.enabled = false;
        else this.controls.enabled = true;
      } else {
        if (this.transformControls && this.transformControls.dragging) {
          this.controls.enabled = false;
        } else {
          this.controls.enabled = true;
        }
      }
    });

    // Raycast selection on click
    this.canvas.addEventListener('click', (e) => {
      if (this.isPlaying) return;
      if (Math.hypot(e.clientX - downPos.x, e.clientY - downPos.y) > 4) return;
      if (this.transformControls && this.transformControls.dragging) return;

      const rect = this.canvas.getBoundingClientRect();
      this.mouse.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      this.mouse.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;

      this.raycaster.setFromCamera(this.mouse, this.activeCamera);
      const meshes = Array.from(this.entities.values()).map(e => e.mesh).filter(m => m && m.geometry.type !== 'PlaneGeometry');
      const intersects = this.raycaster.intersectObjects(meshes, true);

      if (intersects.length > 0) {
        let hitMesh = intersects[0].object;
        while (hitMesh && hitMesh.parent && hitMesh.parent !== this.scene) {
          hitMesh = hitMesh.parent;
        }
        for (const ent of this.entities.values()) {
          if (ent.mesh === hitMesh) {
            this.selectEntity(ent);
            break;
          }
        }
      } else {
        this.selectEntity(null);
      }
    });
  }

  // --- Interactive Transform Gizmo (Translate, Rotate, Scale) ---
  initTransformControls() {
    this.transformControls = new TransformControls(this.activeCamera, this.canvas);
    this.transformControls.size = 0.85;
    this.transformControls.setSpace('world');
    this.transformControls.setMode('translate');

    this.transformControls.addEventListener('dragging-changed', (e) => {
      this.controls.enabled = !e.value;
      if (!e.value && this.selectedEntity) {
        // Update initialPos when dragging ends
        this.selectedEntity.initialPos.copy(this.selectedEntity.mesh.position);
      }
    });

    const helper = this.transformControls.getHelper ? this.transformControls.getHelper() : this.transformControls;
    this.scene.add(helper);
  }

  setGizmoMode(mode) { // 'translate' | 'rotate' | 'scale'
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

    if (entity && entity.mesh && entity.shape !== 'plane') {
      this.transformControls.attach(entity.mesh);

      this.selectionBoxHelper = new THREE.BoxHelper(entity.mesh, 0xff7700);
      this.selectionBoxHelper.material.linewidth = 2;
      this.selectionBoxHelper.material.depthTest = false;
      this.selectionBoxHelper.renderOrder = 999;
      this.scene.add(this.selectionBoxHelper);

      if (this.onEntitySelected) {
        this.onEntitySelected(entity.sunId);
      }
    } else {
      this.transformControls.detach();
      if (this.onEntitySelected) {
        this.onEntitySelected(null);
      }
    }
  }

  selectEntityBySunId(sunId) {
    if (!sunId) {
      this.selectEntity(null);
      return;
    }
    const ent = this.entities.get(sunId);
    if (ent) this.selectEntity(ent);
  }

  // --- Complete Blender Viewport Shortcuts ---
  initShortcuts() {
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT') return;
      if (this.isPlaying) return;

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
    const dist = 14;
    let newPos = new THREE.Vector3();

    switch (viewName) {
      case 'front': newPos.set(target.x, target.y, target.z + dist); break;
      case 'back': newPos.set(target.x, target.y, target.z - dist); break;
      case 'right': newPos.set(target.x + dist, target.y, target.z); break;
      case 'left': newPos.set(target.x - dist, target.y, target.z); break;
      case 'top': newPos.set(target.x, target.y + dist, target.z + 0.001); break;
      case 'bottom': newPos.set(target.x, target.y - dist, target.z + 0.001); break;
    }

    this.animateCameraTo(newPos, target);
  }

  toggleProjection() {
    this.isOrthographic = !this.isOrthographic;
    const oldCam = this.activeCamera;
    const newCam = this.isOrthographic ? this.cameraOrtho : this.cameraPersp;

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

  // --- Dynamic Synchronization with Orbital Logic Suns ---
  syncWithOrbitalSuns(suns) {
    if (!suns || suns.length === 0) {
      this.entities.forEach(ent => {
        if (ent.mesh) {
          this.scene.remove(ent.mesh);
          ent.mesh.geometry?.dispose();
        }
      });
      this.entities.clear();
      this.selectEntity(null);
      return;
    }

    const activeSunIds = new Set(suns.map(s => s.id));

    // 1. Remove deleted entities
    this.entities.forEach((ent, sunId) => {
      if (!activeSunIds.has(sunId)) {
        if (ent.mesh) {
          this.scene.remove(ent.mesh);
          ent.mesh.geometry?.dispose();
        }
        if (this.selectedEntity === ent) this.selectEntity(null);
        this.entities.delete(sunId);
      }
    });

    // 2. Build / Update entity capabilities based strictly on active planets
    suns.forEach((sun, index) => {
      const sunNameUpper = sun.name.toUpperCase();
      let entityType = 'object';
      if (sunNameUpper.includes('PLAYER')) entityType = 'player';
      else if (sunNameUpper.includes('INIMIGO') || sunNameUpper.includes('ENEMY')) entityType = 'enemy';
      else if (sunNameUpper.includes('NPC')) entityType = 'npc';
      else if (sunNameUpper.includes('BLOCO') || sunNameUpper.includes('COLISAO') || sunNameUpper.includes('COLISÃO') || sunNameUpper.includes('WALL') || sunNameUpper.includes('PAREDE')) entityType = 'block';
      else if (sunNameUpper.includes('GATILHO') || sunNameUpper.includes('TRIGGER')) entityType = 'trigger';
      else if (sunNameUpper.includes('PLATAFORMA') || sunNameUpper.includes('PLATFORM')) entityType = 'platform';

      let modelShape = null;
      let hasGroundPlane = false;
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
          hasGroundPlane = true;
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
          jumpForce = jumpMoon && jumpMoon.val !== undefined ? Math.max(0, parseFloat(jumpMoon.val) || 7.5) : 7.5;
        } else if (pName === 'fisica') {
          hasPhysics = true;
          const massMoon = planet.moons?.find(m => m.name.toLowerCase().includes('massa'));
          mass = massMoon && massMoon.val !== undefined ? Math.max(0.1, parseFloat(massMoon.val) || 1.0) : 1.0;
          gravity = 18.0 * mass;
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

      let ent = this.entities.get(sun.id);

      if (!ent) {
        const mesh = this.createMeshForShape(modelShape, entityType, index);
        mesh.name = sun.name;
        this.scene.add(mesh);

        ent = {
          sunId: sun.id,
          sunName: sun.name,
          mesh,
          shape: modelShape,
          type: entityType,
          hasMove, walkSpeed,
          hasRun, runMultiplier,
          hasJump, jumpForce,
          hasPhysics, mass, gravity,
          hasCollision, isSolid,
          hasAnimation,
          hasAttract, attractForce, attractRadius,
          velocity: new THREE.Vector3(),
          isGrounded: true,
          animTime: Math.random() * 10,
          initialPos: mesh.position.clone()
        };
        this.entities.set(sun.id, ent);

        // Auto select player by default
        if (entityType === 'player' && !this.selectedEntity) {
          this.selectEntity(ent);
        }
      } else {
        if (ent.shape !== modelShape || ent.type !== entityType) {
          this.scene.remove(ent.mesh);
          ent.mesh.geometry?.dispose();
          const newMesh = this.createMeshForShape(modelShape, entityType, index);
          newMesh.position.copy(ent.mesh.position);
          newMesh.name = sun.name;
          this.scene.add(newMesh);
          ent.mesh = newMesh;
          ent.shape = modelShape;
          if (this.selectedEntity === ent) this.transformControls.attach(newMesh);
        }

        ent.sunName = sun.name;
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
      }
    });
  }

  createMeshForShape(shape, type, index) {
    let geo;
    let mat;
    let yPos = 1.1;

    if (type === 'platform' || (shape === 'plane' && type !== 'player')) {
      if (type === 'platform') {
        geo = new THREE.BoxGeometry(5.0, 0.4, 5.0);
        mat = new THREE.MeshStandardMaterial({ color: 0x1e3a5f, roughness: 0.4, metalness: 0.3 });
        const mesh = new THREE.Mesh(geo, mat);
        mesh.position.set(index * 4.0 - 4, 0.2, -1.0);
        mesh.receiveShadow = true;
        mesh.castShadow = true;
        const edgeGeo = new THREE.EdgesGeometry(geo);
        const edgeMat = new THREE.LineBasicMaterial({ color: 0x38bdf8, linewidth: 2 });
        mesh.add(new THREE.LineSegments(edgeGeo, edgeMat));
        return mesh;
      }
      geo = new THREE.PlaneGeometry(28, 28);
      mat = new THREE.MeshStandardMaterial({ color: 0x1c1c20, roughness: 0.85, metalness: 0.1 });
      const mesh = new THREE.Mesh(geo, mat);
      mesh.rotation.x = -Math.PI / 2;
      mesh.receiveShadow = true;
      return mesh;
    }

    if (shape === 'sphere') {
      geo = new THREE.SphereGeometry(1.0, 32, 24);
      yPos = 1.0;
    } else if (shape === 'cylinder') {
      geo = new THREE.CylinderGeometry(0.8, 0.8, 2.0, 24);
      yPos = 1.0;
    } else if (shape === 'cone') {
      geo = new THREE.ConeGeometry(1.0, 2.2, 24);
      yPos = 1.1;
    } else {
      geo = new THREE.BoxGeometry(1.8, 2.2, 1.8);
      yPos = 1.1;
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
    mesh.position.set(type === 'player' ? 0 : (index * 3.5 - 3.5), yPos, type === 'player' ? 0 : -3.0);
    mesh.castShadow = !isTransparent;
    mesh.receiveShadow = true;

    const edgeGeo = new THREE.EdgesGeometry(geo);
    const edgeMat = new THREE.LineBasicMaterial({ color: edgeColor, linewidth: 2 });
    const edgeLine = new THREE.LineSegments(edgeGeo, edgeMat);
    mesh.add(edgeLine);

    return mesh;
  }

  initInputs() {
    window.addEventListener('keydown', (e) => {
      if (!this.isPlaying) return;
      const k = e.key.toLowerCase();
      if (k === 'w' || k === 'arrowup') this.keys.w = true;
      if (k === 's' || k === 'arrowdown') this.keys.s = true;
      if (k === 'a' || k === 'arrowleft') this.keys.a = true;
      if (k === 'd' || k === 'arrowright') this.keys.d = true;
      if (k === ' ' || e.code === 'Space') {
        this.keys.space = true;
        const playerEnt = this.getPlayerEntity();
        if (playerEnt && playerEnt.hasJump && playerEnt.isGrounded) {
          playerEnt.velocity.y = playerEnt.jumpForce;
          playerEnt.isGrounded = false;
        }
      }
      if (e.shiftKey) this.keys.shift = true;
    });

    window.addEventListener('keyup', (e) => {
      const k = e.key.toLowerCase();
      if (k === 'w' || k === 'arrowup') this.keys.w = false;
      if (k === 's' || k === 'arrowdown') this.keys.s = false;
      if (k === 'a' || k === 'arrowleft') this.keys.a = false;
      if (k === 'd' || k === 'arrowright') this.keys.d = false;
      if (k === ' ' || e.code === 'Space') this.keys.space = false;
      if (!e.shiftKey) this.keys.shift = false;
    });
  }

  setPlayMode(playing) {
    this.isPlaying = playing;
    if (this.transformControls) {
      if (playing) this.transformControls.detach();
      else if (this.selectedEntity && this.selectedEntity.mesh) this.transformControls.attach(this.selectedEntity.mesh);
    }

    if (!playing) {
      this.entities.forEach(ent => {
        if (ent.mesh && ent.initialPos) {
          ent.mesh.position.copy(ent.initialPos);
          ent.mesh.rotation.set(0, 0, 0);
          ent.mesh.scale.set(1, 1, 1);
          ent.velocity.set(0, 0, 0);
          ent.isGrounded = true;
        }
      });
      this.controls.enabled = true;
    }
  }

  getPlayerEntity() {
    for (const ent of this.entities.values()) {
      if (ent.type === 'player') return ent;
    }
    return null;
  }

  onResize() {
    if (!this.canvas.parentElement) return;
    const rect = this.canvas.parentElement.getBoundingClientRect();
    if (rect.width === 0 || rect.height === 0) return;
    const aspect = rect.width / rect.height;

    this.cameraPersp.aspect = aspect;
    this.cameraPersp.updateProjectionMatrix();

    const frustumSize = 14;
    this.cameraOrtho.left = (-frustumSize * aspect) / 2;
    this.cameraOrtho.right = (frustumSize * aspect) / 2;
    this.cameraOrtho.top = frustumSize / 2;
    this.cameraOrtho.bottom = -frustumSize / 2;
    this.cameraOrtho.updateProjectionMatrix();

    this.renderer.setSize(rect.width, rect.height);
  }

  update(delta) {
    this.controls.update();

    if (!this.isPlaying) return;

    const playerEnt = this.getPlayerEntity();

    this.entities.forEach((ent) => {
      if (!ent.mesh || ent.shape === 'plane') return;

      ent.animTime += delta;

      // 1. KINEMATICS & MOVEMENT
      if (ent.type === 'player') {
        if (ent.hasMove && ent.walkSpeed > 0) {
          const moveDir = new THREE.Vector3();
          if (this.keys.w) moveDir.z -= 1;
          if (this.keys.s) moveDir.z += 1;
          if (this.keys.a) moveDir.x -= 1;
          if (this.keys.d) moveDir.x += 1;

          if (moveDir.lengthSq() > 0) {
            moveDir.normalize();
            const activeSpeed = ent.walkSpeed * (this.keys.shift && ent.hasRun ? ent.runMultiplier : 1.0);
            ent.velocity.x = moveDir.x * activeSpeed;
            ent.velocity.z = moveDir.z * activeSpeed;

            ent.mesh.rotation.y = Math.atan2(-moveDir.z, moveDir.x) + Math.PI / 2;
          } else {
            ent.velocity.x *= 0.75;
            ent.velocity.z *= 0.75;
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

      // 2. GRAVITY & VERTICAL PHYSICS
      if (ent.hasPhysics && ent.gravity > 0) {
        ent.velocity.y -= ent.gravity * delta;
        ent.mesh.position.y += ent.velocity.y * delta;

        if (ent.mesh.position.y <= 1.1) {
          ent.mesh.position.y = 1.1;
          ent.velocity.y = 0;
          ent.isGrounded = true;
        }
      }

      ent.mesh.position.x += ent.velocity.x * delta;
      ent.mesh.position.z += ent.velocity.z * delta;

      // 3. PROCEDURAL ANIMATION
      if (ent.hasAnimation) {
        const speed2D = Math.hypot(ent.velocity.x, ent.velocity.z);
        if (speed2D > 0.1) {
          const bob = Math.abs(Math.sin(ent.animTime * (speed2D * 1.5))) * 0.15;
          ent.mesh.position.y = (ent.hasPhysics ? ent.mesh.position.y : 1.1) + bob;
          ent.mesh.scale.set(1.0 + bob * 0.5, 1.0 - bob * 0.5, 1.0 + bob * 0.5);
        } else {
          const breath = Math.sin(ent.animTime * 3) * 0.03;
          ent.mesh.scale.set(1.0 - breath, 1.0 + breath, 1.0 - breath);
        }
      } else {
        ent.mesh.scale.set(1, 1, 1);
      }
    });

    // 4. GRAVITATIONAL ATTRACTION (ATRAIR MECÂNICA)
    if (playerEnt && playerEnt.mesh && playerEnt.hasAttract) {
      this.entities.forEach((otherEnt) => {
        if (otherEnt === playerEnt || !otherEnt.mesh || otherEnt.shape === 'plane') return;

        const pPos = playerEnt.mesh.position;
        const oPos = otherEnt.mesh.position;

        const toPlayer = new THREE.Vector3().subVectors(pPos, oPos);
        toPlayer.y = 0;
        const dist = toPlayer.length();

        if (dist > 1.2 && dist <= (playerEnt.attractRadius || 15.0)) {
          toPlayer.normalize();
          // Mathematical gravitational attraction: F = G / sqrt(d)
          const pullForce = (playerEnt.attractForce || 12.0) / Math.max(1.0, Math.sqrt(dist));
          otherEnt.mesh.position.x += toPlayer.x * pullForce * delta;
          otherEnt.mesh.position.z += toPlayer.z * pullForce * delta;

          if (this.onCollisionEvent && Math.random() < 0.08) {
            this.onCollisionEvent(playerEnt.sunName, otherEnt.sunName);
          }
        }
      });
    }

    // 5. RIGID BODY COLLISION & TRIGGER DETECTION
    if (playerEnt && playerEnt.mesh) {
      this.entities.forEach((otherEnt) => {
        if (otherEnt === playerEnt || !otherEnt.mesh || otherEnt.shape === 'plane') return;

        const pPos = playerEnt.mesh.position;
        const oPos = otherEnt.mesh.position;
        const dx = pPos.x - oPos.x;
        const dz = pPos.z - oPos.z;
        const dist = Math.hypot(dx, dz);

        // 5.1 TRIGGER DETECTION (GATILHO)
        if (otherEnt.type === 'trigger') {
          if (dist < 2.8) {
            if (otherEnt.mesh.material) {
              otherEnt.mesh.material.opacity = 0.85;
            }
            if (this.onCollisionEvent && Math.random() < 0.12) {
              this.onCollisionEvent(playerEnt.sunName, otherEnt.sunName);
            }
          } else {
            if (otherEnt.mesh.material) {
              otherEnt.mesh.material.opacity = 0.45;
            }
          }
          return;
        }

        // 5.2 SOLID COLLISION WITH BLOCKS, ENEMIES & PROPS
        if (playerEnt.hasCollision && otherEnt.hasCollision) {
          const minDistance = otherEnt.type === 'block' ? 2.1 : 1.8;

          if (dist < minDistance && dist > 0.001) {
            const overlap = minDistance - dist;
            const nx = dx / dist;
            const nz = dz / dist;

            pPos.x += nx * overlap;
            pPos.z += nz * overlap;

            if (this.onCollisionEvent) {
              this.onCollisionEvent(playerEnt.sunName, otherEnt.sunName);
            }
          }
        }
      });
    }

    // Update selection box outline
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
