import * as THREE from 'three';

/**
 * SelectionTools - Blender-identical 2D/3D Multi-Selection Engine
 * 
 * Features:
 * 1. Box Select ('B'): True 2D screen-space AABB & boundary sweep crossing overlap.
 * 2. Lasso Select: Freehand polygon and segment intersection.
 * 3. Circle Select ('C'): 2D circle-to-mesh distance overlap with wheel radius.
 * 4. Brush Select: Continuous sweep paint selection.
 * 5. Real-time selection preview while dragging.
 * 6. Fixes DOM click interference so dragging across objects and releasing in empty space preserves selection.
 */
export class SelectionTools {
  constructor(engine, sceneManager, meshEditor, uiManager) {
    this.engine = engine;
    this.sceneManager = sceneManager;
    this.meshEditor = meshEditor;
    this.uiManager = uiManager;

    this.activeTool = 'box'; // 'box' | 'lasso' | 'circle' | 'brush'
    this.isSelecting = false;
    this.didDrag = false;
    this.justFinishedDragSelection = false;
    this.circleRadius = 45;

    this.startPos = { x: 0, y: 0 };
    this.currentPos = { x: 0, y: 0 };
    this.lassoPoints = [];

    this.createOverlayCanvas();
    this.bindEvents();
  }

  createOverlayCanvas() {
    this.overlay = document.createElement('canvas');
    this.overlay.id = 'selection-overlay-canvas';
    this.overlay.style.position = 'absolute';
    this.overlay.style.top = '0';
    this.overlay.style.left = '0';
    this.overlay.style.width = '100%';
    this.overlay.style.height = '100%';
    this.overlay.style.pointerEvents = 'none';
    this.overlay.style.zIndex = '30';

    this.engine.container.appendChild(this.overlay);
    this.ctx = this.overlay.getContext('2d');
    this.resizeOverlay();

    window.addEventListener('resize', () => this.resizeOverlay());
  }

  resizeOverlay() {
    this.overlay.width = this.engine.container.clientWidth;
    this.overlay.height = this.engine.container.clientHeight;
  }

  setTool(tool) {
    this.activeTool = tool;
    this.clearOverlay();
  }

  bindEvents() {
    const canvas = this.engine.canvas;

    canvas.addEventListener('mousedown', (e) => {
      if (e.button !== 0) return; // Left click only
      
      // 1. If Alt is held AND Emulate 3 Button Mouse is active: Camera Orbit/Pan/Zoom always takes precedence!
      if (e.altKey && this.engine.emulate3Button) {
        this.engine.controls.enabled = true;
        return;
      }

      // 2. If mouse is over Transform Gizmo or transforming, NEVER intercept with box selection!
      const tc = this.uiManager.transformManager.transformControls;
      if (this.uiManager.transformManager.isTransforming || (tc && tc.axis !== null && tc.axis !== '')) {
        return;
      }

      const rect = canvas.getBoundingClientRect();
      this.startPos = { x: e.clientX - rect.left, y: e.clientY - rect.top };
      this.currentPos = { ...this.startPos };
      this.didDrag = false;
      this.justFinishedDragSelection = false;

      if (this.activeTool === 'box' || this.activeTool === 'lasso' || this.activeTool === 'brush' || this.activeTool === 'circle') {
        this.isSelecting = true;
        this.engine.controls.enabled = false;

        if (this.activeTool === 'lasso') {
          this.lassoPoints = [{ ...this.startPos }];
        } else if (this.activeTool === 'brush' || this.activeTool === 'circle') {
          this.applySweepSelectionAt(e.clientX, e.clientY, e.shiftKey);
        }
      }
    });

    window.addEventListener('mousemove', (e) => {
      const rect = canvas.getBoundingClientRect();
      const mx = e.clientX - rect.left;
      const my = e.clientY - rect.top;
      this.currentPos = { x: mx, y: my };

      if (this.activeTool === 'circle') {
        this.drawCircleCursor(mx, my);
      }

      if (!this.isSelecting) return;

      const distMoved = Math.hypot(mx - this.startPos.x, my - this.startPos.y);
      if (distMoved > 5) {
        this.didDrag = true;
      }

      if (this.activeTool === 'box') {
        this.drawBox(this.startPos.x, this.startPos.y, mx, my);
        // Real-time live highlight during drag
        if (this.didDrag) {
          this.applyBoxSelection(this.startPos.x, this.startPos.y, mx, my, e.shiftKey, true);
        }
      } else if (this.activeTool === 'lasso') {
        this.lassoPoints.push({ x: mx, y: my });
        this.drawLasso(this.lassoPoints);
      } else if (this.activeTool === 'brush' || this.activeTool === 'circle') {
        this.applySweepSelectionAt(e.clientX, e.clientY, true);
      }
    });

    window.addEventListener('mouseup', (e) => {
      if (!this.isSelecting) return;
      this.isSelecting = false;
      this.engine.controls.enabled = true;

      const rect = canvas.getBoundingClientRect();
      const endX = e.clientX - rect.left;
      const endY = e.clientY - rect.top;

      if (this.didDrag) {
        this.justFinishedDragSelection = true;
        // Suppress subsequent click event
        setTimeout(() => {
          this.justFinishedDragSelection = false;
        }, 150);

        if (this.activeTool === 'box') {
          this.applyBoxSelection(this.startPos.x, this.startPos.y, endX, endY, e.shiftKey, false);
        } else if (this.activeTool === 'lasso') {
          if (this.lassoPoints.length > 3) {
            this.applyLassoSelection(this.lassoPoints, e.shiftKey);
          }
        }
      }

      this.clearOverlay();
    });

    // Mouse wheel for Circle Select radius
    window.addEventListener('wheel', (e) => {
      if (this.activeTool === 'circle') {
        this.circleRadius = Math.max(12, Math.min(220, this.circleRadius - Math.sign(e.deltaY) * 6));
        this.drawCircleCursor(this.currentPos.x, this.currentPos.y);
      }
    });
  }

