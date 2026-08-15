import * as THREE from 'three';
import { QuadMesh } from '../mesh/QuadMesh.js';
import { ZooPresets } from '../mesh/ZooPresets.js';

export class SceneManager {
  constructor(engine) {
    this.engine = engine;
    this.historyManager = null;
    this.objects = [];
    this.selectedObjects = new Set(); // Multi-selection set
    this.selectedObject = null;       // Active primary selected object
    this.showEdges = true;
    this.onSelectionChange = null;
    this.onPrimitiveCreated = null;

    // Blender 4.1 Viewport Solid Material (Option 3: 2x2 Subdivided Quad Grid)
    this.defaultMaterial = new THREE.MeshStandardMaterial({
      color: 0x9096a2,
      roughness: 0.65,
      metalness: 0.05,
      flatShading: false,
      side: THREE.DoubleSide
    });

    // Blender 4.1 Subtle Edge Overlay lines (for Edit Mode wireframe)
    this.edgeMaterial = new THREE.LineBasicMaterial({
      color: 0x383d47,
      transparent: true,
      opacity: 0.85,
      linewidth: 1
    });

    // Blender Active Object Orange Selection Outline (Object Mode)
    this.selectionOutlineMaterial = new THREE.LineBasicMaterial({
      color: 0xff9800, // Blender Orange (#ff9800)
      linewidth: 2,
      depthTest: true
    });

    this.initDefaultScene();
  }

  setHistoryManager(hm) {
    this.historyManager = hm;
  }

  initDefaultScene() {
    this.createPrimitive('cube', {}, 'Cubo', false);
  }

  createPrimitive(type, params = {}, customName = null, pushHistory = true) {
    let quadMesh;
    const name = customName || `${type.charAt(0).toUpperCase() + type.slice(1)}_${this.objects.length + 1}`;

    const defaultParams = {
      cube: { size: 2 },
      plane: { width: 3, height: 3 },
      cylinder: { radius: 1, height: 2, segments: 12, capTop: true, capBottom: true },
      cone: { radius: 1, height: 2, segments: 12 },
      uvsphere: { radius: 1, segments: 16, rings: 10 },
      torus: { radius: 1.2, tube: 0.4, radialSegments: 8, tubularSegments: 16 },
      icosphere: { radius: 1, subdivisions: 1 }
    };

    const finalParams = { ...(defaultParams[type] || {}), ...params };

    switch (type) {
      case 'cube':
        quadMesh = QuadMesh.createCube(finalParams);
        break;
      case 'plane':
        quadMesh = QuadMesh.createPlane(finalParams);
        break;
      case 'cylinder':
        quadMesh = QuadMesh.createCylinder(finalParams);
        break;
      case 'cone':
        quadMesh = QuadMesh.createCone(finalParams);
        break;
      case 'uvsphere':
        quadMesh = QuadMesh.createUVSphere(finalParams);
        break;
      case 'torus':
        quadMesh = QuadMesh.createTorus(finalParams);
        break;
      case 'icosphere':
        quadMesh = QuadMesh.createIcoSphere(finalParams);
        break;
      default:
        quadMesh = QuadMesh.createCube(finalParams);
    }

    const geometry = quadMesh.toBufferGeometry();
    const material = this.defaultMaterial.clone();

    if (this.objects.length > 0) {
      const hue = (this.objects.length * 0.17) % 1.0;
      material.color.setHSL(hue, 0.35, 0.55);
    }

    const mesh = new THREE.Mesh(geometry, material);
    mesh.name = name;
    mesh.position.set(0, 1, 0);
    mesh.castShadow = true;
    mesh.receiveShadow = true;
    mesh.userData.quadMesh = quadMesh;
    mesh.userData.primitiveType = type;
    mesh.userData.primitiveParams = { ...finalParams };

    // 1. Structural Edge Lines (Black edges for Edit Mode)
    const edgeLineMesh = quadMesh.createEdgeLines(this.edgeMaterial);
    edgeLineMesh.name = `${name}_edges`;
    edgeLineMesh.visible = false;
    mesh.add(edgeLineMesh);

    // 2. Orange Selection Outline (Orange outline for Object Mode)
    const outlineMesh = quadMesh.createEdgeLines(this.selectionOutlineMaterial);
    outlineMesh.name = `${name}_outline`;
    outlineMesh.renderOrder = 999;
    outlineMesh.visible = true;
    mesh.add(outlineMesh);

    this.engine.scene.add(mesh);
    this.objects.push(mesh);
    this.selectObject(mesh, false);

    this.updateStats();

    if (pushHistory && this.historyManager) {
      this.historyManager.push({
        description: `Criar ${name}`,
        undo: () => {
          this.engine.scene.remove(mesh);
          const idx = this.objects.indexOf(mesh);
          if (idx !== -1) this.objects.splice(idx, 1);
          this.selectedObjects.delete(mesh);
          this.selectObject(this.objects[this.objects.length - 1] || null);
          this.updateStats();
        },
        redo: () => {
          this.engine.scene.add(mesh);
          if (!this.objects.includes(mesh)) this.objects.push(mesh);
          this.selectObject(mesh);
          this.updateStats();
        }
      });
    }

    if (this.onPrimitiveCreated) {
      this.onPrimitiveCreated(mesh, type, finalParams);
    }

    return mesh;
  }

