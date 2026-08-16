import { HistoryManager } from '../core/HistoryManager.js';

export class OrbitalGraph {
  constructor(canvas, suns = null, onSelectionChange = null, historyManager = null) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.onSelectionChange = onSelectionChange;
    this.historyManager = historyManager;
    this.onGraphChange = null;

    // Viewport transform
    this.panX = 0;
    this.panY = 0;
    this.zoom = 1.0;
    this.isPanning = false;
    this.lastMouse = { x: 0, y: 0 };
    this.draggedEntity = null;

    // Current Active Tool: 'select' | 'orbit' | 'beam' | 'inspect'
    this.currentTool = 'select';

    // Interactive Tool States
    this.beamDragStartSun = null;
    this.beamDragCurrentPos = null;
    this.permanentBeams = []; // Array of { fromId, toId, color }
    this.orbitCreationPreview = null; // { sun, radius }

    // Orbit Animation Toggle
    this.isOrbitAnimationActive = true;
    this.animTime = 0;

    // Interactive selection
    this.selectedEntity = null;
    this.hoveredEntity = null;

    // Active Gravitational Energy Beams
    this.activeBeams = [];

    // Initial Systems - Clean & Ready for Modular Assembly
    this.suns = suns || [
      {
        id: 'sun-player',
        name: 'PLAYER',
        type: 'sun',
        x: 0,
        y: -110,
        radius: 36,
        color: '#ff7700',
        orbits: [
          { radius: 65, dash: [4, 4] },
          { radius: 115, dash: [3, 4] },
          { radius: 165, dash: [3, 5] }
        ],
        planets: [
          {
            id: 'planet-cubo',
            name: 'Cubo',
            type: 'planet',
            orbitRadius: 65,
            angle: -0.6,
            speed: 0.3,
            radius: 18,
            color: '#382f7e',
            textColor: '#ffffff',
            moons: []
          }
        ]
      },
      {
        id: 'sun-objeto',
        name: 'Objeto',
        type: 'sun',
        x: 0,
        y: 120,
        radius: 36,
        color: '#5856d6',
        orbits: [
          { radius: 65, dash: [4, 4] },
          { radius: 115, dash: [3, 4] }
        ],
        planets: [
          {
            id: 'planet-plano',
            name: 'Plano',
            type: 'planet',
            orbitRadius: 65,
            angle: -0.8,
            speed: 0.25,
            radius: 18,
            color: '#382f7e',
            textColor: '#ffffff',
            moons: []
          }
        ]
      }
    ];

    // Auto-clean any duplicate planet names on the same sun
    this.suns.forEach(sun => {
      const seen = new Set();
      sun.planets = sun.planets.filter(p => {
        const key = p.name.toLowerCase();
        if (seen.has(key)) return false;
        seen.add(key);
        return true;
      });
    });