  drawBox(x1, y1, x2, y2) {
    this.ctx.clearRect(0, 0, this.overlay.width, this.overlay.height);
    const minX = Math.min(x1, x2);
    const minY = Math.min(y1, y2);
    const width = Math.abs(x2 - x1);
    const height = Math.abs(y2 - y1);

    this.ctx.fillStyle = 'rgba(255, 152, 0, 0.15)';
    this.ctx.fillRect(minX, minY, width, height);

    this.ctx.strokeStyle = '#ff9800';
    this.ctx.lineWidth = 1.5;
    this.ctx.setLineDash([4, 4]);
    this.ctx.strokeRect(minX, minY, width, height);
    this.ctx.setLineDash([]);
  }

  drawLasso(points) {
    this.ctx.clearRect(0, 0, this.overlay.width, this.overlay.height);
    if (points.length < 2) return;

    this.ctx.beginPath();
    this.ctx.moveTo(points[0].x, points[0].y);
    for (let i = 1; i < points.length; i++) {
      this.ctx.lineTo(points[i].x, points[i].y);
    }
    this.ctx.closePath();

    this.ctx.fillStyle = 'rgba(255, 152, 0, 0.15)';
    this.ctx.fill();

    this.ctx.strokeStyle = '#ff9800';
    this.ctx.lineWidth = 1.5;
    this.ctx.setLineDash([3, 3]);
    this.ctx.stroke();
    this.ctx.setLineDash([]);
  }

  drawCircleCursor(x, y) {
    this.ctx.clearRect(0, 0, this.overlay.width, this.overlay.height);
    this.ctx.beginPath();
    this.ctx.arc(x, y, this.circleRadius, 0, Math.PI * 2);
    this.ctx.fillStyle = 'rgba(255, 152, 0, 0.15)';
    this.ctx.fill();
    this.ctx.strokeStyle = '#ff9800';
    this.ctx.lineWidth = 1.5;
    this.ctx.stroke();
  }

  clearOverlay() {
    this.ctx.clearRect(0, 0, this.overlay.width, this.overlay.height);
  }

