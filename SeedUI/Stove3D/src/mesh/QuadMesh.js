import * as THREE from 'three';

/**
 * QuadMesh represents a 3D polygonal manifold mesh with clean Quad/Polygon topology
 * matching Blender's BMesh data model.
 * 
 * Includes Parametric Generators for all standard 3D primitives:
 * - Cube (Cubo)
 * - UV Sphere (Esfera UV)
 * - Cylinder (Cilindro)
 * - Cone (Cone)
 * - Plane (Plano)
 * - Torus (Torus)
 * - Icosphere (Esfera Triangulada)
 */
export class QuadMesh {
  constructor() {
    this.vertices = []; // Array of THREE.Vector3
    this.quads = [];    // Array of [v0, v1, v2, v3] (or [v0, v1, v2] for triangles)
    this.edges = [];    // Array of [v0, v1]
  }

  // --- PARAMETRIC PRIMITIVE FACTORIES ---

  static createCube(params = {}) {
    const size = params.size || 2;
    const mesh = new QuadMesh();
    const h = size / 2;

    // 8 Corner Vertices
    mesh.vertices = [
      new THREE.Vector3(-h, -h, -h), // 0: Bottom back-left
      new THREE.Vector3( h, -h, -h), // 1: Bottom back-right
      new THREE.Vector3( h, -h,  h), // 2: Bottom front-right
      new THREE.Vector3(-h, -h,  h), // 3: Bottom front-left
      new THREE.Vector3(-h,  h, -h), // 4: Top back-left
      new THREE.Vector3( h,  h, -h), // 5: Top back-right
      new THREE.Vector3( h,  h,  h), // 6: Top front-right
      new THREE.Vector3(-h,  h,  h)  // 7: Top front-left
    ];

    // 6 Quad Faces (Counter-Clockwise outward winding)
    mesh.quads = [
      [3, 2, 6, 7], // Front (+Z)
      [1, 0, 4, 5], // Back (-Z)
      [7, 6, 5, 4], // Top (+Y)
      [0, 1, 2, 3], // Bottom (-Y)
      [2, 1, 5, 6], // Right (+X)
      [0, 3, 7, 4]  // Left (-X)
    ];

    mesh.rebuildEdges();
    return mesh;
  }

  static createPlane(params = {}) {
    const width = params.width || 3;
    const height = params.height || 3;
    const mesh = new QuadMesh();
    const hw = width / 2;
    const hh = height / 2;

    mesh.vertices = [
      new THREE.Vector3(-hw, 0, -hh),
      new THREE.Vector3( hw, 0, -hh),
      new THREE.Vector3( hw, 0,  hh),
      new THREE.Vector3(-hw, 0,  hh)
    ];

    mesh.quads = [
      [0, 1, 2, 3]
    ];

    mesh.rebuildEdges();
    return mesh;
  }

  static createCylinder(params = {}) {
    const radius = params.radius !== undefined ? params.radius : 1;
    const height = params.height !== undefined ? params.height : 2;
    const segments = Math.max(3, parseInt(params.segments) || 8);
    const capTop = params.capTop !== undefined ? params.capTop : true;
    const capBottom = params.capBottom !== undefined ? params.capBottom : true;

    const mesh = new QuadMesh();
    const hh = height / 2;

    // Bottom Ring (0 to segments - 1)
    for (let i = 0; i < segments; i++) {
      const angle = (i / segments) * Math.PI * 2;
      mesh.vertices.push(new THREE.Vector3(Math.cos(angle) * radius, -hh, Math.sin(angle) * radius));
    }

    // Top Ring (segments to 2 * segments - 1)
    for (let i = 0; i < segments; i++) {
      const angle = (i / segments) * Math.PI * 2;
      mesh.vertices.push(new THREE.Vector3(Math.cos(angle) * radius, hh, Math.sin(angle) * radius));
    }

    // Side Quad Faces
    for (let i = 0; i < segments; i++) {
      const next = (i + 1) % segments;
      const b0 = i;
      const b1 = next;
      const t0 = i + segments;
      const t1 = next + segments;
      mesh.quads.push([b0, b1, t1, t0]);
    }

    // Top Cap Center Vertex
    if (capTop) {
      const topCenterIdx = mesh.vertices.length;
      mesh.vertices.push(new THREE.Vector3(0, hh, 0));
      for (let i = 0; i < segments; i++) {
        const next = (i + 1) % segments;
        mesh.quads.push([i + segments, topCenterIdx, next + segments, i + segments]);
      }
    }

    // Bottom Cap Center Vertex
    if (capBottom) {
      const bottomCenterIdx = mesh.vertices.length;
      mesh.vertices.push(new THREE.Vector3(0, -hh, 0));
      for (let i = 0; i < segments; i++) {
        const next = (i + 1) % segments;
        mesh.quads.push([next, bottomCenterIdx, i, next]);
      }
    }

    mesh.rebuildEdges();
    return mesh;
  }

