import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

/**
 * FilamentBridge.js - Interactive Google Filament & High-Fidelity PBR Studio Engine for Seed3D
 * 
 * Provides:
 * - Live interactive 3D PBR viewport with Orbit, Pan & Zoom camera navigation
 * - Realistic studio physical lighting (Sun + Ambient Sky IBL + Studio Rim lights)
 * - 6 Physically Based Materials (Liquid Glass, 24K Gold, Chrome, Ceramic, Metallic Paint, Obsidian)
 * - Seamless live synchronization with active Scene objects
 * - High-resolution, non-black PNG snapshot rendering
 */
export class FilamentBridge {
  constructor(canvasElement, mainEngine = null, sceneManager = null) {
    this.canvas = canvasElement;
    this.container = canvasElement.parentElement;
    this.mainEngine = mainEngine;
    this.sceneManager = sceneManager;

    this.isReady = false;
    this.isLoading = false;
    this.currentMaterial = 'glass';
    this.sunIntensity = 2.5;
    this.iblIntensity = 1.0;

    this.scene = null;
    this.camera = null;
    this.renderer = null;
    this.controls = null;
    this.renderObjectsGroup = null;
    this.animFrameId = null;

    this.initPBRStudio();
  }

  initPBRStudio() {
    if (!this.canvas || !this.container) return;

    const width = this.container.clientWidth || 640;
    const height = this.container.clientHeight || 480;

    // 1. Scene
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x0a0c12);