  /**
   * Computes the complete 2D screen projected bounding box [minX, maxX, minY, maxY]
   * and all projected screen points for any 3D object.
   */
  getMeshScreenBounds(mesh, rect) {
    const camera = this.engine.activeCamera;
    const points = [];
    let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;

    if (mesh.userData && mesh.userData.quadMesh) {
      const qm = mesh.userData.quadMesh;
      for (let i = 0; i < qm.vertices.length; i++) {
        const vWorld = qm.vertices[i].clone().applyMatrix4(mesh.matrixWorld);
        const vNDC = vWorld.project(camera);
        if (vNDC.z < 1.0) {
          const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
          const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
          points.push({ x: sx, y: sy });
          minX = Math.min(minX, sx);
          maxX = Math.max(maxX, sx);
          minY = Math.min(minY, sy);
          maxY = Math.max(maxY, sy);
        }
      }
    } else {
      if (!mesh.geometry.boundingBox) mesh.geometry.computeBoundingBox();
      const bbox = mesh.geometry.boundingBox;
      const corners = [
        new THREE.Vector3(bbox.min.x, bbox.min.y, bbox.min.z),
        new THREE.Vector3(bbox.max.x, bbox.min.y, bbox.min.z),
        new THREE.Vector3(bbox.min.x, bbox.max.y, bbox.min.z),
        new THREE.Vector3(bbox.max.x, bbox.max.y, bbox.min.z),
        new THREE.Vector3(bbox.min.x, bbox.min.y, bbox.max.z),
        new THREE.Vector3(bbox.max.x, bbox.min.y, bbox.max.z),
        new THREE.Vector3(bbox.min.x, bbox.max.y, bbox.max.z),
        new THREE.Vector3(bbox.max.x, bbox.max.y, bbox.max.z)
      ];
      corners.forEach((c) => {
        c.applyMatrix4(mesh.matrixWorld);
        const vNDC = c.project(camera);
        if (vNDC.z < 1.0) {
          const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
          const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
          points.push({ x: sx, y: sy });
          minX = Math.min(minX, sx);
          maxX = Math.max(maxX, sx);
          minY = Math.min(minY, sy);
          maxY = Math.max(maxY, sy);
        }
      });
    }

    if (points.length === 0) return null;
    return { minX, maxX, minY, maxY, points };
  }

  // --- VISIBILITY / OCCLUSION CULLING HELPERS ---

