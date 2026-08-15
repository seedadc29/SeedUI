import * as THREE from 'three';
import { Engine } from './core/Engine.js';
import { TransformManager } from './core/TransformManager.js';
import { HistoryManager } from './core/HistoryManager.js';
import { SceneManager } from './scene/SceneManager.js';
import { MeshEditor } from './mesh/MeshEditor.js';
import { BlenderUI } from './ui/BlenderUI.js';

console.log('🚀 [BlenderPro3D] Inicializando ambiente 3D autêntico Blender 4.1...');

// 1. Initialize Core Engine (Canvas, WebGL Renderer, Z-Up coordinate system)
const engine = new Engine('viewport-canvas');

// 2. Initialize History Manager (Undo / Redo stack)
const historyManager = new HistoryManager();

// 3. Initialize Scene Manager
const sceneManager = new SceneManager(engine);
sceneManager.setHistoryManager(historyManager);

// 4. Initialize Transform Manager (3D Translate, Rotate, Scale Gizmos)
const transformManager = new TransformManager(engine, sceneManager, historyManager);

// 5. Initialize Mesh Editor (Extrude, Inset, Bevel, Loop Cut, Subdivisions)
const meshEditor = new MeshEditor(engine, sceneManager, transformManager, historyManager);

// 6. Initialize Authentic Blender 4.x UI
const blenderUI = new BlenderUI(engine, sceneManager, transformManager, meshEditor, historyManager);

// Select default Cube
if (sceneManager.objects.length > 0) {
  sceneManager.selectObject(sceneManager.objects[0]);
}

console.log('✨ [BlenderPro3D] Pronto e operacional!');
