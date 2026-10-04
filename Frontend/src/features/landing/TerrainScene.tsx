import { Canvas, useFrame, useThree } from '@react-three/fiber'
import { useMemo, useRef } from 'react'
import * as THREE from 'three'

function TerrainMesh() {
  const meshRef = useRef<THREE.Mesh>(null)
  const { pointer } = useThree()

  const geometry = useMemo(() => {
    const geo = new THREE.PlaneGeometry(24, 24, 48, 48)
    const pos = geo.attributes.position
    for (let i = 0; i < pos.count; i++) {
      const x = pos.getX(i)
      const y = pos.getY(i)
      pos.setZ(i, Math.sin(x * 0.25) * Math.cos(y * 0.25) * 1.4)
    }
    geo.computeVertexNormals()
    return geo
  }, [])

  useFrame(() => {
    if (meshRef.current) {
      meshRef.current.rotation.z = pointer.x * 0.08
      meshRef.current.rotation.x = -Math.PI / 2.6 + pointer.y * 0.04
    }
  })

  return (
    <mesh ref={meshRef} geometry={geometry} rotation={[-Math.PI / 2.6, 0, 0]}>
      <meshBasicMaterial color="#8a1115" wireframe transparent opacity={0.35} />
    </mesh>
  )
}

function RoutePath() {
  const groupRef = useRef<THREE.Group>(null)
  const trainRef = useRef<THREE.Mesh>(null)

  const curve = useMemo(() => {
    const points: THREE.Vector3[] = []
    for (let i = 0; i <= 60; i++) {
      const t = i / 60
      points.push(
        new THREE.Vector3(
          (t - 0.5) * 20,
          Math.sin(t * Math.PI * 1.6) * 2.2,
          Math.sin(t * 10) * 0.3 + Math.cos(t * 4) * 0.6,
        ),
      )
    }
    return new THREE.CatmullRomCurve3(points)
  }, [])

  const tubeGeometry = useMemo(() => new THREE.TubeGeometry(curve, 120, 0.045, 8, false), [curve])

  useFrame((state) => {
    if (groupRef.current) {
      groupRef.current.position.y = 0.6 + Math.sin(state.clock.elapsedTime * 0.6) * 0.08
    }
    if (trainRef.current) {
      const t = (state.clock.elapsedTime * 0.08) % 1
      const p = curve.getPointAt(t)
      trainRef.current.position.set(p.x, p.y, p.z)
    }
  })

  return (
    <group ref={groupRef} rotation={[-Math.PI / 2.6, 0, 0]}>
      <mesh geometry={tubeGeometry}>
        <meshBasicMaterial color="#ff3b30" toneMapped={false} />
      </mesh>
      <mesh ref={trainRef}>
        <sphereGeometry args={[0.16, 16, 16]} />
        <meshBasicMaterial color="#ffd166" toneMapped={false} />
      </mesh>
    </group>
  )
}

export function TerrainScene() {
  return (
    <Canvas
      camera={{ position: [0, 5, 11], fov: 48 }}
      className="!absolute inset-0 pointer-events-none"
      gl={{ antialias: true, alpha: true }}
    >
      <ambientLight intensity={0.6} />
      <TerrainMesh />
      <RoutePath />
    </Canvas>
  )
}