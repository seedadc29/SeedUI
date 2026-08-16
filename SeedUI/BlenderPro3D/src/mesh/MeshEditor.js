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
      } else { // Drag ended: commit history and re-anchor
        if (this.submode !== 'object' && this.activeMesh && this.activeMesh.userData.quadMesh) {
          const qm = this.activeMesh.userData.quadMesh;
          const afterVerticesState = qm.vertices.map(v => v.clone());

          let hasChanged = false;
          if (beforeVerticesState) {
            for (let i = 0; i < qm.vertices.length; i++) {
              if (!qm.vertices[i].equals(beforeVerticesState[i])) {
                hasChanged = true;
                break;
              }
            }
          }

          if (hasChanged && this.historyManager && beforeVerticesState) {
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

          // Crucial: Re-anchor and record current positions for subsequent transforms
          this.anchorInitialPos.copy(this.transformAnchor.position);
          this.recordInitialVerticesState();
          this.updateTransformAnchor();
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
        this.activeMesh.updateMatrixWorld(true);
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

    this.activeMesh.updateMatrixWorld(true);
    const qm = this.activeMesh.userData.quadMesh;

    // 1. Vertex Points Helper
    const isWireframe = (this.engine.currentShading === 'wireframe');

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
        size: 8,
        vertexColors: true,
        sizeAttenuation: false,
        depthTest: !isWireframe,
        depthWrite: !isWireframe,
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
        depthTest: !isWireframe,
        depthWrite: !isWireframe
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
        depthTest: !isWireframe,
        depthWrite: false,
        polygonOffset: true,
        polygonOffsetFactor: -1,
        polygonOffsetUnits: -1
      });

      this.faceHighlightMesh = new THREE.Mesh(faceGeo, faceMat);
      this.faceHighlightMesh.matrixAutoUpdate = false;
      this.faceHighlightMesh.matrix.copy(this.activeMesh.matrixWorld);
      this.helperGroup.add(this.faceHighlightMesh);
    }
  }

  isFaceFrontFacing(fIdx, qm, meshMatrixWorld) {
    const face = qm.quads[fIdx];
    if (!face || face.length < 3) return false;

    const v0 = qm.vertices[face[0]].clone().applyMatrix4(meshMatrixWorld);
    const v1 = qm.vertices[face[1]].clone().applyMatrix4(meshMatrixWorld);
    const v2 = qm.vertices[face[2]].clone().applyMatrix4(meshMatrixWorld);
    const e1 = new THREE.Vector3().subVectors(v1, v0);
    const e2 = new THREE.Vector3().subVectors(v2, v0);
    const normal = new THREE.Vector3().crossVectors(e1, e2).normalize();

    const center = new THREE.Vector3();
    face.forEach(idx => center.add(qm.vertices[idx].clone().applyMatrix4(meshMatrixWorld)));
    center.divideScalar(face.length);

    let viewDir;
    if (this.engine.isOrthographic) {
      viewDir = new THREE.Vector3();
      this.engine.activeCamera.getWorldDirection(viewDir).negate();
    } else {
      viewDir = new THREE.Vector3().subVectors(this.engine.activeCamera.position, center).normalize();
    }

    return normal.dot(viewDir) > 0.05;
  }

  isVertexFrontFacing(vIdx, qm, meshMatrixWorld) {
    const vWorld = qm.vertices[vIdx].clone().applyMatrix4(meshMatrixWorld);
    const camPos = this.engine.activeCamera.position;

    // 1. Must belong to at least one front-facing face
    let hasFrontFace = false;
    for (let fIdx = 0; fIdx < qm.quads.length; fIdx++) {
      const face = qm.quads[fIdx];
      if (face && face.includes(vIdx) && face.length >= 3) {
        if (this.isFaceFrontFacing(fIdx, qm, meshMatrixWorld)) {
          hasFrontFace = true;
          break;
        }
      }
    }
    if (!hasFrontFace) return false;

    // 2. Ray-traced Occlusion Check against front faces that do NOT contain this vertex
    const dir = new THREE.Vector3().subVectors(vWorld, camPos);
    const targetDist = dir.length();
    dir.normalize();

    const ray = new THREE.Ray(camPos, dir);
    const vA = new THREE.Vector3();
    const vB = new THREE.Vector3();
    const vC = new THREE.Vector3();
    const hitPt = new THREE.Vector3();

    for (let fIdx = 0; fIdx < qm.quads.length; fIdx++) {
      const face = qm.quads[fIdx];
      if (!face || face.includes(vIdx)) continue;
      if (!this.isFaceFrontFacing(fIdx, qm, meshMatrixWorld)) continue;

      if (face.length === 4) {
        vA.copy(qm.vertices[face[0]]).applyMatrix4(meshMatrixWorld);
        vB.copy(qm.vertices[face[1]]).applyMatrix4(meshMatrixWorld);
        vC.copy(qm.vertices[face[2]]).applyMatrix4(meshMatrixWorld);
        if (ray.intersectTriangle(vA, vB, vC, true, hitPt)) {
          if (hitPt.distanceTo(camPos) < targetDist - 0.02) return false;
        }
        vB.copy(qm.vertices[face[2]]).applyMatrix4(meshMatrixWorld);
        vC.copy(qm.vertices[face[3]]).applyMatrix4(meshMatrixWorld);
        if (ray.intersectTriangle(vA, vB, vC, true, hitPt)) {
          if (hitPt.distanceTo(camPos) < targetDist - 0.02) return false;
        }
      } else if (face.length === 3) {
        vA.copy(qm.vertices[face[0]]).applyMatrix4(meshMatrixWorld);
        vB.copy(qm.vertices[face[1]]).applyMatrix4(meshMatrixWorld);
        vC.copy(qm.vertices[face[2]]).applyMatrix4(meshMatrixWorld);
        if (ray.intersectTriangle(vA, vB, vC, true, hitPt)) {
          if (hitPt.distanceTo(camPos) < targetDist - 0.02) return false;
        }
      }
    }

    return true;
  }

  isEdgeFrontFacing(eIdx, qm, meshMatrixWorld) {
    const edge = qm.edges[eIdx];
    if (!edge) return false;
    return this.isVertexFrontFacing(edge[0], qm, meshMatrixWorld) && this.isVertexFrontFacing(edge[1], qm, meshMatrixWorld);
  }

  handleSelection(clientX, clientY, shiftKey, raycaster) {
    if (!this.activeMesh || this.submode === 'object' || !this.activeMesh.userData.quadMesh) return false;

    this.activeMesh.updateMatrixWorld(true);
    const qm = this.activeMesh.userData.quadMesh;
    const canvas = this.engine.canvas;
    const rect = canvas.getBoundingClientRect();
    const mouseX = clientX - rect.left;
    const mouseY = clientY - rect.top;
    const isWireframe = (this.engine.currentShading === 'wireframe');
    const meshMatrix = this.activeMesh.matrixWorld;

    if (this.submode === 'vertex') {
      let closestIdx = -1;

      // 1. Surface Collision Raycast (Closest Vertex of Clicked Front Face)
      const surfaceHits = raycaster.intersectObject(this.activeMesh, false);
      if (surfaceHits.length > 0 && surfaceHits[0].point && surfaceHits[0].faceIndex !== undefined) {
        const hitPoint = surfaceHits[0].point;
        const faceIdx = qm.getFaceIndexFromTriangleIndex(surfaceHits[0].faceIndex);
        const face = qm.quads[faceIdx];
        if (face) {
          let minHitDist = Infinity;
          face.forEach((vIdx) => {
            const vWorld = qm.vertices[vIdx].clone().applyMatrix4(meshMatrix);
            const d = hitPoint.distanceTo(vWorld);
            if (d < minHitDist) {
              minHitDist = d;
              closestIdx = vIdx;
            }
          });
        }
      }

      // 2. Direct Points Raycasting (Hardware Accelerated)
      if (closestIdx === -1 && this.vertexPoints) {
        raycaster.params.Points.threshold = 0.45;
        const ptIntersects = raycaster.intersectObject(this.vertexPoints, false);
        if (ptIntersects.length > 0 && ptIntersects[0].index !== undefined) {
          const candIdx = ptIntersects[0].index;
          if (isWireframe || this.isVertexFrontFacing(candIdx, qm, meshMatrix)) {
            closestIdx = candIdx;
          }
        }
      }

      // 3. 3D Ray-to-Point Proximity (World Space)
      if (closestIdx === -1) {
        const ray = raycaster.ray;
        let minRayDist = 0.45;
        for (let i = 0; i < qm.vertices.length; i++) {
          if (!isWireframe && !this.isVertexFrontFacing(i, qm, meshMatrix)) continue;
          const vWorld = qm.vertices[i].clone().applyMatrix4(meshMatrix);
          const d = ray.distanceToPoint(vWorld);
          if (d < minRayDist) {
            minRayDist = d;
            closestIdx = i;
          }
        }
      }

      // 4. 2D Screen-space Projection Fallback (35px radius)
      if (closestIdx === -1) {
        let min2DDist = 35;
        for (let i = 0; i < qm.vertices.length; i++) {
          if (!isWireframe && !this.isVertexFrontFacing(i, qm, meshMatrix)) continue;
          const vWorld = qm.vertices[i].clone().applyMatrix4(meshMatrix);
          const vNDC = vWorld.project(this.engine.activeCamera);

          if (vNDC.z < 1.0) {
            const screenX = (vNDC.x * 0.5 + 0.5) * rect.width;
            const screenY = (-vNDC.y * 0.5 + 0.5) * rect.height;

            const dist = Math.hypot(mouseX - screenX, mouseY - screenY);
            if (dist < min2DDist) {
              min2DDist = dist;
              closestIdx = i;
            }
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

      // 1. Surface Collision Raycast (Closest Edge of Clicked Front Face)
      const surfaceHits = raycaster.intersectObject(this.activeMesh, false);
      if (surfaceHits.length > 0 && surfaceHits[0].point && surfaceHits[0].faceIndex !== undefined) {
        const hitPoint = surfaceHits[0].point;
        const faceIdx = qm.getFaceIndexFromTriangleIndex(surfaceHits[0].faceIndex);
        const face = qm.quads[faceIdx];
        if (face) {
          let minHitDist = Infinity;
          for (let i = 0; i < face.length; i++) {
            const vA = face[i];
            const vB = face[(i + 1) % face.length];
            const pA = qm.vertices[vA].clone().applyMatrix4(meshMatrix);
            const pB = qm.vertices[vB].clone().applyMatrix4(meshMatrix);
            const line = new THREE.Line3(pA, pB);
            const cp = new THREE.Vector3();
            line.closestPointToPoint(hitPoint, true, cp);
            const dist = cp.distanceTo(hitPoint);
            if (dist < minHitDist) {
              minHitDist = dist;
              // Find edge index in qm.edges
              for (let e = 0; e < qm.edges.length; e++) {
                const ed = qm.edges[e];
                if ((ed[0] === vA && ed[1] === vB) || (ed[0] === vB && ed[1] === vA)) {
                  closestEdgeIdx = e;
                  break;
                }
              }
            }
          }
        }
      }

      // 2. 3D Ray-to-Segment Proximity
      if (closestEdgeIdx === -1) {
        const ray = raycaster.ray;
        let minRayDist = 0.4;
        qm.edges.forEach((edge, eIdx) => {
          if (!isWireframe && !this.isEdgeFrontFacing(eIdx, qm, meshMatrix)) return;
          const vA = qm.vertices[edge[0]].clone().applyMatrix4(meshMatrix);
          const vB = qm.vertices[edge[1]].clone().applyMatrix4(meshMatrix);
          const line = new THREE.Line3(vA, vB);
          const ptOnRay = new THREE.Vector3();
          const ptOnLine = new THREE.Vector3();
          ray.distanceSqToSegment(line.start, line.end, ptOnRay, ptOnLine);
          const d = ptOnRay.distanceTo(ptOnLine);
          if (d < minRayDist) {
            minRayDist = d;
            closestEdgeIdx = eIdx;
          }
        });
      }

      // 3. 2D Screen-space Projection Fallback
      if (closestEdgeIdx === -1) {
        let min2DDist = 30;
        qm.edges.forEach((edge, eIdx) => {
          if (!isWireframe && !this.isEdgeFrontFacing(eIdx, qm, meshMatrix)) return;
          const vA = qm.vertices[edge[0]].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);
          const vB = qm.vertices[edge[1]].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);

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

            if (dist < min2DDist) {
              min2DDist = dist;
              closestEdgeIdx = eIdx;
            }
          }
        });
      }

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
        let quadIdx = qm.getFaceIndexFromTriangleIndex(intersects[0].faceIndex);
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

    this.activeMesh.updateMatrixWorld(true);
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
    this.activeMesh.updateMatrixWorld(true);
    const qm = this.activeMesh.userData.quadMesh;
    
    this.initialVertexOffsets.clear();
    this.allInitialVertexOffsets.clear();
    this.anchorInitialPos.copy(this.transformAnchor.position);

    // Record all vertices in world space for accurate proportional and standard transforms
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
    this.activeMesh.updateMatrixWorld(true);
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

  fillFace() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh || this.selectedVertices.size < 3) return;
    const qm = this.activeMesh.userData.quadMesh;
    const beforeVerts = qm.vertices.map(v => v.clone());
    const beforeQuads = qm.quads.map(q => [...q]);

    const result = MeshOperations.fillFace(qm, this.selectedVertices);
    if (result && result.success) {
      this.selectedFaces.clear();
      this.selectedFaces.add(result.newFaceIndex);
      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        const afterQuads = qm.quads.map(q => [...q]);
        this.historyManager.push({
          description: 'Criar Face (Fill Face)',
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

  mergeVertices(mode = 'center') {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh || this.selectedVertices.size < 2) return;
    const qm = this.activeMesh.userData.quadMesh;
    const beforeVerts = qm.vertices.map(v => v.clone());
    const beforeQuads = qm.quads.map(q => [...q]);

    const result = MeshOperations.mergeVertices(qm, this.selectedVertices, mode);
    if (result && result.success) {
      this.selectedVertices.clear();
      this.selectedVertices.add(result.mergedVertex);
      this.selectedEdges.clear();
      this.selectedFaces.clear();
      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        const afterQuads = qm.quads.map(q => [...q]);
        this.historyManager.push({
          description: 'Mesclar Vértices (Merge)',
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

  smoothVertices(factor = 0.5) {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh || this.selectedVertices.size === 0) return;
    const qm = this.activeMesh.userData.quadMesh;
    const beforeVerts = qm.vertices.map(v => v.clone());

    const result = MeshOperations.smoothVertices(qm, this.selectedVertices, factor);
    if (result && result.success) {
      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        this.historyManager.push({
          description: 'Suavizar Vértices (Smooth)',
          undo: () => {
            targetQM.vertices = beforeVerts.map(v => v.clone());
            this.refreshMeshGeometryBuffers();
          },
          redo: () => {
            targetQM.vertices = afterVerts.map(v => v.clone());
            this.refreshMeshGeometryBuffers();
          }
        });
      }
    }
  }

  shrinkFlatten(amount = 0.2) {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh || this.selectedVertices.size === 0) return;
    const qm = this.activeMesh.userData.quadMesh;
    const beforeVerts = qm.vertices.map(v => v.clone());

    const result = MeshOperations.shrinkFlatten(qm, this.selectedVertices, amount);
    if (result && result.success) {
      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        this.historyManager.push({
          description: 'Encolher / Engordar (Shrink/Flatten)',
          undo: () => {
            targetQM.vertices = beforeVerts.map(v => v.clone());
            this.refreshMeshGeometryBuffers();
          },
          redo: () => {
            targetQM.vertices = afterVerts.map(v => v.clone());
            this.refreshMeshGeometryBuffers();
          }
        });
      }
    }
  }

  deleteSelection() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;
    const beforeVerts = qm.vertices.map(v => v.clone());
    const beforeQuads = qm.quads.map(q => [...q]);

    const type = this.submode === 'face' ? 'faces' : 'vertices';
    const result = MeshOperations.deleteElements(qm, this.selectedVertices, this.selectedFaces, type);
    if (result && result.success) {
      this.selectedVertices.clear();
      this.selectedEdges.clear();
      this.selectedFaces.clear();
      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        const afterQuads = qm.quads.map(q => [...q]);
        this.historyManager.push({
          description: 'Excluir Seleção (Delete)',
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

  // --- EDGE DETECTION HELPER ---

  getClosestEdge(raycaster) {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return -1;
    const qm = this.activeMesh.userData.quadMesh;
    const ray = raycaster.ray;
    let minRayDist = 0.45;
    let closestEdgeIdx = -1;

    qm.edges.forEach((edge, eIdx) => {
      const vA = qm.vertices[edge[0]].clone().applyMatrix4(this.activeMesh.matrixWorld);
      const vB = qm.vertices[edge[1]].clone().applyMatrix4(this.activeMesh.matrixWorld);
      const d = ray.distanceSqToSegment(vA, vB);
      const dist = Math.sqrt(d);
      if (dist < minRayDist) {
        minRayDist = dist;
        closestEdgeIdx = eIdx;
      }
    });
    return closestEdgeIdx;
  }

  // --- BLENDER LOOP CUT & SLIDE (Ctrl+R) ---

  startLoopCut() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    this.isLoopCutting = true;
    this.loopCutPhase = 'preview'; // 'preview' | 'slide'
    this.loopCutRing = [];
    this.loopCutFactor = 0.5;
    this.loopCutNumCuts = 1;
    this.loopCutStartMouse = { x: 0, y: 0 };

    this.clearLoopCutPreview();
  }

  updateLoopCutPreview(raycaster, mouse2D) {
    if (!this.isLoopCutting || !this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    if (this.loopCutPhase === 'preview') {
      const intersects = raycaster.intersectObject(this.activeMesh, false);
      if (intersects.length > 0 && intersects[0].faceIndex !== undefined) {
        const quadIdx = qm.getFaceIndexFromTriangleIndex(intersects[0].faceIndex);
        const quad = qm.quads[quadIdx];

        if (quad && quad.length >= 4) {
          const hitPoint = intersects[0].point.clone().applyMatrix4(this.activeMesh.matrixWorld.clone().invert());
          
          // Find closest edge in this quad
          let minEdgeDist = Infinity;
          let closestEdge = [quad[0], quad[1]];

          const q = Array.from(new Set(quad));
          for (let i = 0; i < q.length; i++) {
            const vA = qm.vertices[q[i]];
            const vB = qm.vertices[q[(i + 1) % q.length]];
            const line = new THREE.Line3(vA, vB);
            const cp = new THREE.Vector3();
            line.closestPointToPoint(hitPoint, true, cp);
            const dist = cp.distanceTo(hitPoint);
            if (dist < minEdgeDist) {
              minEdgeDist = dist;
              closestEdge = [q[i], q[(i + 1) % q.length]];
            }
          }

          // Compute Edge Ring along perpendicular direction
          this.loopCutRing = MeshOperations.findEdgeRing(qm, quadIdx, closestEdge[0], closestEdge[1]);
          this.renderLoopCutLines();
        }
      }
    } else if (this.loopCutPhase === 'slide') {
      // Slid factor adjusted by mouse movement
      this.renderLoopCutLines();
    }
  }

  renderLoopCutLines() {
    this.clearLoopCutPreview();
    if (!this.loopCutRing || this.loopCutRing.length === 0 || !this.activeMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    const linePositions = [];
    const tValues = [];

    if (this.loopCutNumCuts === 1) {
      tValues.push(this.loopCutFactor);
    } else {
      const baseSpacing = 1.0 / (this.loopCutNumCuts + 1);
      const slide = (this.loopCutFactor - 0.5) * baseSpacing;
      for (let i = 1; i <= this.loopCutNumCuts; i++) {
        tValues.push(Math.max(0.01, Math.min(0.99, (i * baseSpacing) + slide)));
      }
    }

    tValues.forEach((t) => {
      this.loopCutRing.forEach((ringItem) => {
        const { v0, v1, v2, v3 } = ringItem;
        const p0 = qm.vertices[v0];
        const p1 = qm.vertices[v1];
        const p2 = qm.vertices[v2];
        const p3 = qm.vertices[v3];

        if (p0 && p1 && p2 && p3) {
          const cutA = new THREE.Vector3().lerpVectors(p0, p1, t);
          const cutB = new THREE.Vector3().lerpVectors(p3, p2, t);
          linePositions.push(cutA.x, cutA.y, cutA.z, cutB.x, cutB.y, cutB.z);
        }
      });
    });

    if (linePositions.length > 0) {
      const geo = new THREE.BufferGeometry();
      geo.setAttribute('position', new THREE.Float32BufferAttribute(linePositions, 3));
      const mat = new THREE.LineBasicMaterial({
        color: 0xffff00, // Bright Blender Yellow
        linewidth: 3,
        depthTest: false
      });

      this.loopCutHelperMesh = new THREE.LineSegments(geo, mat);
      this.loopCutHelperMesh.matrixAutoUpdate = false;
      this.loopCutHelperMesh.matrix.copy(this.activeMesh.matrixWorld);
      this.helperGroup.add(this.loopCutHelperMesh);
    }
  }

  clearLoopCutPreview() {
    if (this.loopCutHelperMesh) {
      if (this.loopCutHelperMesh.geometry) this.loopCutHelperMesh.geometry.dispose();
      this.helperGroup.remove(this.loopCutHelperMesh);
      this.loopCutHelperMesh = null;
    }
  }

  handleLoopCutClick(e) {
    if (!this.isLoopCutting) return false;

    if (this.loopCutPhase === 'preview') {
      if (this.loopCutRing && this.loopCutRing.length > 0) {
        this.loopCutPhase = 'slide';
        this.loopCutStartMouse = { x: e.clientX, y: e.clientY };
        return true;
      }
    } else if (this.loopCutPhase === 'slide') {
      this.commitLoopCut();
      return true;
    }
    return false;
  }

  handleLoopCutScroll(deltaY) {
    if (!this.isLoopCutting) return false;
    const change = deltaY < 0 ? 1 : -1;
    this.loopCutNumCuts = Math.max(1, Math.min(10, this.loopCutNumCuts + change));
    this.renderLoopCutLines();
    return true;
  }

  slideLoopCut(deltaX) {
    if (!this.isLoopCutting || this.loopCutPhase !== 'slide') return;
    this.loopCutFactor = Math.max(0.02, Math.min(0.98, this.loopCutFactor + deltaX * 0.005));
    this.renderLoopCutLines();
  }

  commitLoopCut() {
    if (!this.isLoopCutting || !this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    if (this.loopCutRing && this.loopCutRing.length > 0) {
      const beforeVerts = qm.vertices.map(v => v.clone());
      const beforeQuads = qm.quads.map(q => [...q]);

      const result = MeshOperations.loopCut(qm, this.loopCutRing, this.loopCutFactor, this.loopCutNumCuts);

      this.clearLoopCutPreview();
      this.isLoopCutting = false;
      this.loopCutPhase = 'preview';

      if (result) {
        this.selectedVertices = result.cutVertices;
        this.selectedFaces.clear();
        this.selectedEdges.clear();

        this.refreshMeshGeometryBuffers();

        if (this.historyManager) {
          const targetQM = qm;
          const afterVerts = qm.vertices.map(v => v.clone());
          const afterQuads = qm.quads.map(q => [...q]);

          this.historyManager.push({
            description: 'Loop Cut & Slide',
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
    } else {
      this.cancelLoopCut();
    }
  }

  cancelLoopCut() {
    this.isLoopCutting = false;
    this.loopCutPhase = 'preview';
    this.clearLoopCutPreview();
  }

  // --- BEVEL / CHANFRO (Ctrl+B) ---

  bevel() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    let edgesToChamfer = new Set(this.selectedEdges);
    if (edgesToChamfer.size === 0 && this.selectedVertices.size >= 2) {
      qm.edges.forEach((edge, eIdx) => {
        if (this.selectedVertices.has(edge[0]) && this.selectedVertices.has(edge[1])) {
          edgesToChamfer.add(eIdx);
        }
      });
    }

    if (edgesToChamfer.size === 0) return;

    const beforeVerts = qm.vertices.map(v => v.clone());
    const beforeQuads = qm.quads.map(q => [...q]);

    const result = MeshOperations.bevelEdges(qm, edgesToChamfer, 0.15, 1);
    if (result) {
      this.selectedVertices.clear();
      this.selectedEdges.clear();
      this.selectedFaces.clear();

      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        const afterQuads = qm.quads.map(q => [...q]);

        this.historyManager.push({
          description: 'Chanfro (Bevel)',
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

  // --- MERGE VERTICES (M) ---

  merge(mode = 'center') {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;
    if (this.selectedVertices.size < 2 && mode === 'center') return;

    const beforeVerts = qm.vertices.map(v => v.clone());
    const beforeQuads = qm.quads.map(q => [...q]);

    const result = MeshOperations.mergeVertices(qm, this.selectedVertices, mode);
    if (result) {
      if (result.remainingVertex !== undefined) {
        this.selectedVertices = new Set([result.remainingVertex]);
      }
      this.selectedEdges.clear();
      this.selectedFaces.clear();

      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        const afterQuads = qm.quads.map(q => [...q]);

        this.historyManager.push({
          description: `Unir Vértices (${mode === 'center' ? 'No Centro' : 'Por Distância'})`,
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

  // --- FILL FACE (F) ---

  fill() {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;
    if (this.selectedVertices.size < 3) return;

    const beforeVerts = qm.vertices.map(v => v.clone());
    const beforeQuads = qm.quads.map(q => [...q]);

    const result = MeshOperations.fillFace(qm, this.selectedVertices);
    if (result) {
      this.selectedFaces = new Set([result.newFaceIdx]);
      this.selectedEdges.clear();
      this.refreshMeshGeometryBuffers();

      if (this.historyManager) {
        const targetQM = qm;
        const afterVerts = qm.vertices.map(v => v.clone());
        const afterQuads = qm.quads.map(q => [...q]);

        this.historyManager.push({
          description: 'Preencher Face (Fill)',
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

  // --- EDGE LOOP SELECT (Alt+Click) ---

  selectEdgeLoop(edgeIdx, addToSelection = false) {
    if (!this.activeMesh || !this.activeMesh.userData.quadMesh) return;
    const qm = this.activeMesh.userData.quadMesh;

    const loopEdges = MeshOperations.findEdgeLoop(qm, edgeIdx);
    if (!addToSelection) {
      this.selectedEdges.clear();
      this.selectedVertices.clear();
    }

    loopEdges.forEach((eIdx) => {
      this.selectedEdges.add(eIdx);
      const edge = qm.edges[eIdx];
      if (edge) {
        this.selectedVertices.add(edge[0]);
        this.selectedVertices.add(edge[1]);
      }
    });

    this.rebuildEditHelpers();
    this.updateTransformAnchor();
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
    this.loopCutHelperMesh = null;
  }
}
