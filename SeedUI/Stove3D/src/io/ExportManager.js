import * as THREE from 'three';
import { GLTFExporter } from 'three/examples/jsm/exporters/GLTFExporter.js';
import { OBJExporter } from 'three/examples/jsm/exporters/OBJExporter.js';
import { STLExporter } from 'three/examples/jsm/exporters/STLExporter.js';

/**
 * ExportManager handles exporting models and scene files in industry-standard formats:
 * - OBJ (.obj)
 * - GLTF / GLB (.gltf, .glb)
 * - STL (.stl)
 * - Project Save/Load (.stove / .json)
 */
export class ExportManager {
  constructor(sceneManager) {
    this.sceneManager = sceneManager;
  }

  downloadFile(blob, filename) {
    const link = document.createElement('a');
    link.href = URL.createObjectURL(blob);
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    setTimeout(() => URL.revokeObjectURL(link.href), 1000);
  }

  exportOBJ(targetMesh = null) {
    const obj = targetMesh || this.sceneManager.getSelectedObject() || this.sceneManager.sceneGroup;
    const exporter = new OBJExporter();
    const result = exporter.parse(obj);
    const blob = new Blob([result], { type: 'text/plain' });
    const name = (targetMesh && targetMesh.name) ? targetMesh.name : 'modelo_seed3d';
    this.downloadFile(blob, `${name}.obj`);
  }

  exportGLTF(targetMesh = null, binary = true) {
    const obj = targetMesh || this.sceneManager.getSelectedObject() || this.sceneManager.sceneGroup;
    const exporter = new GLTFExporter();
    
    exporter.parse(
      obj,
      (gltf) => {
        if (gltf instanceof ArrayBuffer) {
          const blob = new Blob([gltf], { type: 'application/octet-stream' });
          const name = (targetMesh && targetMesh.name) ? targetMesh.name : 'cena_seed3d';
          this.downloadFile(blob, `${name}.glb`);
        } else {
          const output = JSON.stringify(gltf, null, 2);
          const blob = new Blob([output], { type: 'application/json' });
          const name = (targetMesh && targetMesh.name) ? targetMesh.name : 'cena_seed3d';
          this.downloadFile(blob, `${name}.gltf`);
        }
      },
      (error) => {
        console.error('Erro ao exportar GLTF:', error);
      },
      { binary }
    );
  }

  exportSTL(targetMesh = null) {
    const obj = targetMesh || this.sceneManager.getSelectedObject() || this.sceneManager.sceneGroup;
    const exporter = new STLExporter();
    const result = exporter.parse(obj, { binary: true });
    const blob = new Blob([result], { type: 'application/octet-stream' });
    const name = (targetMesh && targetMesh.name) ? targetMesh.name : 'modelo_seed3d';
    this.downloadFile(blob, `${name}.stl`);
  }

  saveProjectJSON() {
    const meshes = this.sceneManager.getAllMeshes();
    const data = {
      version: '1.0',
      timestamp: Date.now(),
      objects: meshes.map((m) => {
        const qm = m.userData.quadMesh;
        return {
          id: m.uuid,
          name: m.name,
          position: m.position.toArray(),
          rotation: [m.rotation.x, m.rotation.y, m.rotation.z],
          scale: m.scale.toArray(),
          color: m.material.color ? m.material.color.getHexString() : 'ffffff',
          quadMesh: qm ? {
            vertices: qm.vertices.map(v => [v.x, v.y, v.z]),
            quads: qm.quads
          } : null
        };
      })
    };

    const json = JSON.stringify(data, null, 2);
    const blob = new Blob([json], { type: 'application/json' });
    this.downloadFile(blob, 'projeto_seed3d.seed');
  }
}
