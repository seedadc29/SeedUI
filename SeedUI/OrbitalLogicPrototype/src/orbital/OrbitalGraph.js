export class OrbitalGraph {
  constructor(canvas, onSelectionChange, onGraphChange, historyManager) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.onSelectionChange = onSelectionChange;
    this.onGraphChange = onGraphChange;
    this.historyManager = historyManager;

    // Viewport transform
    this.panX = 0;
    this.panY = 0;
    this.zoom = 1.0;
    this.isPanning = false;
    this.lastMouse = { x: 0, y: 0 };

    // Active state
    this.isOrbitAnimationActive = true;
    this.draggedEntity = null;
    this.selectedEntity = null;
    this.hoveredEntity = null;

    // Active Gravitational Energy Beams
    this.activeBeams = [];

    // Initial Systems with Real Functional Capabilities
    this.suns = [
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
          },
          {
            id: 'planet-andar',
            name: 'Andar',
            type: 'planet',
            orbitRadius: 65,
            angle: 1.8,
            speed: 0.3,
            radius: 20,
            color: '#2b2368',
            textColor: '#ffffff',
            subOrbitRadius: 28,
            moons: [
              { name: 'Velocidade', color: '#ffffff', angle: 0, speed: 1.0, val: 6.0 }
            ]
          },
          {
            id: 'planet-pular',
            name: 'Pular',
            type: 'planet',
            orbitRadius: 65,
            angle: 3.6,
            speed: 0.3,
            radius: 20,
            color: '#2b2368',
            textColor: '#ffffff',
            subOrbitRadius: 28,
            moons: [
              { name: 'Força do Pulo', color: '#ff3b30', angle: 0, speed: 1.0, val: 7.5 }
            ]
          },
          {
            id: 'planet-fisica',
            name: 'Fisica',
            type: 'planet',
            orbitRadius: 115,
            angle: 0.8,
            speed: 0.25,
            radius: 20,
            color: '#2b2368',
            textColor: '#ffffff',
            subOrbitRadius: 28,
            moons: [
              { name: 'Massa', color: '#ff9500', angle: 0, speed: 1.0, val: 1.0 }
            ]
          },
          {
            id: 'planet-colisao-p',
            name: 'Colisao',
            type: 'planet',
            orbitRadius: 115,
            angle: 2.2,
            speed: 0.25,
            radius: 20,
            color: '#2b2368',
            textColor: '#ffffff',
            moons: []
          },
          {
            id: 'planet-animacao',
            name: 'Animacao',
            type: 'planet',
            orbitRadius: 165,
            angle: 3.4,
            speed: 0.2,
            radius: 24,
            color: '#2b2368',
            textColor: '#ffffff',
            subOrbitRadius: 36,
            moons: [
              { name: 'Idle', color: '#ffffff', angle: 0, speed: 1.2, val: 1 },
              { name: 'Walk', color: '#ffcc00', angle: 1.25, speed: 1.2, val: 2 },
              { name: 'Run', color: '#34c759', angle: 2.5, speed: 1.2, val: 3 },
              { name: 'Jump', color: '#ff3b30', angle: 3.75, speed: 1.2, val: 4 },
              { name: 'Die', color: '#2a2a2a', angle: 5.0, speed: 1.2, val: 5 }
            ]
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
        color: '#ff7700',
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
          },
          {
            id: 'planet-colisao',
            name: 'Colisao',
            type: 'planet',
            orbitRadius: 115,
            angle: 3.3,
            speed: 0.35,
            radius: 22,
            color: '#2b2368',
            textColor: '#ffffff',
            subOrbitRadius: 34,
            moons: [
              { name: 'Estatico', color: '#5856d6', angle: 3.14, speed: 0.8, val: true }
            ]
          }
        ]
      }
    ];

    this.initCanvas();
    this.initEvents();
    this.animate();

    // Push initial history snapshot
    if (this.historyManager) {
      this.historyManager.pushState(this.suns);
    }
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
    window.addEventListener('mouseup', () => {
      if (!isMouseDown) return;

      if (!isDragging && activeDragTarget) {
        // Selection Click
        this.selectedEntity = activeDragTarget;
        if (this.onSelectionChange) {
          this.onSelectionChange(activeDragTarget);
        }
      } else if (!isDragging && !activeDragTarget) {
        // Click on empty space
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
    if (planet.parentSun) return planet.parentSun;
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
              return { ...moon, type: 'moon', parentPlanet: planet, parentSun: sun };
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
          planet.parentSun = sun;
          return planet;
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
        fromX: fromSun.x,
        fromY: fromSun.y,
        toX: toSun.x,
        toY: toSun.y,
        life: 1.0,
        color
      });
    }
  }

  addEntityFromPalette(data, mousePos) {
    if (this.historyManager) {
      this.historyManager.pushState(this.suns);
    }

    if (data.type === 'sun') {
      const newSun = {
        id: `sun-${Date.now()}`,
        name: data.name,
        type: 'sun',
        x: mousePos.x,
        y: mousePos.y,
        radius: 36,
        color: '#ff7700',
        orbits: [
          { radius: 65, dash: [4, 4] },
          { radius: 115, dash: [3, 4] }
        ],
        planets: [
          {
            id: `planet-${Date.now()}-mesh`,
            name: data.model ? data.model.charAt(0).toUpperCase() + data.model.slice(1) : 'Cubo',
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
      };
      this.suns.push(newSun);
      this.selectedEntity = newSun;
    } else if (data.type === 'planet' || data.type === 'mesh') {
      let closestSun = this.suns[0];
      let minDist = Infinity;
      this.suns.forEach(sun => {
        const d = Math.hypot(sun.x - mousePos.x, sun.y - mousePos.y);
        if (d < minDist) {
          minDist = d;
          closestSun = sun;
        }
      });

      if (closestSun) {
        const dx = mousePos.x - closestSun.x;
        const dy = mousePos.y - closestSun.y;
        const angle = Math.atan2(dy, dx);
        const dist = Math.hypot(dx, dy);

        let orbitR = 115;
        if (dist > 140) {
          orbitR = dist;
          closestSun.orbits.push({ radius: dist, dash: [3, 4] });
        }

        let moons = [];
        if (data.moons) {
          try {
            const rawMoons = typeof data.moons === 'string' ? JSON.parse(data.moons) : data.moons;
            const step = (Math.PI * 2) / rawMoons.length;
            moons = rawMoons.map((m, i) => ({
              name: m.name,
              color: m.color || '#fff',
              angle: i * step,
              speed: 1.0,
              val: m.val
            }));
          } catch(e) {}
        }

        const newPlanet = {
          id: `planet-${Date.now()}`,
          name: data.name,
          type: 'planet',
          orbitRadius: orbitR,
          angle,
          speed: 0.35,
          radius: data.type === 'mesh' ? 18 : 22,
          color: data.type === 'mesh' ? '#382f7e' : '#2b2368',
          textColor: '#ffffff',
          subOrbitRadius: moons.length > 0 ? 34 : 0,
          moons
        };

        closestSun.planets.push(newPlanet);
        this.selectedEntity = newPlanet;
      }
    }

    if (this.onSelectionChange) this.onSelectionChange(this.selectedEntity);
    this.notifyGraphChange();
  }

  update(delta) {
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

    // 1. Draw Active Energy Beams
    this.activeBeams.forEach(beam => {
      this.ctx.save();
      this.ctx.beginPath();
      this.ctx.moveTo(beam.fromX, beam.fromY);
      this.ctx.lineTo(beam.toX, beam.toY);
      this.ctx.strokeStyle = beam.color;
      this.ctx.lineWidth = 4 * beam.life;
      this.ctx.shadowColor = beam.color;
      this.ctx.shadowBlur = 15;
      this.ctx.stroke();
      this.ctx.restore();
    });

    // 2. Draw Solar Systems
    this.suns.forEach(sun => {
      // 2.1 Draw Concentric Dashed Orbits
      sun.orbits.forEach(orbit => {
        this.ctx.save();
        this.ctx.beginPath();
        this.ctx.arc(sun.x, sun.y, orbit.radius, 0, Math.PI * 2);
        this.ctx.setLineDash(orbit.dash || [3, 4]);
        this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.45)';
        this.ctx.lineWidth = 1.2;
        this.ctx.stroke();
        this.ctx.restore();
      });

      // 2.2 Draw Orbiting Planets & Moons
      sun.planets.forEach(planet => {
        const pX = sun.x + Math.cos(planet.angle) * planet.orbitRadius;
        const pY = sun.y + Math.sin(planet.angle) * planet.orbitRadius;

        if (planet.moons && planet.moons.length > 0) {
          const subR = planet.subOrbitRadius || 34;
          this.ctx.save();
          this.ctx.beginPath();
          this.ctx.arc(pX, pY, subR, 0, Math.PI * 2);
          this.ctx.setLineDash([2, 3]);
          this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.35)';
          this.ctx.lineWidth = 1;
          this.ctx.stroke();
          this.ctx.restore();

          planet.moons.forEach(moon => {
            const mX = pX + Math.cos(moon.angle) * subR;
            const mY = pY + Math.sin(moon.angle) * subR;

            this.ctx.save();
            this.ctx.beginPath();
            this.ctx.arc(mX, mY, 9, 0, Math.PI * 2);
            this.ctx.fillStyle = moon.color;
            this.ctx.shadowColor = moon.color;
            this.ctx.shadowBlur = 6;
            this.ctx.fill();

            this.ctx.fillStyle = '#000000';
            this.ctx.font = 'bold 10px sans-serif';
            this.ctx.textAlign = 'center';
            this.ctx.textBaseline = 'middle';
            this.ctx.fillText('-', mX, mY - 0.5);
            this.ctx.restore();
          });
        }

        // Draw Planet Body
        this.ctx.save();
        this.ctx.beginPath();
        this.ctx.arc(pX, pY, planet.radius, 0, Math.PI * 2);
        this.ctx.fillStyle = planet.color;
        this.ctx.fill();
        this.ctx.strokeStyle = this.selectedEntity?.id === planet.id ? '#ffffff' : '#6859b8';
        this.ctx.lineWidth = this.selectedEntity?.id === planet.id ? 2.5 : 1.2;
        this.ctx.stroke();

        this.ctx.fillStyle = planet.textColor || '#ffffff';
        this.ctx.font = '10px sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.textBaseline = 'middle';
        this.ctx.fillText(planet.name, pX, pY);
        this.ctx.restore();
      });

      // 2.3 Draw Central Sun (Orange Core)
      this.ctx.save();
      this.ctx.beginPath();
      this.ctx.arc(sun.x, sun.y, sun.radius, 0, Math.PI * 2);
      this.ctx.fillStyle = sun.color;
      this.ctx.shadowColor = 'rgba(255, 119, 0, 0.6)';
      this.ctx.shadowBlur = 18;
      this.ctx.fill();
      this.ctx.strokeStyle = this.selectedEntity?.id === sun.id ? '#ffffff' : '#ff9433';
      this.ctx.lineWidth = this.selectedEntity?.id === sun.id ? 3 : 1.5;
      this.ctx.stroke();

      const labelText = sun.name;
      this.ctx.font = 'bold 13px sans-serif';
      const textWidth = this.ctx.measureText(labelText).width;

      this.ctx.fillStyle = '#000000';
      this.ctx.fillRect(sun.x - (textWidth / 2) - 4, sun.y - 8, textWidth + 8, 16);

      this.ctx.fillStyle = '#ffffff';
      this.ctx.textAlign = 'center';
      this.ctx.textBaseline = 'middle';
      this.ctx.fillText(labelText, sun.x, sun.y);
      this.ctx.restore();
    });

    this.ctx.restore();
  }

  animate() {
    requestAnimationFrame(() => this.animate());
    const delta = 0.016;
    this.update(delta);
    this.draw();
  }
}