    // 2. Camera (Blender Z-Up: X=Right, Y=Depth, Z=Height)
    this.camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 500);
    this.camera.up.set(0, 0, 1);
    this.camera.position.set(5.5, -6.5, 4.5);
    this.camera.lookAt(0, 0, 0);

    // 3. Renderer with preserveDrawingBuffer enabled (Guarantees crisp non-black snapshots)
    this.renderer = new THREE.WebGLRenderer({
      canvas: this.canvas,
      antialias: true,
      preserveDrawingBuffer: true,
      powerPreference: 'high-performance'
    });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2.0));
    this.renderer.setSize(width, height);
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.15;
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;

    // 4. Interactive Orbit Controls for Studio Camera
    this.controls = new OrbitControls(this.camera, this.canvas);
    this.controls.enableDamping = true;
    this.controls.dampingFactor = 0.08;
    this.controls.target.set(0, 0, 0);

    // 5. Studio Physical Lighting & Shadows
    this.setupLighting();

    // 6. Ground Shadow Catcher Plane
    const groundGeo = new THREE.PlaneGeometry(30, 30);
    const groundMat = new THREE.ShadowMaterial({ opacity: 0.35 });
    this.ground = new THREE.Mesh(groundGeo, groundMat);
    this.ground.rotation.x = 0; // XY plane in Z-Up
    this.ground.position.z = -0.01;
    this.ground.receiveShadow = true;
    this.scene.add(this.ground);

    // 7. Render Group
    this.renderObjectsGroup = new THREE.Group();
    this.renderObjectsGroup.name = 'Filament_Objects_Group';
    this.scene.add(this.renderObjectsGroup);

    this.isReady = true;
    this.startLoop();
  }

  setupLighting() {
    this.lightsGroup = new THREE.Group();

    // Key Light (Main Solar Light with Sharp Physical Shadow)
    this.sunLight = new THREE.DirectionalLight(0xfff8f0, this.sunIntensity);
    this.sunLight.position.set(6, -8, 10);
    this.sunLight.castShadow = true;
    this.sunLight.shadow.mapSize.width = 2048;
    this.sunLight.shadow.mapSize.height = 2048;
    this.sunLight.shadow.bias = -0.0001;
    this.lightsGroup.add(this.sunLight);

    // Fill Light (Soft Cyan/Sky Bounce)
    this.fillLight = new THREE.DirectionalLight(0x8bc34a, 0.4);
    this.fillLight.position.set(-6, 6, 4);
    this.lightsGroup.add(this.fillLight);

    // Rim Light (Back Light for Crisp Edges)
    this.rimLight = new THREE.DirectionalLight(0x38bdf8, 0.9);
    this.rimLight.position.set(0, 8, 3);
    this.lightsGroup.add(this.rimLight);

    // Ambient / IBL Sky Reflection Light
    this.hemiLight = new THREE.HemisphereLight(0xe0f2fe, 0x0f172a, this.iblIntensity);
    this.lightsGroup.add(this.hemiLight);

    this.scene.add(this.lightsGroup);
  }

  /**
   * Generates Physically Based Material according to the selected type
   */
  createPBRMaterial(type) {
    switch (type) {
      case 'glass':
        return new THREE.MeshPhysicalMaterial({
          color: 0xffffff,
          transmission: 0.92,
          opacity: 1.0,
          transparent: true,
          roughness: 0.05,
          ior: 1.52,
          thickness: 1.5,
          specularIntensity: 1.0,
          specularColor: new THREE.Color(0xffffff),
          side: THREE.DoubleSide
        });

      case 'gold':
        return new THREE.MeshStandardMaterial({
          color: 0xffd700,
          metalness: 0.95,
          roughness: 0.12,
          side: THREE.DoubleSide
        });

      case 'chrome':
        return new THREE.MeshStandardMaterial({
          color: 0xf1f5f9,
          metalness: 1.0,
          roughness: 0.02,
          side: THREE.DoubleSide
        });

      case 'ceramic':
        return new THREE.MeshPhysicalMaterial({
          color: 0xf8fafc,
          roughness: 0.18,
          metalness: 0.02,
          clearcoat: 1.0,
          clearcoatRoughness: 0.06,
          side: THREE.DoubleSide
        });

      case 'carpaint':
        return new THREE.MeshPhysicalMaterial({
          color: 0xef4444,
          metalness: 0.85,
          roughness: 0.22,
          clearcoat: 0.9,
          clearcoatRoughness: 0.05,
          side: THREE.DoubleSide
        });

      case 'obsidian':
        return new THREE.MeshPhysicalMaterial({
          color: 0x18181b,
          roughness: 0.32,
          metalness: 0.2,
          clearcoat: 0.4,
          side: THREE.DoubleSide
        });

      default:
        return new THREE.MeshStandardMaterial({
          color: 0x94a3b8,
          roughness: 0.4,
          metalness: 0.1,
          side: THREE.DoubleSide
        });
    }
  }

  /**
   * Synchronizes active meshes from the SceneManager into the Filament PBR Studio
   */
  syncScene(sceneManager) {
    if (!sceneManager || !this.renderObjectsGroup) return;

    // Clear previous studio objects
    while (this.renderObjectsGroup.children.length > 0) {
      const child = this.renderObjectsGroup.children[0];
      this.renderObjectsGroup.remove(child);
      if (child.geometry) child.geometry.dispose();
    }

    const meshes = sceneManager.getAllMeshes().filter(obj => obj.visible && !obj.userData.isCamera && obj.isMesh);
    const pbrMat = this.createPBRMaterial(this.currentMaterial);

    if (meshes.length === 0) {
      // Create a default sphere if scene is empty
      const geom = new THREE.SphereGeometry(1.2, 64, 64);
      const m = new THREE.Mesh(geom, pbrMat);
      m.position.set(0, 0, 1.2);
      m.castShadow = true;
      m.receiveShadow = true;
      this.renderObjectsGroup.add(m);
    } else {
      meshes.forEach((mesh) => {
        if (!mesh.geometry) return;
        const geom = mesh.geometry.clone();
        const studioMesh = new THREE.Mesh(geom, pbrMat);
        studioMesh.position.copy(mesh.position);
        studioMesh.rotation.copy(mesh.rotation);
        studioMesh.scale.copy(mesh.scale);
        studioMesh.castShadow = true;
        studioMesh.receiveShadow = true;
        this.renderObjectsGroup.add(studioMesh);
      });
    }

    this.render();
  }

  setMaterial(materialName) {
    this.currentMaterial = materialName;
    const pbrMat = this.createPBRMaterial(materialName);
    this.renderObjectsGroup.children.forEach((child) => {
      if (child.isMesh) {
        child.material = pbrMat;
      }
    });
    this.render();
  }

  setSunIntensity(intensityVal) {
    this.sunIntensity = (intensityVal / 100000) * 2.5;
    if (this.sunLight) this.sunLight.intensity = this.sunIntensity;
  }

  setIblIntensity(intensityVal) {
    this.iblIntensity = (intensityVal / 35000) * 1.0;
    if (this.hemiLight) this.hemiLight.intensity = this.iblIntensity;
  }

  onResize() {
    if (!this.container || !this.renderer || !this.camera) return;
    const width = this.container.clientWidth;
    const height = this.container.clientHeight;
    if (width === 0 || height === 0) return;

    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(width, height);
    this.render();
  }

  syncCamera(sourceCamera) {
    if (!sourceCamera || !this.camera) return;
    this.camera.position.copy(sourceCamera.position);
    this.camera.rotation.copy(sourceCamera.rotation);
    this.camera.up.copy(sourceCamera.up);
    if (this.controls) this.controls.target.set(0, 0, 0);
  }

  render() {
    if (!this.renderer || !this.scene || !this.camera) return;
    if (this.controls) this.controls.update();
    this.renderer.render(this.scene, this.camera);
  }

  startLoop() {
    const loop = () => {
      this.render();
      this.animFrameId = requestAnimationFrame(loop);
    };
    this.animFrameId = requestAnimationFrame(loop);
  }

  stopLoop() {
    if (this.animFrameId) {
      cancelAnimationFrame(this.animFrameId);
      this.animFrameId = null;
    }
  }

  captureSnapshot() {
    this.render();
    return this.canvas.toDataURL('image/png');
  }

  dispose() {
    this.stopLoop();
    if (this.renderer) {
      this.renderer.dispose();
      this.renderer = null;
    }
  }
}
