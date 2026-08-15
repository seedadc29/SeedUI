/**
 * DiagnosticsManager captures real-time engine telemetry, selection states,
 * transform gizmo attachment, and action logs for instant 1-click clipboard export.
 */
export class DiagnosticsManager {
  constructor(engine, sceneManager, transformManager, meshEditor, uiManager) {
    this.engine = engine;
    this.sceneManager = sceneManager;
    this.transformManager = transformManager;
    this.meshEditor = meshEditor;
    this.uiManager = uiManager;

    this.logs = [];
    this.maxLogs = 30;

    this.initGlobalLogging();
    this.initUI();
  }

  log(category, message, extra = null) {
    const time = new Date().toLocaleTimeString();
    const entry = { time, category, message, extra };
    this.logs.unshift(entry);
    if (this.logs.length > this.maxLogs) {
      this.logs.pop();
    }
    this.updateLogUI();
  }

  initGlobalLogging() {
    this.log('INIT', 'Stove3D Diagnostics inicializado com sucesso.');

    window.addEventListener('keydown', (e) => {
      if (e.target.tagName !== 'INPUT') {
        this.log('KEYBOARD', `Tecla pressionada: "${e.key}" (Shift: ${e.shiftKey}, Ctrl: ${e.ctrlKey}, Alt: ${e.altKey})`);
      }
    });

    this.engine.canvas.addEventListener('click', (e) => {
      const rect = this.engine.canvas.getBoundingClientRect();
      const x = Math.round(e.clientX - rect.left);
      const y = Math.round(e.clientY - rect.top);
      this.log('MOUSE_CLICK', `Clique em (${x}, ${y}) | Submodo: ${this.uiManager.currentSubmode}`);
    });
  }

  getDiagnosticReport() {
    const selectedObj = this.sceneManager.getSelectedObject();
    const qm = selectedObj && selectedObj.userData ? selectedObj.userData.quadMesh : null;
    const tc = this.transformManager.transformControls;

    const report = {
      timestamp: new Date().toISOString(),
      url: window.location.href,
      engine: {
        currentMode: this.uiManager.currentMode,
        currentSubmode: this.uiManager.currentSubmode,
        currentTool: this.uiManager.currentTool,
        totalObjects: this.sceneManager.getAllMeshes().length
      },
      activeObject: selectedObj ? {
        name: selectedObj.name,
        position: selectedObj.position.toArray().map(n => Number(n.toFixed(2))),
        rotation: [selectedObj.rotation.x, selectedObj.rotation.y, selectedObj.rotation.z].map(n => Number(n.toFixed(2))),
        scale: selectedObj.scale.toArray().map(n => Number(n.toFixed(2))),
        hasQuadMesh: !!qm,
        verticesCount: qm ? qm.vertices.length : 0,
        quadsCount: qm ? qm.quads.length : 0,
        edgesCount: qm ? qm.edges.length : 0
      } : null,
      meshEditor: {
        activeMeshName: this.meshEditor.activeMesh ? this.meshEditor.activeMesh.name : null,
        submode: this.meshEditor.submode,
        selectedVertices: Array.from(this.meshEditor.selectedVertices),
        selectedEdges: Array.from(this.meshEditor.selectedEdges),
        selectedFaces: Array.from(this.meshEditor.selectedFaces),
        transformAnchorPos: this.meshEditor.transformAnchor ? this.meshEditor.transformAnchor.position.toArray().map(n => Number(n.toFixed(2))) : null,
        initialOffsetsCount: this.meshEditor.initialVertexOffsets.size
      },
      transformControls: {
        attachedObject: tc && tc.object ? tc.object.name || tc.object.type : 'NENHUM (null)',
        axis: tc ? tc.axis : null,
        mode: tc ? tc.mode : null,
        isTransforming: this.transformManager.isTransforming
      },
      selectionTools: {
        activeTool: this.uiManager.selectionTools.activeTool,
        isSelecting: this.uiManager.selectionTools.isSelecting,
        justFinishedDrag: this.uiManager.selectionTools.justFinishedDragSelection
      },
      recentLogs: this.logs.slice(0, 15)
    };

    return report;
  }

  getFormattedReportString() {
    const data = this.getDiagnosticReport();
    return `=== STOVE 3D DIAGNOSTIC REPORT ===\nData/Hora: ${data.timestamp}\n\n[ESTADO DO SISTEMA]\nModo Geral: ${data.engine.currentMode}\nSubmodo: ${data.engine.currentSubmode}\nFerramenta: ${data.engine.currentTool}\nObjetos na Cena: ${data.engine.totalObjects}\n\n[OBJETO ATIVO]\n${JSON.stringify(data.activeObject, null, 2)}\n\n[MODO EDIÇÃO (SUB-ELEMENTOS)]\nObjeto em Edição: ${data.meshEditor.activeMeshName}\nSubmodo: ${data.meshEditor.submode}\nVértices Selecionados (${data.meshEditor.selectedVertices.length}): [${data.meshEditor.selectedVertices.join(', ')}]\nFaces Selecionadas (${data.meshEditor.selectedFaces.length}): [${data.meshEditor.selectedFaces.join(', ')}]\nArestas Selecionadas (${data.meshEditor.selectedEdges.length}): [${data.meshEditor.selectedEdges.join(', ')}]\nPosição do Ponto Âncora: ${JSON.stringify(data.meshEditor.transformAnchorPos)}\n\n[GIZMO DE TRANSFORMAÇÃO]\nObjeto Anexado ao Gizmo: ${data.transformControls.attachedObject}\nEixo em Foco: ${data.transformControls.axis}\nModo do Gizmo: ${data.transformControls.mode}\nEm Transformação: ${data.transformControls.isTransforming}\n\n[ÚLTIMOS EVENTOS]\n${data.recentLogs.map(l => `[${l.time}] [${l.category}] ${l.message}`).join('\n')}\n==================================`;
  }

