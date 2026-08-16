import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';

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
    this.onCollisionEvent = null;

    // Camera view transition state
    this.isTransitioningCamera = false;

    this.initScene();
    this.initControls();
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

    // Authentic Blender Mouse Mapping:
    // Left Drag (when not in Play Mode): Rotate
    // Middle Drag: Rotate
    // Shift + Middle / Shift + Left: Pan
    // Ctrl + Middle / Wheel: Zoom
    this.controls.mouseButtons = {
      LEFT: THREE.MOUSE.ROTATE,
      MIDDLE: THREE.MOUSE.ROTATE,
      RIGHT: THREE.MOUSE.PAN
    };

    // Smooth gating
    this.canvas.addEventListener('pointerdown', (e) => {
      if (this.isPlaying) {
        // In play mode, LMB is for gameplay interaction
        if (e.button === 0 && !e.altKey) {
          this.controls.enabled = false;
        } else {
          this.controls.enabled = true;
        }
      } else {
        this.controls.enabled = true;
      }
    });
  }

  // --- Complete Blender Viewport Shortcuts ---
  initShortcuts() {
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT') return;
      if (this.isPlaying) return; // In play mode keys move player

      // Numpad 1 / 1: Front View (Trás com Ctrl)
      if (e.code === 'Numpad1' || e.code === 'Digit1') {
        e.preventDefault();
        if (e.ctrlKey) this.setCameraView('back');
        else this.setCameraView('front');
      }
      // Numpad 3 / 3: Right View (Esquerda com Ctrl)
      else if (e.code === 'Numpad3' || e.code === 'Digit3') {
        e.preventDefault();
        if (e.ctrlKey) this.setCameraView('left');
        else this.setCameraView('right');
      }
      // Numpad 7 / 7: Top View (Inferior com Ctrl)
      else if (e.code === 'Numpad7' || e.code === 'Digit7') {
        e.preventDefault();
        if (e.ctrlKey) this.setCameraView('bottom');
        else this.setCameraView('top');
      }
      // Numpad 5 / 5: Toggle Persp / Ortho
      else if (e.code === 'Numpad5' || e.code === 'Digit5') {
        e.preventDefault();
        this.toggleProjection();
      }
      // Numpad . / F / Home: Frame / Focus Selected Entity
      else if (e.code === 'NumpadDecimal' || e.code === 'KeyF' || e.code === 'Home') {
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
    this.controls.update();
    this.onResize();
  }

  frameSelectedEntity() {
    const playerEnt = this.getPlayerEntity();
    const target = playerEnt && playerEnt.mesh ? playerEnt.mesh.position.clone() : new THREE.Vector3(0, 0.8, 0);
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

  // --- Strict Mathematical Sync with Orbital Logic Graph ---
  syncWithOrbitalSuns(suns) {
    if (!suns || suns.length === 0) {
      this.entities.forEach(ent => {
        if (ent.mesh) {
          this.scene.remove(ent.mesh);
          ent.mesh.geometry?.dispose();
        }
      });
      this.entities.clear();
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

      // Inspect orbiting planets
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

      sun.planets.forEach(planet => {
        const pName = planet.name.toLowerCase();

        // Model Mesh
        if (pName.includes('cubo') || pName.includes('cube')) modelShape = 'cube';
        else if (pName.includes('esfera') || pName.includes('sphere')) modelShape = 'sphere';
        else if (pName.includes('cilindro') || pName.includes('cylinder')) modelShape = 'cylinder';
        else if (pName.includes('triangulo') || pName.includes('cone')) modelShape = 'cone';
        else if (pName.includes('plano') || pName.includes('plane') || pName.includes('chao')) {
          modelShape = 'plane';
          hasGroundPlane = true;
        }

        // Capabilities
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
        } else if (pName === 'animacao') {
          hasAnimation = true;
        }
      });

      if (!modelShape) {
        if (entityType === 'player') modelShape = 'cube';
        else if (entityType === 'enemy') modelShape = 'cylinder';
        else if (entityType === 'npc') modelShape = 'sphere';
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
          velocity: new THREE.Vector3(),
          isGrounded: true,
          animTime: Math.random() * 10,
          initialPos: mesh.position.clone()
        };
        this.entities.set(sun.id, ent);
      } else {
        if (ent.shape !== modelShape) {
          this.scene.remove(ent.mesh);
          ent.mesh.geometry?.dispose();
          const newMesh = this.createMeshForShape(modelShape, entityType, index);
          newMesh.position.copy(ent.mesh.position);
          newMesh.name = sun.name;
          this.scene.add(newMesh);
          ent.mesh = newMesh;
          ent.shape = modelShape;
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
      }
    });
  }

  createMeshForShape(shape, type, index) {
    let geo;
    let mat;
    let yPos = 1.1;

    if (shape === 'plane') {
      geo = new THREE.PlaneGeometry(24, 24);
      mat = new THREE.MeshStandardMaterial({ color: 0x222225, roughness: 0.8, metalness: 0.1 });
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
      geo = new THREE.BoxGeometry(1.6, 2.2, 1.6);
      yPos = 1.1;
    }

    let color = 0x1f1f22; // Player dark charcoal
    if (type === 'enemy') color = 0x5a1818; // Crimson Enemy
    else if (type === 'npc') color = 0x184a28; // Forest NPC
    else if (type === 'object') color = index % 2 === 1 ? 0x2a2540 : 0x203242;

    mat = new THREE.MeshStandardMaterial({ color, roughness: 0.45, metalness: 0.2 });
    const mesh = new THREE.Mesh(geo, mat);
    mesh.position.set(type === 'player' ? 0 : (index * 3.5 - 2), yPos, type === 'player' ? 0 : -2.5);
    mesh.castShadow = true;
    mesh.receiveShadow = true;

    // Edge highlight
    const edgeGeo = new THREE.EdgesGeometry(geo);
    const edgeMat = new THREE.LineBasicMaterial({
      color: type === 'player' ? 0x555560 : (type === 'enemy' ? 0xff4444 : 0x44ff88),
      linewidth: 2
    });
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

    // 4. RIGID BODY COLLISION & PENETRATION RESOLUTION
    if (playerEnt && playerEnt.mesh && playerEnt.hasCollision) {
      this.entities.forEach((otherEnt) => {
        if (otherEnt === playerEnt || !otherEnt.mesh || otherEnt.shape === 'plane') return;

        if (otherEnt.hasCollision) {
          const pPos = playerEnt.mesh.position;
          const oPos = otherEnt.mesh.position;

          const dx = pPos.x - oPos.x;
          const dz = pPos.z - oPos.z;
          const dist = Math.hypot(dx, dz);
          const minDistance = 1.8;

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
  }

  animate() {
    requestAnimationFrame(() => this.animate());
    const delta = 0.016;
    this.update(delta);
    this.renderer.render(this.scene, this.activeCamera);
  }
}
