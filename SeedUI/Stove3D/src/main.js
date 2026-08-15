import { Engine } from './core/Engine.js';
import { SceneManager } from './scene/SceneManager.js';
import { TransformManager } from './core/TransformManager.js';
import { HistoryManager } from './core/HistoryManager.js';
import { MeshEditor } from './mesh/MeshEditor.js';
import { PaintManager } from './paint/PaintManager.js';
import { ExportManager } from './io/ExportManager.js';
import { DiagnosticsManager } from './core/DiagnosticsManager.js';
import { UIManager } from './ui/UI.js';

window.addEventListener('DOMContentLoaded', () => {
  console.log('🚀 Inicializando Stove 3D Studio...');

  const engine = new Engine('viewport-canvas');
  const sceneManager = new SceneManager(engine);
  const transformManager = new TransformManager(engine, sceneManager);
  const historyManager = new HistoryManager();
  const meshEditor = new MeshEditor(engine, sceneManager, transformManager, historyManager);
  const paintManager = new PaintManager(engine, sceneManager, historyManager);
  const exportManager = new ExportManager(sceneManager);
  const ui = new UIManager(engine, sceneManager, transformManager, meshEditor, historyManager, paintManager, exportManager);
  const diagnostics = new DiagnosticsManager(engine, sceneManager, transformManager, meshEditor, ui);
  ui.diagnostics = diagnostics;

  // Expose to window for easy debugging/inspection
  window.stove3d = {
    engine,
    sceneManager,
    transformManager,
    historyManager,
    meshEditor,
    paintManager,
    exportManager,
    diagnostics,
    ui
  };

  console.log('✅ Stove 3D Studio - Diagnóstico, Topologia, Pintura 3D e Undo/Redo Ativos!');
});