  static createCone(params = {}) {
    const radius = params.radius !== undefined ? params.radius : 1;
    const height = params.height !== undefined ? params.height : 2;
    const segments = Math.max(3, parseInt(params.segments) || 8);

    const mesh = new QuadMesh();
    const hh = height / 2;

    // Base Ring (0 to segments - 1)
    for (let i = 0; i < segments; i++) {
      const angle = (i / segments) * Math.PI * 2;
      mesh.vertices.push(new THREE.Vector3(Math.cos(angle) * radius, -hh, Math.sin(angle) * radius));
    }

    // Tip Vertex
    const tipIdx = mesh.vertices.length;
    mesh.vertices.push(new THREE.Vector3(0, hh, 0));

    // Base Center Vertex
    const baseCenterIdx = mesh.vertices.length;
    mesh.vertices.push(new THREE.Vector3(0, -hh, 0));

    // Side Faces (Triangles as 4-point degenerate or fan)
    for (let i = 0; i < segments; i++) {
      const next = (i + 1) % segments;
      mesh.quads.push([i, next, tipIdx, i]);
      mesh.quads.push([next, baseCenterIdx, i, next]);
    }

    mesh.rebuildEdges();
    return mesh;
  }

  static createUVSphere(params = {}) {
    const radius = params.radius !== undefined ? params.radius : 1;
    const segments = Math.max(4, parseInt(params.segments) || 12);
    const rings = Math.max(3, parseInt(params.rings) || 8);

    const mesh = new QuadMesh();

    // Top Pole Vertex
    mesh.vertices.push(new THREE.Vector3(0, radius, 0)); // index 0

    // Intermediate Rings
    for (let r = 1; r < rings; r++) {
      const phi = (r / rings) * Math.PI;
      const y = Math.cos(phi) * radius;
      const ringRadius = Math.sin(phi) * radius;

      for (let s = 0; s < segments; s++) {
        const theta = (s / segments) * Math.PI * 2;
        const x = Math.cos(theta) * ringRadius;
        const z = Math.sin(theta) * ringRadius;
        mesh.vertices.push(new THREE.Vector3(x, y, z));
      }
    }

    // Bottom Pole Vertex
    const bottomPoleIdx = mesh.vertices.length;
    mesh.vertices.push(new THREE.Vector3(0, -radius, 0));

    // Top Cap Faces
    for (let s = 0; s < segments; s++) {
      const next = (s + 1) % segments;
      const v1 = 1 + s;
      const v2 = 1 + next;
      mesh.quads.push([0, v1, v2, 0]);
    }

    // Body Quad Faces
    for (let r = 0; r < rings - 2; r++) {
      const r1 = 1 + r * segments;
      const r2 = 1 + (r + 1) * segments;

      for (let s = 0; s < segments; s++) {
        const next = (s + 1) % segments;
        const v0 = r1 + s;
        const v1 = r2 + s;
        const v2 = r2 + next;
        const v3 = r1 + next;
        mesh.quads.push([v0, v1, v2, v3]);
      }
    }

    // Bottom Cap Faces
    const lastRingStart = 1 + (rings - 2) * segments;
    for (let s = 0; s < segments; s++) {
      const next = (s + 1) % segments;
      const v1 = lastRingStart + s;
      const v2 = lastRingStart + next;
      mesh.quads.push([v1, bottomPoleIdx, v2, v1]);
    }

    mesh.rebuildEdges();
    return mesh;
  }

