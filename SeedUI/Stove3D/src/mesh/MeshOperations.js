import * as THREE from 'three';
import { QuadMesh } from './QuadMesh.js';

/**
 * MeshOperations implements topological modeling operators inspired by Blender's BMesh:
 * - Extrude Faces & Edges (E)
 * - Inset Faces (I)
 * - Subdivide Quads
 * - Bevel Edges (Ctrl+B)
 * - Proportional Editing (O)
 */
export class MeshOperations {

  /**
   * Extrude Selected Faces:
   * 1. Copies the vertices of selected faces and displaces them along the average face normal.
   * 2. Replaces the selected face with the new top cap face.
   * 3. Creates connecting bridge quad walls between the boundary edges and the new vertices.
   */
  static extrudeFaces(quadMesh, faceIndices, distance = 0.5) {
    if (!quadMesh || !faceIndices || faceIndices.size === 0) return null;

    const facesToExtrude = Array.from(faceIndices);
    const oldToNewVertMap = new Map();
    const newVertices = [];
    const newTopQuads = [];
    const newSideQuads = [];

    // 1. Calculate average normal of selected faces
    const avgNormal = new THREE.Vector3();
    let normalCount = 0;

    facesToExtrude.forEach((fIdx) => {
      const face = quadMesh.quads[fIdx];
      if (face && face.length >= 3) {
        const unique = Array.from(new Set(face));
        if (unique.length >= 3) {
          const v0 = quadMesh.vertices[unique[0]];
          const v1 = quadMesh.vertices[unique[1]];
          const v2 = quadMesh.vertices[unique[2]];
          const norm = new THREE.Vector3().subVectors(v2, v1).cross(new THREE.Vector3().subVectors(v0, v1)).normalize();
          avgNormal.add(norm);
          normalCount++;
        }
      }
    });

    if (normalCount > 0) avgNormal.normalize();
    else avgNormal.set(0, 1, 0);

    const displacement = avgNormal.clone().multiplyScalar(distance);

    // 2. Duplicate vertices used by selected faces
    facesToExtrude.forEach((fIdx) => {
      const face = quadMesh.quads[fIdx];
      if (face) {
        face.forEach((vIdx) => {
          if (!oldToNewVertMap.has(vIdx)) {
            const oldV = quadMesh.vertices[vIdx];
            const newV = oldV.clone().add(displacement);
            const newIdx = quadMesh.vertices.length + newVertices.length;
            newVertices.push(newV);
            oldToNewVertMap.set(vIdx, newIdx);
          }
        });
      }
    });

    // Append new vertices to quadMesh
    newVertices.forEach(v => quadMesh.vertices.push(v));

    // 3. Find boundary edges of the selected face group (edges that belong to exactly 1 selected face)
    const edgeUsage = new Map(); // key -> { count, edge: [vA, vB], faceIdx }

    facesToExtrude.forEach((fIdx) => {
      const face = quadMesh.quads[fIdx];
      if (face) {
        const unique = Array.from(new Set(face));
        const len = unique.length;
        for (let i = 0; i < len; i++) {
          const vA = unique[i];
          const vB = unique[(i + 1) % len];
          const key = vA < vB ? `${vA}_${vB}` : `${vB}_${vA}`;
          if (!edgeUsage.has(key)) {
            edgeUsage.set(key, { count: 1, vA, vB, isForward: true });
          } else {
            const entry = edgeUsage.get(key);
            entry.count++;
          }
        }
      }
    });

    // 4. Create Side Quads for boundary edges
    edgeUsage.forEach((entry) => {
      if (entry.count === 1) {
        // Boundary edge!
        const b0 = entry.vA;
        const b1 = entry.vB;
        const t0 = oldToNewVertMap.get(b0);
        const t1 = oldToNewVertMap.get(b1);
        if (t0 !== undefined && t1 !== undefined) {
          // Quad: [b0, b1, t1, t0]
          newSideQuads.push([b0, b1, t1, t0]);
        }
      }
    });

    // 5. Replace selected old faces with top cap faces
    const newlyCreatedFaceIndices = new Set();
    facesToExtrude.forEach((fIdx) => {
      const oldFace = quadMesh.quads[fIdx];
      const newTopFace = oldFace.map(vIdx => oldToNewVertMap.get(vIdx) !== undefined ? oldToNewVertMap.get(vIdx) : vIdx);
      quadMesh.quads[fIdx] = newTopFace;
      newlyCreatedFaceIndices.add(fIdx);
    });

    // Add side quads to mesh
    newSideQuads.forEach((sq) => {
      quadMesh.quads.push(sq);
    });

    quadMesh.rebuildEdges();

    // Collect newly selected top vertices
    const newSelectedVerts = new Set(Array.from(oldToNewVertMap.values()));

    return {
      newSelectedVertices: newSelectedVerts,
      newSelectedFaces: newlyCreatedFaceIndices,
      extrudedOffset: displacement
    };
  }

