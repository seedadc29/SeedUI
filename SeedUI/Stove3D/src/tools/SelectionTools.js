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
      
      // 1. If Alt is held: Camera Orbit/Pan Navigation always takes precedence!
      if (e.altKey) {
        this.engine.controls.enabled = true;
        return;
      }

      // 2. If mouse is over Transform Gizmo or transforming, NEVER intercept with box selection!
      const tc = this.uiManager.transformManager.transformControls;
      if (this.uiManager.transformManager.isTransforming || (tc && tc.axis !== null && tc.axis !== '')) {
        return;
      }

      // 3. If Left Click Navigation is active and clicking on empty space: Allow OrbitControls to navigate!
      if (this.engine.navMode === 'left') {
        const rect = canvas.getBoundingClientRect();
        const mouse2D = new THREE.Vector2(
          ((e.clientX - rect.left) / rect.width) * 2 - 1,
          -((e.clientY - rect.top) / rect.height) * 2 + 1
        );
        const raycaster = new THREE.Raycaster();
        raycaster.setFromCamera(mouse2D, this.engine.activeCamera);
        const meshes = this.sceneManager.getAllMeshes().filter(m => m.visible);
        const hits = raycaster.intersectObjects(meshes, false);

        if (hits.length === 0) {
          this.engine.controls.enabled = true;
          return;
        }
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

  // --- SELECTION EXECUTION ALGORITHMS ---

  applySweepSelectionAt(clientX, clientY, shiftKey) {
    const radius = this.activeTool === 'circle' ? this.circleRadius : 22;
    const rect = this.engine.canvas.getBoundingClientRect();
    const mx = clientX - rect.left;
    const my = clientY - rect.top;

    if (this.uiManager.currentSubmode !== 'object' && this.meshEditor.activeMesh && this.meshEditor.activeMesh.userData.quadMesh) {
      const qm = this.meshEditor.activeMesh.userData.quadMesh;
      for (let i = 0; i < qm.vertices.length; i++) {
        const vWorld = qm.vertices[i].clone().applyMatrix4(this.meshEditor.activeMesh.matrixWorld);
        const vNDC = vWorld.project(this.engine.activeCamera);
        if (vNDC.z < 1.0) {
          const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
          const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
          if (Math.hypot(mx - sx, my - sy) <= radius) {
            this.meshEditor.selectedVertices.add(i);
          }
        }
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

    const rect = this.engine.canvas.getBoundingClientRect();

    if (this.uiManager.currentSubmode !== 'object' && this.meshEditor.activeMesh && this.meshEditor.activeMesh.userData.quadMesh) {
      const qm = this.meshEditor.activeMesh.userData.quadMesh;
      if (!shiftKey) {
        this.meshEditor.selectedVertices.clear();
        this.meshEditor.selectedFaces.clear();
        this.meshEditor.selectedEdges.clear();
      }

      for (let i = 0; i < qm.vertices.length; i++) {
        const vWorld = qm.vertices[i].clone().applyMatrix4(this.meshEditor.activeMesh.matrixWorld);
        const vNDC = vWorld.project(this.engine.activeCamera);
        if (vNDC.z < 1.0) {
          const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
          const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
          if (sx >= boxMinX && sx <= boxMaxX && sy >= boxMinY && sy <= boxMaxY) {
            this.meshEditor.selectedVertices.add(i);
          }
        }
      }

      if (this.uiManager.currentSubmode === 'face') {
        qm.quads.forEach((quad, fIdx) => {
          const allEnclosed = quad.every(vIdx => this.meshEditor.selectedVertices.has(vIdx));
          if (allEnclosed) {
            this.meshEditor.selectedFaces.add(fIdx);
          }
        });
      }

      this.meshEditor.rebuildEditHelpers();
      this.meshEditor.updateTransformAnchor();
    } else {
      // Object Mode Box Select: True 2D AABB Overlap / Crossing test
      // If the selection rectangle overlaps or crosses ANY part of the object: MATCHED!
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
      if (!shiftKey) this.meshEditor.selectedVertices.clear();

      for (let i = 0; i < qm.vertices.length; i++) {
        const vWorld = qm.vertices[i].clone().applyMatrix4(this.meshEditor.activeMesh.matrixWorld);
        const vNDC = vWorld.project(this.engine.activeCamera);
        if (vNDC.z < 1.0) {
          const sx = (vNDC.x * 0.5 + 0.5) * rect.width;
          const sy = (-vNDC.y * 0.5 + 0.5) * rect.height;
          if (isPointInPoly(sx, sy, points)) {
            this.meshEditor.selectedVertices.add(i);
          }
        }
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