  static createTorus(params = {}) {
    const radius = params.radius !== undefined ? params.radius : 1.2;
    const tube = params.tube !== undefined ? params.tube : 0.4;
    const radialSegments = Math.max(3, parseInt(params.radialSegments) || 8);
    const tubularSegments = Math.max(4, parseInt(params.tubularSegments) || 16);

    const mesh = new QuadMesh();

    for (let j = 0; j < tubularSegments; j++) {
      const u = (j / tubularSegments) * Math.PI * 2;
      const cosU = Math.cos(u);
      const sinU = Math.sin(u);

      for (let i = 0; i < radialSegments; i++) {
        const v = (i / radialSegments) * Math.PI * 2;
        const cosV = Math.cos(v);
        const sinV = Math.sin(v);

        const x = (radius + tube * cosV) * cosU;
        const y = tube * sinV;
        const z = (radius + tube * cosV) * sinU;

        mesh.vertices.push(new THREE.Vector3(x, y, z));
      }
    }

    for (let j = 0; j < tubularSegments; j++) {
      const nextJ = (j + 1) % tubularSegments;
      for (let i = 0; i < radialSegments; i++) {
        const nextI = (i + 1) % radialSegments;

        const v0 = j * radialSegments + i;
        const v1 = nextJ * radialSegments + i;
        const v2 = nextJ * radialSegments + nextI;
        const v3 = j * radialSegments + nextI;

        mesh.quads.push([v0, v1, v2, v3]);
      }
    }

    mesh.rebuildEdges();
    return mesh;
  }

  static createIcoSphere(params = {}) {
    const radius = params.radius !== undefined ? params.radius : 1;
    const subdivisions = Math.max(0, Math.min(3, parseInt(params.subdivisions) || 1));

    const mesh = new QuadMesh();
    const t = (1.0 + Math.sqrt(5.0)) / 2.0;

    // 12 Initial Icosahedron Vertices
    const baseVerts = [
      new THREE.Vector3(-1,  t,  0).normalize().multiplyScalar(radius),
      new THREE.Vector3( 1,  t,  0).normalize().multiplyScalar(radius),
      new THREE.Vector3(-1, -t,  0).normalize().multiplyScalar(radius),
      new THREE.Vector3( 1, -t,  0).normalize().multiplyScalar(radius),
      new THREE.Vector3( 0, -1,  t).normalize().multiplyScalar(radius),
      new THREE.Vector3( 0,  1,  t).normalize().multiplyScalar(radius),
      new THREE.Vector3( 0, -1, -t).normalize().multiplyScalar(radius),
      new THREE.Vector3( 0,  1, -t).normalize().multiplyScalar(radius),
      new THREE.Vector3( t,  0, -1).normalize().multiplyScalar(radius),
      new THREE.Vector3( t,  0,  1).normalize().multiplyScalar(radius),
      new THREE.Vector3(-t,  0, -1).normalize().multiplyScalar(radius),
      new THREE.Vector3(-t,  0,  1).normalize().multiplyScalar(radius)
    ];

    mesh.vertices = baseVerts;

    // 20 Initial Triangular Faces
    let faces = [
      [0, 11, 5], [0, 5, 1], [0, 1, 7], [0, 7, 10], [0, 10, 11],
      [1, 5, 9], [5, 11, 4], [11, 10, 2], [10, 7, 6], [7, 1, 8],
      [3, 9, 4], [3, 4, 2], [3, 2, 6], [3, 6, 8], [3, 8, 9],
      [4, 9, 5], [2, 4, 11], [6, 2, 10], [8, 6, 7], [9, 8, 1]
    ];

    // Subdivide
    for (let s = 0; s < subdivisions; s++) {
      const midMap = new Map();
      const getMidpoint = (v1, v2) => {
        const key = v1 < v2 ? `${v1}_${v2}` : `${v2}_${v1}`;
        if (midMap.has(key)) return midMap.get(key);
        const mid = new THREE.Vector3().addVectors(mesh.vertices[v1], mesh.vertices[v2]).normalize().multiplyScalar(radius);
        const idx = mesh.vertices.length;
        mesh.vertices.push(mid);
        midMap.set(key, idx);
        return idx;
      };

      const newFaces = [];
      faces.forEach((f) => {
        const m01 = getMidpoint(f[0], f[1]);
        const m12 = getMidpoint(f[1], f[2]);
        const m20 = getMidpoint(f[2], f[0]);
        newFaces.push([f[0], m01, m20]);
        newFaces.push([f[1], m12, m01]);
        newFaces.push([f[2], m20, m12]);
        newFaces.push([m01, m12, m20]);
      });
      faces = newFaces;
    }

    mesh.quads = faces.map(f => [f[0], f[1], f[2], f[0]]);
    mesh.rebuildEdges();
    return mesh;
  }