  isFaceVisible(fIdx, qm, meshMatrixWorld, isWireframe) {
    if (isWireframe) return true;
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

  isVertexVisible(vIdx, qm, meshMatrixWorld, isWireframe) {
    if (isWireframe) return true;
    const vWorld = qm.vertices[vIdx].clone().applyMatrix4(meshMatrixWorld);
    const camPos = this.engine.activeCamera.position;

    // 1. Must belong to at least one front-facing face
    let hasFrontFace = false;
    for (let fIdx = 0; fIdx < qm.quads.length; fIdx++) {
      const face = qm.quads[fIdx];
      if (face && face.includes(vIdx) && face.length >= 3) {
        if (this.isFaceVisible(fIdx, qm, meshMatrixWorld, false)) {
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
      if (!this.isFaceVisible(fIdx, qm, meshMatrixWorld, false)) continue;

      if (face.length === 4) {
        vA.copy(qm.vertices[face[0]]).applyMatrix4(meshMatrixWorld);
        vB.copy(qm.vertices[face[1]]).applyMatrix4(meshMatrixWorld);
        vC.copy(qm.vertices[face[2]]).applyMatrix4(meshMatrixWorld);
        if (ray.intersectTriangle(vA, vB, vC, true, hitPt)) {
          if (hitPt.distanceTo(camPos) < targetDist - 0.02) return false; // Occluded!
        }
        vB.copy(qm.vertices[face[2]]).applyMatrix4(meshMatrixWorld);
        vC.copy(qm.vertices[face[3]]).applyMatrix4(meshMatrixWorld);
        if (ray.intersectTriangle(vA, vB, vC, true, hitPt)) {
          if (hitPt.distanceTo(camPos) < targetDist - 0.02) return false; // Occluded!
        }
      } else if (face.length === 3) {
        vA.copy(qm.vertices[face[0]]).applyMatrix4(meshMatrixWorld);
        vB.copy(qm.vertices[face[1]]).applyMatrix4(meshMatrixWorld);
        vC.copy(qm.vertices[face[2]]).applyMatrix4(meshMatrixWorld);
        if (ray.intersectTriangle(vA, vB, vC, true, hitPt)) {
          if (hitPt.distanceTo(camPos) < targetDist - 0.02) return false; // Occluded!
        }
      }
    }

    return true;
  }

  isEdgeVisible(eIdx, qm, meshMatrixWorld, isWireframe) {
    if (isWireframe) return true;
    const edge = qm.edges[eIdx];
    if (!edge) return false;
    return this.isVertexVisible(edge[0], qm, meshMatrixWorld, isWireframe) && this.isVertexVisible(edge[1], qm, meshMatrixWorld, isWireframe);
  }

  // --- SELECTION EXECUTION ALGORITHMS ---

  applySweepSelectionAt(clientX, clientY, shiftKey) {
    const radius = this.activeTool === 'circle' ? this.circleRadius : 22;
    const rect = this.engine.canvas.getBoundingClientRect();
    const mx = clientX - rect.left;
    const my = clientY - rect.top;
    const isWireframe = (this.engine.currentShading === 'wireframe');

    if (this.uiManager.currentSubmode !== 'object' && this.meshEditor.activeMesh && this.meshEditor.activeMesh.userData.quadMesh) {
      const qm = this.meshEditor.activeMesh.userData.quadMesh;
      const meshMatrix = this.meshEditor.activeMesh.matrixWorld;

      if (this.uiManager.currentSubmode === 'vertex') {
        for (let i = 0; i < qm.vertices.length; i++) {
          const vWorld = qm.vertices[i].clone().applyMatrix4(meshMatrix);
          const vNDC = vWorld.project(this.engine.activeCamera);
          if (vNDC.z < 1.0) {
            const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
            const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
            if (Math.hypot(mx - sx, my - sy) <= radius) {
              if (this.isVertexVisible(i, qm, meshMatrix, isWireframe)) {
                this.meshEditor.selectedVertices.add(i);
              }
            }
          }
        }
      } else if (this.uiManager.currentSubmode === 'edge') {
        qm.edges.forEach((edge, eIdx) => {
          const vA = qm.vertices[edge[0]].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);
          const vB = qm.vertices[edge[1]].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);
          if (vA.z < 1.0 && vB.z < 1.0) {
            const ax = (vA.x * 0.5 + 0.5) * rect.width;
            const ay = (-vA.y * 0.5 + 0.5) * rect.height;
            const bx = (vB.x * 0.5 + 0.5) * rect.width;
            const by = (-vB.y * 0.5 + 0.5) * rect.height;
            const midX = (ax + bx) * 0.5;
            const midY = (ay + by) * 0.5;
            if (Math.hypot(mx - midX, my - midY) <= radius) {
              if (this.isEdgeVisible(eIdx, qm, meshMatrix, isWireframe)) {
                this.meshEditor.selectedEdges.add(eIdx);
                this.meshEditor.selectedVertices.add(edge[0]);
                this.meshEditor.selectedVertices.add(edge[1]);
              }
            }
          }
        });
      } else if (this.uiManager.currentSubmode === 'face') {
        qm.quads.forEach((quad, fIdx) => {
          const center = new THREE.Vector3();
          quad.forEach(vIdx => center.add(qm.vertices[vIdx].clone().applyMatrix4(meshMatrix)));
          center.divideScalar(quad.length);
          const cNDC = center.project(this.engine.activeCamera);
          if (cNDC.z < 1.0) {
            const cx = (cNDC.x * 0.5 + 0.5) * rect.width;
            const cy = (-cNDC.y * 0.5 + 0.5) * rect.height;
            if (Math.hypot(mx - cx, my - cy) <= radius) {
              if (this.isFaceVisible(fIdx, qm, meshMatrix, isWireframe)) {
                this.meshEditor.selectedFaces.add(fIdx);
                quad.forEach(vIdx => this.meshEditor.selectedVertices.add(vIdx));
              }
            }
          }
        });
      }

      this.meshEditor.rebuildEditHelpers();
      this.meshEditor.updateTransformAnchor();
    } else {
      // Object Mode Circle/Brush: Intersects if circle touches the 2D bounding box or any vertex
      const matched = [];
      this.sceneManager.getAllMeshes().forEach((mesh) => {
        if (mesh.visible) {
          const bounds = this.getMeshScreenBounds(mesh, rect);
          if (bounds) {
            const nearestX = Math.max(bounds.minX, Math.min(mx, bounds.maxX));
            const nearestY = Math.max(bounds.minY, Math.min(my, bounds.maxY));
            const dist = Math.hypot(mx - nearestX, my - nearestY);
            if (dist <= radius) {
              matched.push(mesh);
            }
          }
        }
      });

      if (matched.length > 0) {
        this.sceneManager.selectObjects(matched, true);
      }
    }
  }

  applyBoxSelection(x1, y1, x2, y2, shiftKey, isPreview = false) {
    const boxMinX = Math.min(x1, x2);
    const boxMaxX = Math.max(x1, x2);
    const boxMinY = Math.min(y1, y2);
    const boxMaxY = Math.max(y1, y2);
    const isWireframe = (this.engine.currentShading === 'wireframe');

    const rect = this.engine.canvas.getBoundingClientRect();

    if (this.uiManager.currentSubmode !== 'object' && this.meshEditor.activeMesh && this.meshEditor.activeMesh.userData.quadMesh) {
      const qm = this.meshEditor.activeMesh.userData.quadMesh;
      const meshMatrix = this.meshEditor.activeMesh.matrixWorld;

      if (!shiftKey) {
        this.meshEditor.selectedVertices.clear();
        this.meshEditor.selectedFaces.clear();
        this.meshEditor.selectedEdges.clear();
      }

      if (this.uiManager.currentSubmode === 'vertex') {
        for (let i = 0; i < qm.vertices.length; i++) {
          const vWorld = qm.vertices[i].clone().applyMatrix4(meshMatrix);
          const vNDC = vWorld.project(this.engine.activeCamera);
          if (vNDC.z < 1.0) {
            const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
            const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
            if (sx >= boxMinX && sx <= boxMaxX && sy >= boxMinY && sy <= boxMaxY) {
              if (this.isVertexVisible(i, qm, meshMatrix, isWireframe)) {
                this.meshEditor.selectedVertices.add(i);
              }
            }
          }
        }
      } else if (this.uiManager.currentSubmode === 'edge') {
        qm.edges.forEach((edge, eIdx) => {
          const vA = qm.vertices[edge[0]].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);
          const vB = qm.vertices[edge[1]].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);
          if (vA.z < 1.0 && vB.z < 1.0) {
            const ax = (vA.x * 0.5 + 0.5) * rect.width;
            const ay = (-vA.y * 0.5 + 0.5) * rect.height;
            const bx = (vB.x * 0.5 + 0.5) * rect.width;
            const by = (-vB.y * 0.5 + 0.5) * rect.height;
            const inBox = (ax >= boxMinX && ax <= boxMaxX && ay >= boxMinY && ay <= boxMaxY) &&
                          (bx >= boxMinX && bx <= boxMaxX && by >= boxMinY && by <= boxMaxY);
            if (inBox) {
              if (this.isEdgeVisible(eIdx, qm, meshMatrix, isWireframe)) {
                this.meshEditor.selectedEdges.add(eIdx);
                this.meshEditor.selectedVertices.add(edge[0]);
                this.meshEditor.selectedVertices.add(edge[1]);
              }
            }
          }
        });
      } else if (this.uiManager.currentSubmode === 'face') {
        qm.quads.forEach((quad, fIdx) => {
          let allVertsInBox = true;
          for (let vIdx of quad) {
            const vNDC = qm.vertices[vIdx].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);
            if (vNDC.z >= 1.0) { allVertsInBox = false; break; }
            const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
            const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
            if (!(sx >= boxMinX && sx <= boxMaxX && sy >= boxMinY && sy <= boxMaxY)) {
              allVertsInBox = false;
              break;
            }
          }
          if (allVertsInBox) {
            if (this.isFaceVisible(fIdx, qm, meshMatrix, isWireframe)) {
              this.meshEditor.selectedFaces.add(fIdx);
              quad.forEach(vIdx => this.meshEditor.selectedVertices.add(vIdx));
            }
          }
        });
      }

