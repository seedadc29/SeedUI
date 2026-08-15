import * as THREE from 'three';
import { TransformControls } from 'three/addons/controls/TransformControls.js';

export class TransformManager {
  constructor(engine, sceneManager, historyManager) {
    this.engine = engine;
    this.sceneManager = sceneManager;
    this.historyManager = historyManager;
    this.currentMode = 'translate'; // 'translate' | 'rotate' | 'scale'
    this.isTransforming = false;

    this.initialActiveTransform = {
      position: new THREE.Vector3(),
      quaternion: new THREE.Quaternion(),
      scale: new THREE.Vector3()
    };
    this.initialOtherTransforms = new Map();
    this.beforeDragTransforms = [];

    this.initTransformControls();
  }

  setHistoryManager(hm) {
    this.historyManager = hm;
  }

  initTransformControls() {
    this.transformControls = new TransformControls(this.engine.activeCamera, this.engine.canvas);
    this.transformControls.size = 0.85;
    this.transformControls.setSpace('world');

    // Dragging events for multi-object transform & Undo/Redo
    this.transformControls.addEventListener('dragging-changed', (event) => {
      this.engine.controls.enabled = !event.value;
      this.isTransforming = event.value;

      if (event.value) {
        // Drag started: record initial transforms for both delta movement and Undo
        this.recordMultiObjectStarts();
        this.recordBeforeDragTransforms();
      } else {
        // Drag ended: record after transforms and push Undo action
        this.recordAfterDragAndPushHistory();
      }
    });

    this.transformControls.addEventListener('objectChange', () => {
      if (this.transformControls.object) {
        this.applyMultiObjectTransforms();
        this.sceneManager.updateTransformUI();
      }
    });

    this.engine.scene.add(this.transformControls.getHelper());
  }

  recordBeforeDragTransforms() {
    this.beforeDragTransforms = [];
    const targets = this.sceneManager.getSelectedObjects();
    targets.forEach((obj) => {
      this.beforeDragTransforms.push({
        obj,
        position: obj.position.clone(),
        quaternion: obj.quaternion.clone(),
        scale: obj.scale.clone()
      });
    });
  }

  recordAfterDragAndPushHistory() {
    if (this.beforeDragTransforms.length === 0 || !this.historyManager) return;

    const afterDragTransforms = [];
    let hasChanged = false;

    this.beforeDragTransforms.forEach((before) => {
      const currentPos = before.obj.position.clone();
      const currentRot = before.obj.quaternion.clone();
      const currentScale = before.obj.scale.clone();

      if (
        !currentPos.equals(before.position) ||
        !currentRot.equals(before.quaternion) ||
        !currentScale.equals(before.scale)
      ) {
        hasChanged = true;
      }

      afterDragTransforms.push({
        obj: before.obj,
        position: currentPos,
        quaternion: currentRot,
        scale: currentScale
      });
    });

    if (hasChanged) {
      const beforeStates = [...this.beforeDragTransforms];
      const afterStates = [...afterDragTransforms];

      this.historyManager.push({
        description: 'Transformação de Objeto',
        undo: () => {
          beforeStates.forEach((s) => {
            s.obj.position.copy(s.position);
            s.obj.quaternion.copy(s.quaternion);
            s.obj.scale.copy(s.scale);
          });
          this.sceneManager.updateTransformUI();
        },
        redo: () => {
          afterStates.forEach((s) => {
            s.obj.position.copy(s.position);
            s.obj.quaternion.copy(s.quaternion);
            s.obj.scale.copy(s.scale);
          });
          this.sceneManager.updateTransformUI();
        }
      });
    }
  }

  recordMultiObjectStarts() {
    const active = this.transformControls.object;
    if (!active) return;

    this.initialActiveTransform.position.copy(active.position);
    this.initialActiveTransform.quaternion.copy(active.quaternion);
    this.initialActiveTransform.scale.copy(active.scale);

    this.initialOtherTransforms.clear();
    const selected = this.sceneManager.getSelectedObjects();
    selected.forEach((obj) => {
      if (obj !== active) {
        this.initialOtherTransforms.set(obj, {
          position: obj.position.clone(),
          quaternion: obj.quaternion.clone(),
          scale: obj.scale.clone()
        });
      }
    });
  }

  applyMultiObjectTransforms() {
    const active = this.transformControls.object;
    if (!active || this.initialOtherTransforms.size === 0) return;

    const deltaPos = new THREE.Vector3().subVectors(active.position, this.initialActiveTransform.position);
    const deltaRot = new THREE.Quaternion().multiplyQuaternions(
      active.quaternion,
      this.initialActiveTransform.quaternion.clone().invert()
    );
    const deltaScale = new THREE.Vector3(
      active.scale.x / (this.initialActiveTransform.scale.x || 1),
      active.scale.y / (this.initialActiveTransform.scale.y || 1),
      active.scale.z / (this.initialActiveTransform.scale.z || 1)
    );

    this.initialOtherTransforms.forEach((initT, obj) => {
      if (this.currentMode === 'translate') {
        obj.position.copy(initT.position).add(deltaPos);
      } else if (this.currentMode === 'rotate') {
        const offset = initT.position.clone().sub(this.initialActiveTransform.position);
        offset.applyQuaternion(deltaRot);
        obj.position.copy(this.initialActiveTransform.position).add(offset);
        obj.quaternion.multiplyQuaternions(deltaRot, initT.quaternion);
      } else if (this.currentMode === 'scale') {
        const offset = initT.position.clone().sub(this.initialActiveTransform.position);
        offset.multiply(deltaScale);
        obj.position.copy(this.initialActiveTransform.position).add(offset);
        obj.scale.copy(initT.scale).multiply(deltaScale);
      }
    });
  }

  attach(mesh) {
    if (mesh) {
      this.transformControls.attach(mesh);
      this.transformControls.enabled = true;
      this.transformControls.visible = true;
    } else {
      this.detach();
    }
  }

  detach() {
    this.transformControls.detach();
    this.transformControls.enabled = false;
    this.transformControls.visible = false;
  }

  setMode(mode) {
    this.currentMode = mode;
    this.transformControls.setMode(mode);
  }

  getMode() {
    return this.currentMode;
  }

  setSpace(space) {
    this.transformControls.setSpace(space);
  }

  updateCamera(camera) {
    this.transformControls.camera = camera;
  }
}