  initUI() {
    const diagBtn = document.getElementById('btn-open-diagnostics');
    const modal = document.getElementById('diagnostics-modal');
    const closeBtn = document.getElementById('btn-close-diagnostics');
    const copyBtn = document.getElementById('btn-copy-diagnostics');
    const refreshBtn = document.getElementById('btn-refresh-diagnostics');

    if (diagBtn && modal) {
      diagBtn.addEventListener('click', () => {
        this.openModal();
      });
    }

    if (closeBtn && modal) {
      closeBtn.addEventListener('click', () => {
        modal.classList.add('hidden');
      });
    }

    if (refreshBtn) {
      refreshBtn.addEventListener('click', () => {
        this.renderTelemetry();
      });
    }

    if (copyBtn) {
      copyBtn.addEventListener('click', () => {
        const text = this.getFormattedReportString();
        navigator.clipboard.writeText(text).then(() => {
          const originalText = copyBtn.innerHTML;
          copyBtn.innerHTML = '✅ Copiado com Sucesso!';
          copyBtn.style.background = '#22c55e';
          setTimeout(() => {
            copyBtn.innerHTML = originalText;
            copyBtn.style.background = 'var(--accent)';
          }, 2000);
        }).catch(() => {
          // Fallback if clipboard API blocked
          const textarea = document.getElementById('diagnostics-text-output');
          if (textarea) {
            textarea.value = text;
            textarea.select();
            document.execCommand('copy');
            copyBtn.innerHTML = '✅ Copiado via fallback!';
          }
        });
      });
    }
  }

  openModal() {
    const modal = document.getElementById('diagnostics-modal');
    if (!modal) return;
    modal.classList.remove('hidden');
    this.renderTelemetry();
  }

  renderTelemetry() {
    const data = this.getDiagnosticReport();
    const stateEl = document.getElementById('diag-state-content');
    const textOutput = document.getElementById('diagnostics-text-output');

    if (textOutput) {
      textOutput.value = this.getFormattedReportString();
    }

    if (stateEl) {
      stateEl.innerHTML = `
        <div class="diag-grid">
          <div class="diag-card">
            <h4>🖥️ Modo & Ferramenta</h4>
            <p><strong>Modo:</strong> <span class="badge">${data.engine.currentMode}</span></p>
            <p><strong>Submodo:</strong> <span class="badge">${data.engine.currentSubmode}</span></p>
            <p><strong>Ferramenta:</strong> ${data.engine.currentTool}</p>
          </div>
          <div class="diag-card">
            <h4>🧊 Objeto Ativo</h4>
            <p><strong>Nome:</strong> ${data.activeObject ? data.activeObject.name : 'Nenhum'}</p>
            <p><strong>Vértices:</strong> ${data.activeObject ? data.activeObject.verticesCount : 0}</p>
            <p><strong>Faces:</strong> ${data.activeObject ? data.activeObject.quadsCount : 0}</p>
          </div>
          <div class="diag-card">
            <h4>🎯 Sub-elementos Selecionados</h4>
            <p><strong>Vértices (${data.meshEditor.selectedVertices.length}):</strong> [${data.meshEditor.selectedVertices.join(', ')}]</p>
            <p><strong>Faces (${data.meshEditor.selectedFaces.length}):</strong> [${data.meshEditor.selectedFaces.join(', ')}]</p>
            <p><strong>Arestas (${data.meshEditor.selectedEdges.length}):</strong> [${data.meshEditor.selectedEdges.join(', ')}]</p>
          </div>
          <div class="diag-card">
            <h4>🕹️ Gizmo 3D (TransformControls)</h4>
            <p><strong>Anexado em:</strong> <span class="badge">${data.transformControls.attachedObject}</span></p>
            <p><strong>Eixo em foco:</strong> ${data.transformControls.axis || 'Nenhum'}</p>
            <p><strong>Em arrasto:</strong> ${data.transformControls.isTransforming ? 'SIM (ativo)' : 'NÃO'}</p>
          </div>
        </div>
      `;
    }
  }

  updateLogUI() {
    const logsEl = document.getElementById('diag-logs-content');
    if (!logsEl) return;

    logsEl.innerHTML = this.logs.map(l => `
      <div class="diag-log-row">
        <span class="log-time">${l.time}</span>
        <span class="log-cat ${l.category.toLowerCase()}">${l.category}</span>
        <span class="log-msg">${l.message}</span>
      </div>
    `).join('');
  }
}
