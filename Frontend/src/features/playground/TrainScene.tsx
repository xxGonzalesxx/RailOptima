import { Canvas } from '@react-three/fiber';
import { OrbitControls, Environment } from '@react-three/drei';
import { TrainModel } from './TrainModel';
import { RailwayModel } from './RailwayModel';

export function TrainScene() {
  return (
    <div className="w-full h-[600px] rounded-xl overflow-hidden bg-slate-900 shadow-2xl">
      <Canvas shadows camera={{ position: [10, 8, 15], fov: 50 }}>
        <ambientLight intensity={0.7} />
        <directionalLight
          position={[10, 20, 10]}
          intensity={1.5}
          castShadow
          shadow-mapSize={[2048, 2048]}
        />

        <Environment preset="city" />

        {/* Пол */}
        <mesh rotation={[-Math.PI / 2, 0, 0]} position={[0, -0.05, 0]} receiveShadow>
          <planeGeometry args={[50, 50]} />
          <meshStandardMaterial color="#1e293b" roughness={1} />
        </mesh>

        {/* 3D Модели */}
        <RailwayModel />
        <TrainModel />

        <OrbitControls
          enablePan={true}
          enableZoom={true}
          enableRotate={true}
          minDistance={5}
          maxDistance={50}
          target={[0, 1, 0]}
        />
      </Canvas>
    </div>
  );
}