export class PaletteUI {
  constructor(orbitalGraph, scene3D, historyManager, tacticalMap = null) {
    this.orbitalGraph = orbitalGraph;
    this.scene3D = scene3D;
    this.historyManager = historyManager;
    this.tacticalMap = tacticalMap;

    this.maximizedPane = null;

    this.initAccordions();
    this.initDragAndDrop();
    this.initInspector();
    this.initPlayMode();
    this.initSplitters();
    this.initMaximizeControls();
    this.initAnimationToggle();
    this.initHistory();

    // Initial 3D sync
    this.scene3D.syncWithOrbitalSuns(this.orbitalGraph.suns);
  }

  initAccordions() {
    document.querySelectorAll('.accordion-header').forEach((header) => {
      header.addEventListener('click', () => {
        const group = header.closest('.catalog-accordion-group');
        group.classList.toggle('closed');
      });
    });
  }

  initDragAndDrop() {
    const orbitalWrapper = document.getElementById('orbital-wrapper');
    const orbitalCanvas = document.getElementById('canvas-orbital');

    document.querySelectorAll('.palette-chip-item').forEach((item) => {
      // 1. Drag Start
      item.addEventListener('dragstart', (e) => {
        const data = {
          type: item.dataset.type,
          category: item.dataset.category,
          name: item.dataset.name,
          model: item.dataset.model,
          moons: item.dataset.moons
        };
        const jsonStr = JSON.stringify(data);
        e.dataTransfer.setData('text/plain', jsonStr);
        e.dataTransfer.setData('application/json', jsonStr);
        e.dataTransfer.effectAllowed = 'copy';
      });

      // 2. Direct Click to Add (Instant Add to Active / Closest Sun)
      item.addEventListener('click', () => {
        const data = {
          type: item.dataset.type,
          category: item.dataset.category,
          name: item.dataset.name,
          model: item.dataset.model,
          moons: item.dataset.moons
        };
        const pos = { x: 0, y: 0 };
        this.orbitalGraph.addEntityFromPalette(data, pos);
        this.updateStatsCounters();
        this.scene3D.syncWithOrbitalSuns(this.orbitalGraph.suns);
      });
    });

    const setupDropTarget = (target) => {
      if (!target) return;

      target.addEventListener('dragover', (e) => {
        e.preventDefault();
        e.dataTransfer.dropEffect = 'copy';
        orbitalWrapper?.classList.add('drag-over');
      });

      target.addEventListener('dragleave', () => {
        orbitalWrapper?.classList.remove('drag-over');
      });

      target.addEventListener('drop', (e) => {
        e.preventDefault();
        orbitalWrapper?.classList.remove('drag-over');
        try {
          const raw = e.dataTransfer.getData('application/json') || e.dataTransfer.getData('text/plain');
          if (!raw) return;
          const data = JSON.parse(raw);
          const mousePos = this.orbitalGraph.getCanvasPos(e);
          this.orbitalGraph.addEntityFromPalette(data, mousePos);
          this.updateStatsCounters();
          this.scene3D.syncWithOrbitalSuns(this.orbitalGraph.suns);
        } catch (err) {
          console.error('Drop error:', err);
        }
      });
    };

    setupDropTarget(orbitalWrapper);
    setupDropTarget(orbitalCanvas);
  }

  initInspector() {
    this.orbitalGraph.onSelectionChange = (entity) => {
      this.renderInspector(entity);
    };

    this.orbitalGraph.onGraphChange = (suns) => {
      this.updateStatsCounters();
      this.scene3D.syncWithOrbitalSuns(suns);
    };
  }