  createZooPreset(presetType, customName, pushHistory = true) {
    let quadMesh;
    const names = {
      humanoid: 'Humanoide_Base',
      quadruped: 'Quadrupede_Cao',
      bird: 'Passaro_Base',
      fish: 'Peixe_Base',
      tree: 'Arvore_LowPoly'
    };
    const name = customName || `${names[presetType] || 'Criatura'}_${this.objects.length + 1}`;

    switch (presetType) {
      case 'humanoid': quadMesh = ZooPresets.createHumanoid(); break;
      case 'quadruped': quadMesh = ZooPresets.createQuadruped(); break;
      case 'bird': quadMesh = ZooPresets.createBird(); break;
      case 'fish': quadMesh = ZooPresets.createFish(); break;
      case 'tree': quadMesh = ZooPresets.createTree(); break;
      default: quadMesh = ZooPresets.createHumanoid();
    }

    const geometry = quadMesh.toBufferGeometry();
    const material = this.defaultMaterial.clone();

    if (this.objects.length > 0) {
      const hue = (this.objects.length * 0.17) % 1.0;
      material.color.setHSL(hue, 0.35, 0.55);
    }

    const mesh = new THREE.Mesh(geometry, material);
    mesh.name = name;
    mesh.position.set(0, 0, 0);
    mesh.castShadow = true;
    mesh.receiveShadow = true;
    mesh.userData.quadMesh = quadMesh;
    mesh.userData.primitiveType = presetType;
    mesh.userData.isZooPreset = true;

    // 1. Structural Edge Lines
    const edgeLineMesh = quadMesh.createEdgeLines(this.edgeMaterial);
    edgeLineMesh.name = `${name}_edges`;
    edgeLineMesh.visible = false;
    mesh.add(edgeLineMesh);

    // 2. Selection Outline
    const outlineMesh = quadMesh.createEdgeLines(this.selectionOutlineMaterial);
    outlineMesh.name = `${name}_outline`;
    outlineMesh.renderOrder = 999;
    outlineMesh.visible = true;
    mesh.add(outlineMesh);

    this.engine.scene.add(mesh);
    this.objects.push(mesh);
    this.selectObject(mesh, false);
    this.updateStats();

    if (pushHistory && this.historyManager) {
      this.historyManager.push({
        description: `Criar ${name}`,
        undo: () => {
          this.engine.scene.remove(mesh);
          const idx = this.objects.indexOf(mesh);
          if (idx !== -1) this.objects.splice(idx, 1);
          this.selectedObjects.delete(mesh);
          this.selectObject(this.objects[this.objects.length - 1] || null);
          this.updateStats();
        },
        redo: () => {
          this.engine.scene.add(mesh);
          if (!this.objects.includes(mesh)) this.objects.push(mesh);
          this.selectObject(mesh);
          this.updateStats();
        }
      });
    }

    return mesh;
  }

