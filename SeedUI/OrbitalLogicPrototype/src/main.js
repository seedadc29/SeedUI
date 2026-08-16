import { Scene3D } from './3d/Scene3D.js';
import { OrbitalGraph } from './orbital/OrbitalGraph.js';
import { PaletteUI } from './ui/PaletteUI.js';

document.addEventListener('DOMContentLoaded', () => {
  // 1. Initialize 3D Viewport Scene
  const canvas3D = document.getElementById('canvas-3d');
  const scene3D = new Scene3D(canvas3D);

  // 2. Initialize 2D Orbital Logic Graph
  const canvasOrbital = document.getElementById('canvas-orbital');
  const orbitalGraph = new OrbitalGraph(canvasOrbital, null);

  // 3. Initialize UI, Inspector & Accordions
  const paletteUI = new PaletteUI(orbitalGraph, scene3D);

  // Reset focus button
  document.getElementById('btn-reset-zoom')?.addEventListener('click', () => {
    orbitalGraph.panX = canvasOrbital.parentElement.clientWidth / 2;
    orbitalGraph.panY = canvasOrbital.parentElement.clientHeight / 2;
    orbitalGraph.zoom = 1.0;
  });

  // Clear scene button
  document.getElementById('btn-clear-scene')?.addEventListener('click', () => {
    orbitalGraph.suns = [];
    paletteUI.updateStatsCounters();
    paletteUI.renderInspector(null);
  });

  // Zoom tools
  document.getElementById('tool-zoom-in')?.addEventListener('click', () => {
    orbitalGraph.zoom = Math.min(2.5, orbitalGraph.zoom * 1.15);
  });

  document.getElementById('tool-zoom-out')?.addEventListener('click', () => {
    orbitalGraph.zoom = Math.max(0.4, orbitalGraph.zoom * 0.85);
  });

  console.log('🪐 Seed Studio - Protótipo de Lógica Orbital inicializado com sucesso!');
});
