import { Engine } from './core/Engine.js';
import { SceneManager } from './scene/SceneManager.js';
import { TransformManager } from './core/TransformManager.js';
import { HistoryManager } from './core/HistoryManager.js';
import { MeshEditor } from './mesh/MeshEditor.js';
import { PaintManager } from './paint/PaintManager.js';
import { ExportManager } from './io/ExportManager.js';
import { ShadingManager } from './render/ShadingManager.js';
import { DiagnosticsManager } from './core/DiagnosticsManager.js';
import { UIManager } from './ui/UI.js';

window.addEventListener('DOMContentLoaded', () => {
  console.log('🚀 Inicializando Seed3D Studio...');

  const engine = new Engine('viewport-canvas');
  const sceneManager = new SceneManager(engine);
  const transformManager = new TransformManager(engine, sceneManager);
  const historyManager = new HistoryManager();
  const meshEditor = new MeshEditor(engine, sceneManager, transformManager, historyManager);
  const paintManager = new PaintManager(engine, sceneManager, historyManager);
  const exportManager = new ExportManager(sceneManager);
  const shadingManager = new ShadingManager(engine, sceneManager);
  const ui = new UIManager(engine, sceneManager, transformManager, meshEditor, historyManager, paintManager, exportManager);
  ui.shadingManager = shadingManager;
  const diagnostics = new DiagnosticsManager(engine, sceneManager, transformManager, meshEditor, ui);
  ui.diagnostics = diagnostics;

  // Expose to window for easy debugging/inspection
  window.seed3d = {
    engine,
    sceneManager,
    transformManager,
    historyManager,
    meshEditor,
    paintManager,
    exportManager,
    shadingManager,
    diagnostics,
    ui
  };
  window.stove3d = window.seed3d; // Alias para compatibilidade

  console.log('✅ Seed3D Studio - Topologia Quad BMesh, Shading Studio, UV Atlas e Diagnóstico Ativos!');
});
