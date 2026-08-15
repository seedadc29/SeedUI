import * as THREE from 'three';

const cameraGroup = new THREE.Group();
cameraGroup.up.set(0, 0, 1);
cameraGroup.position.set(7.3589, -6.9258, 4.9583);
cameraGroup.lookAt(0, 0, 0);

// Camera instance inside group (rotated 180 deg to look forward along +Z with the gizmo)
const camera = new THREE.PerspectiveCamera(50, 16 / 9, 0.1, 1000);
camera.rotation.y = Math.PI; // Invert camera so its -Z becomes group +Z
cameraGroup.add(camera);

cameraGroup.updateMatrixWorld(true);
camera.updateMatrixWorld(true);

// Forward vector of camera in world space:
const camForward = new THREE.Vector3(0, 0, -1).applyQuaternion(camera.getWorldQuaternion(new THREE.Quaternion()));
console.log('Camera forward look vector:', camForward);

const dirToCube = new THREE.Vector3(0, 0, 0).sub(cameraGroup.position).normalize();
console.log('Direction to cube:', dirToCube);

console.log('Dot product (should be 1.0 for perfect alignment):', camForward.dot(dirToCube));