    this.initCanvas();
    this.initEvents();
    this.animate();
  }

  initCanvas() {
    this.resize();
    window.addEventListener('resize', () => this.resize());
  }

  resize() {
    if (!this.canvas.parentElement) return;
    const parent = this.canvas.parentElement;
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    const width = parent.clientWidth;
    const height = parent.clientHeight;
    if (width === 0 || height === 0) return;

    this.canvas.width = width * dpr;
    this.canvas.height = height * dpr;
    this.ctx.scale(dpr, dpr);

    if (this.panX === 0 && this.panY === 0) {
      this.panX = width / 2;
      this.panY = height / 2;
    }
  }

  initEvents() {
    let isMouseDown = false;
    let downPos = { x: 0, y: 0 };
    let downMousePos = { x: 0, y: 0 };
    let startSunPos = { x: 0, y: 0 };
    let activeDragTarget = null;
    let isDragging = false;

    // Mouse Down
    this.canvas.addEventListener('mousedown', (e) => {
      if (e.button !== 0 && e.button !== 1) return;

      isMouseDown = true;
      isDragging = false;
      downPos = { x: e.clientX, y: e.clientY };
      this.lastMouse = { x: e.clientX, y: e.clientY };
      downMousePos = this.getCanvasPos(e);

      const hit = this.hitTest(downMousePos.x, downMousePos.y);

      // TOOL: CREATE ORBIT (O)
      if (this.currentTool === 'orbit') {
        const closestSun = this.findClosestSun(downMousePos.x, downMousePos.y);
        if (closestSun) {
          const dist = Math.hypot(downMousePos.x - closestSun.x, downMousePos.y - closestSun.y);
          this.orbitCreationPreview = {
            sun: closestSun,
            radius: Math.max(50, Math.round(dist))
          };
        }
        return;
      }

      // TOOL: CONNECT GRAVITATIONAL BEAM (L)
      if (this.currentTool === 'beam') {
        const hitSun = hit && hit.type === 'sun' ? hit : this.findClosestSun(downMousePos.x, downMousePos.y);
        if (hitSun) {
          this.beamDragStartSun = hitSun;
          this.beamDragCurrentPos = { ...downMousePos };
        }
        return;
      }

      // TOOL: SELECT & MOVE (DEFAULT)
      if (hit) {
        activeDragTarget = hit;
        if (hit.type === 'sun') {
          startSunPos = { x: hit.x, y: hit.y };
        }
      } else {
        activeDragTarget = null;
        this.isPanning = true;
        this.canvas.parentElement?.classList.add('is-panning');
      }
    });

    // Mouse Move
    window.addEventListener('mousemove', (e) => {
      const mouse = this.getCanvasPos(e);

      if (!isMouseDown) {
        this.hoveredEntity = this.hitTest(mouse.x, mouse.y);
        this.lastMouse = { x: e.clientX, y: e.clientY };
        return;
      }

      const moveDist = Math.hypot(e.clientX - downPos.x, e.clientY - downPos.y);

      // 1. TOOL: ORBIT CREATION DRAG
      if (this.currentTool === 'orbit' && this.orbitCreationPreview) {
        const sun = this.orbitCreationPreview.sun;
        const dist = Math.hypot(mouse.x - sun.x, mouse.y - sun.y);
        this.orbitCreationPreview.radius = Math.max(40, Math.round(dist));
        this.lastMouse = { x: e.clientX, y: e.clientY };
        return;
      }

      // 2. TOOL: BEAM CONNECT DRAG
      if (this.currentTool === 'beam' && this.beamDragStartSun) {
        this.beamDragCurrentPos = { ...mouse };
        this.lastMouse = { x: e.clientX, y: e.clientY };
        return;
      }

      // 3. TOOL: SELECT & MOVE DRAG
      if (activeDragTarget) {
        if (moveDist > 4) {
          isDragging = true;
          this.draggedEntity = activeDragTarget;

          if (activeDragTarget.type === 'sun') {
            activeDragTarget.x = startSunPos.x + (mouse.x - downMousePos.x);
            activeDragTarget.y = startSunPos.y + (mouse.y - downMousePos.y);
          } else if (activeDragTarget.type === 'planet') {
            const planet = activeDragTarget;
            const sun = planet.parentSun || this.getPlanetParentSun(planet);
            if (sun && sun.orbits && sun.orbits.length > 0) {
              const dx = mouse.x - sun.x;
              const dy = mouse.y - sun.y;
              planet.angle = Math.atan2(dy, dx);
              const dist = Math.hypot(dx, dy);

              let closestOrbit = sun.orbits[0].radius;
              let minDiff = Infinity;
              sun.orbits.forEach((o) => {
                if (o && o.radius) {
                  const diff = Math.abs(o.radius - dist);
                  if (diff < minDiff) {
                    minDiff = diff;
                    closestOrbit = o.radius;
                  }
                }
              });
              planet.orbitRadius = closestOrbit;
            }
          }
        }
      } else if (this.isPanning) {
        this.panX += e.clientX - this.lastMouse.x;
        this.panY += e.clientY - this.lastMouse.y;
      }

      this.lastMouse = { x: e.clientX, y: e.clientY };
    });

    // Mouse Up / Release
    window.addEventListener('mouseup', (e) => {
      if (!isMouseDown) return;
      const mouse = this.getCanvasPos(e);

      // 1. FINALIZE ORBIT CREATION
      if (this.currentTool === 'orbit' && this.orbitCreationPreview) {
        const sun = this.orbitCreationPreview.sun;
        const radius = this.orbitCreationPreview.radius;
        if (!sun.orbits.some(o => Math.abs(o.radius - radius) < 12)) {
          if (this.historyManager) this.historyManager.pushState(this.suns);
          sun.orbits.push({ radius, dash: [3, 4] });
          this.notifyGraphChange();
        }
        this.orbitCreationPreview = null;
      }

      // 2. FINALIZE BEAM CONNECTION
      if (this.currentTool === 'beam' && this.beamDragStartSun) {
        const targetHit = this.hitTest(mouse.x, mouse.y);
        const targetSun = targetHit && targetHit.type === 'sun' ? targetHit : this.findClosestSun(mouse.x, mouse.y);

        if (targetSun && targetSun.id !== this.beamDragStartSun.id) {
          if (this.historyManager) this.historyManager.pushState(this.suns);
          this.permanentBeams.push({
            fromId: this.beamDragStartSun.id,
            toId: targetSun.id,
            color: '#32ade6'
          });
          this.triggerEnergyBeam(this.beamDragStartSun.name, targetSun.name, '#32ade6');
          this.notifyGraphChange();
        }

        this.beamDragStartSun = null;
        this.beamDragCurrentPos = null;
      }

      // 3. FINALIZE SELECTION & MOVE
      if (!isDragging && activeDragTarget) {
        this.selectedEntity = activeDragTarget;
        if (this.onSelectionChange) {
          this.onSelectionChange(activeDragTarget);
        }
      } else if (!isDragging && !activeDragTarget && this.currentTool === 'select') {
        this.selectedEntity = null;
        if (this.onSelectionChange) {
          this.onSelectionChange(null);
        }
      }

      if (isDragging) {
        if (this.historyManager) {
          this.historyManager.pushState(this.suns);
        }
        this.notifyGraphChange();
      }

      isMouseDown = false;
      isDragging = false;
      activeDragTarget = null;
      this.draggedEntity = null;
      this.isPanning = false;
      this.canvas.parentElement?.classList.remove('is-panning');
    });

    window.addEventListener('blur', () => {
      isMouseDown = false;
      isDragging = false;
      activeDragTarget = null;
      this.draggedEntity = null;
      this.isPanning = false;
      this.orbitCreationPreview = null;
      this.beamDragStartSun = null;
      this.canvas.parentElement?.classList.remove('is-panning');
    });

    // Zoom on wheel
    this.canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      const zoomFactor = e.deltaY < 0 ? 1.08 : 0.92;
      this.zoom = Math.max(0.4, Math.min(2.5, this.zoom * zoomFactor));
    }, { passive: false });

    // Keyboard Shortcuts (Delete, Backspace)
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT') return;
      if (e.key === 'Delete' || e.key === 'Backspace' || e.key === 'x' || e.key === 'X') {
        if (this.selectedEntity) {
          this.deleteEntity(this.selectedEntity);
        }
      }
    });
  }

  findClosestSun(x, y) {
    if (this.suns.length === 0) return null;
    let closest = this.suns[0];
    let minDist = Infinity;
    this.suns.forEach(sun => {
      const d = Math.hypot(sun.x - x, sun.y - y);
      if (d < minDist) {
        minDist = d;
        closest = sun;
      }
    });
    return closest;
  }

  notifyGraphChange() {
    if (this.onGraphChange) {
      this.onGraphChange(this.suns);
    }
  }

  deleteEntity(entity) {
    if (!entity) return;

    if (this.historyManager) {
      this.historyManager.pushState(this.suns);
    }

    if (entity.type === 'sun') {
      this.suns = this.suns.filter(s => s.id !== entity.id);
      this.permanentBeams = this.permanentBeams.filter(b => b.fromId !== entity.id && b.toId !== entity.id);
    } else if (entity.type === 'planet') {
      this.suns.forEach(sun => {
        sun.planets = sun.planets.filter(p => p.id !== entity.id && p.name !== entity.name);
      });
    } else if (entity.type === 'moon') {
      this.suns.forEach(sun => {
        sun.planets.forEach(planet => {
          if (planet.moons) {
            planet.moons = planet.moons.filter(m => m.name !== entity.name);
          }
        });
      });
    }

    this.selectedEntity = null;
    if (this.onSelectionChange) this.onSelectionChange(null);
    this.notifyGraphChange();
  }

  getCanvasPos(e) {
    const rect = this.canvas.getBoundingClientRect();
    const clientX = e.clientX - rect.left;
    const clientY = e.clientY - rect.top;
    return {
      x: (clientX - this.panX) / this.zoom,
      y: (clientY - this.panY) / this.zoom
    };
  }

  getPlanetParentSun(planet) {
    if (!planet) return null;
    const targetSunId = planet.sunId || planet.parentSunId;
    if (targetSunId) {
      const found = this.suns.find(s => s.id === targetSunId);
      if (found) return found;
    }
    for (const sun of this.suns) {
      if (sun.planets && sun.planets.some(p => p.id === planet.id || p.name === planet.name)) return sun;
    }
    return this.suns[0] || null;
  }

  hitTest(x, y) {
    // 1. Check Moons first
    for (const sun of this.suns) {
      for (const planet of sun.planets) {
        if (planet.moons && planet.moons.length > 0) {
          const pX = sun.x + Math.cos(planet.angle) * planet.orbitRadius;
          const pY = sun.y + Math.sin(planet.angle) * planet.orbitRadius;
          const subR = planet.subOrbitRadius || 32;

          for (const moon of planet.moons) {
            const mX = pX + Math.cos(moon.angle) * subR;
            const mY = pY + Math.sin(moon.angle) * subR;
            if (Math.hypot(x - mX, y - mY) <= 12) {
              return { ...moon, type: 'moon', parentPlanetId: planet.id, parentSunId: sun.id };
            }
          }
        }
      }
    }

    // 2. Check Planets
    for (const sun of this.suns) {
      for (const planet of sun.planets) {
        const pX = sun.x + Math.cos(planet.angle) * planet.orbitRadius;
        const pY = sun.y + Math.sin(planet.angle) * planet.orbitRadius;
        if (Math.hypot(x - pX, y - pY) <= planet.radius + 4) {
          return { ...planet, parentSunId: sun.id };
        }
      }
    }

    // 3. Check Suns
    for (const sun of this.suns) {
      if (Math.hypot(x - sun.x, y - sun.y) <= sun.radius + 6) {
        return sun;
      }
    }

    return null;
  }

  triggerEnergyBeam(fromSunName, toSunName, color = '#ff7700') {
    const fromSun = this.suns.find(s => s.name.toUpperCase().includes(fromSunName.toUpperCase()));
    const toSun = this.suns.find(s => s.name.toUpperCase().includes(toSunName.toUpperCase()));

    if (fromSun && toSun) {
      this.activeBeams.push({
        x1: fromSun.x,
        y1: fromSun.y,
        x2: toSun.x,
        y2: toSun.y,
        color,
        life: 1.0
      });
    }
  }

  addEntityFromPalette(data, mousePos) {
    if (!mousePos) mousePos = { x: 0, y: 0 };

    if (this.historyManager) {
      this.historyManager.pushState(this.suns);
    }

    // 1. ADDING AN ENTITY SYSTEM (SUN)
    if (data.type === 'sun') {
      const upperName = data.name.toUpperCase();

      // DEDUPLICATION: Check if this Entity Sun already exists in the scene
      const existingSun = this.suns.find(s => s.name.toUpperCase() === upperName);
      if (existingSun) {
        this.selectedEntity = existingSun;
        this.triggerEnergyBeam(existingSun.name, existingSun.name, '#32ade6');
        if (this.onSelectionChange) this.onSelectionChange(this.selectedEntity);
        this.notifyGraphChange();
        return;
      }

      let spawnX = mousePos.x;
      let spawnY = mousePos.y;

      if (spawnX === 0 && spawnY === 0) {
        const count = this.suns.length;
        spawnX = (count % 2 === 0 ? 1 : -1) * (150 + Math.floor(count / 2) * 130);
        spawnY = (count % 3 === 0 ? 1 : -1) * 90;
      }

      let sunColor = '#ff7700';
      let defaultModel = 'Cubo';

      if (upperName.includes('PLAYER')) {
        sunColor = '#ff7700';
        defaultModel = 'Cubo';
      } else if (upperName.includes('INIMIGO')) {
        sunColor = '#ff3b30';
        defaultModel = 'Cilindro';
      } else if (upperName.includes('NPC')) {
        sunColor = '#34c759';
        defaultModel = 'Esfera';
      } else if (upperName.includes('BLOCO') || upperName.includes('COLISAO') || upperName.includes('COLISÃO')) {
        sunColor = '#5856d6';
        defaultModel = 'Cubo';
      } else if (upperName.includes('GATILHO') || upperName.includes('TRIGGER')) {
        sunColor = '#ff9500';
        defaultModel = 'Cubo';
      } else if (upperName.includes('PLATAFORMA')) {
        sunColor = '#32ade6';
        defaultModel = 'Plano';
      }

      // BAREBONES MODULAR SETUP: Only the shape planet on orbit 65. All other orbits empty!
      const newSun = {
        id: `sun-${Date.now()}`,
        name: data.name,
        type: 'sun',
        x: spawnX,
        y: spawnY,
        radius: 36,
        color: sunColor,
        orbits: [
          { radius: 65, dash: [4, 4] },
          { radius: 115, dash: [3, 4] },
          { radius: 165, dash: [3, 5] }
        ],
        planets: [
          {
            id: `planet-${Date.now()}-mesh`,
            name: defaultModel,
            type: 'planet',
            orbitRadius: 65,
            angle: -0.6,
            speed: 0.3,
            radius: 18,
            color: '#382f7e',
            textColor: '#ffffff',
            moons: [],
            sunId: null
          }
        ]
      };

      newSun.planets[0].sunId = newSun.id;
      this.suns.push(newSun);
      this.selectedEntity = newSun;
      this.triggerEnergyBeam(newSun.name, this.suns[0]?.name || newSun.name, '#32ade6');
    }

    // 2. ADDING A MESH OR MECHANIC (PLANET)
    else if (data.type === 'planet' || data.type === 'mesh') {
      let targetSun = null;

      if (this.selectedEntity) {
        if (this.selectedEntity.type === 'sun') targetSun = this.selectedEntity;
        else if (this.selectedEntity.parentSun) targetSun = this.selectedEntity.parentSun;
      }

      if (!targetSun && this.suns.length > 0) {
        let minDist = Infinity;
        this.suns.forEach(sun => {
          const d = Math.hypot(sun.x - mousePos.x, sun.y - mousePos.y);
          if (d < minDist) {
            minDist = d;
            targetSun = sun;
          }
        });
      }

      if (!targetSun) {
        targetSun = {
          id: `sun-${Date.now()}`,
          name: 'Objeto',
          type: 'sun',
          x: mousePos.x || 0,
          y: mousePos.y || 0,
          radius: 36,
          color: '#ff7700',
          orbits: [
            { radius: 65, dash: [4, 4] },
            { radius: 115, dash: [3, 4] }
          ],
          planets: []
        };
        this.suns.push(targetSun);
      }

      if (data.type === 'mesh') {
        const meshShapes = ['cubo', 'plano', 'cilindro', 'triangulo', 'esfera', 'cube', 'plane', 'cylinder', 'cone', 'sphere'];
        const existingMeshPlanet = targetSun.planets.find(p => meshShapes.some(s => p.name.toLowerCase().includes(s)));

        if (existingMeshPlanet) {
          existingMeshPlanet.name = data.name;
          this.selectedEntity = existingMeshPlanet;
        } else {
          const newPlanet = {
            id: `planet-${Date.now()}`,
            name: data.name,
            type: 'planet',
            orbitRadius: 65,
            angle: -0.6,
            speed: 0.3,
            radius: 18,
            color: '#382f7e',
            textColor: '#ffffff',
            moons: [],
            sunId: targetSun.id
          };
          targetSun.planets.push(newPlanet);
          this.selectedEntity = newPlanet;
        }
      } else {
        // Adding a Mechanic Planet (Andar, Pular, Correr, Fisica, Colisao, Animacao)
        let orbitR = data.name.toLowerCase().includes('animacao') ? 165 : 115;
        if (!targetSun.orbits.some(o => Math.abs(o.radius - orbitR) < 15)) {
          targetSun.orbits.push({ radius: orbitR, dash: [3, 4] });
        }

        let moons = [];
        if (data.moons) {
          try {
            const rawMoons = typeof data.moons === 'string' ? JSON.parse(data.moons) : data.moons;
            if (Array.isArray(rawMoons)) {
              const step = (Math.PI * 2) / rawMoons.length;
              moons = rawMoons.map((m, i) => ({
                name: m.name,
                color: m.color || '#ffffff',
                angle: i * step,
                speed: 1.0,
                val: m.val
              }));
            }
          } catch(e) {}
        }

        // DEDUPLICATION: Check if this mechanic already exists on targetSun
        const existingPlanet = targetSun.planets.find(p => p.name.toLowerCase() === data.name.toLowerCase());

        if (existingPlanet) {
          // Update existing planet's moons & select it
          if (moons.length > 0) {
            existingPlanet.moons = moons;
          }
          this.selectedEntity = existingPlanet;
          this.triggerEnergyBeam(targetSun.name, targetSun.name, '#32ade6');
        } else {
          // Distribute new planet angle evenly
          const angle = (targetSun.planets.length * 1.35) % (Math.PI * 2);
          const newPlanet = {
            id: `planet-${Date.now()}-${data.name}`,
            name: data.name,
            type: 'planet',
            orbitRadius: orbitR,
            angle,
            speed: 0.3,
            radius: 20,
            color: '#2b2368',
            textColor: '#ffffff',
            subOrbitRadius: moons.length > 0 ? 32 : 0,
            moons,
            sunId: targetSun.id
          };

          targetSun.planets.push(newPlanet);
          this.selectedEntity = newPlanet;
        }
      }
    }

    if (this.onSelectionChange) this.onSelectionChange(this.selectedEntity);
    this.notifyGraphChange();
  }

  update(delta) {
    this.animTime += delta;

    if (this.isOrbitAnimationActive) {
      this.suns.forEach(sun => {
        sun.planets.forEach(planet => {
          if (this.draggedEntity && this.draggedEntity.id === planet.id) return;
          planet.angle += planet.speed * delta;
          if (planet.moons) {
            planet.moons.forEach(moon => {
              moon.angle += moon.speed * delta;
            });
          }
        });
      });
    }

    this.activeBeams.forEach(b => b.life -= delta * 1.5);
    this.activeBeams = this.activeBeams.filter(b => b.life > 0);
  }

  draw() {
    if (!this.canvas.parentElement) return;
    const w = this.canvas.parentElement.clientWidth;
    const h = this.canvas.parentElement.clientHeight;
    if (w === 0 || h === 0) return;

    this.ctx.clearRect(0, 0, w, h);

    this.ctx.save();
    this.ctx.translate(this.panX, this.panY);
    this.ctx.scale(this.zoom, this.zoom);

    // 1. Draw Permanent Gravitational Beams (Connected Systems)
    this.permanentBeams.forEach(beam => {
      const sunA = this.suns.find(s => s.id === beam.fromId);
      const sunB = this.suns.find(s => s.id === beam.toId);
      if (sunA && sunB) {
        this.ctx.save();
        this.ctx.beginPath();
        this.ctx.moveTo(sunA.x, sunA.y);
        this.ctx.lineTo(sunB.x, sunB.y);
        this.ctx.strokeStyle = 'rgba(50, 173, 230, 0.4)';
        this.ctx.setLineDash([4, 6]);
        this.ctx.lineWidth = 1.5;
        this.ctx.stroke();

        // Traveling energy photon
        const progress = (this.animTime * 0.8) % 1.0;
        const px = sunA.x + (sunB.x - sunA.x) * progress;
        const py = sunA.y + (sunB.y - sunA.y) * progress;
        this.ctx.beginPath();
        this.ctx.arc(px, py, 4, 0, Math.PI * 2);
        this.ctx.fillStyle = '#32ade6';
        this.ctx.shadowColor = '#32ade6';
        this.ctx.shadowBlur = 10;
        this.ctx.fill();
        this.ctx.restore();
      }
    });

    // 2. Draw Active Transient Energy Beams
    this.activeBeams.forEach(beam => {
      this.ctx.save();
      this.ctx.beginPath();
      this.ctx.moveTo(beam.x1, beam.y1);
      this.ctx.lineTo(beam.x2, beam.y2);
      this.ctx.strokeStyle = beam.color;
      this.ctx.lineWidth = 3 * beam.life;
      this.ctx.shadowColor = beam.color;
      this.ctx.shadowBlur = 15;
      this.ctx.stroke();
      this.ctx.restore();
    });

    // 3. Draw Solar Systems (Suns, Orbits, Planets, Moons)
    this.suns.forEach(sun => {
      // 3.1 Concentric Dashed Orbits
      sun.orbits.forEach(orbit => {
        this.ctx.beginPath();
        this.ctx.arc(sun.x, sun.y, orbit.radius, 0, Math.PI * 2);
        this.ctx.setLineDash(orbit.dash || [3, 4]);
        this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.28)';
        this.ctx.lineWidth = 1.0;
        this.ctx.stroke();
        this.ctx.setLineDash([]);
      });

      // 3.2 Orbit Creation Preview
      if (this.orbitCreationPreview && this.orbitCreationPreview.sun === sun) {
        this.ctx.save();
        this.ctx.beginPath();
        this.ctx.arc(sun.x, sun.y, this.orbitCreationPreview.radius, 0, Math.PI * 2);
        this.ctx.setLineDash([4, 4]);
        this.ctx.strokeStyle = '#32ade6';
        this.ctx.lineWidth = 2;
        this.ctx.shadowColor = '#32ade6';
        this.ctx.shadowBlur = 12;
        this.ctx.stroke();
        this.ctx.restore();
      }

      // 3.3 Orbiting Planets & Moons
      sun.planets.forEach(planet => {
        const pX = sun.x + Math.cos(planet.angle) * planet.orbitRadius;
        const pY = sun.y + Math.sin(planet.angle) * planet.orbitRadius;

        if (planet.moons && planet.moons.length > 0) {
          const subR = planet.subOrbitRadius || 34;
          this.ctx.save();
          this.ctx.beginPath();
          this.ctx.arc(pX, pY, subR, 0, Math.PI * 2);
          this.ctx.setLineDash([2, 3]);
          this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.2)';
          this.ctx.lineWidth = 0.8;
          this.ctx.stroke();
          this.ctx.restore();

          planet.moons.forEach(moon => {
            const mX = pX + Math.cos(moon.angle) * subR;
            const mY = pY + Math.sin(moon.angle) * subR;

            this.ctx.beginPath();
            this.ctx.arc(mX, mY, 9, 0, Math.PI * 2);
            this.ctx.fillStyle = moon.color || '#ffffff';
            this.ctx.shadowColor = moon.color || '#ffffff';
            this.ctx.shadowBlur = 6;
            this.ctx.fill();

            this.ctx.fillStyle = '#000000';
            this.ctx.font = 'bold 10px sans-serif';
            this.ctx.textAlign = 'center';
            this.ctx.textBaseline = 'middle';
            this.ctx.fillText('-', mX, mY);
          });
        }

        // Planet Body
        this.ctx.beginPath();
        this.ctx.arc(pX, pY, planet.radius, 0, Math.PI * 2);
        this.ctx.fillStyle = planet.color || '#2b2368';
        this.ctx.shadowColor = '#6050dc';
        this.ctx.shadowBlur = 10;
        this.ctx.fill();

        this.ctx.strokeStyle = this.selectedEntity?.id === planet.id ? '#ffffff' : 'rgba(255, 255, 255, 0.35)';
        this.ctx.lineWidth = this.selectedEntity?.id === planet.id ? 2.5 : 1.2;
        this.ctx.stroke();

        this.ctx.fillStyle = planet.textColor || '#ffffff';
        this.ctx.font = '10px sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.textBaseline = 'middle';
        this.ctx.fillText(planet.name, pX, pY);
      });

      // 3.4 Central Sun Body
      this.ctx.beginPath();
      this.ctx.arc(sun.x, sun.y, sun.radius, 0, Math.PI * 2);
      this.ctx.fillStyle = sun.color;
      this.ctx.shadowColor = sun.color;
      this.ctx.shadowBlur = 18;
      this.ctx.fill();

      this.ctx.strokeStyle = this.selectedEntity?.id === sun.id ? '#ffffff' : '#ffaa44';
      this.ctx.lineWidth = this.selectedEntity?.id === sun.id ? 3 : 1.5;
      this.ctx.stroke();

      const labelText = sun.name;
      this.ctx.font = 'bold 13px sans-serif';
      const textWidth = this.ctx.measureText(labelText).width;

      this.ctx.fillStyle = 'rgba(0, 0, 0, 0.85)';
      this.ctx.fillRect(sun.x - textWidth / 2 - 8, sun.y - 10, textWidth + 16, 20);

      this.ctx.fillStyle = '#ffffff';
      this.ctx.textAlign = 'center';
      this.ctx.textBaseline = 'middle';
      this.ctx.fillText(labelText, sun.x, sun.y);
    });

    // 4. Draw Live Beam Connection Drag
    if (this.beamDragStartSun && this.beamDragCurrentPos) {
      this.ctx.save();
      this.ctx.beginPath();
      this.ctx.moveTo(this.beamDragStartSun.x, this.beamDragStartSun.y);
      this.ctx.lineTo(this.beamDragCurrentPos.x, this.beamDragCurrentPos.y);
      this.ctx.strokeStyle = '#32ade6';
      this.ctx.lineWidth = 2.5;
      this.ctx.shadowColor = '#32ade6';
      this.ctx.shadowBlur = 15;
      this.ctx.stroke();
      this.ctx.restore();
    }

    this.ctx.restore();
  }

  animate() {
    requestAnimationFrame(() => this.animate());
    const delta = 0.016;
    this.update(delta);
    this.draw();
  }
}
