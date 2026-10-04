import { useMemo, useState } from 'react'
import { motion } from 'motion/react'

const GROUND_TYPES = [
  { label: 'Скала', coef: 2.6 },
  { label: 'Глина', coef: 1.4 },
  { label: 'Песок', coef: 1.1 },
  { label: 'Чернозём', coef: 1.0 },
  { label: 'Болото', coef: 3.2 },
] as const

export function CostCalculator() {
  const [length, setLength] = useState(1200)
  const [slope, setSlope] = useState(8)
  const [angle, setAngle] = useState(15)
  const [groundIndex, setGroundIndex] = useState(2)

  const ground = GROUND_TYPES[groundIndex]

  const cost = useMemo(() => {
    const slopeFactor = 1 + Math.abs(slope) * 0.1
    const angleFactor = 1 + 0.5 * (angle / 90) ** 2
    return length * slopeFactor * ground.coef * angleFactor
  }, [length, slope, angle, ground])

  return (
    <motion.div
      initial={{ opacity: 0, y: 24 }}
      whileInView={{ opacity: 1, y: 0 }}
      viewport={{ once: true, margin: '-100px' }}
      transition={{ duration: 0.6 }}
      className="rounded-3xl border border-white/10 bg-white/[0.03] p-8 backdrop-blur-xl"
    >
      <p className="font-mono text-xs uppercase tracking-widest text-red-400">
        Формула стоимости · live-демо
      </p>
      <h3 className="mt-2 font-[var(--font-display)] text-2xl font-bold text-white">
        Посчитайте участок трассы
      </h3>

      <div className="mt-8 grid gap-6 sm:grid-cols-2">
        <div className="space-y-6">
          <Slider label="Длина участка" value={length} min={100} max={5000} step={50} unit="м" onChange={setLength} />
          <Slider label="Уклон" value={slope} min={0} max={45} step={1} unit="°" onChange={setSlope} />
          <Slider label="Угол поворота" value={angle} min={0} max={90} step={1} unit="°" onChange={setAngle} />

          <div>
            <p className="mb-2 text-sm text-white/60">Тип грунта</p>
            <div className="flex flex-wrap gap-2">
              {GROUND_TYPES.map((g, i) => (
                <button
                  key={g.label}
                  onClick={() => setGroundIndex(i)}
                  className={`rounded-full border px-3 py-1.5 text-sm transition ${
                    i === groundIndex
                      ? 'border-red-400/60 bg-red-400/15 text-red-200'
                      : 'border-white/10 text-white/60 hover:border-white/25'
                  }`}
                >
                  {g.label}
                </button>
              ))}
            </div>
          </div>
        </div>

        <div className="flex flex-col items-center justify-center rounded-2xl border border-white/10 bg-black/30 p-6">
          <p className="text-sm text-white/50">Условная стоимость</p>
          <p className="tabular mt-2 font-mono text-4xl font-bold text-amber-300">
            {cost.toLocaleString('ru-RU', { maximumFractionDigits: 0 })}
          </p>
          <p className="mt-1 text-xs text-white/40">усл. ед.</p>
        </div>
      </div>
    </motion.div>
  )
}

function Slider({
  label,
  value,
  min,
  max,
  step,
  unit,
  onChange,
}: {
  label: string
  value: number
  min: number
  max: number
  step: number
  unit: string
  onChange: (v: number) => void
}) {
  return (
    <div>
      <div className="mb-1 flex justify-between text-sm">
        <span className="text-white/60">{label}</span>
        <span className="tabular font-mono text-white">{value}{unit}</span>
      </div>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        className="w-full accent-red-500"
      />
    </div>
  )
}