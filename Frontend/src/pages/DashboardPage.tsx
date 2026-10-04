import { useState } from 'react'
import { useGetMyProjects } from '@/api/generated/project-controller/project-controller'
import { useAuthStore } from '@/store/authStore'
import { useNavigate } from '@tanstack/react-router'
import { CreateProjectForm } from '@/features/projects/CreateProjectForm'

export function DashboardPage() {
  const navigate = useNavigate()
  const logout = useAuthStore((s) => s.logout)
  const [showForm, setShowForm] = useState(false)
  const { data, isLoading, isError } = useGetMyProjects({
    pageable: { page: 0, size: 20, sort: ['createdAt,desc'] },
  })

  const handleLogout = () => {
    logout()
    navigate({ to: '/login' })
  }

  const projects = data?.data?.content ?? []

  return (
    <div className="min-h-screen bg-[#07070a] p-8 text-white">
      <div className="mb-6 flex items-center justify-between">
        <h1 className="text-2xl font-semibold">Мои проекты</h1>
        <div className="flex gap-3">
          <button
            onClick={() => setShowForm((v) => !v)}
            className="rounded-lg bg-red-500 px-4 py-2 text-sm font-medium hover:bg-red-400"
          >
            {showForm ? 'Отмена' : '+ Новый проект'}
          </button>
          <button
            onClick={handleLogout}
            className="rounded-lg border border-white/15 px-4 py-2 text-sm hover:border-white/30"
          >
            Выйти
          </button>
        </div>
      </div>

      {showForm && (
        <div className="mb-6">
          <CreateProjectForm onCreated={() => setShowForm(false)} />
        </div>
      )}

      {isLoading ? (
        <p className="text-white/50">Загрузка проектов…</p>
      ) : isError ? (
        <p className="text-red-400">Не удалось загрузить проекты</p>
      ) : projects.length === 0 ? (
        <p className="text-white/50">Пока нет проектов</p>
      ) : (
        <ul className="space-y-2">
          {projects.map((p) => (
            <li key={p.id} className="rounded-lg border border-white/10 bg-white/[0.03] p-4">
              <div className="font-medium">{p.name}</div>
              <div className="text-sm text-white/50">{p.status}</div>
            </li>
          ))}
        </ul>
      )}
    </div>
  )
}