  // --- TOPOLOGY HELPERS ---

  rebuildEdges() {
    const edgeMap = new Map();

    this.quads.forEach((face) => {
      // Handle triangles [0,1,2,0] or quads [0,1,2,3]
      const unique = Array.from(new Set(face));
      const len = unique.length;
      for (let i = 0; i < len; i++) {
        const vA = unique[i];
        const vB = unique[(i + 1) % len];
        const key = vA < vB ? `${vA}_${vB}` : `${vB}_${vA}`;
        if (!edgeMap.has(key)) {
          edgeMap.set(key, [vA, vB]);
        }
      }
    });

    this.edges = Array.from(edgeMap.values());
  }

  getConnectedEdges(vIdx) {
    const connected = [];
    this.edges.forEach((edge, eIdx) => {
      if (edge[0] === vIdx || edge[1] === vIdx) {
        connected.push(eIdx);
      }
    });
    return connected;
  }

  computeQuadCornerNormals(v0, v1, v2, v3) {
    // Corner 0: (v1 - v0) x (v3 - v0)
    const e01 = new THREE.Vector3().subVectors(v1, v0);
    const e03 = new THREE.Vector3().subVectors(v3, v0);
    const n0 = new THREE.Vector3().crossVectors(e01, e03).normalize();

    // Corner 1: (v2 - v1) x (v0 - v1)
    const e12 = new THREE.Vector3().subVectors(v2, v1);
    const e10 = new THREE.Vector3().subVectors(v0, v1);
    const n1 = new THREE.Vector3().crossVectors(e12, e10).normalize();

    // Corner 2: (v3 - v2) x (v1 - v2)
    const e23 = new THREE.Vector3().subVectors(v3, v2);
    const e21 = new THREE.Vector3().subVectors(v1, v2);
    const n2 = new THREE.Vector3().crossVectors(e23, e21).normalize();

    // Corner 3: (v0 - v3) x (v2 - v3)
    const e30 = new THREE.Vector3().subVectors(v0, v3);
    const e32 = new THREE.Vector3().subVectors(v2, v3);
    const n3 = new THREE.Vector3().crossVectors(e30, e32).normalize();

    return { n0, n1, n2, n3 };
  }

