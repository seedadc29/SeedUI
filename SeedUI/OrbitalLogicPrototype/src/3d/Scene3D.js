import * as THREE from 'three';

export class Scene3D {
  constructor(canvas) {
    this.canvas = canvas;
    this.isPlaying = false;

    // Keys state
    this.keys = {
      w: false, a: false, s: false, d: false,
      space: false, shift: false
    };

    // Movement parameters (synced with orbital logic planets)
    this.walkSpeed = 6.0;
    this.runMultiplier = 1.8;
    this.jumpForce = 7.5;
    this.gravity = 18.0;

    // Dynamic 3D Mesh Entities Map (sun.id -> THREE.Mesh)
    this.meshEntities = new Map();
    this.playerEntityKey = null;

    // Player physics state
    this.playerVelocity = new THREE.Vector3();
    this.isGrounded = true;

    // Callbacks
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
    this.camera.position.set(7, 6, 9);
    this.camera.lookAt(0, 0.8, 0);

    this.renderer = new THREE.WebGLRenderer({ canvas: this.canvas, antialias: true });
    this.renderer.setSize(rect.width || 400, rect.height || 400);
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;

    // Lights
    const hemiLight = new THREE.HemisphereLight(0xffffff, 0x222222, 0.6);
    this.scene.add(hemiLight);

    const dirLight = new THREE.DirectionalLight(0xffffff, 0.9);
    dirLight.position.set(6, 12, 8);
    dirLight.castShadow = true;
    dirLight.shadow.mapSize.width = 1024;
    dirLight.shadow.mapSize.height = 1024;
    this.scene.add(dirLight);

    // Static Base Grid
    this.grid = new THREE.GridHelper(24, 24, 0x3a3a3a, 0x2a2a2a);
    this.grid.position.y = 0.01;
    this.scene.add(this.grid);

    window.addEventListener('resize', () => this.onResize());
  }

