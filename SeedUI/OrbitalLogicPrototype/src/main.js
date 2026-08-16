import { Scene3D } from './3d/Scene3D.js';
import { OrbitalGraph } from './orbital/OrbitalGraph.js';
import { PaletteUI } from './ui/PaletteUI.js';
import { HistoryManager } from './core/HistoryManager.js';

document.addEventListener('DOMContentLoaded', () => {
  // 1. History Manager (Undo / Redo)
  const historyManager = new HistoryManager(null);

  // 2. Initialize 3D Viewport Scene
  const canvas3D = document.getElementById('canvas-3d');
  const scene3D = new Scene3D(canvas3D);

  // 3. Initialize 2D Orbital Logic Graph
  const canvasOrbital = document.getElementById('canvas-orbital');
  const orbitalGraph = new OrbitalGraph(canvasOrbital, null, null, historyManager);

  // 4. Initialize UI, Inspector, Splitters & Accordions
  const paletteUI = new PaletteUI(orbitalGraph, scene3D, historyManager);

  // Reset focus button
  document.getElementById('btn-reset-zoom')?.addEventListener('click', () => {
    orbitalGraph.panX = canvasOrbital.parentElement.clientWidth / 2;
    orbitalGraph.panY = canvasOrbital.parentElement.clientHeight / 2;
    orbitalGraph.zoom = 1.0;
  });

  // Clear scene button (with full 3D and history sync)
  document.getElementById('btn-clear-scene')?.addEventListener('click', () => {
    if (orbitalGraph.suns.length > 0) {
      historyManager.pushState(orbitalGraph.suns);
      orbitalGraph.suns = [];
      orbitalGraph.selectedEntity = null;
      paletteUI.updateStatsCounters();
      paletteUI.renderInspector(null);
      scene3D.syncWithOrbitalSuns([]);
    }
  });

  // Zoom tools
  document.getElementById('tool-zoom-in')?.addEventListener('click', () => {
    orbitalGraph.zoom = Math.min(2.5, orbitalGraph.zoom * 1.15);
  });

  document.getElementById('tool-zoom-out')?.addEventListener('click', () => {
    orbitalGraph.zoom = Math.max(0.4, orbitalGraph.zoom * 0.85);
  });

  console.log('🪐 Seed Studio - Lógica Orbital Funcional Inicializada!');
});