  computeAutoSmoothNormals(angleThresholdDeg = 30) {
    const angleThresholdRad = (angleThresholdDeg * Math.PI) / 180;
    const cosThreshold = Math.cos(angleThresholdRad);

    const triNormals = [];
    const triVertices = [];
    const triFaceMap = [];

    this.quads.forEach((face, fIdx) => {
      const unique = Array.from(new Set(face));
      if (unique.length === 4) {
        const v0 = this.vertices[face[0]];
        const v1 = this.vertices[face[1]];
        const v2 = this.vertices[face[2]];
        const v3 = this.vertices[face[3]];

        const t0Norm = new THREE.Vector3().subVectors(v2, v1).cross(new THREE.Vector3().subVectors(v0, v1)).normalize();
        const t1Norm = new THREE.Vector3().subVectors(v0, v3).cross(new THREE.Vector3().subVectors(v2, v3)).normalize();

        triNormals.push(t0Norm);
        triVertices.push([face[0], face[1], face[2]]);
        triFaceMap.push(fIdx);

        triNormals.push(t1Norm);
        triVertices.push([face[0], face[2], face[3]]);
        triFaceMap.push(fIdx);
      } else if (unique.length === 3) {
        const v0 = this.vertices[unique[0]];
        const v1 = this.vertices[unique[1]];
        const v2 = this.vertices[unique[2]];
        const tNorm = new THREE.Vector3().subVectors(v2, v1).cross(new THREE.Vector3().subVectors(v0, v1)).normalize();
        triNormals.push(tNorm);
        triVertices.push([unique[0], unique[1], unique[2]]);
        triFaceMap.push(fIdx);
      }
    });

    const vertexNormals = [];

    for (let tIdx = 0; tIdx < triNormals.length; tIdx++) {
      const myNorm = triNormals[tIdx];
      const myVerts = triVertices[tIdx];
      const myFaceIdx = triFaceMap[tIdx];

      for (let corner = 0; corner < 3; corner++) {
        const vId = myVerts[corner];
        const accumNorm = new THREE.Vector3().copy(myNorm);

        for (let otherTIdx = 0; otherTIdx < triNormals.length; otherTIdx++) {
          if (otherTIdx === tIdx) continue;
          const otherVerts = triVertices[otherTIdx];
          if (otherVerts.includes(vId)) {
            const otherNorm = triNormals[otherTIdx];
            const otherFaceIdx = triFaceMap[otherTIdx];
            const cosAngle = myNorm.dot(otherNorm);

            // Same Quad face ALWAYS shares normals; across separate faces only if <= 30 deg
            if (myFaceIdx === otherFaceIdx || cosAngle >= cosThreshold) {
              accumNorm.add(otherNorm);
            }
          }
        }

        accumNorm.normalize();
        vertexNormals.push(accumNorm);
      }
    }

    return vertexNormals;
  }

  // --- THREE.JS BUFFER GENERATION (BLENDER EXACT AUTO-SMOOTH) ---

