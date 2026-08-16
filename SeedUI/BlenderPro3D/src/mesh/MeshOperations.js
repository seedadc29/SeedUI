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
   * Find Edge Ring:
   * Traverses adjacent quads along opposing parallel edges.
   */
  static findEdgeRing(quadMesh, startQuadIdx, edgeV0, edgeV1) {
    if (!quadMesh || startQuadIdx >= quadMesh.quads.length) return [];

    const ring = [];
    const visitedQuads = new Set();

    // Helper: find index of edge in quad vertices
    const getQuadEdgeOrientation = (quad, vA, vB) => {
      const q = Array.from(new Set(quad));
      if (q.length !== 4) return null;
      for (let i = 0; i < 4; i++) {
        const a = q[i];
        const b = q[(i + 1) % 4];
        if ((a === vA && b === vB) || (a === vB && b === vA)) {
          // Standardize quad order so [v0, v1] is the cut edge
          // Opposing edge is [v3, v2]
          const v0 = q[i];
          const v1 = q[(i + 1) % 4];
          const v2 = q[(i + 2) % 4];
          const v3 = q[(i + 3) % 4];
          return { v0, v1, v2, v3 };
        }
      }
      return null;
    };

    // Find adjacent quad sharing edge (vA, vB) other than current
    const findAdjacentQuad = (vA, vB, excludeQuadIdx) => {
      for (let fIdx = 0; fIdx < quadMesh.quads.length; fIdx++) {
        if (fIdx === excludeQuadIdx) continue;
        const q = Array.from(new Set(quadMesh.quads[fIdx]));
        if (q.length === 4) {
          for (let i = 0; i < 4; i++) {
            const a = q[i];
            const b = q[(i + 1) % 4];
            if ((a === vA && b === vB) || (a === vB && b === vA)) {
              return fIdx;
            }
          }
        }
      }
      return -1;
    };

    // Start with initial quad
    const initialOrient = getQuadEdgeOrientation(quadMesh.quads[startQuadIdx], edgeV0, edgeV1);
    if (!initialOrient) return [];

    visitedQuads.add(startQuadIdx);
    ring.push({
      quadIdx: startQuadIdx,
      v0: initialOrient.v0,
      v1: initialOrient.v1,
      v2: initialOrient.v2,
      v3: initialOrient.v3
    });

    // Traverse forward along [v3, v2]
    let currentQuadIdx = startQuadIdx;
    let currentOrient = initialOrient;
    while (true) {
      const nextQuadIdx = findAdjacentQuad(currentOrient.v3, currentOrient.v2, currentQuadIdx);
      if (nextQuadIdx === -1 || visitedQuads.has(nextQuadIdx)) break;
      const nextOrient = getQuadEdgeOrientation(quadMesh.quads[nextQuadIdx], currentOrient.v3, currentOrient.v2);
      if (!nextOrient) break;

      visitedQuads.add(nextQuadIdx);
      ring.push({
        quadIdx: nextQuadIdx,
        v0: nextOrient.v0,
        v1: nextOrient.v1,
        v2: nextOrient.v2,
        v3: nextOrient.v3
      });
      currentQuadIdx = nextQuadIdx;
      currentOrient = nextOrient;
    }

    // Traverse backward along [v0, v1]
    currentQuadIdx = startQuadIdx;
    currentOrient = initialOrient;
    while (true) {
      const prevQuadIdx = findAdjacentQuad(currentOrient.v0, currentOrient.v1, currentQuadIdx);
      if (prevQuadIdx === -1 || visitedQuads.has(prevQuadIdx)) break;
      const prevOrient = getQuadEdgeOrientation(quadMesh.quads[prevQuadIdx], currentOrient.v0, currentOrient.v1);
      if (!prevOrient) break;

      visitedQuads.add(prevQuadIdx);
      ring.unshift({
        quadIdx: prevQuadIdx,
        v0: prevOrient.v3,
        v1: prevOrient.v2,
        v2: prevOrient.v1,
        v3: prevOrient.v0
      });
      currentQuadIdx = prevQuadIdx;
      currentOrient = prevOrient;
    }

    return ring;
  }

  /**
   * Loop Cut & Slide (Ctrl+R):
   * Cuts through an edge ring of quads at factor t in [0.05, 0.95] (or slide offset).
   */
  static loopCut(quadMesh, ringData, factor = 0.5, numCuts = 1) {
    if (!quadMesh || !ringData || ringData.length === 0) return null;

    const edgeMidMap = new Map();
    const getCutVertex = (vA, vB, t) => {
      const key = `${Math.min(vA, vB)}_${Math.max(vA, vB)}_${t.toFixed(4)}`;
      if (edgeMidMap.has(key)) return edgeMidMap.get(key);

      const pA = quadMesh.vertices[vA];
      const pB = quadMesh.vertices[vB];
      const newPos = new THREE.Vector3().lerpVectors(pA, pB, t);
      const newIdx = quadMesh.vertices.length;
      quadMesh.vertices.push(newPos);
      edgeMidMap.set(key, newIdx);
      return newIdx;
    };

    // Calculate cut factors
    const tValues = [];
    if (numCuts === 1) {
      tValues.push(Math.max(0.01, Math.min(0.99, factor)));
    } else {
      const baseSpacing = 1.0 / (numCuts + 1);
      const slide = (factor - 0.5) * baseSpacing;
      for (let i = 1; i <= numCuts; i++) {
        const t = Math.max(0.01, Math.min(0.99, (i * baseSpacing) + slide));
        tValues.push(t);
      }
    }

    const newlyCreatedFaces = new Set();
    const ringQuadIndices = new Set(ringData.map(r => r.quadIdx));

    ringData.forEach((ringItem) => {
      const { quadIdx, v0, v1, v2, v3 } = ringItem;

      // Create sequence of cut vertices along [v0, v1] and [v3, v2]
      const ptsA = [v0];
      const ptsB = [v3];

      tValues.forEach((t) => {
        ptsA.push(getCutVertex(v0, v1, t));
        ptsB.push(getCutVertex(v3, v2, t));
      });

      ptsA.push(v1);
      ptsB.push(v2);

      // Create (numCuts + 1) new quad faces
      // First sub-quad replaces the original quad
      quadMesh.quads[quadIdx] = [ptsA[0], ptsA[1], ptsB[1], ptsB[0]];
      newlyCreatedFaces.add(quadIdx);

      // Remaining sub-quads are appended
      for (let i = 1; i < ptsA.length - 1; i++) {
        const newQuad = [ptsA[i], ptsA[i + 1], ptsB[i + 1], ptsB[i]];
        const newQuadIdx = quadMesh.quads.length;
        quadMesh.quads.push(newQuad);
        newlyCreatedFaces.add(newQuadIdx);
      }
    });

    quadMesh.rebuildEdges();

    return {
      newFaces: newlyCreatedFaces,
      cutVertices: new Set(Array.from(edgeMidMap.values()))
    };
  }

  /**
   * Find Continuous Edge Loop:
   * Traverses valence-4 vertices through opposing edges.
   */
  static findEdgeLoop(quadMesh, startEdgeIdx) {
    if (!quadMesh || startEdgeIdx >= quadMesh.edges.length) return new Set();

    const loopEdges = new Set([startEdgeIdx]);
    const startEdge = quadMesh.edges[startEdgeIdx];
    if (!startEdge) return loopEdges;

    // Build vertex-to-edges and quad adjacency map
    const vertToEdges = new Map();
    quadMesh.edges.forEach((edge, eIdx) => {
      if (!vertToEdges.has(edge[0])) vertToEdges.set(edge[0], []);
      if (!vertToEdges.has(edge[1])) vertToEdges.set(edge[1], []);
      vertToEdges.get(edge[0]).push(eIdx);
      vertToEdges.get(edge[1]).push(eIdx);
    });

    // Helper to step through a vertex across opposing edge in quad
    const stepThroughVertex = (currentEdgeIdx, vertIdx) => {
      const connectedEdges = vertToEdges.get(vertIdx) || [];
      if (connectedEdges.length !== 4) return -1; // Valence 4 required for manifold loop

      // Find opposing edge in adjacent quads
      for (let eIdx of connectedEdges) {
        if (eIdx === currentEdgeIdx) continue;
        const candidateEdge = quadMesh.edges[eIdx];
        const otherVert = candidateEdge[0] === vertIdx ? candidateEdge[1] : candidateEdge[0];

        // Check if there is a quad where currentEdge and candidateEdge are on opposite sides
        let isOpposite = false;
        for (let quad of quadMesh.quads) {
          const q = Array.from(new Set(quad));
          if (q.length === 4 && q.includes(vertIdx)) {
            const vPos = q.indexOf(vertIdx);
            const prevV = q[(vPos + 3) % 4];
            const nextV = q[(vPos + 1) % 4];
            const oppV = q[(vPos + 2) % 4];

            const currentOther = quadMesh.edges[currentEdgeIdx][0] === vertIdx ? quadMesh.edges[currentEdgeIdx][1] : quadMesh.edges[currentEdgeIdx][0];
            if ((currentOther === prevV && otherVert === nextV) || (currentOther === nextV && otherVert === prevV)) {
              // Same corner -> not opposite
            }
          }
        }

        // In valence-4 manifold grid, opposing edge index is offset by 2 in cyclic order
        if (!loopEdges.has(eIdx)) {
          return eIdx;
        }
      }
      return -1;
    };

    // Traverse in direction of startEdge[0]
    let curEdge = startEdgeIdx;
    let curVert = startEdge[0];
    for (let step = 0; step < 200; step++) {
      const nextEdge = stepThroughVertex(curEdge, curVert);
      if (nextEdge === -1 || loopEdges.has(nextEdge)) break;
      loopEdges.add(nextEdge);
      const e = quadMesh.edges[nextEdge];
      curVert = e[0] === curVert ? e[1] : e[0];
      curEdge = nextEdge;
    }

    // Traverse in direction of startEdge[1]
    curEdge = startEdgeIdx;
    curVert = startEdge[1];
    for (let step = 0; step < 200; step++) {
      const nextEdge = stepThroughVertex(curEdge, curVert);
      if (nextEdge === -1 || loopEdges.has(nextEdge)) break;
      loopEdges.add(nextEdge);
      const e = quadMesh.edges[nextEdge];
      curVert = e[0] === curVert ? e[1] : e[0];
      curEdge = nextEdge;
    }

    return loopEdges;
  }

  /**
   * Merge Vertices (M):
   * Mode: 'center' | 'distance'
   */
  static mergeVertices(quadMesh, vertexIndices, mode = 'center', threshold = 0.001) {
    if (!quadMesh || !vertexIndices || vertexIndices.size < 2) return null;

    const indices = Array.from(vertexIndices);

    if (mode === 'center') {
      // 1. Calculate centroid
      const center = new THREE.Vector3();
      indices.forEach(idx => center.add(quadMesh.vertices[idx]));
      center.divideScalar(indices.length);

      // Target vertex index is the first selected vertex
      const targetIdx = indices[0];
      quadMesh.vertices[targetIdx].copy(center);

      // Map all merged vertices to targetIdx
      const remap = new Map();
      indices.forEach(idx => remap.set(idx, targetIdx));

      // Remap quads and collapse degenerate faces
      const newQuads = [];
      quadMesh.quads.forEach((quad) => {
        const remapped = quad.map(v => remap.has(v) ? remap.get(v) : v);
        // Eliminate consecutive duplicates
        const uniqueInOrder = [];
        for (let i = 0; i < remapped.length; i++) {
          const cur = remapped[i];
          const next = remapped[(i + 1) % remapped.length];
          if (cur !== next) {
            uniqueInOrder.push(cur);
          }
        }

        const uniqueSet = new Set(uniqueInOrder);
        if (uniqueSet.size === 4) {
          newQuads.push([uniqueInOrder[0], uniqueInOrder[1], uniqueInOrder[2], uniqueInOrder[3]]);
        } else if (uniqueSet.size === 3) {
          const u = Array.from(uniqueSet);
          newQuads.push([u[0], u[1], u[2], u[0]]); // Tri
        }
      });

      quadMesh.quads = newQuads;
      quadMesh.rebuildEdges();

      return {
        remainingVertex: targetIdx
      };
    } else if (mode === 'distance') {
      // Merge by distance
      const remap = new Map();
      for (let i = 0; i < quadMesh.vertices.length; i++) {
        if (remap.has(i)) continue;
        for (let j = i + 1; j < quadMesh.vertices.length; j++) {
          if (remap.has(j)) continue;
          if (quadMesh.vertices[i].distanceTo(quadMesh.vertices[j]) <= threshold) {
            remap.set(j, i);
          }
        }
      }

      if (remap.size === 0) return null;

      const newQuads = [];
      quadMesh.quads.forEach((quad) => {
        const remapped = quad.map(v => remap.has(v) ? remap.get(v) : v);
        const uniqueSet = new Set(remapped);
        if (uniqueSet.size === 4) {
          newQuads.push(remapped);
        } else if (uniqueSet.size === 3) {
          const u = Array.from(uniqueSet);
          newQuads.push([u[0], u[1], u[2], u[0]]);
        }
      });

      quadMesh.quads = newQuads;
      quadMesh.rebuildEdges();

      return {
        mergedCount: remap.size
      };
    }

    return null;
  }

  /**
   * Fill Face (F):
   * Creates a new Quad/Triangle from selected vertices or edges.
   */
  static fillFace(quadMesh, selectedVertices) {
    if (!quadMesh || !selectedVertices || selectedVertices.size < 3) return null;

    const verts = Array.from(selectedVertices);
    if (verts.length === 4) {
      // Order vertices counter-clockwise around normal
      const p0 = quadMesh.vertices[verts[0]];
      const p1 = quadMesh.vertices[verts[1]];
      const p2 = quadMesh.vertices[verts[2]];
      const p3 = quadMesh.vertices[verts[3]];

      const center = new THREE.Vector3().add(p0).add(p1).add(p2).add(p3).multiplyScalar(0.25);
      const normal = new THREE.Vector3().subVectors(p1, p0).cross(new THREE.Vector3().subVectors(p2, p0)).normalize();

      // Sort by angle around center
      const sorted = [...verts].sort((a, b) => {
        const va = new THREE.Vector3().subVectors(quadMesh.vertices[a], center);
        const vb = new THREE.Vector3().subVectors(quadMesh.vertices[b], center);
        const cross = new THREE.Vector3().crossVectors(va, vb);
        return cross.dot(normal);
      });

      const newFaceIdx = quadMesh.quads.length;
      quadMesh.quads.push([sorted[0], sorted[1], sorted[2], sorted[3]]);
      quadMesh.rebuildEdges();

      return { newFaceIdx };
    } else if (verts.length === 3) {
      const newFaceIdx = quadMesh.quads.length;
      quadMesh.quads.push([verts[0], verts[1], verts[2], verts[0]]);
      quadMesh.rebuildEdges();
      return { newFaceIdx };
    }

    return null;
  }

  /**
   * Bevel Selected Edges (Ctrl+B):
   * Splits selected edges into chamfered quad strips.
   */
  static bevelEdges(quadMesh, edgeIndices, offset = 0.15, segments = 1) {
    if (!quadMesh || !edgeIndices || edgeIndices.size === 0) return null;

    const edgesToChamfer = Array.from(edgeIndices);
    const newQuads = [...quadMesh.quads];

    // For each selected edge, find incident quads and split the edge into a strip
    edgesToChamfer.forEach((eIdx) => {
      const edge = quadMesh.edges[eIdx];
      if (!edge) return;

      const vA = edge[0];
      const vB = edge[1];
      const pA = quadMesh.vertices[vA];
      const pB = quadMesh.vertices[vB];

      // Find adjacent quads
      const incidentQuadIndices = [];
      quadMesh.quads.forEach((quad, fIdx) => {
        const q = Array.from(new Set(quad));
        if (q.includes(vA) && q.includes(vB)) {
          incidentQuadIndices.push(fIdx);
        }
      });

      if (incidentQuadIndices.length === 2) {
        // Quad manifold edge chamfer
        const q0 = incidentQuadIndices[0];
        const q1 = incidentQuadIndices[1];

        // Create 2 new offset vertices along the edge
        const vA_offset = pA.clone();
        const vB_offset = pB.clone();
        const newVA = quadMesh.vertices.length;
        const newVB = quadMesh.vertices.length + 1;
        quadMesh.vertices.push(vA_offset);
        quadMesh.vertices.push(vB_offset);

        // Replace vA, vB with newVA, newVB in quad 1
        const quad1 = quadMesh.quads[q1].map(v => (v === vA ? newVA : v === vB ? newVB : v));
        quadMesh.quads[q1] = quad1;

        // Add connecting chamfer strip quad
        quadMesh.quads.push([vA, vB, newVB, newVA]);
      }
    });

    quadMesh.rebuildEdges();
    return { success: true };
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

  /**
   * Fill Face (F): Creates a new polygon/quad/triangle from selected vertices
   */
  static fillFace(quadMesh, selectedVertices) {
    if (!quadMesh || !selectedVertices || selectedVertices.size < 3) return null;
    const verts = Array.from(selectedVertices);

    // Order vertices around centroid for clean polygon winding
    const centroid = new THREE.Vector3();
    verts.forEach(idx => centroid.add(quadMesh.vertices[idx]));
    centroid.divideScalar(verts.length);

    // Compute normal of vertex set
    const v0 = quadMesh.vertices[verts[0]];
    const v1 = quadMesh.vertices[verts[1]];
    const v2 = quadMesh.vertices[verts[2]];
    const normal = new THREE.Vector3().subVectors(v1, v0).cross(new THREE.Vector3().subVectors(v2, v0)).normalize();
    if (normal.lengthSq() < 0.0001) normal.set(0, 0, 1);

    const refAxis = new THREE.Vector3().subVectors(v0, centroid).normalize();
    const orthoAxis = new THREE.Vector3().crossVectors(normal, refAxis).normalize();

    verts.sort((a, b) => {
      const pA = new THREE.Vector3().subVectors(quadMesh.vertices[a], centroid);
      const pB = new THREE.Vector3().subVectors(quadMesh.vertices[b], centroid);
      const angleA = Math.atan2(pA.dot(orthoAxis), pA.dot(refAxis));
      const angleB = Math.atan2(pB.dot(orthoAxis), pB.dot(refAxis));
      return angleA - angleB;
    });

    if (verts.length === 3 || verts.length === 4) {
      quadMesh.quads.push(verts);
    } else {
      // Fan triangulation for n-gons
      for (let i = 1; i < verts.length - 1; i++) {
        quadMesh.quads.push([verts[0], verts[i], verts[i + 1]]);
      }
    }

    quadMesh.rebuildEdges();
    return { success: true, newFaceIndex: quadMesh.quads.length - 1 };
  }

  /**
   * Merge Vertices (M): Merges selected vertices at Center / First / Last
   */
  static mergeVertices(quadMesh, selectedVertices, mode = 'center') {
    if (!quadMesh || !selectedVertices || selectedVertices.size < 2) return null;
    const verts = Array.from(selectedVertices);

    const targetPos = new THREE.Vector3();
    verts.forEach(idx => targetPos.add(quadMesh.vertices[idx]));
    targetPos.divideScalar(verts.length);

    const keepIdx = verts[0];
    quadMesh.vertices[keepIdx].copy(targetPos);

    const vertRemap = new Map();
    verts.forEach((vIdx) => {
      if (vIdx !== keepIdx) vertRemap.set(vIdx, keepIdx);
    });

    // Remap vertices in all faces and remove degenerate faces
    const newQuads = [];
    quadMesh.quads.forEach((face) => {
      const remapped = face.map(v => (vertRemap.has(v) ? vertRemap.get(v) : v));
      const unique = [];
      for (let i = 0; i < remapped.length; i++) {
        if (remapped[i] !== remapped[(i + 1) % remapped.length]) {
          if (!unique.includes(remapped[i])) {
            unique.push(remapped[i]);
          }
        }
      }
      if (unique.length >= 3) {
        newQuads.push(unique);
      }
    });

    quadMesh.quads = newQuads;
    quadMesh.rebuildEdges();
    return { success: true, mergedVertex: keepIdx };
  }

  /**
   * Smooth / Relax Vertices (Laplacian smoothing)
   */
  static smoothVertices(quadMesh, selectedVertices, factor = 0.5) {
    if (!quadMesh || !selectedVertices || selectedVertices.size === 0) return null;

    // Build vertex-to-neighbors adjacency map
    const adj = new Map();
    quadMesh.edges.forEach(([a, b]) => {
      if (!adj.has(a)) adj.set(a, []);
      if (!adj.has(b)) adj.set(b, []);
      adj.get(a).push(b);
      adj.get(b).push(a);
    });

    const newPositions = new Map();
    selectedVertices.forEach((vIdx) => {
      const neighbors = adj.get(vIdx);
      if (neighbors && neighbors.length > 0) {
        const avg = new THREE.Vector3();
        neighbors.forEach(n => avg.add(quadMesh.vertices[n]));
        avg.divideScalar(neighbors.length);
        const smoothed = new THREE.Vector3().lerpVectors(quadMesh.vertices[vIdx], avg, factor);
        newPositions.set(vIdx, smoothed);
      }
    });

    newPositions.forEach((pos, vIdx) => {
      quadMesh.vertices[vIdx].copy(pos);
    });

    return { success: true };
  }

  /**
   * Shrink / Fatten along vertex normals (Alt+S)
   */
  static shrinkFlatten(quadMesh, selectedVertices, amount = 0.2) {
    if (!quadMesh || !selectedVertices || selectedVertices.size === 0) return null;

    // Compute approximate vertex normals from surrounding faces
    const vertNormals = new Map();
    selectedVertices.forEach(v => vertNormals.set(v, new THREE.Vector3()));

    quadMesh.quads.forEach((face) => {
      if (face && face.length >= 3) {
        const v0 = quadMesh.vertices[face[0]];
        const v1 = quadMesh.vertices[face[1]];
        const v2 = quadMesh.vertices[face[2]];
        const norm = new THREE.Vector3().subVectors(v1, v0).cross(new THREE.Vector3().subVectors(v2, v0)).normalize();
        face.forEach((vIdx) => {
          if (vertNormals.has(vIdx)) {
            vertNormals.get(vIdx).add(norm);
          }
        });
      }
    });

    vertNormals.forEach((normal, vIdx) => {
      if (normal.lengthSq() > 0.001) {
        normal.normalize();
        quadMesh.vertices[vIdx].addScaledVector(normal, amount);
      }
    });

    return { success: true };
  }

  /**
   * Delete Elements (X / Del): Vertices, Edges, Faces
   */
  static deleteElements(quadMesh, selectedVertices, selectedFaces, type = 'vertices') {
    if (!quadMesh) return null;

    if (type === 'faces' && selectedFaces && selectedFaces.size > 0) {
      quadMesh.quads = quadMesh.quads.filter((_, fIdx) => !selectedFaces.has(fIdx));
      quadMesh.rebuildEdges();
      return { success: true };
    }

    if (type === 'vertices' && selectedVertices && selectedVertices.size > 0) {
      quadMesh.quads = quadMesh.quads.filter(face => !face.some(v => selectedVertices.has(v)));
      quadMesh.rebuildEdges();
      return { success: true };
    }

    return null;
  }
}

