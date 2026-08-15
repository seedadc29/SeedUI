import * as THREE from 'three';
import { MeshOperations } from './MeshOperations.js';

export class MeshEditor {
  constructor(engine, sceneManager, transformManager, historyManager) {
    this.engine = engine;
    this.sceneManager = sceneManager;
    this.transformManager = transformManager;
    this.historyManager = historyManager;

    this.activeMesh = null;
    this.submode = 'object'; // 'object' | 'vertex' | 'edge' | 'face'
    
    this.selectedVertices = new Set();
    this.selectedEdges = new Set();
    this.selectedFaces = new Set();

    this.proportionalEditing = false;
    this.proportionalRadius = 2.0;

    // Visual overlay helpers
    this.helperGroup = new THREE.Group();
    this.helperGroup.name = 'EditMode_Helpers';
    this.engine.scene.add(this.helperGroup);

    this.vertexPoints = null;
    this.activeEdgesHelper = null;
    this.faceHighlightMesh = null;

    // Anchor dummy object for attaching Transform Gizmos
    this.transformAnchor = new THREE.Object3D();
    this.transformAnchor.name = 'EditMode_TransformAnchor';
    this.engine.scene.add(this.transformAnchor);

    this.anchorInitialPos = new THREE.Vector3();
    this.initialVertexOffsets = new Map();
    this.allInitialVertexOffsets = new Map();

    this.initAnchorEvents();
  }

  initAnchorEvents() {
    this.transformManager.transformControls.addEventListener('objectChange', () => {
      if (this.submode !== 'object' && this.activeMesh && this.selectedVertices.size > 0) {
        this.applyAnchorTransform();
      }
    });

    let beforeVerticesState = null;

    this.transformManager.transformControls.addEventListener('dragging-changed', (e) => {
      if (e.value) { // Drag started: record initial state
        if (this.submode !== 'object' && this.activeMesh && this.activeMesh.userData.quadMesh) {
          const qm = this.activeMesh.userData.quadMesh;
          beforeVerticesState = qm.vertices.map(v => v.clone());
          this.recordInitialVerticesState();
        }
      } else { // Drag ended
        if (this.submode !== 'object' && this.activeMesh && this.activeMesh.userData.quadMesh && beforeVerticesState) {
          const qm = this.activeMesh.userData.quadMesh;
          const afterVerticesState = qm.vertices.map(v => v.clone());

          let hasChanged = false;
          for (let i = 0; i < qm.vertices.length; i++) {
            if (!qm.vertices[i].equals(beforeVerticesState[i])) {
              hasChanged = true;
              break;
            }
          }

          if (hasChanged && this.historyManager) {
            const targetMesh = this.activeMesh;
            const targetQM = qm;
            const beforeV = [...beforeVerticesState];
            const afterV = [...afterVerticesState];

            this.historyManager.push({
              description: 'Edição de Vértices',
              undo: () => {
                for (let i = 0; i < beforeV.length; i++) {
                  targetQM.vertices[i].copy(beforeV[i]);
                }
                const lineMeshes = targetMesh.children.filter(c => c.isLineSegments);
                lineMeshes.forEach(lm => targetQM.updateGeometryPositions(targetMesh.geometry, lm));
                this.rebuildEditHelpers();
                this.updateTransformAnchor();
              },
              redo: () => {
                for (let i = 0; i < afterV.length; i++) {
                  targetQM.vertices[i].copy(afterV[i]);
                }
                const lineMeshes = targetMesh.children.filter(c => c.isLineSegments);
                lineMeshes.forEach(lm => targetQM.updateGeometryPositions(targetMesh.geometry, lm));
                this.rebuildEditHelpers();
                this.updateTransformAnchor();
              }
            });
          }

          beforeVerticesState = null;
        }
      }
    });
  }

