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

    // 1. Isometric Ground Plane (matching user mockup)
    const groundGeo = new THREE.PlaneGeometry(24, 24);
    const groundMat = new THREE.MeshStandardMaterial({
      color: 0x222225,
      roughness: 0.8,
      metalness: 0.1
    });
    this.groundMesh = new THREE.Mesh(groundGeo, groundMat);
    this.groundMesh.rotation.x = -Math.PI / 2;
    this.groundMesh.receiveShadow = true;
    this.groundMesh.name = 'Plano_Chao';
    this.scene.add(this.groundMesh);

    // Grid Helper
    const grid = new THREE.GridHelper(24, 24, 0x3a3a3a, 0x2a2a2a);
    grid.position.y = 0.01;
    this.scene.add(grid);

    // 2. Default Player Entity (Dark Charcoal Cube matching user mockup)
    const playerGeo = new THREE.BoxGeometry(1.6, 2.2, 1.6);
    const playerMat = new THREE.MeshStandardMaterial({
      color: 0x1f1f22,
      roughness: 0.4,
      metalness: 0.2
    });
    this.playerMesh = new THREE.Mesh(playerGeo, playerMat);
    this.playerMesh.position.set(0, 1.1, 0);
    this.playerMesh.castShadow = true;
    this.playerMesh.name = 'PLAYER_Cube';
    this.scene.add(this.playerMesh);

    // Player Top Highlight edge (matching mockup top cap edge)
    const capGeo = new THREE.EdgesGeometry(playerGeo);
    const capMat = new THREE.LineBasicMaterial({ color: 0x44444a, linewidth: 2 });
    const capLine = new THREE.LineSegments(capGeo, capMat);
    this.playerMesh.add(capLine);

    // 3. Secondary Obstacle Entity (Objeto)
    const obstGeo = new THREE.BoxGeometry(1.2, 1.2, 1.2);
    const obstMat = new THREE.MeshStandardMaterial({
      color: 0x2a2540,
      roughness: 0.6,
      metalness: 0.3
    });
    this.obstacleMesh = new THREE.Mesh(obstGeo, obstMat);
    this.obstacleMesh.position.set(4, 0.6, -2);
    this.obstacleMesh.castShadow = true;
    this.obstacleMesh.name = 'Objeto_Obstaculo';
    this.scene.add(this.obstacleMesh);

    window.addEventListener('resize', () => this.onResize());
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
    if (!playing) {
      // Reset player position on stop
      this.playerMesh.position.set(0, 1.1, 0);
      this.playerVelocity.set(0, 0, 0);
      this.isGrounded = true;
    }
  }

  onResize() {
    const rect = this.canvas.parentElement.getBoundingClientRect();
    this.camera.aspect = rect.width / rect.height;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(rect.width, rect.height);
  }

  update(delta) {
    if (!this.isPlaying) return;

    // Movement calculation
    const moveDir = new THREE.Vector3();
    if (this.keys.w) moveDir.z -= 1;
    if (this.keys.s) moveDir.z += 1;
    if (this.keys.a) moveDir.x -= 1;
    if (this.keys.d) moveDir.x += 1;

    if (moveDir.lengthSq() > 0) {
      moveDir.normalize();
      const currentSpeed = this.walkSpeed * (this.keys.shift ? this.runMultiplier : 1.0);
      this.playerMesh.position.x += moveDir.x * currentSpeed * delta;
      this.playerMesh.position.z += moveDir.z * currentSpeed * delta;
      
      // Slight tilt on run
      this.playerMesh.rotation.y = Math.atan2(-moveDir.z, moveDir.x) + Math.PI / 2;
    }

    // Gravity & Jump Physics
    this.playerVelocity.y -= this.gravity * delta;
    this.playerMesh.position.y += this.playerVelocity.y * delta;

    if (this.playerMesh.position.y <= 1.1) {
      this.playerMesh.position.y = 1.1;
      this.playerVelocity.y = 0;
      this.isGrounded = true;
    }

    // Check collision with obstacle
    if (this.obstacleMesh) {
      const dist = this.playerMesh.position.distanceTo(this.obstacleMesh.position);
      if (dist < 1.8) {
        if (this.onCollisionEvent) {
          this.onCollisionEvent(this.playerMesh, this.obstacleMesh);
        }
      }
    }
  }

  animate() {
    requestAnimationFrame(() => this.animate());
    const delta = 0.016;
    this.update(delta);
    this.renderer.render(this.scene, this.camera);
  }
}