  renderRoomInspector(room) {
    const titleEl = document.getElementById('inspector-node-title');
    const typeEl = document.getElementById('inspector-node-type');
    const bodyEl = document.getElementById('inspector-body');
    if (!titleEl || !bodyEl) return;

    if (!room) {
      this.renderInspector(null);
      return;
    }

    titleEl.textContent = room.name;
    typeEl.textContent = `SALA TÁTICA (${room.shape.toUpperCase()})`;

    let html = `
      <div class="inspector-row">
        <span class="inspector-label">Nome da Sala:</span>
        <input type="text" class="inspector-input" value="${room.name}" id="inp-room-name" />
      </div>
      <div class="inspector-row">
        <span class="inspector-label">Forma Geométrica:</span>
        <span style="color:#38bdf8;font-weight:700;">${room.shape.toUpperCase()}</span>
      </div>
    `;

    if (room.shape === 'circle') {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Raio (m):</span>
          <input type="number" step="0.5" class="inspector-input" value="${room.radius}" id="inp-room-radius" />
        </div>
      `;
    } else {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Largura (W):</span>
          <input type="number" step="0.5" class="inspector-input" value="${room.w}" id="inp-room-w" />
        </div>
        <div class="inspector-row">
          <span class="inspector-label">Altura (H):</span>
          <input type="number" step="0.5" class="inspector-input" value="${room.h}" id="inp-room-h" />
        </div>
      `;
    }

    html += `
      <div class="inspector-row">
        <span class="inspector-label">Posição Grid:</span>
        <span style="color:var(--seed-text-muted);font-size:11px;">X: ${room.x}m, Z: ${room.z}m</span>
      </div>
      <button class="btn-delete-node" id="btn-del-room-node" style="margin-top:10px;"><i class="ti ti-trash"></i> Excluir Sala</button>
    `;

    bodyEl.innerHTML = html;

    // Attach listeners
    document.getElementById('inp-room-name')?.addEventListener('input', (e) => {
      room.name = e.target.value;
    });

    document.getElementById('inp-room-radius')?.addEventListener('input', (e) => {
      room.radius = Math.max(1, parseFloat(e.target.value) || 1);
    });

    document.getElementById('inp-room-w')?.addEventListener('input', (e) => {
      room.w = Math.max(1.5, parseFloat(e.target.value) || 1.5);
    });

    document.getElementById('inp-room-h')?.addEventListener('input', (e) => {
      room.h = Math.max(1.5, parseFloat(e.target.value) || 1.5);
    });

    document.getElementById('btn-del-room-node')?.addEventListener('click', () => {
      if (this.tacticalMap) {
        this.tacticalMap.rooms = this.tacticalMap.rooms.filter(r => r.id !== room.id);
        this.tacticalMap.selectedRoom = null;
        this.renderInspector(null);
      }
    });
  }