  updatePrimitiveGeometry(mesh, newParams) {
    if (!mesh || !mesh.userData || !mesh.userData.primitiveType) return;
    const type = mesh.userData.primitiveType;
    mesh.userData.primitiveParams = { ...mesh.userData.primitiveParams, ...newParams };
    const params = mesh.userData.primitiveParams;

    let newQM;
    switch (type) {
      case 'cube': newQM = QuadMesh.createCube(params); break;
      case 'plane': newQM = QuadMesh.createPlane(params); break;
      case 'cylinder': newQM = QuadMesh.createCylinder(params); break;
      case 'cone': newQM = QuadMesh.createCone(params); break;
      case 'uvsphere': newQM = QuadMesh.createUVSphere(params); break;
      case 'torus': newQM = QuadMesh.createTorus(params); break;
      case 'icosphere': newQM = QuadMesh.createIcoSphere(params); break;
      default: newQM = QuadMesh.createCube(params);
    }

    mesh.userData.quadMesh = newQM;
    mesh.geometry.dispose();
    mesh.geometry = newQM.toBufferGeometry();

    // Recreate child line meshes
    const toRemove = mesh.children.filter(c => c.isLineSegments);
    toRemove.forEach(c => {
      mesh.remove(c);
      if (c.geometry) c.geometry.dispose();
    });

    const edgeLineMesh = newQM.createEdgeLines(this.edgeMaterial);
    edgeLineMesh.name = `${mesh.name}_edges`;
    edgeLineMesh.visible = false;
    mesh.add(edgeLineMesh);

    const outlineMesh = newQM.createEdgeLines(this.selectionOutlineMaterial);
    outlineMesh.name = `${mesh.name}_outline`;
    outlineMesh.renderOrder = 999;
    outlineMesh.visible = this.selectedObjects.has(mesh);
    mesh.add(outlineMesh);

    this.updateStats();

    if (this.onMeshGeometryUpdated) {
      this.onMeshGeometryUpdated(mesh);
    }
  }

  duplicateObject(mesh) {
    const targets = this.selectedObjects.size > 0 ? Array.from(this.selectedObjects) : (mesh ? [mesh] : []);
    if (targets.length === 0) return null;

    const newMeshes = [];

    targets.forEach((target) => {
      let newMesh;
      if (target.userData && target.userData.quadMesh) {
        const originalQM = target.userData.quadMesh;
        const clonedQM = new QuadMesh();
        clonedQM.vertices = originalQM.vertices.map(v => v.clone());
        clonedQM.quads = originalQM.quads.map(q => [...q]);
        clonedQM.edges = originalQM.edges.map(e => [...e]);

        const geo = clonedQM.toBufferGeometry();
        const mat = target.material.clone();
        newMesh = new THREE.Mesh(geo, mat);
        newMesh.userData.quadMesh = clonedQM;
        newMesh.userData.primitiveType = target.userData.primitiveType;
        newMesh.userData.primitiveParams = target.userData.primitiveParams ? { ...target.userData.primitiveParams } : null;

        const edgeLineMesh = clonedQM.createEdgeLines(this.edgeMaterial);
        edgeLineMesh.name = `${target.name}_Copia_edges`;
        edgeLineMesh.visible = false;
        newMesh.add(edgeLineMesh);

        const outlineMesh = clonedQM.createEdgeLines(this.selectionOutlineMaterial);
        outlineMesh.name = `${target.name}_Copia_outline`;
        outlineMesh.renderOrder = 999;
        newMesh.add(outlineMesh);
      } else {
        const clonedGeo = target.geometry.clone();
        const clonedMat = target.material.clone();
        newMesh = new THREE.Mesh(clonedGeo, clonedMat);
      }

      newMesh.name = `${target.name}_Copia`;
      newMesh.position.copy(target.position).add(new THREE.Vector3(1, 0, 1));
      newMesh.rotation.copy(target.rotation);
      newMesh.scale.copy(target.scale);
      newMesh.castShadow = true;
      newMesh.receiveShadow = true;

      this.engine.scene.add(newMesh);
      this.objects.push(newMesh);
      newMeshes.push(newMesh);
    });

    this.selectObjects(newMeshes, false);
    this.updateStats();

    if (this.historyManager) {
      const createdMeshes = [...newMeshes];
      this.historyManager.push({
        description: 'Duplicar Objetos',
        undo: () => {
          createdMeshes.forEach(m => {
            this.engine.scene.remove(m);
            const idx = this.objects.indexOf(m);
            if (idx !== -1) this.objects.splice(idx, 1);
            this.selectedObjects.delete(m);
          });
          this.selectObject(this.objects[this.objects.length - 1] || null);
          this.updateStats();
        },
        redo: () => {
          createdMeshes.forEach(m => {
            this.engine.scene.add(m);
            if (!this.objects.includes(m)) this.objects.push(m);
          });
          this.selectObjects(createdMeshes);
          this.updateStats();
        }
      });
    }

    return newMeshes[0];
  }

