export class PaletteUI {
  constructor(orbitalGraph, scene3D) {
    this.orbitalGraph = orbitalGraph;
    this.scene3D = scene3D;

    this.initAccordions();
    this.initDragAndDrop();
    this.initInspector();
    this.initPlayMode();
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

    // 1. Dragstart on chips
    document.querySelectorAll('.palette-chip-item').forEach((item) => {
      item.addEventListener('dragstart', (e) => {
        const data = {
          type: item.dataset.type,
          category: item.dataset.category,
          name: item.dataset.name,
          model: item.dataset.model,
          moons: item.dataset.moons
        };
        e.dataTransfer.setData('text/plain', JSON.stringify(data));
        e.dataTransfer.effectAllowed = 'copy';
      });
    });

    // 2. Dragover & Drop on Canvas
    if (orbitalWrapper) {
      orbitalWrapper.addEventListener('dragover', (e) => {
        e.preventDefault();
        e.dataTransfer.dropEffect = 'copy';
        orbitalWrapper.classList.add('drag-over');
      });

      orbitalWrapper.addEventListener('dragleave', () => {
        orbitalWrapper.classList.remove('drag-over');
      });

      orbitalWrapper.addEventListener('drop', (e) => {
        e.preventDefault();
        orbitalWrapper.classList.remove('drag-over');
        try {
          const raw = e.dataTransfer.getData('text/plain');
          if (!raw) return;
          const data = JSON.parse(raw);
          const mousePos = this.orbitalGraph.getCanvasPos(e);
          this.orbitalGraph.addEntityFromPalette(data, mousePos);
          this.updateStatsCounters();
        } catch (err) {
          console.error(err);
        }
      });
    }
  }

  initInspector() {
    this.orbitalGraph.onSelectionChange = (entity) => {
      this.renderInspector(entity);
    };
  }

  renderInspector(entity) {
    const titleEl = document.getElementById('inspector-node-title');
    const typeEl = document.getElementById('inspector-node-type');
    const bodyEl = document.getElementById('inspector-body');
    if (!titleEl || !bodyEl) return;

    if (!entity) {
      titleEl.textContent = 'Propriedades';
      typeEl.textContent = 'Nenhum';
      bodyEl.innerHTML = `<div class="inspector-empty-hint">Clique em qualquer Sol, Planeta ou Lua para editar seus valores em tempo real.</div>`;
      return;
    }

    titleEl.textContent = entity.name;
    typeEl.textContent = entity.type ? entity.type.toUpperCase() : 'NÓ';

    let html = '';
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
    } else if (entity.type === 'planet') {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Função / Mecânica:</span>
          <span style="color:#a288ff;font-weight:700;">${entity.name}</span>
        </div>
        <div class="inspector-row">
          <span class="inspector-label">Velocidade Orbital:</span>
          <input type="number" step="0.1" class="inspector-input" value="${entity.speed}" id="inp-planet-speed" />
        </div>
        <div class="inspector-row">
          <span class="inspector-label">Raio da Órbita:</span>
          <input type="number" class="inspector-input" value="${entity.orbitRadius}" id="inp-planet-orbit" />
        </div>
        <div class="inspector-row">
          <span class="inspector-label">Luas Acopladas:</span>
          <span style="color:#ffffff;font-weight:700;">${entity.moons ? entity.moons.length : 0}</span>
        </div>
      `;
    } else if (entity.type === 'moon') {
      html += `
        <div class="inspector-row">
          <span class="inspector-label">Parâmetro / Lua:</span>
          <span style="color:${entity.color};font-weight:700;">${entity.name}</span>
        </div>
        <div class="inspector-row">
          <span class="inspector-label">Valor Ativo:</span>
          <input type="text" class="inspector-input" value="${entity.val}" id="inp-moon-val" />
        </div>
      `;
    }

    bodyEl.innerHTML = html;

    // Attach real-time edit listeners
    const inpSunName = document.getElementById('inp-sun-name');
    if (inpSunName) {
      inpSunName.addEventListener('input', (e) => {
        entity.name = e.target.value;
      });
    }

    const inpPlanetSpeed = document.getElementById('inp-planet-speed');
    if (inpPlanetSpeed) {
      inpPlanetSpeed.addEventListener('input', (e) => {
        entity.speed = parseFloat(e.target.value) || 0.1;
      });
    }

    const inpPlanetOrbit = document.getElementById('inp-planet-orbit');
    if (inpPlanetOrbit) {
      inpPlanetOrbit.addEventListener('input', (e) => {
        entity.orbitRadius = parseFloat(e.target.value) || 60;
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
        playLabel.textContent = 'Pausar';
        if (chip3D) {
          chip3D.textContent = 'Executando (W/A/S/D)';
          chip3D.style.borderColor = '#34c759';
          chip3D.style.color = '#34c759';
        }
      } else {
        playBtn.classList.remove('is-playing');
        playIcon.className = 'ti ti-player-play';
        playLabel.textContent = 'Executar';
        if (chip3D) {
          chip3D.textContent = 'Pronto (3D)';
          chip3D.style.borderColor = '#32ade6';
          chip3D.style.color = '#32ade6';
        }
      }
    };

    if (playBtn) playBtn.addEventListener('click', toggle);

    // F5 toggle
    window.addEventListener('keydown', (e) => {
      if (e.key === 'F5') {
        e.preventDefault();
        toggle();
      }
    });

    // Handle collision trigger from 3D scene to orbital graph
    this.scene3D.onCollisionEvent = (player, obstacle) => {
      this.orbitalGraph.triggerEnergyBeam('PLAYER', 'Objeto', '#ff3b30');
    };
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
