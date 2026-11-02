import { Link } from '@tanstack/react-router'
import { motion } from 'motion/react'
import { CursorGlow } from '@/features/landing/CursorGlow'

const CURRENT_YEAR = new Date().getFullYear()

const stats = [
  { value: 'Авто', label: 'Расчёт по СНиП и ГОСТ', hint: 'без ручных таблиц' },
  { value: '9', label: 'типов грунта', hint: '4 базовых + 5 расширенных' },
  { value: 'O(log n)', label: 'обновление приоритета', hint: 'собственная бинарная куча' },
  { value: '2026', label: 'старт проекта' },
]

const steps = [
  { n: '01', title: 'Загрузите карту рельефа', text: 'Импорт высотной карты участка местности.' },
  { n: '02', title: 'Постройте маршрут', text: 'Алгоритм предложит оптимальный путь с учётом стоимости.' },
  { n: '03', title: 'Получите проект', text: 'Экспорт параметров и расчётов, готовых к согласованию.' },
]

const fadeUp = {
  hidden: { opacity: 0, y: 24 },
  visible: { opacity: 1, y: 0 },
}

export function LandingPage() {
  return (
    <div className="relative min-h-screen overflow-hidden bg-[#07070a]">
      <CursorGlow />

      <div className="pointer-events-none absolute -top-40 left-1/4 h-[32rem] w-[32rem] rounded-full bg-red-600/15 blur-[120px]" />
      <div className="pointer-events-none absolute top-1/3 right-0 h-[28rem] w-[28rem] rounded-full bg-amber-500/10 blur-[120px]" />

      <nav className="relative z-20 mx-auto flex max-w-7xl items-center justify-between px-6 py-6">
        <span className="font-[var(--font-display)] text-lg font-bold tracking-tight text-white">
          Rail<span className="text-red-500">Optima</span>
        </span>
        <Link
          to="/login"
          className="rounded-full border border-white/15 px-5 py-2 text-sm font-medium text-white/90 backdrop-blur transition hover:border-white/30 hover:bg-white/5"
        >
          Войти
        </Link>
      </nav>

      <section className="relative mx-auto flex min-h-[85vh] max-w-7xl flex-col items-center justify-center px-6 text-center">
        <div className="absolute inset-0 bg-grid [mask-image:radial-gradient(ellipse_60%_60%_at_50%_30%,black,transparent)]" />

        <motion.div
          initial="hidden"
          animate="visible"
          variants={fadeUp}
          transition={{ duration: 0.7 }}
          className="relative z-10"
        >
          <span className="mb-6 inline-block rounded-full border border-red-400/30 bg-red-400/10 px-4 py-1.5 text-xs font-medium tracking-wide text-red-300">
            Проектирование железнодорожных трасс нового поколения
          </span>

          <h1 className="font-[var(--font-display)] text-5xl font-bold leading-[1.05] tracking-tight text-white sm:text-7xl">
            Трасса длиной
            <br />
            <span className="bg-gradient-to-r from-red-500 via-amber-300 to-red-400 bg-clip-text text-transparent">
              в один клик
            </span>
          </h1>

          <p className="mx-auto mt-6 max-w-xl text-lg text-white/60">
            RailOptima считает оптимальный маршрут по рельефу, визуализирует его в 3D
            и формирует проектные данные по нормам СНиП и ГОСТ — от идеи до расчёта.
          </p>

          <div className="mt-10 flex items-center justify-center gap-4">
            <Link
              to="/login"
              className="rounded-full bg-red-500 px-7 py-3 text-sm font-semibold text-white shadow-[0_0_40px_-10px_rgba(255,59,48,0.6)] transition hover:bg-red-400"
            >
              Начать проект
            </Link>
            <a
              href="#how-it-works"
              className="rounded-full border border-white/15 px-7 py-3 text-sm font-medium text-white/80 transition hover:border-white/30"
            >
              Узнать больше
            </a>
          </div>
        </motion.div>
      </section>

      <section className="relative z-10 mx-auto max-w-7xl px-6 py-16">
        <div className="grid grid-cols-2 gap-4 sm:grid-cols-4">
          {stats.map((s, i) => (
            <motion.div
              key={s.label}
              initial="hidden"
              whileInView="visible"
              viewport={{ once: true, margin: '-80px' }}
              variants={fadeUp}
              transition={{ duration: 0.5, delay: i * 0.08 }}
              className="rounded-2xl border border-white/10 bg-white/[0.03] p-5 backdrop-blur-xl"
            >
              <p className="font-mono text-2xl font-bold text-amber-300">{s.value}</p>
              <p className="mt-1 text-sm font-medium text-white">{s.label}</p>
              {s.hint && <p className="mt-1 text-xs text-white/40">{s.hint}</p>}
            </motion.div>
          ))}
        </div>
      </section>

      <section id="how-it-works" className="relative z-10 mx-auto max-w-7xl px-6 py-24">
        <h2 className="font-[var(--font-display)] text-3xl font-bold text-white sm:text-4xl">Как это работает</h2>
        <div className="mt-12 grid gap-8 sm:grid-cols-3">
          {steps.map((s, i) => (
            <motion.div
              key={s.n}
              initial="hidden"
              whileInView="visible"
              viewport={{ once: true, margin: '-80px' }}
              variants={fadeUp}
              transition={{ duration: 0.5, delay: i * 0.1 }}
              className="relative pl-2"
            >
              <span className="font-[var(--font-display)] text-4xl font-bold text-white/10">{s.n}</span>
              <h3 className="mt-2 text-lg font-semibold text-white">{s.title}</h3>
              <p className="mt-2 text-sm text-white/55">{s.text}</p>
            </motion.div>
          ))}
        </div>
      </section>

      <footer className="relative z-10 border-t border-white/10 px-6 py-10 text-center text-sm text-white/40">
        © {CURRENT_YEAR} RailOptima.
      </footer>
    </div>
  )
}