      this.meshEditor.rebuildEditHelpers();
      this.meshEditor.updateTransformAnchor();
    } else {
      // Object Mode Box Select: True 2D AABB Overlap / Crossing test
      const matched = [];
      this.sceneManager.getAllMeshes().forEach((mesh) => {
        if (mesh.visible) {
          const bounds = this.getMeshScreenBounds(mesh, rect);
          if (bounds) {
            const overlaps = !(
              bounds.maxX < boxMinX ||
              bounds.minX > boxMaxX ||
              bounds.maxY < boxMinY ||
              bounds.minY > boxMaxY
            );
            if (overlaps) {
              matched.push(mesh);
            }
          }
        }
      });

      if (matched.length > 0) {
        this.sceneManager.selectObjects(matched, shiftKey);
      } else if (!shiftKey && !isPreview) {
        this.sceneManager.deselectAll();
      }
    }
  }

  applyLassoSelection(points, shiftKey) {
    const rect = this.engine.canvas.getBoundingClientRect();
    const isWireframe = (this.engine.currentShading === 'wireframe');

    const isPointInPoly = (px, py, poly) => {
      let inside = false;
      for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
        const xi = poly[i].x, yi = poly[i].y;
        const xj = poly[j].x, yj = poly[j].y;
        const intersect = ((yi > py) !== (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi);
        if (intersect) inside = !inside;
      }
      return inside;
    };

    if (this.uiManager.currentSubmode !== 'object' && this.meshEditor.activeMesh && this.meshEditor.activeMesh.userData.quadMesh) {
      const qm = this.meshEditor.activeMesh.userData.quadMesh;
      const meshMatrix = this.meshEditor.activeMesh.matrixWorld;

      if (!shiftKey) {
        this.meshEditor.selectedVertices.clear();
        this.meshEditor.selectedFaces.clear();
        this.meshEditor.selectedEdges.clear();
      }

      if (this.uiManager.currentSubmode === 'vertex') {
        for (let i = 0; i < qm.vertices.length; i++) {
          const vWorld = qm.vertices[i].clone().applyMatrix4(meshMatrix);
          const vNDC = vWorld.project(this.engine.activeCamera);
          if (vNDC.z < 1.0) {
            const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
            const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
            if (isPointInPoly(sx, sy, points)) {
              if (this.isVertexVisible(i, qm, meshMatrix, isWireframe)) {
                this.meshEditor.selectedVertices.add(i);
              }
            }
          }
        }
      } else if (this.uiManager.currentSubmode === 'edge') {
        qm.edges.forEach((edge, eIdx) => {
          const vA = qm.vertices[edge[0]].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);
          const vB = qm.vertices[edge[1]].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);
          if (vA.z < 1.0 && vB.z < 1.0) {
            const ax = (vA.x * 0.5 + 0.5) * rect.width;
            const ay = (-vA.y * 0.5 + 0.5) * rect.height;
            const bx = (vB.x * 0.5 + 0.5) * rect.width;
            const by = (-vB.y * 0.5 + 0.5) * rect.height;
            if (isPointInPoly(ax, ay, points) && isPointInPoly(bx, by, points)) {
              if (this.isEdgeVisible(eIdx, qm, meshMatrix, isWireframe)) {
                this.meshEditor.selectedEdges.add(eIdx);
                this.meshEditor.selectedVertices.add(edge[0]);
                this.meshEditor.selectedVertices.add(edge[1]);
              }
            }
          }
        });
      } else if (this.uiManager.currentSubmode === 'face') {
        qm.quads.forEach((quad, fIdx) => {
          let allInPoly = true;
          for (let vIdx of quad) {
            const vNDC = qm.vertices[vIdx].clone().applyMatrix4(meshMatrix).project(this.engine.activeCamera);
            if (vNDC.z >= 1.0) { allInPoly = false; break; }
            const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
            const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
            if (!isPointInPoly(sx, sy, points)) { allInPoly = false; break; }
          }
          if (allInPoly) {
            if (this.isFaceVisible(fIdx, qm, meshMatrix, isWireframe)) {
              this.meshEditor.selectedFaces.add(fIdx);
              quad.forEach(vIdx => this.meshEditor.selectedVertices.add(vIdx));
            }
          }
        });
      }

      this.meshEditor.rebuildEditHelpers();
      this.meshEditor.updateTransformAnchor();
    } else {
      // Object Mode Lasso Select
      const matched = [];
      this.sceneManager.getAllMeshes().forEach((mesh) => {
        if (mesh.visible) {
          const bounds = this.getMeshScreenBounds(mesh, rect);
          if (bounds) {
            const hasPointInside = bounds.points.some(p => isPointInPoly(p.x, p.y, points));
            const centerInside = isPointInPoly((bounds.minX + bounds.maxX) / 2, (bounds.minY + bounds.maxY) / 2, points);
            if (hasPointInside || centerInside) {
              matched.push(mesh);
            }
          }
        }
      });

      if (matched.length > 0) {
        this.sceneManager.selectObjects(matched, shiftKey);
      } else if (!shiftKey) {
        this.sceneManager.deselectAll();
      }
    }
  }
}