  setSubmode(submode) {
    this.submode = submode;
    this.selectedVertices.clear();
    this.selectedEdges.clear();
    this.selectedFaces.clear();

    const isEditMode = (submode !== 'object');
    this.sceneManager.setEditModeView(isEditMode);

    if (submode === 'object') {
      this.clearHelpers();
      this.activeMesh = this.sceneManager.getSelectedObject();
      if (this.activeMesh) {
        this.transformManager.attach(this.activeMesh);
      } else {
        this.transformManager.detach();
      }
    } else {
      this.activeMesh = this.sceneManager.getSelectedObject();
      if (this.activeMesh) {
        this.rebuildEditHelpers();
        this.updateTransformAnchor();
      } else {
        this.transformManager.detach();
      }
    }
  }

  rebuildEditHelpers() {
    this.clearHelpers();
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;

    const qm = this.activeMesh.userData.quadMesh;

    // 1. Vertex Points Helper
    if (this.submode === 'vertex' || this.submode === 'edge' || this.submode === 'face') {
      const pointGeo = new THREE.BufferGeometry();
      const positions = new Float32Array(qm.vertices.length * 3);
      const colors = new Float32Array(qm.vertices.length * 3);

      for (let i = 0; i < qm.vertices.length; i++) {
        positions[i * 3]     = qm.vertices[i].x;
        positions[i * 3 + 1] = qm.vertices[i].y;
        positions[i * 3 + 2] = qm.vertices[i].z;

        if (this.selectedVertices.has(i)) {
          colors[i * 3] = 1.0; colors[i * 3 + 1] = 0.6; colors[i * 3 + 2] = 0.0; // Bright Blender Orange
        } else {
          colors[i * 3] = 0.2; colors[i * 3 + 1] = 0.22; colors[i * 3 + 2] = 0.26; // Crisp Dark Gray
        }
      }

      pointGeo.setAttribute('position', new THREE.BufferAttribute(positions, 3));
      pointGeo.setAttribute('color', new THREE.BufferAttribute(colors, 3));

      const pointMat = new THREE.PointsMaterial({
        size: 9,
        vertexColors: true,
        sizeAttenuation: false,
        depthTest: false,
        transparent: true
      });

      this.vertexPoints = new THREE.Points(pointGeo, pointMat);
      this.vertexPoints.matrixAutoUpdate = false;
      this.vertexPoints.matrix.copy(this.activeMesh.matrixWorld);
      this.helperGroup.add(this.vertexPoints);
    }

    // 2. Active Edges Highlight
    const activeEdgeIndices = new Set();
    if (this.submode === 'vertex' && this.selectedVertices.size > 0) {
      this.selectedVertices.forEach((vIdx) => {
        const connected = qm.getConnectedEdges(vIdx);
        connected.forEach(eIdx => activeEdgeIndices.add(eIdx));
      });
    } else if (this.submode === 'edge') {
      this.selectedEdges.forEach(eIdx => activeEdgeIndices.add(eIdx));
    }

    if (activeEdgeIndices.size > 0) {
      const linePositions = [];
      activeEdgeIndices.forEach((eIdx) => {
        const edge = qm.edges[eIdx];
        if (edge) {
          const vA = qm.vertices[edge[0]];
          const vB = qm.vertices[edge[1]];
          if (vA && vB) {
            linePositions.push(vA.x, vA.y, vA.z, vB.x, vB.y, vB.z);
          }
        }
      });

      const activeEdgeGeo = new THREE.BufferGeometry();
      activeEdgeGeo.setAttribute('position', new THREE.Float32BufferAttribute(linePositions, 3));
      const activeEdgeMat = new THREE.LineBasicMaterial({
        color: 0xff8c00,
        linewidth: 2,
        depthTest: false
      });

      this.activeEdgesHelper = new THREE.LineSegments(activeEdgeGeo, activeEdgeMat);
      this.activeEdgesHelper.matrixAutoUpdate = false;
      this.activeEdgesHelper.matrix.copy(this.activeMesh.matrixWorld);
      this.helperGroup.add(this.activeEdgesHelper);
    }

    // 3. Face Highlight
    if (this.submode === 'face' && this.selectedFaces.size > 0) {
      const facePositions = [];
      this.selectedFaces.forEach((fIdx) => {
        const face = qm.quads[fIdx];
        if (face) {
          const unique = Array.from(new Set(face));
          if (unique.length === 4) {
            const v0 = qm.vertices[unique[0]];
            const v1 = qm.vertices[unique[1]];
            const v2 = qm.vertices[unique[2]];
            const v3 = qm.vertices[unique[3]];
            facePositions.push(v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
            facePositions.push(v0.x, v0.y, v0.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z);
          } else if (unique.length === 3) {
            const v0 = qm.vertices[unique[0]];
            const v1 = qm.vertices[unique[1]];
            const v2 = qm.vertices[unique[2]];
            facePositions.push(v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
          }
        }
      });

      const faceGeo = new THREE.BufferGeometry();
      faceGeo.setAttribute('position', new THREE.Float32BufferAttribute(facePositions, 3));
      const faceMat = new THREE.MeshBasicMaterial({
        color: 0xff8c00,
        side: THREE.DoubleSide,
        transparent: true,
        opacity: 0.45,
        depthTest: false
      });

      this.faceHighlightMesh = new THREE.Mesh(faceGeo, faceMat);
      this.faceHighlightMesh.matrixAutoUpdate = false;
      this.faceHighlightMesh.matrix.copy(this.activeMesh.matrixWorld);
      this.helperGroup.add(this.faceHighlightMesh);
    }
  }

  handleSelection(clientX, clientY, shiftKey, raycaster) {
    if (!this.activeMesh || this.submode === 'object' || !this.activeMesh.userData.quadMesh) return false;

    const qm = this.activeMesh.userData.quadMesh;
    const canvas = this.engine.canvas;
    const rect = canvas.getBoundingClientRect();
    const mouseX = clientX - rect.left;
    const mouseY = clientY - rect.top;

    if (this.submode === 'vertex') {
      let closestIdx = -1;
      let minDistance = 24;

      for (let i = 0; i < qm.vertices.length; i++) {
        const vWorld = qm.vertices[i].clone().applyMatrix4(this.activeMesh.matrixWorld);
        const vNDC = vWorld.project(this.engine.activeCamera);

        if (vNDC.z < 1.0) {
          const screenX = (vNDC.x * 0.5 + 0.5) * rect.width;
          const screenY = (-vNDC.y * 0.5 + 0.5) * rect.height;

          const dist = Math.hypot(mouseX - screenX, mouseY - screenY);
          if (dist < minDistance) {
            minDistance = dist;
            closestIdx = i;
          }
        }
      }

      if (closestIdx !== -1) {
        if (shiftKey && this.selectedVertices.has(closestIdx)) {
          this.selectedVertices.delete(closestIdx);
        } else {
          if (!shiftKey) this.selectedVertices.clear();
          this.selectedVertices.add(closestIdx);
        }
        this.rebuildEditHelpers();
        this.updateTransformAnchor();
        return true;
      } else {
        if (!shiftKey) {
          this.selectedVertices.clear();
          this.rebuildEditHelpers();
          this.updateTransformAnchor();
        }
      }
    } else if (this.submode === 'edge') {
      let closestEdgeIdx = -1;
      let minDistance = 20;

      qm.edges.forEach((edge, eIdx) => {
        const vA = qm.vertices[edge[0]].clone().applyMatrix4(this.activeMesh.matrixWorld).project(this.engine.activeCamera);
        const vB = qm.vertices[edge[1]].clone().applyMatrix4(this.activeMesh.matrixWorld).project(this.engine.activeCamera);

        if (vA.z < 1.0 && vB.z < 1.0) {
          const ax = (vA.x * 0.5 + 0.5) * rect.width;
          const ay = (-vA.y * 0.5 + 0.5) * rect.height;
          const bx = (vB.x * 0.5 + 0.5) * rect.width;
          const by = (-vB.y * 0.5 + 0.5) * rect.height;

          const l2 = (bx - ax) * (bx - ax) + (by - ay) * (by - ay);
          let t = ((mouseX - ax) * (bx - ax) + (mouseY - ay) * (by - ay)) / (l2 || 1);
          t = Math.max(0, Math.min(1, t));
          const projX = ax + t * (bx - ax);
          const projY = ay + t * (by - ay);
          const dist = Math.hypot(mouseX - projX, mouseY - projY);

          if (dist < minDistance) {
            minDistance = dist;
            closestEdgeIdx = eIdx;
          }
        }
      });

      if (closestEdgeIdx !== -1) {
        if (shiftKey && this.selectedEdges.has(closestEdgeIdx)) {
          this.selectedEdges.delete(closestEdgeIdx);
          const edge = qm.edges[closestEdgeIdx];
          this.selectedVertices.delete(edge[0]);
          this.selectedVertices.delete(edge[1]);
        } else {
          if (!shiftKey) {
            this.selectedEdges.clear();
            this.selectedVertices.clear();
          }
          this.selectedEdges.add(closestEdgeIdx);
          const edge = qm.edges[closestEdgeIdx];
          this.selectedVertices.add(edge[0]);
          this.selectedVertices.add(edge[1]);
        }
        this.rebuildEditHelpers();
        this.updateTransformAnchor();
        return true;
      } else {
        if (!shiftKey) {
          this.selectedEdges.clear();
          this.selectedVertices.clear();
          this.rebuildEditHelpers();
          this.updateTransformAnchor();
        }
      }
    } else if (this.submode === 'face') {
      const intersects = raycaster.intersectObject(this.activeMesh, false);
      if (intersects.length > 0 && intersects[0].faceIndex !== undefined) {
        let quadIdx = Math.floor(intersects[0].faceIndex / 2);
        if (quadIdx >= qm.quads.length) quadIdx = qm.quads.length - 1;

        if (quadIdx >= 0 && quadIdx < qm.quads.length) {
          if (shiftKey && this.selectedFaces.has(quadIdx)) {
            this.selectedFaces.delete(quadIdx);
            const quad = qm.quads[quadIdx];
            quad.forEach(vIdx => this.selectedVertices.delete(vIdx));
          } else {
            if (!shiftKey) {
              this.selectedFaces.clear();
              this.selectedVertices.clear();
            }
            this.selectedFaces.add(quadIdx);
            const quad = qm.quads[quadIdx];
            quad.forEach(vIdx => this.selectedVertices.add(vIdx));
          }
          this.rebuildEditHelpers();
          this.updateTransformAnchor();
          return true;
        }
      } else {
        if (!shiftKey) {
          this.selectedFaces.clear();
          this.selectedVertices.clear();
          this.rebuildEditHelpers();
          this.updateTransformAnchor();
        }
      }
    }

    return false;
  }

  selectAll() {
    if (!this.activeMesh || this.submode === 'object' || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    for (let i = 0; i < qm.vertices.length; i++) {
      this.selectedVertices.add(i);
    }

    if (this.submode === 'face') {
      for (let i = 0; i < qm.quads.length; i++) {
        this.selectedFaces.add(i);
      }
    } else if (this.submode === 'edge') {
      for (let i = 0; i < qm.edges.length; i++) {
        this.selectedEdges.add(i);
      }
    }

    this.rebuildEditHelpers();
    this.updateTransformAnchor();
  }

  deselectAll() {
    this.selectedVertices.clear();
    this.selectedEdges.clear();
    this.selectedFaces.clear();
    this.rebuildEditHelpers();
    this.updateTransformAnchor();
  }

  updateTransformAnchor() {
    if (this.submode === 'object' || !this.activeMesh || this.selectedVertices.size === 0 || !this.activeMesh.userData.quadMesh) {
      this.transformManager.detach();
      return;
    }

    const qm = this.activeMesh.userData.quadMesh;
    const center = new THREE.Vector3();
    let count = 0;

    this.selectedVertices.forEach((idx) => {
      const v = qm.vertices[idx];
      if (v) {
        const worldPos = v.clone().applyMatrix4(this.activeMesh.matrixWorld);
        center.add(worldPos);
        count++;
      }
    });

    if (count === 0) {
      this.transformManager.detach();
      return;
    }

    center.divideScalar(count);

    this.transformAnchor.position.copy(center);
    this.transformAnchor.rotation.copy(this.activeMesh.rotation);
    this.transformAnchor.scale.set(1, 1, 1);

    this.anchorInitialPos.copy(center);
    this.recordInitialVerticesState();

    this.transformManager.attach(this.transformAnchor);
  }

  recordInitialVerticesState() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;
    
    this.initialVertexOffsets.clear();
    this.allInitialVertexOffsets.clear();
    this.anchorInitialPos.copy(this.transformAnchor.position);

    // Record all vertices in world space for proportional editing
    qm.vertices.forEach((v, idx) => {
      const worldPos = v.clone().applyMatrix4(this.activeMesh.matrixWorld);
      this.allInitialVertexOffsets.set(idx, worldPos);
      if (this.selectedVertices.has(idx)) {
        this.initialVertexOffsets.set(idx, worldPos.clone());
      }
    });
  }

  applyAnchorTransform() {
    if (!this.activeMesh || this.selectedVertices.size === 0 || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    const delta = new THREE.Vector3().subVectors(this.transformAnchor.position, this.anchorInitialPos);
    const invMatrix = this.activeMesh.matrixWorld.clone().invert();

    if (!this.proportionalEditing) {
      // Standard direct vertex move
      this.selectedVertices.forEach((idx) => {
        let initialWorldPos = this.initialVertexOffsets.get(idx);
        if (!initialWorldPos) {
          const v = qm.vertices[idx];
          if (v) {
            initialWorldPos = v.clone().applyMatrix4(this.activeMesh.matrixWorld);
            this.initialVertexOffsets.set(idx, initialWorldPos);
          }
        }
        if (initialWorldPos) {
          const newWorldPos = initialWorldPos.clone().add(delta);
          const localPos = newWorldPos.applyMatrix4(invMatrix);
          qm.vertices[idx].copy(localPos);
        }
      });
    } else {
      // Proportional Smooth Falloff (O)
      qm.vertices.forEach((v, idx) => {
        const initialWorldPos = this.allInitialVertexOffsets.get(idx);
        if (initialWorldPos) {
          if (this.selectedVertices.has(idx)) {
            const newWorldPos = initialWorldPos.clone().add(delta);
            const localPos = newWorldPos.applyMatrix4(invMatrix);
            qm.vertices[idx].copy(localPos);
          } else {
            const dist = initialWorldPos.distanceTo(this.anchorInitialPos);
            const weight = MeshOperations.getFalloffWeight(dist, this.proportionalRadius, 'smooth');
            if (weight > 0) {
              const scaledDelta = delta.clone().multiplyScalar(weight);
              const newWorldPos = initialWorldPos.clone().add(scaledDelta);
              const localPos = newWorldPos.applyMatrix4(invMatrix);
              qm.vertices[idx].copy(localPos);
            }
          }
        }
      });
    }

    const lineMeshes = this.activeMesh.children.filter(c => c.isLineSegments);
    lineMeshes.forEach(lm => qm.updateGeometryPositions(this.activeMesh.geometry, lm));

    this.rebuildEditHelpers();
  }

  // --- MODELING OPERATORS (MILESTONE 04) ---

  extrude() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    let facesToExtrude = new Set(this.selectedFaces);
    if (facesToExtrude.size === 0 && this.selectedVertices.size >= 3) {
      qm.quads.forEach((face, fIdx) => {
        const unique = Array.from(new Set(face));
        if (unique.every(v => this.selectedVertices.has(v))) {
          facesToExtrude.add(fIdx);
        }
      });
    }

    if (facesToExtrude.size === 0) return;

    const beforeVerts = qm.vertices.map(v => v.clone());
    const beforeQuads = qm.quads.map(q => [...q]);

    const result = MeshOperations.extrudeFaces(qm, facesToExtrude, 0.6);
    if (result) {
      this.selectedVertices = result.newSelectedVertices;
      this.selectedFaces = result.newSelectedFaces;
      this.selectedEdges.clear();

      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        const afterQuads = qm.quads.map(q => [...q]);

        this.historyManager.push({
          description: 'Extrusão (Extrude)',
          undo: () => {
            targetQM.vertices = beforeVerts.map(v => v.clone());
            targetQM.quads = beforeQuads.map(q => [...q]);
            targetQM.rebuildEdges();
            this.refreshMeshGeometryBuffers();
          },
          redo: () => {
            targetQM.vertices = afterVerts.map(v => v.clone());
            targetQM.quads = afterQuads.map(q => [...q]);
            targetQM.rebuildEdges();
            this.refreshMeshGeometryBuffers();
          }
        });
      }
    }
  }

  inset() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    let facesToInset = new Set(this.selectedFaces);
    if (facesToInset.size === 0 && this.selectedVertices.size >= 3) {
      qm.quads.forEach((face, fIdx) => {
        const unique = Array.from(new Set(face));
        if (unique.every(v => this.selectedVertices.has(v))) {
          facesToInset.add(fIdx);
        }
      });
    }

    if (facesToInset.size === 0) return;

    const beforeVerts = qm.vertices.map(v => v.clone());
    const beforeQuads = qm.quads.map(q => [...q]);

    const result = MeshOperations.insetFaces(qm, facesToInset, 0.25);
    if (result) {
      this.selectedVertices = result.newSelectedVertices;
      this.selectedFaces = result.newSelectedFaces;
      this.selectedEdges.clear();

      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        const afterQuads = qm.quads.map(q => [...q]);

        this.historyManager.push({
          description: 'Inserir Face (Inset)',
          undo: () => {
            targetQM.vertices = beforeVerts.map(v => v.clone());
            targetQM.quads = beforeQuads.map(q => [...q]);
            targetQM.rebuildEdges();
            this.refreshMeshGeometryBuffers();
          },
          redo: () => {
            targetQM.vertices = afterVerts.map(v => v.clone());
            targetQM.quads = afterQuads.map(q => [...q]);
            targetQM.rebuildEdges();
            this.refreshMeshGeometryBuffers();
          }
        });
      }
    }
  }

  subdivide() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    const beforeVerts = qm.vertices.map(v => v.clone());
    const beforeQuads = qm.quads.map(q => [...q]);

    MeshOperations.subdivide(qm);
    this.selectedVertices.clear();
    this.selectedEdges.clear();
    this.selectedFaces.clear();

    this.refreshMeshGeometryBuffers();

    if (this.historyManager) {
      const targetQM = qm;
      const afterVerts = qm.vertices.map(v => v.clone());
      const afterQuads = qm.quads.map(q => [...q]);

      this.historyManager.push({
        description: 'Subdivisão (Subdivide)',
        undo: () => {
          targetQM.vertices = beforeVerts.map(v => v.clone());
          targetQM.quads = beforeQuads.map(q => [...q]);
          targetQM.rebuildEdges();
          this.refreshMeshGeometryBuffers();
        },
        redo: () => {
          targetQM.vertices = afterVerts.map(v => v.clone());
          targetQM.quads = afterQuads.map(q => [...q]);
          targetQM.rebuildEdges();
          this.refreshMeshGeometryBuffers();
        }
      });
    }
  }

  toggleProportionalEditing() {
    this.proportionalEditing = !this.proportionalEditing;
    return this.proportionalEditing;
  }

  refreshMeshGeometryBuffers() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    this.activeMesh.geometry.dispose();
    this.activeMesh.geometry = qm.toBufferGeometry();

    const lineChildren = this.activeMesh.children.filter(c => c.isLineSegments);
    lineChildren.forEach(c => {
      this.activeMesh.remove(c);
      if (c.geometry) c.geometry.dispose();
    });

    const edgeLineMesh = qm.createEdgeLines(this.sceneManager.edgeMaterial);
    edgeLineMesh.name = `${this.activeMesh.name}_edges`;
    edgeLineMesh.visible = (this.submode !== 'object');
    this.activeMesh.add(edgeLineMesh);

    const outlineMesh = qm.createEdgeLines(this.sceneManager.selectionOutlineMaterial);
    outlineMesh.name = `${this.activeMesh.name}_outline`;
    outlineMesh.renderOrder = 999;
    outlineMesh.visible = (this.submode === 'object');
    this.activeMesh.add(outlineMesh);

    this.rebuildEditHelpers();
    this.updateTransformAnchor();
    this.sceneManager.updateStats();
  }

  clearHelpers() {
    while (this.helperGroup.children.length > 0) {
      const child = this.helperGroup.children[0];
      if (child.geometry) child.geometry.dispose();
      this.helperGroup.remove(child);
    }
    this.vertexPoints = null;
    this.activeEdgesHelper = null;
    this.faceHighlightMesh = null;
  }
}
