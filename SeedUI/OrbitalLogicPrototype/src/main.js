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

  // 5. Cross-Selection Synchronization (3D <-> Orbital Graph)
  let isSyncing = false;

  scene3D.onEntitySelected = (sunId) => {
    if (isSyncing) return;
    isSyncing = true;
    try {
      if (!sunId) {
        orbitalGraph.selectedEntity = null;
        paletteUI.renderInspector(null);
      } else {
        const sun = orbitalGraph.suns.find(s => s.id === sunId);
        if (sun) {
          orbitalGraph.selectedEntity = sun;
          paletteUI.renderInspector(sun);
        }
      }
    } finally {
      isSyncing = false;
    }
  };

  const origSelectionChange = orbitalGraph.onSelectionChange;
  orbitalGraph.onSelectionChange = (entity) => {
    if (origSelectionChange) origSelectionChange(entity);
    if (isSyncing) return;
    isSyncing = true;
    try {
      if (entity) {
        let targetSunId = null;
        if (entity.type === 'sun') {
          targetSunId = entity.id;
        } else if (entity.parentSun) {
          targetSunId = entity.parentSun.id;
        } else {
          const parent = orbitalGraph.getPlanetParentSun(entity);
          if (parent) targetSunId = parent.id;
        }

        if (targetSunId) {
          scene3D.selectEntityBySunId(targetSunId);
        }
      } else {
        scene3D.selectEntity(null);
      }
    } finally {
      isSyncing = false;
    }
  };

  // 6. Toolshelf Buttons & Full Tool Switching
  const toolButtons = document.querySelectorAll('.seed-shelf-btn[data-tool]');
  const setTool = (toolName) => {
    toolButtons.forEach(btn => {
      btn.classList.toggle('active', btn.dataset.tool === toolName);
    });

    if (toolName === 'translate' || toolName === 'rotate' || toolName === 'scale') {
      scene3D.setGizmoMode(toolName);
      orbitalGraph.currentTool = 'select';
    } else if (toolName === 'orbit') {
      orbitalGraph.currentTool = 'orbit';
    } else if (toolName === 'beam') {
      orbitalGraph.currentTool = 'beam';
    } else if (toolName === 'inspect') {
      orbitalGraph.currentTool = 'select';
      const inspectorBox = document.getElementById('node-inspector');
      inspectorBox?.scrollIntoView({ behavior: 'smooth' });
    } else {
      orbitalGraph.currentTool = 'select';
    }
  };

  document.getElementById('tool-select')?.addEventListener('click', () => setTool('select'));
  document.getElementById('tool-move')?.addEventListener('click', () => setTool('translate'));
  document.getElementById('tool-rotate')?.addEventListener('click', () => setTool('rotate'));
  document.getElementById('tool-scale')?.addEventListener('click', () => setTool('scale'));
  document.getElementById('tool-orbit')?.addEventListener('click', () => setTool('orbit'));
  document.getElementById('tool-beam')?.addEventListener('click', () => setTool('beam'));
  document.getElementById('tool-inspect')?.addEventListener('click', () => setTool('inspect'));

  // Keyboard shortcut listener for active shelf button updates
  window.addEventListener('keydown', (e) => {
    if (e.target.tagName === 'INPUT') return;
    if (e.code === 'KeyG') setTool('translate');
    else if (e.code === 'KeyR') setTool('rotate');
    else if (e.code === 'KeyS' && !e.ctrlKey) setTool('scale');
    else if (e.code === 'KeyO') setTool('orbit');
    else if (e.code === 'KeyL') setTool('beam');
    else if (e.code === 'KeyI') setTool('inspect');
    else if (e.code === 'KeyV' || e.code === 'KeyW') setTool('select');
  });

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

  console.log('🪐 Seed Studio - Lógica Orbital Funcional com Gizmo 3D Inicializado!');
});