  renderInspector(entity) {
    const titleEl = document.getElementById('inspector-node-title');
    const typeEl = document.getElementById('inspector-node-type');
    const bodyEl = document.getElementById('inspector-body');
    if (!titleEl || !bodyEl) return;

    if (!entity) {
      titleEl.textContent = 'Propriedades';
      typeEl.textContent = 'Nenhum';
      bodyEl.innerHTML = `<div class="inspector-empty-hint">Clique em qualquer Sol, Planeta, Lua ou Sala Tática para editar em tempo real.</div>`;
      return;
    }

    titleEl.textContent = entity.name;
    typeEl.textContent = entity.type ? entity.type.toUpperCase() : 'NÓ';

    let html = '';
    const isTrigger = entity.name.toUpperCase().includes('GATILHO') || entity.name.toUpperCase().includes('TRIGGER');

    if (entity.type === 'sun') {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Nome da Entidade:</span>
          <input type="text" class="inspector-input" value="${entity.name}" id="inp-sun-name" />
        </div>
        <div class="inspector-row">
          <span class="inspector-label">Raio do Núcleo:</span>
          <input type="number" class="inspector-input" value="${entity.radius}" id="inp-sun-radius" />
        </div>
        <div class="inspector-row">
          <span class="inspector-label">Planetas Orbitando:</span>
          <span style="color:#ff7700;font-weight:700;">${entity.planets.length}</span>
        </div>
      `;

      if (isTrigger) {
        html += `
          <div class="trigger-action-table-box" style="margin-top: 12px; padding: 10px; background: rgba(255, 149, 0, 0.1); border: 1px solid rgba(255, 149, 0, 0.35); border-radius: 6px;">
            <div style="font-weight: 700; font-size: 11px; color: #ff9500; margin-bottom: 8px; display: flex; align-items: center; gap: 4px;">
              <i class="ti ti-table"></i> TABELA DE AÇÕES DO GATILHO
            </div>
            <div class="inspector-row">
              <span class="inspector-label">Condição:</span>
              <select class="inspector-input" id="sel-trig-condition">
                <option value="enter">Ao Entrar na Área (On Enter)</option>
                <option value="exit">Ao Sair da Área (On Exit)</option>
                <option value="key_e">Pressionar Tecla 'E'</option>
              </select>
            </div>
            <div class="inspector-row">
              <span class="inspector-label">Ação / Sinal:</span>
              <select class="inspector-input" id="sel-trig-action">
                <option value="raise_block">Elevar Bloco / Abrir Porta (+Y)</option>
                <option value="checkpoint">Salvar Checkpoint (Sinal)</option>
                <option value="spawn_enemy">Spawnar Inimigo</option>
                <option value="victory">Vitória / Fim de Fase</option>
              </select>
            </div>
            <div class="inspector-row">
              <span class="inspector-label">Alvo Conectado:</span>
              <span style="color: #38bdf8; font-weight: 700; font-size: 11px;">Bloco de Colisão / Porta</span>
            </div>
          </div>
        `;
      }

      html += `
        <button class="btn-delete-node" id="btn-del-selected-node" style="margin-top:10px;"><i class="ti ti-trash"></i> Excluir Sistema</button>
      `;
    } else if (entity.type === 'planet') {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Mecânica Ativa:</span>
          <span style="color:#a288ff;font-weight:700;">${entity.name}</span>
        </div>
      `;

      // Render all 3D mechanical parameters (Moons) directly inside the planet inspector
      if (entity.moons && entity.moons.length > 0) {
        entity.moons.forEach((moon, idx) => {
          html += `
            <div class="inspector-row">
              <span class="inspector-label">${moon.name}:</span>
              <input type="number" step="0.1" class="inspector-input inp-planet-moon-param" data-index="${idx}" value="${moon.val}" />
            </div>
          `;
        });
      }

      html += `
        <button class="btn-delete-node" id="btn-del-selected-node"><i class="ti ti-trash"></i> Excluir Mecânica</button>
      `;
    } else if (entity.type === 'moon') {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Parâmetro 3D:</span>
          <span style="color:${entity.color || '#ffffff'};font-weight:700;">${entity.name}</span>
        </div>
        <div class="inspector-row">
          <span class="inspector-label">Valor (3D):</span>
          <input type="number" step="0.1" class="inspector-input" value="${entity.val}" id="inp-moon-val" />
        </div>
        <button class="btn-delete-node" id="btn-del-selected-node"><i class="ti ti-trash"></i> Excluir Parâmetro</button>
      `;
    }

    bodyEl.innerHTML = html;

    // Attach real-time edit listeners
    const inpSunName = document.getElementById('inp-sun-name');
    if (inpSunName) {
      inpSunName.addEventListener('input', (e) => {
        entity.name = e.target.value;
        this.scene3D.syncWithOrbitalSuns(this.orbitalGraph.suns);
      });
    }

    const inpSunRadius = document.getElementById('inp-sun-radius');
    if (inpSunRadius) {
      inpSunRadius.addEventListener('input', (e) => {
        entity.radius = parseFloat(e.target.value) || 36;
      });
    }

    // Direct 3D Parameter edits on Planet
    document.querySelectorAll('.inp-planet-moon-param').forEach(input => {
      input.addEventListener('input', (e) => {
        const idx = parseInt(e.target.dataset.index, 10);
        if (entity.moons && entity.moons[idx]) {
          entity.moons[idx].val = parseFloat(e.target.value) || 0;
          this.scene3D.syncWithOrbitalSuns(this.orbitalGraph.suns);
        }
      });
    });

    const inpMoonVal = document.getElementById('inp-moon-val');
    if (inpMoonVal) {
      inpMoonVal.addEventListener('input', (e) => {
        entity.val = parseFloat(e.target.value) || 0;
        this.scene3D.syncWithOrbitalSuns(this.orbitalGraph.suns);
      });
    }

    // Delete Button in Inspector
    const btnDel = document.getElementById('btn-del-selected-node');
    if (btnDel) {
      btnDel.addEventListener('click', () => {
        this.orbitalGraph.deleteEntity(entity);
      });
    }
  }

  initPlayMode() {
    const playBtn = document.getElementById('btn-toggle-play');
    const playIcon = document.getElementById('play-btn-icon');
    const playLabel = document.getElementById('play-btn-label');
    const chip3D = document.getElementById('chip-3d-status');

    let isPlaying = false;

    const toggle = () => {
      isPlaying = !isPlaying;
      this.scene3D.setPlayMode(isPlaying);

      if (isPlaying) {
        playBtn.classList.add('is-playing');
        playIcon.className = 'ti ti-player-pause';
        playLabel.textContent = 'Pausar (Enter)';
        if (chip3D) {
          chip3D.textContent = 'Executando (W/A/S/D)';
          chip3D.style.borderColor = '#34c759';
          chip3D.style.color = '#34c759';
        }
      } else {
        playBtn.classList.remove('is-playing');
        playIcon.className = 'ti ti-player-play';
        playLabel.textContent = 'Executar (Enter)';
        if (chip3D) {
          chip3D.textContent = 'Pronto (3D)';
          chip3D.style.borderColor = '#32ade6';
          chip3D.style.color = '#32ade6';
        }
      }
    };

    if (playBtn) {
      playBtn.addEventListener('click', () => {
        toggle();
        playBtn.blur(); // Remove HTML button focus so Space key only triggers 3D Jump!
      });
    }

    // Enter / F5 Shortcut for Play Mode (Ignored when typing in inputs)
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT') return;
      if (e.key === 'Enter' || e.key === 'F5') {
        e.preventDefault();
        toggle();
      }
    });

    this.scene3D.onCollisionEvent = (player, obstacle) => {
      this.orbitalGraph.triggerEnergyBeam('PLAYER', 'Objeto', '#ff3b30');
    };
  }

  // --- Resizable Splitters ---
  initSplitters() {
    const splitterLeft = document.getElementById('splitter-left');
    const splitterRight = document.getElementById('splitter-right');
    const pane3D = document.getElementById('pane-3d');
    const paneOrbital = document.getElementById('pane-orbital');
    const paneCatalog = document.getElementById('pane-catalog');
    const workspace = document.getElementById('main-workspace');

    let isDraggingLeft = false;
    let isDraggingRight = false;

    if (splitterLeft) {
      splitterLeft.addEventListener('pointerdown', (e) => {
        isDraggingLeft = true;
        splitterLeft.classList.add('is-dragging');
        document.body.style.cursor = 'col-resize';
      });
    }

    if (splitterRight) {
      splitterRight.addEventListener('pointerdown', (e) => {
        isDraggingRight = true;
        splitterRight.classList.add('is-dragging');
        document.body.style.cursor = 'col-resize';
      });
    }

    window.addEventListener('pointermove', (e) => {
      if (!isDraggingLeft && !isDraggingRight) return;
      const wsRect = workspace.getBoundingClientRect();
      const relativeX = e.clientX - wsRect.left;

      if (isDraggingLeft) {
        const leftPercent = Math.max(15, Math.min(65, (relativeX / wsRect.width) * 100));
        pane3D.style.flex = `0 0 ${leftPercent}%`;
        pane3D.style.width = `${leftPercent}%`;
        this.scene3D.onResize();
        this.orbitalGraph.resize();
      } else if (isDraggingRight) {
        const rightWidth = wsRect.right - e.clientX;
        const rightPercent = Math.max(12, Math.min(45, (rightWidth / wsRect.width) * 100));
        paneCatalog.style.flex = `0 0 ${rightPercent}%`;
        paneCatalog.style.width = `${rightPercent}%`;
        this.orbitalGraph.resize();
      }
    });

    window.addEventListener('pointerup', () => {
      if (isDraggingLeft || isDraggingRight) {
        isDraggingLeft = false;
        isDraggingRight = false;
        splitterLeft?.classList.remove('is-dragging');
        splitterRight?.classList.remove('is-dragging');
        document.body.style.cursor = '';
        this.scene3D.onResize();
        this.orbitalGraph.resize();
      }
    });
  }

  // --- Maximize / Solo Windows ---
  initMaximizeControls() {
    const pane3D = document.getElementById('pane-3d');
    const paneOrbital = document.getElementById('pane-orbital');
    const paneCatalog = document.getElementById('pane-catalog');
    const splitterLeft = document.getElementById('splitter-left');
    const splitterRight = document.getElementById('splitter-right');

    const resetLayout = () => {
      this.maximizedPane = null;
      [pane3D, paneOrbital, paneCatalog].forEach(p => {
        p.classList.remove('is-maximized', 'is-hidden');
        p.style.flex = '';
        p.style.width = '';
      });
      splitterLeft.style.display = '';
      splitterRight.style.display = '';
      document.getElementById('icon-max-3d').className = 'ti ti-maximize';
      document.getElementById('icon-max-orbital').className = 'ti ti-maximize';
      document.getElementById('icon-max-catalog').className = 'ti ti-maximize';
      setTimeout(() => {
        this.scene3D.onResize();
        this.orbitalGraph.resize();
      }, 50);
    };

    const toggleMaximize = (targetPane, iconId) => {
      if (this.maximizedPane === targetPane) {
        resetLayout();
        return;
      }

      this.maximizedPane = targetPane;
      const allPanes = [pane3D, paneOrbital, paneCatalog];
      allPanes.forEach(p => {
        if (p === targetPane) {
          p.classList.add('is-maximized');
          p.classList.remove('is-hidden');
        } else {
          p.classList.remove('is-maximized');
          p.classList.add('is-hidden');
        }
      });

      splitterLeft.style.display = 'none';
      splitterRight.style.display = 'none';

      document.getElementById('icon-max-3d').className = 'ti ti-maximize';
      document.getElementById('icon-max-orbital').className = 'ti ti-maximize';
      document.getElementById('icon-max-catalog').className = 'ti ti-maximize';
      document.getElementById(iconId).className = 'ti ti-minimize';

      setTimeout(() => {
        this.scene3D.onResize();
        this.orbitalGraph.resize();
      }, 50);
    };

    document.getElementById('btn-max-3d')?.addEventListener('click', () => toggleMaximize(pane3D, 'icon-max-3d'));
    document.getElementById('btn-max-orbital')?.addEventListener('click', () => toggleMaximize(paneOrbital, 'icon-max-orbital'));
    document.getElementById('btn-max-catalog')?.addEventListener('click', () => toggleMaximize(paneCatalog, 'icon-max-catalog'));
    document.getElementById('btn-reset-layout')?.addEventListener('click', resetLayout);
  }

  // --- Toggle Planet Rotation Animation ---
  initAnimationToggle() {
    const btnAnim = document.getElementById('btn-toggle-orbit-anim');
    const iconAnim = document.getElementById('icon-orbit-anim');
    const labelAnim = document.getElementById('label-orbit-anim');

    const btnTopbar = document.getElementById('btn-toggle-orbit-topbar');
    const iconTopbar = document.getElementById('icon-orbit-topbar');
    const labelTopbar = document.getElementById('label-orbit-topbar');

    const updateVisuals = () => {
      const active = this.orbitalGraph.isOrbitAnimationActive;
      if (btnAnim) {
        if (active) {
          btnAnim.classList.add('active');
          if (iconAnim) iconAnim.className = 'ti ti-rotate';
          if (labelAnim) labelAnim.textContent = 'Girar Órbitas';
        } else {
          btnAnim.classList.remove('active');
          if (iconAnim) iconAnim.className = 'ti ti-player-pause';
          if (labelAnim) labelAnim.textContent = 'Pausado';
        }
      }

      if (btnTopbar) {
        if (active) {
          btnTopbar.classList.remove('is-paused');
          btnTopbar.classList.add('is-rotating');
          if (iconTopbar) iconTopbar.className = 'ti ti-rotate';
          if (labelTopbar) labelTopbar.textContent = 'Girar Órbitas (Ativo)';
        } else {
          btnTopbar.classList.add('is-paused');
          btnTopbar.classList.remove('is-rotating');
          if (iconTopbar) iconTopbar.className = 'ti ti-player-pause';
          if (labelTopbar) labelTopbar.textContent = 'Rotação Pausada';
        }
      }
    };

    const toggle = () => {
      this.orbitalGraph.isOrbitAnimationActive = !this.orbitalGraph.isOrbitAnimationActive;
      updateVisuals();
    };

    if (btnAnim) btnAnim.addEventListener('click', toggle);
    if (btnTopbar) btnTopbar.addEventListener('click', toggle);
  }

  // --- Undo / Redo History Support ---
  initHistory() {
    const btnUndo = document.getElementById('btn-undo');
    const btnRedo = document.getElementById('btn-redo');

    const handleUndo = () => {
      if (this.historyManager) {
        const restored = this.historyManager.undo(this.orbitalGraph.suns);
        if (restored) {
          this.orbitalGraph.suns = restored;
          this.orbitalGraph.selectedEntity = null;
          this.renderInspector(null);
          this.updateStatsCounters();
          this.scene3D.syncWithOrbitalSuns(restored);
        }
      }
    };

    const handleRedo = () => {
      if (this.historyManager) {
        const restored = this.historyManager.redo(this.orbitalGraph.suns);
        if (restored) {
          this.orbitalGraph.suns = restored;
          this.orbitalGraph.selectedEntity = null;
          this.renderInspector(null);
          this.updateStatsCounters();
          this.scene3D.syncWithOrbitalSuns(restored);
        }
      }
    };

    if (btnUndo) btnUndo.addEventListener('click', handleUndo);
    if (btnRedo) btnRedo.addEventListener('click', handleRedo);

    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT') return;
      if (e.ctrlKey || e.metaKey) {
        if (e.key === 'z' || e.key === 'Z') {
          e.preventDefault();
          if (e.shiftKey) handleRedo();
          else handleUndo();
        } else if (e.key === 'y' || e.key === 'Y') {
          e.preventDefault();
          handleRedo();
        }
      }
    });
  }

  updateStatsCounters() {
    const statSuns = document.getElementById('stat-suns');
    const statPlanets = document.getElementById('stat-planets');
    const statMoons = document.getElementById('stat-moons');

    let totalPlanets = 0;
    let totalMoons = 0;

    this.orbitalGraph.suns.forEach(s => {
      totalPlanets += s.planets.length;
      s.planets.forEach(p => {
        if (p.moons) totalMoons += p.moons.length;
      });
    });

    if (statSuns) statSuns.textContent = `${this.orbitalGraph.suns.length} Sistemas`;
    if (statPlanets) statPlanets.textContent = `${totalPlanets} Planetas`;
    if (statMoons) statMoons.textContent = `${totalMoons} Luas`;
  }
}