  deleteObject(mesh) {
    const targets = this.selectedObjects.size > 0 ? Array.from(this.selectedObjects) : (mesh ? [mesh] : []);
    if (targets.length === 0) return;

    const deletedMeshes = [...targets];

    deletedMeshes.forEach((target) => {
      this.engine.scene.remove(target);
      const index = this.objects.indexOf(target);
      if (index !== -1) {
        this.objects.splice(index, 1);
      }
      this.selectedObjects.delete(target);
    });

    this.selectedObject = this.objects.length > 0 ? this.objects[this.objects.length - 1] : null;
    if (this.selectedObject) {
      this.selectObject(this.selectedObject, false);
    } else {
      this.deselectAll();
    }

    this.updateStats();

    if (this.historyManager) {
      this.historyManager.push({
        description: 'Excluir Objeto',
        undo: () => {
          deletedMeshes.forEach(m => {
            this.engine.scene.add(m);
            if (!this.objects.includes(m)) this.objects.push(m);
          });
          this.selectObjects(deletedMeshes);
          this.updateStats();
        },
        redo: () => {
          deletedMeshes.forEach(m => {
            this.engine.scene.remove(m);
            const idx = this.objects.indexOf(m);
            if (idx !== -1) this.objects.splice(idx, 1);
            this.selectedObjects.delete(m);
          });
          this.selectObject(this.objects[this.objects.length - 1] || null);
          this.updateStats();
        }
      });
    }
  }

  toggleObjectVisibility(mesh) {
    if (!mesh) return;
    mesh.visible = !mesh.visible;
    if (!mesh.visible) {
      this.selectedObjects.delete(mesh);
      if (this.selectedObject === mesh) {
        this.selectedObject = Array.from(this.selectedObjects)[0] || null;
      }
    }
    this.updateSelectionVisuals();
    this.updateOutlinerUI();
  }

  selectObject(mesh, addToSelection = false) {
    if (!mesh) {
      this.deselectAll();
      return;
    }

    if (!addToSelection) {
      this.selectedObjects.clear();
      this.selectedObjects.add(mesh);
    } else {
      if (this.selectedObjects.has(mesh)) {
        this.selectedObjects.delete(mesh);
      } else {
        this.selectedObjects.add(mesh);
      }
    }

    this.selectedObject = this.selectedObjects.has(mesh) ? mesh : (Array.from(this.selectedObjects)[0] || null);

    this.updateSelectionVisuals();
    this.updateOutlinerUI();
    this.updateTransformUI();

    if (this.onSelectionChange) {
      this.onSelectionChange(this.selectedObject);
    }
  }

