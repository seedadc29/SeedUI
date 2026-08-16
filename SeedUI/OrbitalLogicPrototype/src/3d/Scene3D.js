import * as THREE from 'three';

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
    // EntityData: {
    //   sunId, sunName, mesh, type ('player'|'enemy'|'npc'|'object'),
    //   hasMove, walkSpeed, hasRun, runMultiplier,
    //   hasJump, jumpForce, hasPhysics, mass, gravity,
    //   hasCollision, isSolid, hasAnimation,
    //   velocity: Vector3, isGrounded: boolean, animTime: number
    // }
    this.entities = new Map();

    this.onCollisionEvent = null;

    this.initScene();
    this.initInputs();
    this.animate();
  }

  initScene() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x181818);

    const rect = this.canvas.getBoundingClientRect();
    this.camera = new THREE.PerspectiveCamera(45, (rect.width || 400) / (rect.height || 400), 0.1, 1000);
    this.camera.position.set(8, 7, 10);
    this.camera.lookAt(0, 0.8, 0);

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

      // Default model if none specified
      if (!modelShape) {
        if (entityType === 'player') modelShape = 'cube';
        else if (entityType === 'enemy') modelShape = 'cylinder';
        else if (entityType === 'npc') modelShape = 'sphere';
        else modelShape = 'plane';
      }

      let ent = this.entities.get(sun.id);

      if (!ent) {
        // Create 3D Mesh
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
        // Check if shape changed
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

        // Update capabilities
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
        // Trigger Jump for player if capability exists
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
      // Reset all entities to initial positions
      this.entities.forEach(ent => {
        if (ent.mesh && ent.initialPos) {
          ent.mesh.position.copy(ent.initialPos);
          ent.mesh.rotation.set(0, 0, 0);
          ent.mesh.scale.set(1, 1, 1);
          ent.velocity.set(0, 0, 0);
          ent.isGrounded = true;
        }
      });
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
    this.camera.aspect = rect.width / rect.height;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(rect.width, rect.height);
  }

  // --- Real-time Physics & Kinematics Loop ---
  update(delta) {
    if (!this.isPlaying) return;

    const playerEnt = this.getPlayerEntity();

    this.entities.forEach((ent) => {
      if (!ent.mesh || ent.shape === 'plane') return;

      ent.animTime += delta;

      // 1. KINEMATICS & MOVEMENT
      if (ent.type === 'player') {
        // Player Control (STRICT: If Andar is removed, walkSpeed is 0 and player CANNOT move!)
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

            // Facing direction rotation
            ent.mesh.rotation.y = Math.atan2(-moveDir.z, moveDir.x) + Math.PI / 2;
          } else {
            // Friction deceleration
            ent.velocity.x *= 0.75;
            ent.velocity.z *= 0.75;
          }
        } else {
          // No Move capability -> complete freeze
          ent.velocity.x = 0;
          ent.velocity.z = 0;
        }
      } else if (ent.type === 'enemy') {
        // Enemy AI Vector Kinematics (Tracks player if hasMove)
        if (ent.hasMove && playerEnt && playerEnt.mesh) {
          const toPlayer = new THREE.Vector3().subVectors(playerEnt.mesh.position, ent.mesh.position);
          toPlayer.y = 0;
          const dist = toPlayer.length();

          if (dist > 1.8 && dist < 12) {
            toPlayer.normalize();
            ent.velocity.x = toPlayer.x * (ent.walkSpeed || 3.0);
            ent.velocity.z = toPlayer.z * (ent.walkSpeed || 3.0);
            ent.mesh.rotation.y = Math.atan2(-toPlayer.z, toPlayer.x) + Math.PI / 2;
          } else {
            ent.velocity.x = 0;
            ent.velocity.z = 0;
          }
        }

        // Periodic jump if enemy has Pular
        if (ent.hasJump && ent.isGrounded && Math.sin(ent.animTime * 2) > 0.95) {
          ent.velocity.y = ent.jumpForce;
          ent.isGrounded = false;
        }
      } else if (ent.type === 'npc') {
        // NPC Patrol Oscillation
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

      // Apply horizontal position displacement: P = P0 + V * dt
      ent.mesh.position.x += ent.velocity.x * delta;
      ent.mesh.position.z += ent.velocity.z * delta;

      // 3. PROCEDURAL ANIMATION (Squash & Stretch Math)
      if (ent.hasAnimation) {
        const speed2D = Math.hypot(ent.velocity.x, ent.velocity.z);
        if (speed2D > 0.1) {
          // Walk / Run bobbing
          const bob = Math.abs(Math.sin(ent.animTime * (speed2D * 1.5))) * 0.15;
          ent.mesh.position.y = (ent.hasPhysics ? ent.mesh.position.y : 1.1) + bob;
          ent.mesh.scale.set(1.0 + bob * 0.5, 1.0 - bob * 0.5, 1.0 + bob * 0.5);
        } else {
          // Idle breathing sinus
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

        // Mathematical Collision: ONLY resolve if the other object HAS Colisao planet!
        if (otherEnt.hasCollision) {
          const pPos = playerEnt.mesh.position;
          const oPos = otherEnt.mesh.position;

          const dx = pPos.x - oPos.x;
          const dz = pPos.z - oPos.z;
          const dist = Math.hypot(dx, dz);
          const minDistance = 1.8; // Radius overlap sum

          if (dist < minDistance && dist > 0.001) {
            // Mathematical Penetration Depth Vector: delta = (minDist - dist) * normal
            const overlap = minDistance - dist;
            const nx = dx / dist;
            const nz = dz / dist;

            // Push player back
            pPos.x += nx * overlap;
            pPos.z += nz * overlap;

            // Trigger energy beam in orbital graph
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
    this.renderer.render(this.scene, this.camera);
  }
}
