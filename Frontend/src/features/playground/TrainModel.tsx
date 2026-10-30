import { useLoader } from '@react-three/fiber';
import { OBJLoader } from 'three/examples/jsm/loaders/OBJLoader';
import { MTLLoader } from 'three/examples/jsm/loaders/MTLLoader';
import { useMemo } from 'react';
import * as THREE from 'three';

export function TrainModel() {
  // 1. Загружаем материалы для локомотива
  const locoMaterials = useLoader(MTLLoader, '/models/train/Locomotive_Front.mtl');
  const locoObj = useLoader(OBJLoader, '/models/train/Locomotive_Front.obj', (loader) => {
    locoMaterials.preload();
    loader.setMaterials(locoMaterials);
  });

  // 2. Загружаем материалы для вагона
  const wagonMaterials = useLoader(MTLLoader, '/models/train/Locomotive_Wagon.mtl');
  const wagonObj = useLoader(OBJLoader, '/models/train/Locomotive_Wagon.obj', (loader) => {
    wagonMaterials.preload();
    loader.setMaterials(wagonMaterials);
  });

  const scene = useMemo(() => {
    // Группируем локомотив и вагон
    const group = new THREE.Group();

    // Настройка Локомотива
    locoObj.scale.set(1, 1, 1);
    locoObj.position.set(0, 0, 0); // Впереди
    locoObj.traverse((child) => {
      if (child instanceof THREE.Mesh) {
        child.castShadow = true;
        child.receiveShadow = true;
      }
    });

    // Настройка Вагона (ставим сзади)
    wagonObj.scale.set(1, 1, 1);
    wagonObj.position.set(0, 0, -6); // Сзади локомотива (сдвиг по Z)
    wagonObj.traverse((child) => {
      if (child instanceof THREE.Mesh) {
        child.castShadow = true;
        child.receiveShadow = true;
      }
    });

    group.add(locoObj);
    group.add(wagonObj);

    return group;
  }, [locoObj, wagonObj]);

  return <primitive object={scene} />;
}