  toBufferGeometry() {
    const geo = new THREE.BufferGeometry();
    const positions = [];
    const normals = [];
    const uvs = [];

    const vertNormals = this.computeAutoSmoothNormals(30);
    let vNormIdx = 0;

    this.quads.forEach((face) => {
      const unique = Array.from(new Set(face));
      if (unique.length === 4) {
        const v0 = this.vertices[face[0]];
        const v1 = this.vertices[face[1]];
        const v2 = this.vertices[face[2]];
        const v3 = this.vertices[face[3]];

        const n0 = vertNormals[vNormIdx++];
        const n1 = vertNormals[vNormIdx++];
        const n2 = vertNormals[vNormIdx++];

        const n3 = vertNormals[vNormIdx++];
        const n4 = vertNormals[vNormIdx++];
        const n5 = vertNormals[vNormIdx++];

        // Tri 1: (v0, v1, v2)
        positions.push(v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
        normals.push(n0.x, n0.y, n0.z, n1.x, n1.y, n1.z, n2.x, n2.y, n2.z);
        uvs.push(0, 0, 1, 0, 1, 1);

        // Tri 2: (v0, v2, v3)
        positions.push(v0.x, v0.y, v0.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z);
        normals.push(n3.x, n3.y, n3.z, n4.x, n4.y, n4.z, n5.x, n5.y, n5.z);
        uvs.push(0, 0, 1, 1, 0, 1);
      } else if (unique.length === 3) {
        const v0 = this.vertices[unique[0]];
        const v1 = this.vertices[unique[1]];
        const v2 = this.vertices[unique[2]];

        const n0 = vertNormals[vNormIdx++];
        const n1 = vertNormals[vNormIdx++];
        const n2 = vertNormals[vNormIdx++];

        positions.push(v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
        normals.push(n0.x, n0.y, n0.z, n1.x, n1.y, n1.z, n2.x, n2.y, n2.z);
        uvs.push(0, 0, 1, 0, 0.5, 1);
      }
    });

    geo.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
    geo.setAttribute('normal', new THREE.Float32BufferAttribute(normals, 3));
    geo.setAttribute('uv', new THREE.Float32BufferAttribute(uvs, 2));
    geo.computeBoundingBox();
    geo.computeBoundingSphere();

    return geo;
  }

  createEdgeLines(material) {
    const linePositions = [];
    this.edges.forEach((edge) => {
      const vA = this.vertices[edge[0]];
      const vB = this.vertices[edge[1]];
      if (vA && vB) {
        linePositions.push(vA.x, vA.y, vA.z, vB.x, vB.y, vB.z);
      }
    });

    const lineGeo = new THREE.BufferGeometry();
    lineGeo.setAttribute('position', new THREE.Float32BufferAttribute(linePositions, 3));

    const lineMesh = new THREE.LineSegments(lineGeo, material);
    return lineMesh;
  }

  updateGeometryPositions(geo, lineMesh) {
    const posAttr = geo.attributes.position;
    const normAttr = geo.attributes.normal;
    let ptr = 0;

    const vertNormals = this.computeAutoSmoothNormals(30);
    let vNormIdx = 0;

    this.quads.forEach((face) => {
      const unique = Array.from(new Set(face));
      if (unique.length === 4) {
        const v0 = this.vertices[face[0]];
        const v1 = this.vertices[face[1]];
        const v2 = this.vertices[face[2]];
        const v3 = this.vertices[face[3]];

        const n0 = vertNormals[vNormIdx++];
        const n1 = vertNormals[vNormIdx++];
        const n2 = vertNormals[vNormIdx++];

        const n3 = vertNormals[vNormIdx++];
        const n4 = vertNormals[vNormIdx++];
        const n5 = vertNormals[vNormIdx++];

        // Tri 1: (v0, v1, v2)
        posAttr.setXYZ(ptr, v0.x, v0.y, v0.z); normAttr.setXYZ(ptr, n0.x, n0.y, n0.z); ptr++;
        posAttr.setXYZ(ptr, v1.x, v1.y, v1.z); normAttr.setXYZ(ptr, n1.x, n1.y, n1.z); ptr++;
        posAttr.setXYZ(ptr, v2.x, v2.y, v2.z); normAttr.setXYZ(ptr, n2.x, n2.y, n2.z); ptr++;

        // Tri 2: (v0, v2, v3)
        posAttr.setXYZ(ptr, v0.x, v0.y, v0.z); normAttr.setXYZ(ptr, n3.x, n3.y, n3.z); ptr++;
        posAttr.setXYZ(ptr, v2.x, v2.y, v2.z); normAttr.setXYZ(ptr, n4.x, n4.y, n4.z); ptr++;
        posAttr.setXYZ(ptr, v3.x, v3.y, v3.z); normAttr.setXYZ(ptr, n5.x, n5.y, n5.z); ptr++;
      } else if (unique.length === 3) {
        const v0 = this.vertices[unique[0]];
        const v1 = this.vertices[unique[1]];
        const v2 = this.vertices[unique[2]];

        const n0 = vertNormals[vNormIdx++];
        const n1 = vertNormals[vNormIdx++];
        const n2 = vertNormals[vNormIdx++];

        posAttr.setXYZ(ptr, v0.x, v0.y, v0.z); normAttr.setXYZ(ptr, n0.x, n0.y, n0.z); ptr++;
        posAttr.setXYZ(ptr, v1.x, v1.y, v1.z); normAttr.setXYZ(ptr, n1.x, n1.y, n1.z); ptr++;
        posAttr.setXYZ(ptr, v2.x, v2.y, v2.z); normAttr.setXYZ(ptr, n2.x, n2.y, n2.z); ptr++;
      }
    });

    posAttr.needsUpdate = true;
    if (normAttr) normAttr.needsUpdate = true;
    geo.computeBoundingBox();
    geo.computeBoundingSphere();

    // Update edge wireframe lines
    if (lineMesh && lineMesh.geometry) {
      const linePos = lineMesh.geometry.attributes.position;
      let lptr = 0;
      this.edges.forEach((edge) => {
        const vA = this.vertices[edge[0]];
        const vB = this.vertices[edge[1]];
        if (vA && vB && lptr + 1 < linePos.count) {
          linePos.setXYZ(lptr++, vA.x, vA.y, vA.z);
          linePos.setXYZ(lptr++, vB.x, vB.y, vB.z);
        }
      });
      linePos.needsUpdate = true;
    }
  }
}