  // --- Dynamic Synchronization with Orbital Logic Suns ---
  syncWithOrbitalSuns(suns) {
    if (!suns || suns.length === 0) {
      // If all suns are removed/cleared, delete all 3D meshes
      this.meshEntities.forEach((mesh) => {
        this.scene.remove(mesh);
        if (mesh.geometry) mesh.geometry.dispose();
      });
      this.meshEntities.clear();
      this.playerEntityKey = null;
      return;
    }

    const currentSunIds = new Set(suns.map(s => s.id));

    // 1. Remove 3D meshes whose sun was deleted
    this.meshEntities.forEach((mesh, sunId) => {
      if (!currentSunIds.has(sunId)) {
        this.scene.remove(mesh);
        if (mesh.geometry) mesh.geometry.dispose();
        this.meshEntities.delete(sunId);
        if (this.playerEntityKey === sunId) this.playerEntityKey = null;
      }
    });

    // 2. Add or update 3D meshes for each Sun
    suns.forEach((sun, index) => {
      // Find model shape from orbiting planets
      let modelType = 'cube';
      let hasGroundPlane = false;

      // Check planets in this Sun
      sun.planets.forEach((p) => {
        const pName = p.name.toLowerCase();
        if (pName.includes('plano') || pName.includes('plane') || pName.includes('chao')) {
          hasGroundPlane = true;
        } else if (pName.includes('esfera') || pName.includes('sphere')) {
          modelType = 'sphere';
        } else if (pName.includes('cilindro') || pName.includes('cylinder')) {
          modelType = 'cylinder';
        } else if (pName.includes('triangulo') || pName.includes('cone')) {
          modelType = 'cone';
        } else if (pName.includes('cubo') || pName.includes('cube')) {
          modelType = 'cube';
        }

        // Sync Mechanics (Andar, Correr, Pular, Fisica)
        if (pName === 'andar') {
          const speedMoon = p.moons?.find(m => m.name.toLowerCase().includes('velocidade'));
          if (speedMoon && speedMoon.val) this.walkSpeed = parseFloat(speedMoon.val) || 6.0;
        } else if (pName === 'correr') {
          const multMoon = p.moons?.find(m => m.name.toLowerCase().includes('multiplicador'));
          if (multMoon && multMoon.val) this.runMultiplier = parseFloat(multMoon.val) || 1.8;
        } else if (pName === 'pular') {
          const jumpMoon = p.moons?.find(m => m.name.toLowerCase().includes('força') || m.name.toLowerCase().includes('forca'));
          if (jumpMoon && jumpMoon.val) this.jumpForce = parseFloat(jumpMoon.val) || 7.5;
        } else if (pName === 'fisica') {
          const massMoon = p.moons?.find(m => m.name.toLowerCase().includes('massa'));
          if (massMoon && massMoon.val) this.gravity = 18.0 * (parseFloat(massMoon.val) || 1.0);
        }
      });

      const isPlayer = sun.name.toUpperCase().includes('PLAYER');

      if (!this.meshEntities.has(sun.id)) {
        // Create new 3D mesh for this sun
        let mesh;
        if (hasGroundPlane || sun.name.toLowerCase().includes('objeto') || sun.name.toLowerCase().includes('plano')) {
          // Ground Plane Mesh
          const groundGeo = new THREE.PlaneGeometry(24, 24);
          const groundMat = new THREE.MeshStandardMaterial({
            color: 0x222225,
            roughness: 0.8,
            metalness: 0.1
          });
          mesh = new THREE.Mesh(groundGeo, groundMat);
          mesh.rotation.x = -Math.PI / 2;
          mesh.receiveShadow = true;
          mesh.name = sun.name;
        } else {
          // Geometry for entity
          let geo;
          let yPos = 1.1;
          if (modelType === 'sphere') {
            geo = new THREE.SphereGeometry(1.0, 32, 24);
            yPos = 1.0;
          } else if (modelType === 'cylinder') {
            geo = new THREE.CylinderGeometry(0.8, 0.8, 2.0, 24);
            yPos = 1.0;
          } else if (modelType === 'cone') {
            geo = new THREE.ConeGeometry(1.0, 2.2, 24);
            yPos = 1.1;
          } else {
            geo = new THREE.BoxGeometry(1.6, 2.2, 1.6);
            yPos = 1.1;
          }

          const mat = new THREE.MeshStandardMaterial({
            color: isPlayer ? 0x1f1f22 : (index % 2 === 1 ? 0x2a2540 : 0x223545),
            roughness: 0.5,
            metalness: 0.2
          });

          mesh = new THREE.Mesh(geo, mat);
          mesh.position.set(isPlayer ? 0 : (index * 3.5 - 2), yPos, isPlayer ? 0 : -2);
          mesh.castShadow = true;
          mesh.receiveShadow = true;
          mesh.name = sun.name;

          // Wireframe edge highlight
          const edgeGeo = new THREE.EdgesGeometry(geo);
          const edgeMat = new THREE.LineBasicMaterial({ color: isPlayer ? 0x44444a : 0x554477, linewidth: 2 });
          const edgeLine = new THREE.LineSegments(edgeGeo, edgeMat);
          mesh.add(edgeLine);
        }

        this.scene.add(mesh);
        this.meshEntities.set(sun.id, mesh);

        if (isPlayer) {
          this.playerEntityKey = sun.id;
        }
      }
    });
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
        if (this.isGrounded) {
          this.playerVelocity.y = this.jumpForce;
          this.isGrounded = false;
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
    const playerMesh = this.getPlayerMesh();
    if (!playing && playerMesh) {
      playerMesh.position.set(0, 1.1, 0);
      playerMesh.rotation.set(0, 0, 0);
      this.playerVelocity.set(0, 0, 0);
      this.isGrounded = true;
    }
  }

  getPlayerMesh() {
    if (this.playerEntityKey && this.meshEntities.has(this.playerEntityKey)) {
      return this.meshEntities.get(this.playerEntityKey);
    }
    // Fallback: search by name
    for (let mesh of this.meshEntities.values()) {
      if (mesh.name && mesh.name.toUpperCase().includes('PLAYER')) return mesh;
    }
    return Array.from(this.meshEntities.values())[0] || null;
  }

  onResize() {
    if (!this.canvas.parentElement) return;
    const rect = this.canvas.parentElement.getBoundingClientRect();
    if (rect.width === 0 || rect.height === 0) return;
    this.camera.aspect = rect.width / rect.height;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(rect.width, rect.height);
  }

  update(delta) {
    if (!this.isPlaying) return;

    const playerMesh = this.getPlayerMesh();
    if (!playerMesh) return;

    // Movement calculation
    const moveDir = new THREE.Vector3();
    if (this.keys.w) moveDir.z -= 1;
    if (this.keys.s) moveDir.z += 1;
    if (this.keys.a) moveDir.x -= 1;
    if (this.keys.d) moveDir.x += 1;

    if (moveDir.lengthSq() > 0) {
      moveDir.normalize();
      const currentSpeed = this.walkSpeed * (this.keys.shift ? this.runMultiplier : 1.0);
      playerMesh.position.x += moveDir.x * currentSpeed * delta;
      playerMesh.position.z += moveDir.z * currentSpeed * delta;
      
      // Rotate facing direction
      playerMesh.rotation.y = Math.atan2(-moveDir.z, moveDir.x) + Math.PI / 2;
    }

    // Gravity & Jump Physics
    this.playerVelocity.y -= this.gravity * delta;
    playerMesh.position.y += this.playerVelocity.y * delta;

    if (playerMesh.position.y <= 1.1) {
      playerMesh.position.y = 1.1;
      this.playerVelocity.y = 0;
      this.isGrounded = true;
    }

    // Check collision with other non-player obstacle meshes
    this.meshEntities.forEach((mesh, id) => {
      if (id !== this.playerEntityKey && mesh !== playerMesh && mesh.geometry.type !== 'PlaneGeometry') {
        const dist = playerMesh.position.distanceTo(mesh.position);
        if (dist < 1.8) {
          if (this.onCollisionEvent) {
            this.onCollisionEvent(playerMesh, mesh);
          }
        }
      }
    });
  }

  animate() {
    requestAnimationFrame(() => this.animate());
    const delta = 0.016;
    this.update(delta);
    this.renderer.render(this.scene, this.camera);
  }
}