  selectObjects(meshes, addToSelection = false) {
    if (!addToSelection) {
      this.selectedObjects.clear();
    }

    meshes.forEach((mesh) => {
      if (mesh && mesh.visible) {
        this.selectedObjects.add(mesh);
      }
    });

    this.selectedObject = Array.from(this.selectedObjects)[this.selectedObjects.size - 1] || null;

    this.updateSelectionVisuals();
    this.updateOutlinerUI();
    this.updateTransformUI();

    if (this.onSelectionChange) {
      this.onSelectionChange(this.selectedObject);
    }
  }

  deselectAll() {
    this.selectedObjects.clear();
    this.selectedObject = null;

    this.updateSelectionVisuals();
    this.updateOutlinerUI();
    this.updateTransformUI();

    if (this.onSelectionChange) {
      this.onSelectionChange(null);
    }
  }

  updateSelectionVisuals() {
    this.objects.forEach((obj) => {
      const isSelected = this.selectedObjects.has(obj) && obj.visible;
      obj.children.forEach((child) => {
        if (child.name.endsWith('_outline')) {
          child.visible = isSelected;
        }
      });
    });
  }

  setEditModeView(isEditMode) {
    this.objects.forEach((obj) => {
      const isSelected = this.selectedObjects.has(obj);
      obj.children.forEach((child) => {
        if (child.name.endsWith('_outline')) {
          child.visible = !isEditMode && isSelected;
        } else if (child.name.endsWith('_edges')) {
          child.visible = isEditMode && isSelected;
        }
      });
    });
  }

  getSelectedObject() {
    return this.selectedObject;
  }

  getSelectedObjects() {
    return Array.from(this.selectedObjects);
  }

  getAllMeshes() {
    return this.objects;
  }

  setShadeMode(mode, targetMesh = null, pushHistory = true) {
    const mesh = targetMesh || this.getSelectedObject();
    if (!mesh || !mesh.material) return;

    const prevMode = mesh.userData.shading || (mesh.material.flatShading ? 'flat' : 'smooth');

    if (mode === 'smooth') {
      mesh.material.flatShading = false;
      mesh.material.needsUpdate = true;
      mesh.userData.shading = 'smooth';
    } else if (mode === 'flat') {
      mesh.material.flatShading = true;
      mesh.material.needsUpdate = true;
      mesh.userData.shading = 'flat';
    } else if (mode === 'auto') {
      mesh.material.flatShading = false;
      mesh.material.needsUpdate = true;
      mesh.userData.shading = 'auto';
    }

    if (pushHistory && this.historyManager) {
      this.historyManager.push({
        description: `Sombreamento: ${mode === 'smooth' ? 'Suave' : 'Plano'}`,
        undo: () => {
          this.setShadeMode(prevMode, mesh, false);
        },
        redo: () => {
          this.setShadeMode(mode, mesh, false);
        }
      });
    }
  }

  toggleEdges(show) {
    this.showEdges = show;
    this.objects.forEach((mesh) => {
      mesh.children.forEach((child) => {
        if (child.name.endsWith('_edges')) {
          child.visible = show;
        }
      });
    });
  }

  updateStats() {
    let totalVerts = 0;
    let totalFaces = 0;

    this.objects.forEach((obj) => {
      if (obj.visible) {
        if (obj.userData && obj.userData.quadMesh) {
          totalVerts += obj.userData.quadMesh.vertices.length;
          totalFaces += obj.userData.quadMesh.quads.length;
        } else if (obj.geometry) {
          const count = obj.geometry.attributes.position ? obj.geometry.attributes.position.count : 0;
          totalVerts += count;
          totalFaces += count / 3;
        }
      }
    });

    const vertsEl = document.getElementById('stat-verts');
    const facesEl = document.getElementById('stat-faces');
    if (vertsEl) vertsEl.textContent = `${totalVerts} Verts`;
    if (facesEl) facesEl.textContent = `${totalFaces} Faces`;
  }

