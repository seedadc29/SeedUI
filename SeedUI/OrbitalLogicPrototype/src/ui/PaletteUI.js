export class PaletteUI {
  constructor(orbitalGraph, scene3D, historyManager, tacticalMap = null) {
    this.orbitalGraph = orbitalGraph;
    this.scene3D = scene3D;
    this.historyManager = historyManager;
    this.tacticalMap = tacticalMap;

    this.maximizedPane = null;
    this.isProportionalScale = false;

    this.initAccordions();
    this.initDragAndDrop();
    this.initInspector();
    this.initPlayMode();
    this.initSplitters();
    this.initMaximizeControls();
    this.initAnimationToggle();
    this.initHistory();
    this.initGameCameraButton();

    // Initial 3D sync
    this.scene3D.syncWithOrbitalSuns(this.orbitalGraph.suns);

    // Live update Inspector when user drags 3D Transform Gizmo
    this.scene3D.onTransformChange = (ent) => {
      this.syncInspectorWith3DTransform(ent);
    };

    this.scene3D.onCameraConfigChange = () => {
      if (this.scene3D.selectedEntity?.type === 'camera') {
        this.renderInspector(this.scene3D.selectedEntity);
      }
    };
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
    typeEl.textContent = `SALA TÁTICA (${room.shape ? room.shape.toUpperCase() : 'RETÂNGULO'})`;

    let html = `
      <div class="inspector-row">
        <span class="inspector-label">Nome da Sala / Zona:</span>
        <input type="text" class="inspector-input" value="${room.name}" id="inp-room-name" />
      </div>
      <div class="inspector-row">
        <span class="inspector-label">Posição X (Grid):</span>
        <input type="number" step="0.5" class="inspector-input" value="${room.x}" id="inp-room-x" />
      </div>
      <div class="inspector-row">
        <span class="inspector-label">Posição Z (Grid):</span>
        <input type="number" step="0.5" class="inspector-input" value="${room.z}" id="inp-room-z" />
      </div>
    `;

    if (room.shape === 'circle') {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Raio (Metros):</span>
          <input type="number" step="0.5" min="1.0" class="inspector-input" value="${room.radius || 3.5}" id="inp-room-rad" />
        </div>
      `;
    } else {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Largura (W):</span>
          <input type="number" step="0.5" min="1.5" class="inspector-input" value="${room.w || 6.0}" id="inp-room-w" />
        </div>
        <div class="inspector-row">
          <span class="inspector-label">Altura / Profundidade (H):</span>
          <input type="number" step="0.5" min="1.5" class="inspector-input" value="${room.h || 5.0}" id="inp-room-h" />
        </div>
      `;
    }

    html += `
      <button class="btn-delete-node" id="btn-del-room-node" style="margin-top: 14px;"><i class="ti ti-trash"></i> Excluir Sala Tática</button>
    `;

    bodyEl.innerHTML = html;

    document.getElementById('inp-room-name')?.addEventListener('input', (e) => {
      room.name = e.target.value.trim() || 'Nova Sala';
    });

    document.getElementById('inp-room-x')?.addEventListener('input', (e) => {
      room.x = parseFloat(e.target.value) || 0;
    });

    document.getElementById('inp-room-z')?.addEventListener('input', (e) => {
      room.z = parseFloat(e.target.value) || 0;
    });

    document.getElementById('inp-room-rad')?.addEventListener('input', (e) => {
      room.radius = Math.max(1.0, parseFloat(e.target.value) || 1.0);
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

  renderCameraInspector(targetEntity = null) {
    const titleEl = document.getElementById('inspector-node-title');
    const typeEl = document.getElementById('inspector-node-type');
    const bodyEl = document.getElementById('inspector-body');
    if (!titleEl || !bodyEl) return;

    const camCfg = this.scene3D.cameraConfig;
    const camMesh = this.scene3D.cameraHelperMesh;
    const cameraSun = this.orbitalGraph.suns.find(s => s.name.toUpperCase().includes('CAMERA') || s.name.toUpperCase().includes('CÂMERA'));

    titleEl.textContent = cameraSun?.name || 'Câmera de Jogo';
    typeEl.textContent = 'ENTIDADE / FAMÍLIA ORBITAL';

    const posX = camMesh ? camMesh.position.x.toFixed(1) : '0.0';
    const posY = camMesh ? camMesh.position.y.toFixed(1) : '3.5';
    const posZ = camMesh ? camMesh.position.z.toFixed(1) : '14.0';

    const rotXDeg = camMesh ? Math.round((camMesh.rotation.x * 180) / Math.PI) % 360 : 0;
    const rotYDeg = camMesh ? Math.round((camMesh.rotation.y * 180) / Math.PI) % 360 : 0;
    const rotZDeg = camMesh ? Math.round((camMesh.rotation.z * 180) / Math.PI) % 360 : 0;

    const normRotX = rotXDeg < 0 ? rotXDeg + 360 : rotXDeg;
    const normRotY = rotYDeg < 0 ? rotYDeg + 360 : rotYDeg;
    const normRotZ = rotZDeg < 0 ? rotZDeg + 360 : rotZDeg;

    let html = `
      <div class="inspector-row" style="background: rgba(56, 189, 248, 0.12); padding: 8px; border-radius: 6px; border: 1px solid rgba(56, 189, 248, 0.35); margin-bottom: 10px;">
        <div style="display:flex; justify-content:space-between; align-items:center; width:100%;">
          <div>
            <div style="font-size: 12px; font-weight: 700; color: #38bdf8;">
              <i class="ti ti-video"></i> Modo da Câmera
            </div>
            <div style="font-size: 10px; color: #94a3b8; margin-top: 2px;">
              ${camCfg.mode === 'follow' ? '🏃 Segue o jogador no cenário' : '🔒 Fixa estática na sala'}
            </div>
          </div>
          <button class="btn-scenery-preset ${this.scene3D.isPilotingGameCamera ? 'active' : ''}" id="btn-cam-pilot-toggle" style="padding: 4px 8px; color: #38bdf8; border-color: #38bdf8;">
            <i class="ti ti-eye"></i> ${this.scene3D.isPilotingGameCamera ? 'Sair da Visão' : 'Ver Visão (C)'}
          </button>
        </div>
      </div>

      <div class="inspector-row">
        <span class="inspector-label">Nome da Família:</span>
        <input type="text" class="inspector-input" value="${cameraSun?.name || 'Câmera de Jogo'}" id="inp-cam-sun-name" />
      </div>

      <div class="scenery-transform-box">
        <div class="scenery-box-header">
          <i class="ti ti-settings"></i> <span>🎮 COMPORTAMENTO & PRESETS</span>
        </div>

        <div class="inspector-row" style="margin-top: 6px;">
          <span class="inspector-label">Comportamento:</span>
          <select class="inspector-input" id="sel-cam-mode" style="width: 150px;">
            <option value="follow" ${camCfg.mode === 'follow' ? 'selected' : ''}>🏃 Seguir Jogador</option>
            <option value="static" ${camCfg.mode === 'static' ? 'selected' : ''}>🔒 Câmera Estática (Fixa)</option>
          </select>
        </div>

        <div class="scenery-presets-label" style="margin-top: 8px;">Presets Rápidos de Visão:</div>
        <div class="scenery-presets-grid" style="grid-template-columns: 1fr 1fr; gap: 4px;">
          <button class="btn-scenery-preset ${camCfg.preset === 'platformer' ? 'active' : ''}" id="preset-cam-platformer" title="Visão lateral 2.5D de lado perfeita para jogos de plataforma">🎮 Plataforma 2.5D</button>
          <button class="btn-scenery-preset ${camCfg.preset === 'third_person' ? 'active' : ''}" id="preset-cam-third" title="Câmera atrás e acima do personagem em terceira pessoa">🕹️ 3ª Pessoa 3D</button>
          <button class="btn-scenery-preset ${camCfg.preset === 'top_down' ? 'active' : ''}" id="preset-cam-topdown" title="Câmera no alto olhando para baixo">🚁 Top-Down Aérea</button>
          <button class="btn-scenery-preset ${camCfg.preset === 'first_person' ? 'active' : ''}" id="preset-cam-first" title="Câmera na altura dos olhos do personagem">👁️ 1ª Pessoa</button>
          <button class="btn-scenery-preset ${camCfg.preset === 'static' ? 'active' : ''}" id="preset-cam-static" style="grid-column: 1 / -1;" title="Câmera estática no ponto atual da cena">🎥 Fixa / Estática</button>
        </div>

        <!-- 1. 3D POSITION / OFFSET -->
        <div class="scenery-box-header" style="margin-top: 12px;">
          <i class="ti ti-arrows-move"></i> <span>📍 POSIÇÃO / DISTÂNCIA DA CÂMERA</span>
        </div>
        <div class="dim-control-group">
          <div class="dim-row">
            <span class="dim-axis-badge axis-x">X</span>
            <span class="dim-label">Posição X:</span>
            <input type="range" min="-30" max="30" step="0.5" value="${posX}" class="dim-slider" id="slider-cam-x" />
            <input type="number" step="0.1" value="${posX}" class="dim-number" id="inp-cam-x" />
            <span class="dim-unit">m</span>
          </div>
          <div class="dim-row">
            <span class="dim-axis-badge axis-y">Y</span>
            <span class="dim-label">Altura (Y):</span>
            <input type="range" min="0.5" max="30" step="0.5" value="${posY}" class="dim-slider" id="slider-cam-y" />
            <input type="number" step="0.1" value="${posY}" class="dim-number" id="inp-cam-y" />
            <span class="dim-unit">m</span>
          </div>
          <div class="dim-row">
            <span class="dim-axis-badge axis-z">Z</span>
            <span class="dim-label">Distância (Z):</span>
            <input type="range" min="-30" max="40" step="0.5" value="${posZ}" class="dim-slider" id="slider-cam-z" />
            <input type="number" step="0.1" value="${posZ}" class="dim-number" id="inp-cam-z" />
            <span class="dim-unit">m</span>
          </div>
        </div>

        <!-- 2. ROTATION -->
        <div class="scenery-box-header" style="margin-top: 10px;">
          <i class="ti ti-rotate-3d"></i> <span>🔄 INCLINAÇÃO / ROTAÇÃO (GRAUS)</span>
        </div>
        <div class="rot-control-group">
          <div class="dim-row">
            <span class="dim-axis-badge axis-x">X</span>
            <span class="dim-label">Pitch (X):</span>
            <input type="range" min="0" max="360" step="5" value="${normRotX}" class="dim-slider" id="slider-cam-rot-x" />
            <input type="number" min="0" max="360" step="1" value="${normRotX}" class="dim-number" id="inp-cam-rot-x" />
            <span class="dim-unit">°</span>
          </div>
          <div class="dim-row">
            <span class="dim-axis-badge axis-y">Y</span>
            <span class="dim-label">Yaw (Y):</span>
            <input type="range" min="0" max="360" step="5" value="${normRotY}" class="dim-slider" id="slider-cam-rot-y" />
            <input type="number" min="0" max="360" step="1" value="${normRotY}" class="dim-number" id="inp-cam-rot-y" />
            <span class="dim-unit">°</span>
          </div>
        </div>

        <!-- 3. OPTICS & FOV -->
        <div class="scenery-box-header" style="margin-top: 10px;">
          <i class="ti ti-camera"></i> <span>🔭 ÓPTICA & CAMPO DE VISÃO (FOV)</span>
        </div>
        <div class="dim-row">
          <span class="dim-label" style="width: 100px;">FOV (Ângulo):</span>
          <input type="range" min="25" max="100" step="1" value="${camCfg.fov || 48}" class="dim-slider" id="slider-cam-fov" />
          <input type="number" min="25" max="100" step="1" value="${camCfg.fov || 48}" class="dim-number" id="inp-cam-fov" />
          <span class="dim-unit">°</span>
        </div>

        <!-- 4. MOUSE LOOK & MOVIMENTO DO MOUSE (EXPERIÊNCIA DE JOGADOR) -->
        <div class="scenery-box-header" style="margin-top: 12px;">
          <i class="ti ti-mouse"></i> <span>🖱️ MOVIMENTAÇÃO DO MOUSE (SEM PRECISAR CLICAR)</span>
        </div>
        <div class="inspector-row" style="margin-top: 4px;">
          <label style="display:flex; align-items:center; gap:6px; font-size:12px; color:#f1f5f9; cursor:pointer;">
            <input type="checkbox" id="chk-cam-mouselook" ${camCfg.mouseLook?.enabled ? 'checked' : ''} />
            <b>Ativar Rotação com Mouse (Mouse Look Livre)</b>
          </label>
        </div>
        <div class="inspector-row" style="margin-top: 4px; background: rgba(255, 204, 0, 0.08); padding: 6px 8px; border-radius: 4px; border: 1px solid rgba(255, 204, 0, 0.25);">
          <label style="display:flex; align-items:center; gap:6px; font-size:11px; color:#ffcc00; cursor:pointer;">
            <input type="checkbox" id="chk-cam-lock-mouse" ${camCfg.mouseLook?.lockMouse ? 'checked' : ''} />
            <b>🔒 Travar Mouse (Visão 2D / Plataforma Estática)</b>
          </label>
        </div>
        <div class="dim-row" style="margin-top: 6px;">
          <span class="dim-label" style="width: 110px;">Sensibilidade:</span>
          <input type="range" min="0.5" max="4.0" step="0.1" value="${((camCfg.mouseLook?.sensitivityX || 0.003) * 1000).toFixed(1)}" class="dim-slider" id="slider-cam-mousesens" />
          <input type="number" min="0.5" max="4.0" step="0.1" value="${((camCfg.mouseLook?.sensitivityX || 0.003) * 1000).toFixed(1)}" class="dim-number" id="inp-cam-mousesens" />
          <span class="dim-unit">x</span>
        </div>
        <div class="inspector-row" style="gap: 12px; margin-top: 4px;">
          <label style="display:flex; align-items:center; gap:6px; font-size:11px; color:#cbd5e1; cursor:pointer;">
            <input type="checkbox" id="chk-cam-invert-y" ${camCfg.mouseLook?.invertY ? 'checked' : ''} />
            Inverter Eixo Y
          </label>
          <label style="display:flex; align-items:center; gap:6px; font-size:11px; color:#cbd5e1; cursor:pointer;">
            <input type="checkbox" id="chk-cam-yaw-limit" ${camCfg.mouseLook?.enableYawLimit ? 'checked' : ''} />
            Travar Giro Horizontal
          </label>
        </div>

        <!-- 5. MOVEMENT & ANGLE CLAMPING (DELIMITAÇÃO DE MOVIMENTOS) -->
        <div class="scenery-box-header" style="margin-top: 12px;">
          <i class="ti ti-barrier-block"></i> <span>🚧 DELIMITAÇÃO DE MOVIMENTOS & ÂNGULOS</span>
        </div>

        <!-- 5.1 Pitch Limits (Inclinação Vertical) -->
        <div class="dim-row">
          <span class="dim-axis-badge axis-x">V</span>
          <span class="dim-label">Pitch Mín (Baixo):</span>
          <input type="range" min="-85" max="0" step="5" value="${camCfg.mouseLook?.minPitchDeg || -60}" class="dim-slider" id="slider-cam-pitch-min" />
          <input type="number" min="-85" max="0" step="1" value="${camCfg.mouseLook?.minPitchDeg || -60}" class="dim-number" id="inp-cam-pitch-min" />
          <span class="dim-unit">°</span>
        </div>
        <div class="dim-row">
          <span class="dim-axis-badge axis-x">V</span>
          <span class="dim-label">Pitch Máx (Cima):</span>
          <input type="range" min="0" max="85" step="5" value="${camCfg.mouseLook?.maxPitchDeg || 75}" class="dim-slider" id="slider-cam-pitch-max" />
          <input type="number" min="0" max="85" step="1" value="${camCfg.mouseLook?.maxPitchDeg || 75}" class="dim-number" id="inp-cam-pitch-max" />
          <span class="dim-unit">°</span>
        </div>

        <!-- 5.2 Yaw Limits (Giro Horizontal) -->
        <div class="dim-row">
          <span class="dim-axis-badge axis-y">H</span>
          <span class="dim-label">Giro Mínimo:</span>
          <input type="range" min="-180" max="0" step="5" value="${camCfg.mouseLook?.minYawDeg || -180}" class="dim-slider" id="slider-cam-yaw-min" />
          <input type="number" min="-180" max="0" step="1" value="${camCfg.mouseLook?.minYawDeg || -180}" class="dim-number" id="inp-cam-yaw-min" />
          <span class="dim-unit">°</span>
        </div>
        <div class="dim-row">
          <span class="dim-axis-badge axis-y">H</span>
          <span class="dim-label">Giro Máximo:</span>
          <input type="range" min="0" max="180" step="5" value="${camCfg.mouseLook?.maxYawDeg || 180}" class="dim-slider" id="slider-cam-yaw-max" />
          <input type="number" min="0" max="180" step="1" value="${camCfg.mouseLook?.maxYawDeg || 180}" class="dim-number" id="inp-cam-yaw-max" />
          <span class="dim-unit">°</span>
        </div>

        <!-- 5.3 Distance / Zoom Clamps -->
        <div class="dim-row" style="margin-top: 6px;">
          <span class="dim-axis-badge axis-z">D</span>
          <span class="dim-label">Distância Mín:</span>
          <input type="range" min="1.0" max="15.0" step="0.5" value="${camCfg.distanceLimits?.minDistance || 2.0}" class="dim-slider" id="slider-cam-dist-min" />
          <input type="number" min="1.0" max="15.0" step="0.5" value="${camCfg.distanceLimits?.minDistance || 2.0}" class="dim-number" id="inp-cam-dist-min" />
          <span class="dim-unit">m</span>
        </div>
        <div class="dim-row">
          <span class="dim-axis-badge axis-z">D</span>
          <span class="dim-label">Distância Máx:</span>
          <input type="range" min="5.0" max="40.0" step="0.5" value="${camCfg.distanceLimits?.maxDistance || 30.0}" class="dim-slider" id="slider-cam-dist-max" />
          <input type="number" min="5.0" max="40.0" step="0.5" value="${camCfg.distanceLimits?.maxDistance || 30.0}" class="dim-number" id="inp-cam-dist-max" />
          <span class="dim-unit">m</span>
        </div>

        <!-- 5.4 World Position Clamps (Limites de Sala / Parede) -->
        <div class="inspector-row" style="margin-top: 8px;">
          <label style="display:flex; align-items:center; gap:6px; font-size:12px; color:#f1f5f9; cursor:pointer;">
            <input type="checkbox" id="chk-cam-limits-enabled" ${camCfg.limits?.enabled ? 'checked' : ''} />
            <b>Ativar Delimitação de Posição no Cenário</b>
          </label>
        </div>
        <div class="dim-row">
          <span class="dim-axis-badge axis-x">X</span>
          <span class="dim-label">Limites X:</span>
          <input type="number" step="1" value="${camCfg.limits?.minX ?? -25}" class="dim-number" id="inp-cam-lim-minx" style="width:55px;" />
          <span style="font-size:11px; color:#64748b;">até</span>
          <input type="number" step="1" value="${camCfg.limits?.maxX ?? 25}" class="dim-number" id="inp-cam-lim-maxx" style="width:55px;" />
          <span class="dim-unit">m</span>
        </div>
        <div class="dim-row">
          <span class="dim-axis-badge axis-y">Y</span>
          <span class="dim-label">Limites Y:</span>
          <input type="number" step="0.5" value="${camCfg.limits?.minY ?? 0.5}" class="dim-number" id="inp-cam-lim-miny" style="width:55px;" />
          <span style="font-size:11px; color:#64748b;">até</span>
          <input type="number" step="0.5" value="${camCfg.limits?.maxY ?? 20}" class="dim-number" id="inp-cam-lim-maxy" style="width:55px;" />
          <span class="dim-unit">m</span>
        </div>
        <div class="dim-row">
          <span class="dim-axis-badge axis-z">Z</span>
          <span class="dim-label">Limites Z:</span>
          <input type="number" step="1" value="${camCfg.limits?.minZ ?? -25}" class="dim-number" id="inp-cam-lim-minz" style="width:55px;" />
          <span style="font-size:11px; color:#64748b;">até</span>
          <input type="number" step="1" value="${camCfg.limits?.maxZ ?? 25}" class="dim-number" id="inp-cam-lim-maxz" style="width:55px;" />
          <span class="dim-unit">m</span>
        </div>
      </div>

      <button class="btn-delete-node" id="btn-del-camera-node" style="margin-top: 14px;"><i class="ti ti-trash"></i> Excluir Entidade Câmera</button>
    `;

    bodyEl.innerHTML = html;

    const syncCameraSunMoons = () => {
      const cSun = this.orbitalGraph.suns.find(s => s.name.toUpperCase().includes('CAMERA') || s.name.toUpperCase().includes('CÂMERA'));
      if (!cSun) return;

      const modePlanet = cSun.planets.find(p => p.name.toLowerCase().includes('seguir') || p.name.toLowerCase().includes('estatica') || p.name.toLowerCase().includes('modo'));
      if (modePlanet) {
        modePlanet.name = camCfg.mode === 'follow' ? 'Seguir' : 'Estática';
        const presetMoon = modePlanet.moons?.find(m => m.name.toLowerCase().includes('preset'));
        if (presetMoon) presetMoon.val = camCfg.preset;
      }

      const opticsPlanet = cSun.planets.find(p => p.name.toLowerCase().includes('lente') || p.name.toLowerCase().includes('optica') || p.name.toLowerCase().includes('óptica'));
      if (opticsPlanet) {
        const fovMoon = opticsPlanet.moons?.find(m => m.name.toLowerCase().includes('fov'));
        if (fovMoon) fovMoon.val = camCfg.fov;
        const distMoon = opticsPlanet.moons?.find(m => m.name.toLowerCase().includes('distancia') || m.name.toLowerCase().includes('distância'));
        if (distMoon) distMoon.val = camCfg.offset.z;
        const altMoon = opticsPlanet.moons?.find(m => m.name.toLowerCase().includes('altura'));
        if (altMoon) altMoon.val = camCfg.offset.y;
      }
    };

    // Rename camera sun
    document.getElementById('inp-cam-sun-name')?.addEventListener('input', (e) => {
      if (cameraSun) {
        cameraSun.name = e.target.value;
        const camEnt = this.scene3D.entities.get('game-camera-1');
        if (camEnt) {
          camEnt.name = e.target.value;
          camEnt.familyName = e.target.value;
        }
      }
    });

    // Mouse Look Handlers
    document.getElementById('chk-cam-mouselook')?.addEventListener('change', (e) => {
      this.scene3D.setGameCameraMouseLook({ enabled: e.target.checked });
    });

    document.getElementById('chk-cam-lock-mouse')?.addEventListener('change', (e) => {
      this.scene3D.setGameCameraMouseLook({ lockMouse: e.target.checked });
    });

    const onSensChange = (val) => {
      const sens = Math.max(0.1, val) * 0.001;
      this.scene3D.setGameCameraMouseLook({ sensitivityX: sens, sensitivityY: sens });
      const sl = document.getElementById('slider-cam-mousesens');
      const num = document.getElementById('inp-cam-mousesens');
      if (sl && document.activeElement !== sl) sl.value = val;
      if (num && document.activeElement !== num) num.value = val;
    };
    document.getElementById('slider-cam-mousesens')?.addEventListener('input', (e) => onSensChange(parseFloat(e.target.value) || 1.0));
    document.getElementById('inp-cam-mousesens')?.addEventListener('input', (e) => onSensChange(parseFloat(e.target.value) || 1.0));

    document.getElementById('chk-cam-invert-y')?.addEventListener('change', (e) => {
      this.scene3D.setGameCameraMouseLook({ invertY: e.target.checked });
    });

    document.getElementById('chk-cam-yaw-limit')?.addEventListener('change', (e) => {
      this.scene3D.setGameCameraMouseLook({ enableYawLimit: e.target.checked });
    });

    // Pitch & Yaw Clamps Handlers
    const setupPitch = (prop, sliderId, numId) => {
      const sl = document.getElementById(sliderId);
      const num = document.getElementById(numId);
      const onVal = (val) => {
        this.scene3D.setGameCameraMouseLook({ [prop]: val });
        if (sl && document.activeElement !== sl) sl.value = val;
        if (num && document.activeElement !== num) num.value = val;
      };
      sl?.addEventListener('input', (e) => onVal(parseFloat(e.target.value) || 0));
      num?.addEventListener('input', (e) => onVal(parseFloat(e.target.value) || 0));
    };
    setupPitch('minPitchDeg', 'slider-cam-pitch-min', 'inp-cam-pitch-min');
    setupPitch('maxPitchDeg', 'slider-cam-pitch-max', 'inp-cam-pitch-max');
    setupPitch('minYawDeg', 'slider-cam-yaw-min', 'inp-cam-yaw-min');
    setupPitch('maxYawDeg', 'slider-cam-yaw-max', 'inp-cam-yaw-max');

    // Distance Clamps Handlers
    const setupDist = (prop, sliderId, numId) => {
      const sl = document.getElementById(sliderId);
      const num = document.getElementById(numId);
      const onVal = (val) => {
        this.scene3D.setGameCameraDistanceLimits({ [prop]: val });
        if (sl && document.activeElement !== sl) sl.value = val;
        if (num && document.activeElement !== num) num.value = val;
      };
      sl?.addEventListener('input', (e) => onVal(parseFloat(e.target.value) || 2.0));
      num?.addEventListener('input', (e) => onVal(parseFloat(e.target.value) || 2.0));
    };
    setupDist('minDistance', 'slider-cam-dist-min', 'inp-cam-dist-min');
    setupDist('maxDistance', 'slider-cam-dist-max', 'inp-cam-dist-max');

    // World Limits Handlers
    document.getElementById('chk-cam-limits-enabled')?.addEventListener('change', (e) => {
      this.scene3D.setGameCameraLimits({ enabled: e.target.checked });
    });

    const setupLimInp = (prop, elemId) => {
      document.getElementById(elemId)?.addEventListener('input', (e) => {
        this.scene3D.setGameCameraLimits({ [prop]: parseFloat(e.target.value) || 0 });
      });
    };
    setupLimInp('minX', 'inp-cam-lim-minx');
    setupLimInp('maxX', 'inp-cam-lim-maxx');
    setupLimInp('minY', 'inp-cam-lim-miny');
    setupLimInp('maxY', 'inp-cam-lim-maxy');
    setupLimInp('minZ', 'inp-cam-lim-minz');
    setupLimInp('maxZ', 'inp-cam-lim-maxz');

    // Mode Selector Handler
    document.getElementById('sel-cam-mode')?.addEventListener('change', (e) => {
      this.scene3D.setGameCameraMode(e.target.value);
      syncCameraSunMoons();
    });

    // Preset Handlers
    document.getElementById('preset-cam-platformer')?.addEventListener('click', () => {
      this.scene3D.setGameCameraPreset('platformer');
      syncCameraSunMoons();
      this.renderCameraInspector();
    });

    document.getElementById('preset-cam-third')?.addEventListener('click', () => {
      this.scene3D.setGameCameraPreset('third_person');
      syncCameraSunMoons();
      this.renderCameraInspector();
    });

    document.getElementById('preset-cam-topdown')?.addEventListener('click', () => {
      this.scene3D.setGameCameraPreset('top_down');
      syncCameraSunMoons();
      this.renderCameraInspector();
    });

    document.getElementById('preset-cam-first')?.addEventListener('click', () => {
      this.scene3D.setGameCameraPreset('first_person');
      syncCameraSunMoons();
      this.renderCameraInspector();
    });

    document.getElementById('preset-cam-static')?.addEventListener('click', () => {
      this.scene3D.setGameCameraPreset('static');
      syncCameraSunMoons();
      this.renderCameraInspector();
    });

    // Pilot Button
    document.getElementById('btn-cam-pilot-toggle')?.addEventListener('click', () => {
      this.scene3D.toggleGameCameraView();
      this.renderCameraInspector();
    });

    // Position Handlers
    const setupCamPos = (axis, sliderId, numId) => {
      const slider = document.getElementById(sliderId);
      const num = document.getElementById(numId);

      const onVal = (val) => {
        const curX = parseFloat(document.getElementById('inp-cam-x')?.value) || 0;
        const curY = parseFloat(document.getElementById('inp-cam-y')?.value) || 3.5;
        const curZ = parseFloat(document.getElementById('inp-cam-z')?.value) || 14.0;

        if (axis === 'x') this.scene3D.setEntityPosition('game-camera-1', val, curY, curZ);
        else if (axis === 'y') this.scene3D.setEntityPosition('game-camera-1', curX, val, curZ);
        else this.scene3D.setEntityPosition('game-camera-1', curX, curY, val);

        syncCameraSunMoons();
        if (slider && document.activeElement !== slider) slider.value = val;
        if (num && document.activeElement !== num) num.value = val;
      };

      slider?.addEventListener('input', (e) => onVal(parseFloat(e.target.value) || 0));
      num?.addEventListener('input', (e) => onVal(parseFloat(e.target.value) || 0));
    };

    setupCamPos('x', 'slider-cam-x', 'inp-cam-x');
    setupCamPos('y', 'slider-cam-y', 'inp-cam-y');
    setupCamPos('z', 'slider-cam-z', 'inp-cam-z');

    // Rotation Handlers
    const setupCamRot = (axis, sliderId, numId) => {
      const slider = document.getElementById(sliderId);
      const num = document.getElementById(numId);

      const onVal = (deg) => {
        const curX = parseFloat(document.getElementById('inp-cam-rot-x')?.value) || 0;
        const curY = parseFloat(document.getElementById('inp-cam-rot-y')?.value) || 0;

        if (axis === 'x') this.scene3D.setEntityRotation('game-camera-1', deg, curY, 0);
        else this.scene3D.setEntityRotation('game-camera-1', curX, deg, 0);

        if (slider && document.activeElement !== slider) slider.value = deg;
        if (num && document.activeElement !== num) num.value = deg;
      };

      slider?.addEventListener('input', (e) => onVal(parseFloat(e.target.value) || 0));
      num?.addEventListener('input', (e) => onVal(parseFloat(e.target.value) || 0));
    };

    setupCamRot('x', 'slider-cam-rot-x', 'inp-cam-rot-x');
    setupCamRot('y', 'slider-cam-rot-y', 'inp-cam-rot-y');

    // FOV Slider & Input
    const sliderFov = document.getElementById('slider-cam-fov');
    const inpFov = document.getElementById('inp-cam-fov');
    const onFovChange = (fovVal) => {
      camCfg.fov = fovVal;
      this.scene3D.gameCamera.fov = fovVal;
      this.scene3D.gameCamera.updateProjectionMatrix();
      if (this.scene3D.cameraHelper) this.scene3D.cameraHelper.update();
      syncCameraSunMoons();
      if (sliderFov && document.activeElement !== sliderFov) sliderFov.value = fovVal;
      if (inpFov && document.activeElement !== inpFov) inpFov.value = fovVal;
    };
    sliderFov?.addEventListener('input', (e) => onFovChange(parseFloat(e.target.value) || 48));
    inpFov?.addEventListener('input', (e) => onFovChange(parseFloat(e.target.value) || 48));

    // Delete Button
    document.getElementById('btn-del-camera-node')?.addEventListener('click', () => {
      const cSun = this.orbitalGraph.suns.find(s => s.name.toUpperCase().includes('CAMERA') || s.name.toUpperCase().includes('CÂMERA'));
      if (cSun) {
        this.orbitalGraph.deleteEntity(cSun);
      }
      this.renderInspector(null);
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
      bodyEl.innerHTML = `<div class="inspector-empty-hint">Clique diretamente na <b>Câmera</b>, em qualquer <b>Objeto 3D</b> no mundo, Sol, Planeta ou Sala Tática para alterar posição, tamanho e rotação.</div>`;
      return;
    }

    if (entity.type === 'camera' || (entity.type === 'sun' && (entity.name.toUpperCase().includes('CAMERA') || entity.name.toUpperCase().includes('CÂMERA')))) {
      this.renderCameraInspector(entity);
      return;
    }

    // Resolve target 3D instance and target Family Sun
    let ent3D = null;
    let targetSun = null;

    if (entity.mesh) {
      ent3D = entity;
      targetSun = this.orbitalGraph.suns.find(s => s.id === ent3D.familyId) || null;
    } else if (entity.type === 'sun') {
      targetSun = entity;
      if (this.scene3D.selectedEntity && this.scene3D.selectedEntity.familyId === targetSun.id) {
        ent3D = this.scene3D.selectedEntity;
      } else {
        for (const e of this.scene3D.entities.values()) {
          if (e.familyId === targetSun.id) { ent3D = e; break; }
        }
      }
    }

    const familyInstances = targetSun ? Array.from(this.scene3D.entities.values()).filter(e => e.familyId === targetSun.id) : [];

    titleEl.textContent = ent3D ? ent3D.name : (entity.name || 'Entidade');
    typeEl.textContent = entity.type ? entity.type.toUpperCase() : (ent3D ? `INSTÂNCIA 3D` : 'NÓ');

    let html = '';
    const isTrigger = (entity.name || ent3D?.familyName || '').toUpperCase().includes('GATILHO') || (entity.name || '').toUpperCase().includes('TRIGGER');

    if (targetSun || ent3D) {
      const baseW = ent3D?.baseSize?.x || 1.8;
      const baseH = ent3D?.baseSize?.y || 2.2;
      const baseD = ent3D?.baseSize?.z || 1.8;

      const scaleX = ent3D?.mesh ? ent3D.mesh.scale.x : 1.0;
      const scaleY = ent3D?.mesh ? ent3D.mesh.scale.y : 1.0;
      const scaleZ = ent3D?.mesh ? ent3D.mesh.scale.z : 1.0;

      const dimX = (baseW * scaleX).toFixed(1);
      const dimY = (baseH * scaleY).toFixed(1);
      const dimZ = (baseD * scaleZ).toFixed(1);

      const posX = ent3D?.mesh ? ent3D.mesh.position.x.toFixed(1) : '0.0';
      const posY = ent3D?.mesh ? ent3D.mesh.position.y.toFixed(1) : '1.1';
      const posZ = ent3D?.mesh ? ent3D.mesh.position.z.toFixed(1) : '0.0';

      const rotXDeg = ent3D?.mesh ? Math.round((ent3D.mesh.rotation.x * 180) / Math.PI) % 360 : 0;
      const rotYDeg = ent3D?.mesh ? Math.round((ent3D.mesh.rotation.y * 180) / Math.PI) % 360 : 0;
      const rotZDeg = ent3D?.mesh ? Math.round((ent3D.mesh.rotation.z * 180) / Math.PI) % 360 : 0;

      const normRotX = rotXDeg < 0 ? rotXDeg + 360 : rotXDeg;
      const normRotY = rotYDeg < 0 ? rotYDeg + 360 : rotYDeg;
      const normRotZ = rotZDeg < 0 ? rotZDeg + 360 : rotZDeg;

      html += `
        <div class="inspector-row" style="background: rgba(56, 189, 248, 0.08); padding: 6px 8px; border-radius: 6px; border: 1px solid rgba(56, 189, 248, 0.2); margin-bottom: 8px;">
          <div>
            <div style="font-size: 11px; font-weight: 700; color: #38bdf8;">
              <i class="ti ti-box"></i> ${ent3D ? ent3D.name : 'Instância #1'}
            </div>
            <div style="font-size: 10px; color: #94a3b8; margin-top: 2px;">
              Família Orbital: <span style="color:#ff7700; font-weight:700;">${targetSun?.name || ent3D?.familyName}</span> (${familyInstances.length} no mundo)
            </div>
          </div>
          <div style="display:flex; gap:4px;">
            <button class="btn-scenery-preset" id="btn-duplicate-inspector" title="Duplicar Objeto 3D no Mundo (Ctrl+D)" style="padding: 3px 6px; color:#38bdf8; border-color:#38bdf8;">
              <i class="ti ti-copy"></i> Duplicar
            </button>
            <button class="btn-scenery-preset" id="btn-detach-family" title="Separar este objeto em uma nova Família Orbital única" style="padding: 3px 6px; color:#fb923c;">
              <i class="ti ti-star"></i> Separar
            </button>
          </div>
        </div>

        <div class="inspector-row">
          <span class="inspector-label">Nome da Família:</span>
          <input type="text" class="inspector-input" value="${targetSun ? targetSun.name : (ent3D?.familyName || '')}" id="inp-sun-name" />
        </div>

        <!-- 1. 3D POSITION IN WORLD (POSIÇÃO) -->
        <div class="scenery-transform-box">
          <div class="scenery-box-header">
            <i class="ti ti-arrows-move"></i> <span>📍 POSIÇÃO 3D NO MUNDO</span>
          </div>
          <div class="dim-control-group">
            <div class="dim-row">
              <span class="dim-axis-badge axis-x">X</span>
              <span class="dim-label">Posição X:</span>
              <input type="range" min="-30" max="30" step="0.5" value="${posX}" class="dim-slider" id="slider-pos-x" />
              <input type="number" step="0.1" value="${posX}" class="dim-number" id="inp-pos-x" />
              <span class="dim-unit">m</span>
            </div>
            <div class="dim-row">
              <span class="dim-axis-badge axis-y">Y</span>
              <span class="dim-label">Posição Y:</span>
              <input type="range" min="0" max="25" step="0.2" value="${posY}" class="dim-slider" id="slider-pos-y" />
              <input type="number" step="0.1" value="${posY}" class="dim-number" id="inp-pos-y" />
              <span class="dim-unit">m</span>
            </div>
            <div class="dim-row">
              <span class="dim-axis-badge axis-z">Z</span>
              <span class="dim-label">Posição Z:</span>
              <input type="range" min="-30" max="30" step="0.5" value="${posZ}" class="dim-slider" id="slider-pos-z" />
              <input type="number" step="0.1" value="${posZ}" class="dim-number" id="inp-pos-z" />
              <span class="dim-unit">m</span>
            </div>
          </div>

          <!-- 2. 3D ROTATION (ROTAÇÃO) -->
          <div class="scenery-box-header" style="margin-top: 10px;">
            <i class="ti ti-rotate-3d"></i> <span>🔄 ROTAÇÃO 3D (GRAUS)</span>
          </div>
          <div class="rot-control-group">
            <div class="dim-row">
              <span class="dim-axis-badge axis-x">X</span>
              <span class="dim-label">Pitch (X):</span>
              <input type="range" min="0" max="360" step="5" value="${normRotX}" class="dim-slider" id="slider-rot-x" />
              <input type="number" min="0" max="360" step="1" value="${normRotX}" class="dim-number" id="inp-rot-x" />
              <span class="dim-unit">°</span>
            </div>
            <div class="dim-row">
              <span class="dim-axis-badge axis-y">Y</span>
              <span class="dim-label">Giro / Yaw (Y):</span>
              <input type="range" min="0" max="360" step="5" value="${normRotY}" class="dim-slider" id="slider-rot-y" />
              <input type="number" min="0" max="360" step="1" value="${normRotY}" class="dim-number" id="inp-rot-y" />
              <span class="dim-unit">°</span>
            </div>
            <div class="dim-row">
              <span class="dim-axis-badge axis-z">Z</span>
              <span class="dim-label">Roll (Z):</span>
              <input type="range" min="0" max="360" step="5" value="${normRotZ}" class="dim-slider" id="slider-rot-z" />
              <input type="number" min="0" max="360" step="1" value="${normRotZ}" class="dim-number" id="inp-rot-z" />
              <span class="dim-unit">°</span>
            </div>
          </div>

          <!-- 3. 3D DIMENSIONS / STRETCHING (TAMANHO) -->
          <div class="scenery-box-header" style="margin-top: 10px;">
            <i class="ti ti-dimensions"></i> <span>📐 DIMENSÕES / TAMANHO 3D</span>
          </div>

          <div class="inspector-toggle-row">
            <label class="inspector-toggle-label" title="Trava a proporção para aumentar/diminuir uniformemente todos os lados">
              <input type="checkbox" id="chk-proportional-scale" ${this.isProportionalScale ? 'checked' : ''} />
              <span>🔗 Escala Proporcional Uniforme</span>
            </label>
          </div>

          <div class="dim-control-group">
            <div class="dim-row">
              <span class="dim-axis-badge axis-x">X</span>
              <span class="dim-label">Largura (X):</span>
              <input type="range" min="0.2" max="25" step="0.2" value="${dimX}" class="dim-slider" id="slider-dim-x" />
              <input type="number" min="0.1" step="0.1" value="${dimX}" class="dim-number" id="inp-dim-x" />
              <span class="dim-unit">m</span>
            </div>

            <div class="dim-row">
              <span class="dim-axis-badge axis-y">Y</span>
              <span class="dim-label">Altura (Y):</span>
              <input type="range" min="0.2" max="15" step="0.2" value="${dimY}" class="dim-slider" id="slider-dim-y" />
              <input type="number" min="0.1" step="0.1" value="${dimY}" class="dim-number" id="inp-dim-y" />
              <span class="dim-unit">m</span>
            </div>

            <div class="dim-row">
              <span class="dim-axis-badge axis-z">Z</span>
              <span class="dim-label">Profundidade (Z):</span>
              <input type="range" min="0.2" max="25" step="0.2" value="${dimZ}" class="dim-slider" id="slider-dim-z" />
              <input type="number" min="0.1" step="0.1" value="${dimZ}" class="dim-number" id="inp-dim-z" />
              <span class="dim-unit">m</span>
            </div>
          </div>

          <div class="scenery-presets-label">Presets Rápidos de Cenário:</div>
          <div class="scenery-presets-grid">
            <button class="btn-scenery-preset" data-px="6.0" data-py="3.0" data-pz="0.5" title="Parede padrão 6m de largura por 3m de altura">🧱 Parede 6m</button>
            <button class="btn-scenery-preset" data-px="12.0" data-py="3.0" data-pz="0.5" title="Parede longa de 12m">🧱 Parede 12m</button>
            <button class="btn-scenery-preset" data-px="8.0" data-py="0.4" data-pz="8.0" title="Chão / Plataforma larga 8x8m">🪜 Chão 8x8m</button>
            <button class="btn-scenery-preset" data-px="2.0" data-py="2.0" data-pz="2.0" title="Cubo / Bloco 2x2x2m">📦 Cubo 2m</button>
            <button class="btn-scenery-preset" data-px="1.2" data-py="6.0" data-pz="1.2" title="Coluna / Pilar vertical 6m">🚪 Pilar 6m</button>
          </div>

          <!-- 4. COLLISION & PHYSICS -->
          <div class="scenery-box-header" style="margin-top: 10px;">
            <i class="ti ti-shield"></i> <span>COLISÃO & FÍSICA DE CENÁRIO</span>
          </div>
          <div class="inspector-row">
            <span class="inspector-label">Comportamento:</span>
            <select class="inspector-input" id="sel-collision-type" style="width: 140px;">
              <option value="solid" ${ent3D?.isSolid ? 'selected' : ''}>Sólido (Parede / Piso)</option>
              <option value="trigger" ${ent3D?.type === 'trigger' ? 'selected' : ''}>Gatilho (Atravessável)</option>
              <option value="none" ${!ent3D?.hasCollision ? 'selected' : ''}>Desativada (Passável)</option>
            </select>
          </div>

          <div class="inspector-toggle-row" style="margin-top: 6px;">
            <label class="inspector-toggle-label" title="Permite ao jogador pular e ficar em pé em cima do objeto como uma plataforma ou parede sólida">
              <input type="checkbox" id="chk-walkable-top" ${ent3D?.walkableTop !== false ? 'checked' : ''} />
              <span>🪜 Permitir Andar / Ficar em Cima</span>
            </label>
          </div>

          <div class="dim-row" style="margin-top: 6px;">
            <span class="dim-label" style="width: 110px;" title="Espaço ou folga extra do colisor além da malha visual">Margem / Padding:</span>
            <input type="range" min="0.0" max="1.0" step="0.05" value="${ent3D?.collisionPadding || 0}" class="dim-slider" id="slider-col-padding" />
            <input type="number" min="0.0" max="2.0" step="0.05" value="${ent3D?.collisionPadding || 0}" class="dim-number" id="inp-col-padding" />
            <span class="dim-unit">m</span>
          </div>

          <div class="inspector-row" style="margin-top: 6px; font-size: 10px; color: #38bdf8;">
            <span>Altura do Topo (Cota Y):</span>
            <span style="font-weight: bold;" id="lbl-top-y">${(((ent3D?.mesh?.position.y || 1.1) + (baseH * scaleY) / 2)).toFixed(2)} m</span>
          </div>
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
        <button class="btn-delete-node" id="btn-del-selected-node" style="margin-top:10px;"><i class="ti ti-trash"></i> Excluir Instância 3D</button>
      `;
    } else if (entity.type === 'planet') {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Mecânica Ativa:</span>
          <span style="color:#a288ff;font-weight:700;">${entity.name}</span>
        </div>
      `;

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

    // Duplicate button
    document.getElementById('btn-duplicate-inspector')?.addEventListener('click', () => {
      this.scene3D.duplicateSelectedEntity();
    });

    // Detach to new unique family
    document.getElementById('btn-detach-family')?.addEventListener('click', () => {
      if (!ent3D) return;
      if (this.historyManager) this.historyManager.saveSnapshot();

      const newFamilyId = `sun-${Date.now()}`;
      const newFamilyName = `${ent3D.name} (Família)`;
      const origSun = this.orbitalGraph.suns.find(s => s.id === ent3D.familyId);

      const newSun = {
        id: newFamilyId,
        name: newFamilyName,
        type: 'sun',
        x: (origSun?.x || 0) + 120,
        y: (origSun?.y || 0) + 40,
        radius: 36,
        color: origSun?.color || '#38bdf8',
        orbits: JSON.parse(JSON.stringify(origSun?.orbits || [{ radius: 65, dash: [4, 4] }])),
        planets: JSON.parse(JSON.stringify(origSun?.planets || []))
      };

      this.orbitalGraph.suns.push(newSun);
      ent3D.familyId = newFamilyId;
      ent3D.familyName = newFamilyName;
      this.orbitalGraph.selectedEntity = newSun;
      this.renderInspector(ent3D);
      this.orbitalGraph.notifyGraphChange();
    });

    // Attach real-time edit listeners
    const inpSunName = document.getElementById('inp-sun-name');
    if (inpSunName && targetSun) {
      inpSunName.addEventListener('input', (e) => {
        targetSun.name = e.target.value;
        if (ent3D) ent3D.familyName = e.target.value;
        this.scene3D.syncWithOrbitalSuns(this.orbitalGraph.suns);
      });
    }

    // 1. Position Handlers (X, Y, Z)
    const setupPosListeners = (axis, sliderId, numId) => {
      const slider = document.getElementById(sliderId);
      const num = document.getElementById(numId);

      const onValChange = (val) => {
        const activeEnt = this.scene3D.selectedEntity || ent3D;
        if (!activeEnt) return;
        const curX = parseFloat(document.getElementById('inp-pos-x')?.value) || (activeEnt.mesh?.position.x || 0);
        const curY = parseFloat(document.getElementById('inp-pos-y')?.value) || (activeEnt.mesh?.position.y || 1.1);
        const curZ = parseFloat(document.getElementById('inp-pos-z')?.value) || (activeEnt.mesh?.position.z || 0);

        if (axis === 'x') this.scene3D.setEntityPosition(activeEnt.id, val, curY, curZ);
        else if (axis === 'y') this.scene3D.setEntityPosition(activeEnt.id, curX, val, curZ);
        else this.scene3D.setEntityPosition(activeEnt.id, curX, curY, val);

        if (slider && document.activeElement !== slider) slider.value = val;
        if (num && document.activeElement !== num) num.value = val;
      };

      slider?.addEventListener('input', (e) => onValChange(parseFloat(e.target.value) || 0));
      num?.addEventListener('input', (e) => onValChange(parseFloat(e.target.value) || 0));
    };

    setupPosListeners('x', 'slider-pos-x', 'inp-pos-x');
    setupPosListeners('y', 'slider-pos-y', 'inp-pos-y');
    setupPosListeners('z', 'slider-pos-z', 'inp-pos-z');

    // 2. Rotation Handlers (X, Y, Z)
    const setupRotListeners = (axis, sliderId, numId) => {
      const slider = document.getElementById(sliderId);
      const num = document.getElementById(numId);

      const onValChange = (deg) => {
        const activeEnt = this.scene3D.selectedEntity || ent3D;
        if (!activeEnt) return;
        const curX = parseFloat(document.getElementById('inp-rot-x')?.value) || 0;
        const curY = parseFloat(document.getElementById('inp-rot-y')?.value) || 0;
        const curZ = parseFloat(document.getElementById('inp-rot-z')?.value) || 0;

        if (axis === 'x') this.scene3D.setEntityRotation(activeEnt.id, deg, curY, curZ);
        else if (axis === 'y') this.scene3D.setEntityRotation(activeEnt.id, curX, deg, curZ);
        else this.scene3D.setEntityRotation(activeEnt.id, curX, curY, deg);

        if (slider && document.activeElement !== slider) slider.value = deg;
        if (num && document.activeElement !== num) num.value = deg;
      };

      slider?.addEventListener('input', (e) => onValChange(parseFloat(e.target.value) || 0));
      num?.addEventListener('input', (e) => onValChange(parseFloat(e.target.value) || 0));
    };

    setupRotListeners('x', 'slider-rot-x', 'inp-rot-x');
    setupRotListeners('y', 'slider-rot-y', 'inp-rot-y');
    setupRotListeners('z', 'slider-rot-z', 'inp-rot-z');

    // 3. Proportional Scale Toggle
    const chkProp = document.getElementById('chk-proportional-scale');
    if (chkProp) {
      chkProp.addEventListener('change', (e) => {
        this.isProportionalScale = e.target.checked;
      });
    }

    // 4. 3D Dimension Handlers (X, Y, Z)
    const applyDimensions = (newX, newY, newZ) => {
      const activeEnt = this.scene3D.selectedEntity || ent3D;
      if (!activeEnt) return;
      this.scene3D.setEntityDimensions(activeEnt.id, newX, newY, newZ);
      const sliderX = document.getElementById('slider-dim-x');
      const numX = document.getElementById('inp-dim-x');
      const sliderY = document.getElementById('slider-dim-y');
      const numY = document.getElementById('inp-dim-y');
      const sliderZ = document.getElementById('slider-dim-z');
      const numZ = document.getElementById('inp-dim-z');

      if (sliderX && document.activeElement !== sliderX) sliderX.value = newX;
      if (numX && document.activeElement !== numX) numX.value = newX;
      if (sliderY && document.activeElement !== sliderY) sliderY.value = newY;
      if (numY && document.activeElement !== numY) numY.value = newY;
      if (sliderZ && document.activeElement !== sliderZ) sliderZ.value = newZ;
      if (numZ && document.activeElement !== numZ) numZ.value = newZ;
    };

    const setupDimListeners = (axis, sliderId, numId) => {
      const slider = document.getElementById(sliderId);
      const num = document.getElementById(numId);

      const onValChange = (val) => {
        const curX = parseFloat(document.getElementById('inp-dim-x')?.value) || 1.8;
        const curY = parseFloat(document.getElementById('inp-dim-y')?.value) || 2.2;
        const curZ = parseFloat(document.getElementById('inp-dim-z')?.value) || 1.8;

        if (this.isProportionalScale) {
          let ratio = 1.0;
          if (axis === 'x') ratio = val / Math.max(0.1, curX);
          else if (axis === 'y') ratio = val / Math.max(0.1, curY);
          else ratio = val / Math.max(0.1, curZ);

          applyDimensions(
            Math.round(curX * ratio * 10) / 10,
            Math.round(curY * ratio * 10) / 10,
            Math.round(curZ * ratio * 10) / 10
          );
        } else {
          if (axis === 'x') applyDimensions(val, curY, curZ);
          else if (axis === 'y') applyDimensions(curX, val, curZ);
          else applyDimensions(curX, curY, val);
        }
      };

      slider?.addEventListener('input', (e) => onValChange(parseFloat(e.target.value) || 0.1));
      num?.addEventListener('input', (e) => onValChange(parseFloat(e.target.value) || 0.1));
    };

    setupDimListeners('x', 'slider-dim-x', 'inp-dim-x');
    setupDimListeners('y', 'slider-dim-y', 'inp-dim-y');
    setupDimListeners('z', 'slider-dim-z', 'inp-dim-z');

    // Scenery Quick Presets
    document.querySelectorAll('.btn-scenery-preset').forEach((btn) => {
      btn.addEventListener('click', () => {
        const px = parseFloat(btn.dataset.px) || 6.0;
        const py = parseFloat(btn.dataset.py) || 3.0;
        const pz = parseFloat(btn.dataset.pz) || 0.5;
        applyDimensions(px, py, pz);
      });
    });

    // Collision Type Change
    document.getElementById('sel-collision-type')?.addEventListener('change', (e) => {
      const activeEnt = this.scene3D.selectedEntity || ent3D;
      if (!activeEnt) return;
      if (e.target.value === 'solid') {
        activeEnt.hasCollision = true;
        activeEnt.isSolid = true;
        activeEnt.type = 'block';
      } else if (e.target.value === 'trigger') {
        activeEnt.hasCollision = true;
        activeEnt.isSolid = false;
        activeEnt.type = 'trigger';
      } else {
        activeEnt.hasCollision = false;
        activeEnt.isSolid = false;
      }
    });

    // Walkable Top Toggle
    document.getElementById('chk-walkable-top')?.addEventListener('change', (e) => {
      const activeEnt = this.scene3D.selectedEntity || ent3D;
      if (activeEnt) {
        activeEnt.walkableTop = e.target.checked;
      }
    });

    // Collision Padding / Margin Slider & Number
    const sliderColPad = document.getElementById('slider-col-padding');
    const inpColPad = document.getElementById('inp-col-padding');
    const onPadChange = (pad) => {
      const activeEnt = this.scene3D.selectedEntity || ent3D;
      if (activeEnt) {
        activeEnt.collisionPadding = pad;
      }
      if (sliderColPad && document.activeElement !== sliderColPad) sliderColPad.value = pad;
      if (inpColPad && document.activeElement !== inpColPad) inpColPad.value = pad;
    };
    sliderColPad?.addEventListener('input', (e) => onPadChange(parseFloat(e.target.value) || 0));
    inpColPad?.addEventListener('input', (e) => onPadChange(parseFloat(e.target.value) || 0));

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
        if (ent3D) {
          this.scene3D.deleteSelectedEntity();
        } else if (targetSun) {
          this.orbitalGraph.deleteEntity(targetSun);
        } else {
          this.orbitalGraph.deleteEntity(entity);
        }
      });
    }
  }

  syncInspectorWith3DTransform(ent) {
    if (!ent || !ent.mesh) return;

    if (ent.type === 'camera') {
      const posX = ent.mesh.position.x.toFixed(1);
      const posY = ent.mesh.position.y.toFixed(1);
      const posZ = ent.mesh.position.z.toFixed(1);

      const sliderPosX = document.getElementById('slider-cam-x');
      const numPosX = document.getElementById('inp-cam-x');
      const sliderPosY = document.getElementById('slider-cam-y');
      const numPosY = document.getElementById('inp-cam-y');
      const sliderPosZ = document.getElementById('slider-cam-z');
      const numPosZ = document.getElementById('inp-cam-z');

      if (sliderPosX && document.activeElement !== sliderPosX) sliderPosX.value = posX;
      if (numPosX && document.activeElement !== numPosX) numPosX.value = posX;
      if (sliderPosY && document.activeElement !== sliderPosY) sliderPosY.value = posY;
      if (numPosY && document.activeElement !== numPosY) numPosY.value = posY;
      if (sliderPosZ && document.activeElement !== sliderPosZ) sliderPosZ.value = posZ;
      if (numPosZ && document.activeElement !== numPosZ) numPosZ.value = posZ;
      return;
    }

    // 1. Position sync
    const posX = ent.mesh.position.x.toFixed(1);
    const posY = ent.mesh.position.y.toFixed(1);
    const posZ = ent.mesh.position.z.toFixed(1);

    const sliderPosX = document.getElementById('slider-pos-x');
    const numPosX = document.getElementById('inp-pos-x');
    const sliderPosY = document.getElementById('slider-pos-y');
    const numPosY = document.getElementById('inp-pos-y');
    const sliderPosZ = document.getElementById('slider-pos-z');
    const numPosZ = document.getElementById('inp-pos-z');

    if (sliderPosX && document.activeElement !== sliderPosX) sliderPosX.value = posX;
    if (numPosX && document.activeElement !== numPosX) numPosX.value = posX;
    if (sliderPosY && document.activeElement !== sliderPosY) sliderPosY.value = posY;
    if (numPosY && document.activeElement !== numPosY) numPosY.value = posY;
    if (sliderPosZ && document.activeElement !== sliderPosZ) sliderPosZ.value = posZ;
    if (numPosZ && document.activeElement !== numPosZ) numPosZ.value = posZ;

    // 2. Rotation sync
    const rotXDeg = Math.round((ent.mesh.rotation.x * 180) / Math.PI) % 360;
    const rotYDeg = Math.round((ent.mesh.rotation.y * 180) / Math.PI) % 360;
    const rotZDeg = Math.round((ent.mesh.rotation.z * 180) / Math.PI) % 360;

    const normRotX = rotXDeg < 0 ? rotXDeg + 360 : rotXDeg;
    const normRotY = rotYDeg < 0 ? rotYDeg + 360 : rotYDeg;
    const normRotZ = rotZDeg < 0 ? rotZDeg + 360 : rotZDeg;

    const sliderRotX = document.getElementById('slider-rot-x');
    const inpRotX = document.getElementById('inp-rot-x');
    const sliderRotY = document.getElementById('slider-rot-y');
    const inpRotY = document.getElementById('inp-rot-y');
    const sliderRotZ = document.getElementById('slider-rot-z');
    const inpRotZ = document.getElementById('inp-rot-z');

    if (sliderRotX && document.activeElement !== sliderRotX) sliderRotX.value = normRotX;
    if (inpRotX && document.activeElement !== inpRotX) inpRotX.value = normRotX;
    if (sliderRotY && document.activeElement !== sliderRotY) sliderRotY.value = normRotY;
    if (inpRotY && document.activeElement !== inpRotY) inpRotY.value = normRotY;
    if (sliderRotZ && document.activeElement !== sliderRotZ) sliderRotZ.value = normRotZ;
    if (inpRotZ && document.activeElement !== inpRotZ) inpRotZ.value = normRotZ;

    // 3. Dimensions sync
    const baseW = ent.baseSize?.x || 1.8;
    const baseH = ent.baseSize?.y || 2.2;
    const baseD = ent.baseSize?.z || 1.8;

    const dimX = (baseW * ent.mesh.scale.x).toFixed(1);
    const dimY = (baseH * ent.mesh.scale.y).toFixed(1);
    const dimZ = (baseD * ent.mesh.scale.z).toFixed(1);

    const sliderX = document.getElementById('slider-dim-x');
    const numX = document.getElementById('inp-dim-x');
    const sliderY = document.getElementById('slider-dim-y');
    const numY = document.getElementById('inp-dim-y');
    const sliderZ = document.getElementById('slider-dim-z');
    const numZ = document.getElementById('inp-dim-z');

    if (sliderX && document.activeElement !== sliderX) sliderX.value = dimX;
    if (numX && document.activeElement !== numX) numX.value = dimX;
    if (sliderY && document.activeElement !== sliderY) sliderY.value = dimY;
    if (numY && document.activeElement !== numY) numY.value = dimY;
    if (sliderZ && document.activeElement !== sliderZ) sliderZ.value = dimZ;
    if (numZ && document.activeElement !== numZ) numZ.value = dimZ;

    const lblTopY = document.getElementById('lbl-top-y');
    if (lblTopY) {
      const topY = ((ent.mesh.position.y || 1.1) + (baseH * ent.mesh.scale.y) / 2).toFixed(2);
      lblTopY.textContent = `${topY} m`;
    }
  }

  initGameCameraButton() {
    const btn = document.getElementById('btn-toggle-game-camera');
    const icon = document.getElementById('icon-game-camera');
    const lbl = document.getElementById('lbl-game-camera');

    if (btn) {
      btn.addEventListener('click', () => {
        const isPiloting = this.scene3D.toggleGameCameraView();
        if (isPiloting) {
          btn.style.background = '#0284c7';
          btn.style.color = '#ffffff';
          if (icon) icon.className = 'ti ti-eye';
          if (lbl) lbl.textContent = 'Visão de Jogo (Ativa)';
        } else {
          btn.style.background = '';
          btn.style.color = '#38bdf8';
          if (icon) icon.className = 'ti ti-video';
          if (lbl) lbl.textContent = 'Câmera de Jogo (C)';
        }
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
      if (isPlaying) {
        this.scene3D.startPlay();
        playBtn.classList.add('is-playing');
        playIcon.className = 'ti ti-player-pause';
        playLabel.textContent = 'Pausar (Enter)';
        if (chip3D) {
          chip3D.textContent = 'Executando (W/A/S/D)';
          chip3D.style.borderColor = '#34c759';
          chip3D.style.color = '#34c759';
        }
      } else {
        this.scene3D.stopPlay();
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
        playBtn.blur();
      });
    }

    // Enter / F5 Shortcut for Play Mode (Ignored when typing in inputs)
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;
      if (e.key === 'Enter' || e.key === 'F5') {
        e.preventDefault();
        toggle();
      }
    });

    this.scene3D.onCollisionEvent = (fromName, toName, color = '#ff3b30') => {
      if (fromName && toName) {
        this.orbitalGraph.triggerEnergyBeam(fromName, toName, color);
      }
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

  // --- Panes Maximize & Expand ---
  initMaximizeControls() {
    const setupMax = (btnId, paneId) => {
      const btn = document.getElementById(btnId);
      const pane = document.getElementById(paneId);
      if (!btn || !pane) return;

      btn.addEventListener('click', () => {
        const isMax = pane.classList.contains('is-maximized');
        document.querySelectorAll('.seed-pane').forEach((p) => p.classList.remove('is-maximized'));

        if (!isMax) {
          pane.classList.add('is-maximized');
          this.maximizedPane = pane;
          btn.querySelector('i').className = 'ti ti-minimize';
        } else {
          this.maximizedPane = null;
          btn.querySelector('i').className = 'ti ti-maximize';
        }

        setTimeout(() => {
          this.scene3D.onResize();
          this.orbitalGraph.resize();
        }, 50);
      });
    };

    setupMax('btn-max-3d', 'pane-3d');
    setupMax('btn-max-orbital', 'pane-orbital');
    setupMax('btn-max-catalog', 'pane-catalog');
    setupMax('btn-max-tactical', 'pane-tactical');
  }

  // --- Animation Live Toggle ---
  initAnimationToggle() {
    const animToggle = document.getElementById('chk-enable-anim');
    if (animToggle) {
      animToggle.addEventListener('change', (e) => {
        const isEnabled = e.target.checked;
        this.orbitalGraph.enableAnimations = isEnabled;
      });
    }
  }

  // --- History (Undo / Redo) Keyboard Hooks & Button Handlers ---
  initHistory() {
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;

      if ((e.ctrlKey || e.metaKey) && e.code === 'KeyZ' && !e.shiftKey) {
        e.preventDefault();
        this.historyManager.undo();
      } else if ((e.ctrlKey || e.metaKey) && (e.code === 'KeyY' || (e.code === 'KeyZ' && e.shiftKey))) {
        e.preventDefault();
        this.historyManager.redo();
      }
    });

    document.getElementById('btn-undo')?.addEventListener('click', () => {
      this.historyManager.undo();
    });

    document.getElementById('btn-redo')?.addEventListener('click', () => {
      this.historyManager.redo();
    });
  }

  updateStatsCounters() {
    const sunCount = this.orbitalGraph.suns.length;
    let planetCount = 0;
    let moonCount = 0;

    this.orbitalGraph.suns.forEach((s) => {
      planetCount += s.planets.length;
      s.planets.forEach((p) => {
        if (p.moons) moonCount += p.moons.length;
      });
    });

    const statSuns = document.getElementById('stat-active-suns');
    const statPlanets = document.getElementById('stat-active-planets');
    const statMoons = document.getElementById('stat-active-moons');

    if (statSuns) statSuns.textContent = sunCount;
    if (statPlanets) statPlanets.textContent = planetCount;
    if (statMoons) statMoons.textContent = moonCount;
  }
}
