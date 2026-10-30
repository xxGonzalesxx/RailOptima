import { useLoader } from '@react-three/fiber';
import { OBJLoader } from 'three/examples/jsm/loaders/OBJLoader';
import { useMemo } from 'react';
import * as THREE from 'three';

export function RealTrain() {
  // Загружаем только .obj (без .mtl)
  const obj = useLoader(OBJLoader, '/models/base.obj');
  const scene = useMemo(() => {
    // 1. Масштаб. Если модель слишком большая/маленькая — меняй 1 на 0.1 или 10
    obj.scale.set(1, 1, 1);

    // 2. Позиция
    obj.position.set(0, 0, 0);

    // 3. Применяем материал и тени, так как в .obj без .mtl материала нет
    obj.traverse((child) => {
      if (child instanceof THREE.Mesh) {
        child.castShadow = true;
        child.receiveShadow = true;

        // Если у модели нет своего цвета, ставим стандартный
        if (!child.material || (child.material as THREE.Material).type === 'MeshBasicMaterial') {
          child.material = new THREE.MeshStandardMaterial({
            color: '#4b5563', // Серый металл
            roughness: 0.5,
            metalness: 0.5,
          });
        }
      }
    });

    return obj;
  }, [obj]);

  return <primitive object={scene} />;
}