  updateOutlinerUI() {
    const listEl = document.getElementById('outliner-list');
    if (!listEl) return;

    listEl.innerHTML = '';
    this.objects.forEach((obj) => {
      const isSelected = this.selectedObjects.has(obj);
      const item = document.createElement('div');
      item.className = `outliner-item ${isSelected ? 'selected' : ''}`;
      item.innerHTML = `
        <div class="outliner-item-left">
          <span class="outliner-icon">${obj.visible ? '🧊' : '👁️‍🗨️'}</span>
          <span class="outliner-name ${!obj.visible ? 'muted' : ''}">${obj.name}</span>
        </div>
        <div class="outliner-item-actions">
          <button class="outliner-btn vis-btn" title="${obj.visible ? 'Ocultar' : 'Exibir'}">${obj.visible ? '👁️' : '🕶️'}</button>
          <button class="outliner-btn del-btn" title="Excluir">🗑️</button>
        </div>
      `;

      item.querySelector('.outliner-item-left').addEventListener('click', (e) => {
        this.selectObject(obj, e.shiftKey || e.ctrlKey);
      });
      
      item.querySelector('.vis-btn').addEventListener('click', (e) => {
        e.stopPropagation();
        this.toggleObjectVisibility(obj);
      });

      item.querySelector('.del-btn').addEventListener('click', (e) => {
        e.stopPropagation();
        this.deleteObject(obj);
      });

      listEl.appendChild(item);
    });
  }

  updateTransformUI() {
    if (!this.selectedObject) return;

    const posX = document.getElementById('prop-pos-x');
    const posY = document.getElementById('prop-pos-y');
    const posZ = document.getElementById('prop-pos-z');

    const rotX = document.getElementById('prop-rot-x');
    const rotY = document.getElementById('prop-rot-y');
    const rotZ = document.getElementById('prop-rot-z');

    const sclX = document.getElementById('prop-scl-x');
    const sclY = document.getElementById('prop-scl-y');
    const sclZ = document.getElementById('prop-scl-z');

    if (posX) posX.value = this.selectedObject.position.x.toFixed(2);
    if (posY) posY.value = this.selectedObject.position.y.toFixed(2);
    if (posZ) posZ.value = this.selectedObject.position.z.toFixed(2);

    if (rotX) rotX.value = THREE.MathUtils.radToDeg(this.selectedObject.rotation.x).toFixed(0);
    if (rotY) rotY.value = THREE.MathUtils.radToDeg(this.selectedObject.rotation.y).toFixed(0);
    if (rotZ) rotZ.value = THREE.MathUtils.radToDeg(this.selectedObject.rotation.z).toFixed(0);

    if (sclX) sclX.value = this.selectedObject.scale.x.toFixed(2);
    if (sclY) sclY.value = this.selectedObject.scale.y.toFixed(2);
    if (sclZ) sclZ.value = this.selectedObject.scale.z.toFixed(2);
  }

  applyTransformFromUI() {
    if (!this.selectedObject) return;

    const posX = parseFloat(document.getElementById('prop-pos-x')?.value || 0);
    const posY = parseFloat(document.getElementById('prop-pos-y')?.value || 0);
    const posZ = parseFloat(document.getElementById('prop-pos-z')?.value || 0);

    const rotX = THREE.MathUtils.degToRad(parseFloat(document.getElementById('prop-rot-x')?.value || 0));
    const rotY = THREE.MathUtils.degToRad(parseFloat(document.getElementById('prop-rot-y')?.value || 0));
    const rotZ = THREE.MathUtils.degToRad(parseFloat(document.getElementById('prop-rot-z')?.value || 0));

    const sclX = Math.max(0.01, parseFloat(document.getElementById('prop-scl-x')?.value || 1));
    const sclY = Math.max(0.01, parseFloat(document.getElementById('prop-scl-y')?.value || 1));
    const sclZ = Math.max(0.01, parseFloat(document.getElementById('prop-scl-z')?.value || 1));

    this.selectedObject.position.set(posX, posY, posZ);
    this.selectedObject.rotation.set(rotX, rotY, rotZ);
    this.selectedObject.scale.set(sclX, sclY, sclZ);
  }
}
