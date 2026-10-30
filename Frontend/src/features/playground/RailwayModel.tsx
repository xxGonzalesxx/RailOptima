import { useLoader } from '@react-three/fiber';
import { OBJLoader } from 'three/examples/jsm/loaders/OBJLoader';
import { MTLLoader } from 'three/examples/jsm/loaders/MTLLoader';
import { useMemo } from 'react';
import * as THREE from 'three';

export function RailwayModel() {
  const materials = useLoader(MTLLoader, '/models/train/RailwayTrack_Straight.mtl');
  const obj = useLoader(OBJLoader, '/models/train/RailwayTrack_Straight.obj', (loader) => {
    materials.preload();
    loader.setMaterials(materials);
  });

  const scene = useMemo(() => {
    const group = new THREE.Group();

    // Делаем 10 сегментов рельсов подряд
    for (let i = 0; i < 10; i++) {
      const clonedObj = obj.clone();
      clonedObj.position.set(0, 0, i * 2); // Шаг между сегментами
      clonedObj.traverse((child) => {
        if (child instanceof THREE.Mesh) {
          child.receiveShadow = true;
        }
      });
      group.add(clonedObj);
    }

    return group;
  }, [obj]);

  return <primitive object={scene} />;
}