  /**
   * Inset Selected Faces (I):
   * Creates an inner concentric face with adjustable inset factor, bridged by 4 side quads.
   */
  static insetFaces(quadMesh, faceIndices, factor = 0.25) {
    if (!quadMesh || !faceIndices || faceIndices.size === 0) return null;

    const facesToInset = Array.from(faceIndices);
    const newSelectedVerts = new Set();
    const newSelectedFaces = new Set();

    facesToInset.forEach((fIdx) => {
      const face = quadMesh.quads[fIdx];
      if (!face) return;

      const unique = Array.from(new Set(face));
      if (unique.length < 3) return;

      // Compute face center
      const center = new THREE.Vector3();
      unique.forEach(vIdx => center.add(quadMesh.vertices[vIdx]));
      center.divideScalar(unique.length);

      // Create new inner vertices interpolated towards center
      const innerVertIndices = [];
      unique.forEach((vIdx) => {
        const outerV = quadMesh.vertices[vIdx];
        const innerV = new THREE.Vector3().lerpVectors(outerV, center, factor);
        const newIdx = quadMesh.vertices.length;
        quadMesh.vertices.push(innerV);
        innerVertIndices.push(newIdx);
        newSelectedVerts.add(newIdx);
      });

      // Create perimeter bridge quads connecting outer edge to inner edge
      const len = unique.length;
      for (let i = 0; i < len; i++) {
        const o0 = unique[i];
        const o1 = unique[(i + 1) % len];
        const i0 = innerVertIndices[i];
        const i1 = innerVertIndices[(i + 1) % len];
        quadMesh.quads.push([o0, o1, i1, i0]);
      }

      // Replace original face with inner face
      if (innerVertIndices.length === 4) {
        quadMesh.quads[fIdx] = [innerVertIndices[0], innerVertIndices[1], innerVertIndices[2], innerVertIndices[3]];
      } else if (innerVertIndices.length === 3) {
        quadMesh.quads[fIdx] = [innerVertIndices[0], innerVertIndices[1], innerVertIndices[2], innerVertIndices[0]];
      }
      newSelectedFaces.add(fIdx);
    });

    quadMesh.rebuildEdges();

    return {
      newSelectedVertices: newSelectedVerts,
      newSelectedFaces: newSelectedFaces
    };
  }

  /**
   * Subdivide Quads:
   * Splits each quad face into 4 clean sub-quads by inserting edge midpoints and face center vertices.
   */
  static subdivide(quadMesh) {
    if (!quadMesh || quadMesh.quads.length === 0) return;

    const edgeMidMap = new Map();

    const getMidpoint = (v0, v1) => {
      const key = v0 < v1 ? `${v0}_${v1}` : `${v1}_${v0}`;
      if (edgeMidMap.has(key)) return edgeMidMap.get(key);

      const p0 = quadMesh.vertices[v0];
      const p1 = quadMesh.vertices[v1];
      const mid = new THREE.Vector3().addVectors(p0, p1).multiplyScalar(0.5);
      const newIdx = quadMesh.vertices.length;
      quadMesh.vertices.push(mid);
      edgeMidMap.set(key, newIdx);
      return newIdx;
    };

    const newQuads = [];

    quadMesh.quads.forEach((face) => {
      const unique = Array.from(new Set(face));
      if (unique.length === 4) {
        const v0 = unique[0];
        const v1 = unique[1];
        const v2 = unique[2];
        const v3 = unique[3];

        const m01 = getMidpoint(v0, v1);
        const m12 = getMidpoint(v1, v2);
        const m23 = getMidpoint(v2, v3);
        const m30 = getMidpoint(v3, v0);

        // Face Center Vertex
        const center = new THREE.Vector3()
          .add(quadMesh.vertices[v0])
          .add(quadMesh.vertices[v1])
          .add(quadMesh.vertices[v2])
          .add(quadMesh.vertices[v3])
          .multiplyScalar(0.25);
        const centerIdx = quadMesh.vertices.length;
        quadMesh.vertices.push(center);

        // 4 Sub-Quads
        newQuads.push([v0, m01, centerIdx, m30]);
        newQuads.push([m01, v1, m12, centerIdx]);
        newQuads.push([centerIdx, m12, v2, m23]);
        newQuads.push([m30, centerIdx, m23, v3]);
      } else if (unique.length === 3) {
        // Triangle subdivision into 3 quads
        const v0 = unique[0];
        const v1 = unique[1];
        const v2 = unique[2];

        const m01 = getMidpoint(v0, v1);
        const m12 = getMidpoint(v1, v2);
        const m20 = getMidpoint(v2, v0);

        const center = new THREE.Vector3()
          .add(quadMesh.vertices[v0])
          .add(quadMesh.vertices[v1])
          .add(quadMesh.vertices[v2])
          .divideScalar(3);
        const centerIdx = quadMesh.vertices.length;
        quadMesh.vertices.push(center);

        newQuads.push([v0, m01, centerIdx, m20]);
        newQuads.push([m01, v1, m12, centerIdx]);
        newQuads.push([m20, centerIdx, m12, v2]);
      }
    });

    quadMesh.quads = newQuads;
    quadMesh.rebuildEdges();
  }

  /**
   * Proportional Editing Falloff calculation:
   * Smooth Gaussian / Cosine falloff within influence radius.
   */
  static getFalloffWeight(distance, radius, type = 'smooth') {
    if (distance >= radius) return 0;
    const t = distance / radius;
    if (type === 'smooth') {
      // 3t^2 - 2t^3 Hermite Smoothstep inverted
      return Math.cos(t * Math.PI * 0.5);
    } else if (type === 'linear') {
      return 1 - t;
    } else if (type === 'sphere') {
      return Math.sqrt(Math.max(0, 1 - t * t));
    }
    return Math.max(0, 1 - t);
  }
}
