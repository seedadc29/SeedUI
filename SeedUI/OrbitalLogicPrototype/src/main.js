import { Scene3D } from './3d/Scene3D.js';
import { OrbitalGraph } from './orbital/OrbitalGraph.js';
import { TacticalMap } from './tactical/TacticalMap.js';
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

  // 4. Initialize Level 2D Blueprint / Tactical Command Map
  const canvasTactical = document.getElementById('canvas-tactical');
  const tacticalMap = new TacticalMap(
    canvasTactical,
    scene3D,
    orbitalGraph,
    (entity) => {
      if (entity && entity.sunId) {
        scene3D.selectEntityBySunId(entity.sunId);
      }
    },
    (room) => {
      paletteUI.renderRoomInspector(room);
    }
  );

  // 5. Initialize UI, Inspector, Splitters & Accordions
  const paletteUI = new PaletteUI(orbitalGraph, scene3D, historyManager, tacticalMap);

  // 6. Cross-Selection Synchronization (3D <-> Orbital Graph <-> Tactical Map)
  let isSyncing = false;

  scene3D.onEntitySelected = (sunId) => {
    if (isSyncing) return;
    isSyncing = true;
    try {
      if (!sunId) {
        orbitalGraph.selectedEntity = null;
        tacticalMap.selectedItem = null;
        paletteUI.renderInspector(null);
      } else {
        const sun = orbitalGraph.suns.find(s => s.id === sunId);
        if (sun) {
          orbitalGraph.selectedEntity = sun;
          tacticalMap.selectedItem = scene3D.entities.get(sunId) || null;
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
        } else if (entity.parentSunId || entity.sunId) {
          targetSunId = entity.parentSunId || entity.sunId;
        } else {
          const parent = orbitalGraph.getPlanetParentSun(entity);
          if (parent) targetSunId = parent.id;
        }

        if (targetSunId) {
          scene3D.selectEntityBySunId(targetSunId);
          tacticalMap.selectedItem = scene3D.entities.get(targetSunId) || null;
        }
      } else {
        scene3D.selectEntity(null);
        tacticalMap.selectedItem = null;
      }
    } finally {
      isSyncing = false;
    }
  };

  // 7. Horizontal Splitter (Resize Orbital Graph vs Tactical Map)
  const splitterCenterH = document.getElementById('splitter-center-horizontal');
  const paneOrbital = document.getElementById('pane-orbital');
  const paneTactical = document.getElementById('pane-tactical');

  if (splitterCenterH && paneOrbital && paneTactical) {
    let isDraggingH = false;
    let startY = 0;
    let startTopH = 0;

    splitterCenterH.addEventListener('mousedown', (e) => {
      isDraggingH = true;
      startY = e.clientY;
      startTopH = paneOrbital.getBoundingClientRect().height;
      splitterCenterH.classList.add('is-dragging');
      document.body.style.cursor = 'row-resize';
      document.body.style.userSelect = 'none';
    });

    window.addEventListener('mousemove', (e) => {
      if (!isDraggingH) return;
      const dy = e.clientY - startY;
      const newTopH = Math.max(100, Math.min(window.innerHeight - 200, startTopH + dy));
      paneOrbital.style.flex = `0 0 ${newTopH}px`;
      paneTactical.style.flex = '1 1 auto';
      orbitalGraph.resize();
      tacticalMap.resize();
    });

    window.addEventListener('mouseup', () => {
      if (isDraggingH) {
        isDraggingH = false;
        splitterCenterH.classList.remove('is-dragging');
        document.body.style.cursor = '';
        document.body.style.userSelect = '';
        orbitalGraph.resize();
        tacticalMap.resize();
      }
    });
  }

  // 8. Toolshelf Buttons & 3D Viewport Tool Switching
  const toolButtons = document.querySelectorAll('.seed-shelf-btn[data-tool]');
  const v3dButtons = document.querySelectorAll('.v3d-tool-btn[data-vtool]');

  const setTool = (toolName) => {
    toolButtons.forEach(btn => {
      btn.classList.toggle('active', btn.dataset.tool === toolName);
    });

    v3dButtons.forEach(btn => {
      btn.classList.toggle('active', btn.dataset.vtool === toolName);
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
      scene3D.setGizmoMode('translate');
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

  // 3D Viewport Toolbar Buttons
  document.getElementById('btn-3d-select')?.addEventListener('click', () => setTool('select'));
  document.getElementById('btn-3d-translate')?.addEventListener('click', () => setTool('translate'));
  document.getElementById('btn-3d-rotate')?.addEventListener('click', () => setTool('rotate'));
  document.getElementById('btn-3d-scale')?.addEventListener('click', () => setTool('scale'));
  document.getElementById('btn-3d-frame')?.addEventListener('click', () => scene3D.frameSelectedEntity());

  // Keyboard shortcut listener
  window.addEventListener('keydown', (e) => {
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;
    if (e.code === 'KeyG') setTool('translate');
    else if (e.code === 'KeyR') setTool('rotate');
    else if (e.code === 'KeyS' && !e.ctrlKey) setTool('scale');
    else if (e.code === 'KeyO') setTool('orbit');
    else if (e.code === 'KeyL') setTool('beam');
    else if (e.code === 'KeyI') setTool('inspect');
    else if (e.code === 'KeyV' || e.code === 'KeyW') setTool('select');
    else if (e.code === 'KeyF') scene3D.frameSelectedEntity();
  });

  // 9. Tactical Map Tool Selector Buttons (Selecionar, Mover, Quadrado, Círculo, Triângulo, Pan)
  document.getElementById('tool-tactical-select')?.addEventListener('click', () => {
    tacticalMap.setTool('select');
  });

  document.getElementById('tool-tactical-move')?.addEventListener('click', () => {
    tacticalMap.setTool('move');
  });

  document.getElementById('tool-tactical-rect')?.addEventListener('click', () => {
    tacticalMap.addShape('rect');
  });

  document.getElementById('tool-tactical-circle')?.addEventListener('click', () => {
    tacticalMap.addShape('circle');
  });

  document.getElementById('tool-tactical-triangle')?.addEventListener('click', () => {
    tacticalMap.addShape('triangle');
  });

  document.getElementById('tool-tactical-pan')?.addEventListener('click', () => {
    tacticalMap.setTool('pan');
  });

  // Reset tactical zoom button
  document.getElementById('btn-reset-tactical-zoom')?.addEventListener('click', () => {
    tacticalMap.panX = canvasTactical.parentElement.clientWidth / 2;
    tacticalMap.panY = canvasTactical.parentElement.clientHeight / 2;
    tacticalMap.zoom = 1.0;
  });

  // Reset focus button
  document.getElementById('btn-reset-zoom')?.addEventListener('click', () => {
    orbitalGraph.panX = canvasOrbital.parentElement.clientWidth / 2;
    orbitalGraph.panY = canvasOrbital.parentElement.clientHeight / 2;
    orbitalGraph.zoom = 1.0;
  });

  // Clear scene button
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
    tacticalMap.zoom = Math.min(2.5, tacticalMap.zoom * 1.15);
  });

  document.getElementById('tool-zoom-out')?.addEventListener('click', () => {
    orbitalGraph.zoom = Math.max(0.4, orbitalGraph.zoom * 0.85);
    tacticalMap.zoom = Math.max(0.5, tacticalMap.zoom * 0.85);
  });

  console.log('🪐 Seed Studio - Lógica Orbital 3D & Mapa Tático 2D Ativos!');
});
