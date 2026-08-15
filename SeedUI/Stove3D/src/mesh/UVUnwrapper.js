import * as THREE from 'three';

/**
 * UVUnwrapper - Automatic UV Unwrapping & Atlas Packing Engine (Milestone 08)
 * Generates clean, non-overlapping UV atlas layouts for polygonal QuadMeshes
 * enabling seamless 3D texture painting and multi-material export.
 */
export class UVUnwrapper {

  /**
   * Generates a Box / Atlas UV layout for a QuadMesh
   * @param {QuadMesh} quadMesh 
   * @returns {Float32Array} UV Buffer Attribute Data
   */
  static generateAtlasUVs(quadMesh) {
    const numFaces = quadMesh.quads.length;
    if (numFaces === 0) return new Float32Array(0);

    // Compute grid layout: e.g. 16 faces -> 4x4 grid
    const cols = Math.ceil(Math.sqrt(numFaces));
    const rows = Math.ceil(numFaces / cols);
    const cellW = 1.0 / cols;
    const cellH = 1.0 / rows;
    const padding = 0.04 * Math.min(cellW, cellH);

    const uvs = [];

    quadMesh.quads.forEach((face, fIdx) => {
      const col = fIdx % cols;
      const row = Math.floor(fIdx / cols);

      const uMin = col * cellW + padding;
      const uMax = (col + 1) * cellW - padding;
      const vMin = row * cellH + padding;
      const vMax = (row + 1) * cellH - padding;

      const unique = Array.from(new Set(face));
      if (unique.length === 4) {
        // Quad face: 4 Radial Triangles meeting at center
        const uMid = (uMin + uMax) * 0.5;
        const vMid = (vMin + vMax) * 0.5;

        // Tri 0: (v0, v1, vCenter)
        uvs.push(uMin, vMin, uMax, vMin, uMid, vMid);
        // Tri 1: (v1, v2, vCenter)
        uvs.push(uMax, vMin, uMax, vMax, uMid, vMid);
        // Tri 2: (v2, v3, vCenter)
        uvs.push(uMax, vMax, uMin, vMax, uMid, vMid);
        // Tri 3: (v3, v0, vCenter)
        uvs.push(uMin, vMax, uMin, vMin, uMid, vMid);
      } else if (unique.length === 3) {
        // Triangle face: 1 Triangle -> 3 UV vertices
        uvs.push(uMin, vMin);
        uvs.push(uMax, vMin);
        uvs.push((uMin + uMax) * 0.5, vMax);
      }
    });

    return new Float32Array(uvs);
  }

  /**
   * Generates Box Project UVs based on face normal dominant plane (X, Y, Z)
   * @param {QuadMesh} quadMesh 
   */
  static generateBoxProjectUVs(quadMesh) {
    const uvs = [];

    quadMesh.quads.forEach((face) => {
      const unique = Array.from(new Set(face));
      const v0 = quadMesh.vertices[unique[0]];
      const v1 = quadMesh.vertices[unique[1]];
      const v2 = quadMesh.vertices[unique[2]];
      const v3 = unique.length === 4 ? quadMesh.vertices[unique[3]] : null;

      if (!v0 || !v1 || !v2) return;

      const cb = new THREE.Vector3().subVectors(v2, v1);
      const ab = new THREE.Vector3().subVectors(v0, v1);
      const normal = cb.cross(ab).normalize();

      const absX = Math.abs(normal.x);
      const absY = Math.abs(normal.y);
      const absZ = Math.abs(normal.z);

      const projectVert = (v) => {
        if (absX >= absY && absX >= absZ) {
          return [ (v.z + 2) * 0.25, (v.y + 2) * 0.25 ];
        } else if (absY >= absX && absY >= absZ) {
          return [ (v.x + 2) * 0.25, (v.z + 2) * 0.25 ];
        } else {
          return [ (v.x + 2) * 0.25, (v.y + 2) * 0.25 ];
        }
      };

      const uv0 = projectVert(v0);
      const uv1 = projectVert(v1);
      const uv2 = projectVert(v2);

      if (unique.length === 4 && v3) {
        const uv3 = projectVert(v3);
        uvs.push(uv0[0], uv0[1]); uvs.push(uv1[0], uv1[1]); uvs.push(uv2[0], uv2[1]);
        uvs.push(uv0[0], uv0[1]); uvs.push(uv2[0], uv2[1]); uvs.push(uv3[0], uv3[1]);
      } else {
        uvs.push(uv0[0], uv0[1]); uvs.push(uv1[0], uv1[1]); uvs.push(uv2[0], uv2[1]);
      }
    });

    return new Float32Array(uvs);
  }
}
