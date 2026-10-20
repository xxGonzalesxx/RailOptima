import { Canvas, useFrame, useThree } from '@react-three/fiber'
import { useMemo, useRef } from 'react'
import * as THREE from 'three'

const RAIL_GAUGE = 0.22
const SEGMENTS = 160
const FORWARD = new THREE.Vector3(0, 0, 1)
const UP = new THREE.Vector3(0, 1, 0)

function buildCenterCurve() {
  const points: THREE.Vector3[] = []
  for (let i = 0; i <= 80; i++) {
    const t = i / 80
    const x = (t - 0.5) * 16
    const z = Math.sin(t * Math.PI * 2.4) * 3 + Math.cos(t * Math.PI * 1.1) * 1.5
    const y = Math.sin(t * Math.PI * 1.2) * 0.35
    points.push(new THREE.Vector3(x, y, z))
  }
  return new THREE.CatmullRomCurve3(points)
}

function buildRailCurve(center: THREE.CatmullRomCurve3, side: 1 | -1) {
  const pts: THREE.Vector3[] = []
  for (let i = 0; i <= SEGMENTS; i++) {
    const t = i / SEGMENTS
    const p = center.getPointAt(t)
    const tangent = center.getTangentAt(t)
    const normal = new THREE.Vector3().crossVectors(tangent, UP).normalize()
    pts.push(p.clone().addScaledVector(normal, (RAIL_GAUGE / 2) * side))
  }
  return new THREE.CatmullRomCurve3(pts)
}

function RailPair({ progressRef }: { progressRef: React.RefObject<number> }) {
  const centerCurve = useMemo(() => buildCenterCurve(), [])
  const leftCurve = useMemo(() => buildRailCurve(centerCurve, 1), [centerCurve])
  const rightCurve = useMemo(() => buildRailCurve(centerCurve, -1), [centerCurve])

  const leftGeo = useMemo(() => new THREE.TubeGeometry(leftCurve, SEGMENTS, 0.016, 6, false), [leftCurve])
  const rightGeo = useMemo(() => new THREE.TubeGeometry(rightCurve, SEGMENTS, 0.016, 6, false), [rightCurve])

  useFrame(() => {
    const count = leftGeo.index ? leftGeo.index.count : 0
    const visible = Math.floor(count * progressRef.current)
    leftGeo.setDrawRange(0, visible)
    rightGeo.setDrawRange(0, visible)
  })

  return (
    <group rotation={[0, 0, 0]}>
      <mesh geometry={leftGeo}>
        <meshBasicMaterial color="#ff3b30" toneMapped={false} />
      </mesh>
      <mesh geometry={rightGeo}>
        <meshBasicMaterial color="#ff3b30" toneMapped={false} />
      </mesh>
    </group>
  )
}

function Train({ progressRef }: { progressRef: React.RefObject<number> }) {
  const centerCurve = useMemo(() => buildCenterCurve(), [])
  const groupRefs = [useRef<THREE.Group>(null), useRef<THREE.Group>(null), useRef<THREE.Group>(null), useRef<THREE.Group>(null)]
  const offsets = [0, 0.022, 0.044, 0.066]

  useFrame(() => {
    groupRefs.forEach((ref, i) => {
      const t = Math.max(0.001, Math.min(0.999, progressRef.current - 0.03 - offsets[i]))
      if (!ref.current) return
      const p = centerCurve.getPointAt(t)
      const tangent = centerCurve.getTangentAt(t)
      ref.current.position.copy(p)
      ref.current.position.y += 0.07
      ref.current.quaternion.setFromUnitVectors(FORWARD, tangent.clone().normalize())
      ref.current.visible = progressRef.current > 0.05 + offsets[i]
    })
  })

  return (
    <>
      {groupRefs.map((ref, i) => (
        <group ref={ref} key={i}>
          <mesh>
            <boxGeometry args={[0.16, i === 0 ? 0.14 : 0.1, 0.3]} />
            <meshBasicMaterial color={i === 0 ? '#ffd166' : '#f2f2f2'} toneMapped={false} />
          </mesh>
        </group>
      ))}
    </>
  )
}

function CameraRig() {
  const { camera } = useThree()
  const cinematic = useMemo(() => new THREE.Vector3(0, 4.2, 9.5), [])
  const topDown = useMemo(() => new THREE.Vector3(0.001, 13, 0.001), [])
  const tmp = useMemo(() => new THREE.Vector3(), [])

  useFrame((state) => {
    const u = (Math.sin(state.clock.elapsedTime * 0.1) + 1) / 2
    const eased = u * u * (3 - 2 * u)
    tmp.lerpVectors(cinematic, topDown, eased)
    camera.position.lerp(tmp, 0.03)
    camera.lookAt(0, 0, 0)
  })

  return null
}

function Scene() {
  const progressRef = useRef(0)

  useFrame((_, delta) => {
    progressRef.current = (progressRef.current + delta * 0.06) % 1
  })

  return (
    <>
      <CameraRig />
      <RailPair progressRef={progressRef} />
      <Train progressRef={progressRef} />
      <gridHelper args={[20, 20, '#3a0a0c', '#1a0506']} position={[0, -0.12, 0]} />
    </>
  )
}

export function RailBuildScene() {
  return (
    <Canvas
      camera={{ position: [0, 4.2, 9.5], fov: 45 }}
      className="!absolute inset-0 pointer-events-none"
      gl={{ antialias: true, alpha: true }}
    >
      <Scene />
    </Canvas>
  )
}