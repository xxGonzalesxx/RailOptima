import { useState } from 'react'
import { AppShell } from '@/components/layout/AppShell'
import { Button } from '@/components/ui/Button'
import { CreateProjectForm } from '@/features/projects/CreateProjectForm'
import { useGetMyProjects } from '@/api/generated/project-controller/project-controller'
import type { ProjectResponse } from '@/api/generated/models/projectResponse'

const statusLabels: Record<string, string> = {
  CREATED: 'Создан',
  IN_PROGRESS: 'В работе',
  COMPLETED: 'Завершён',
}

const statusStyles: Record<string, string> = {
  CREATED: 'bg-neutral-100 text-neutral-700 dark:bg-neutral-800 dark:text-neutral-300',
  IN_PROGRESS: 'bg-amber-50 text-amber-700 dark:bg-amber-500/10 dark:text-amber-400',
  COMPLETED: 'bg-emerald-50 text-emerald-700 dark:bg-emerald-500/10 dark:text-emerald-400',
}

function StatusBadge({ status }: { status?: string }) {
  if (!status) return null
  return (
    <span
      className={`inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-medium ${
        statusStyles[status] ?? statusStyles.CREATED
      }`}
    >
      {statusLabels[status] ?? status}
    </span>
  )
}

function formatDate(value?: string) {
  if (!value) return '—'
  return new Date(value).toLocaleDateString('ru-RU', {
    day: '2-digit',
    month: 'short',
    year: 'numeric',
  })
}

export function ProjectsPage() {
  const [isFormOpen, setIsFormOpen] = useState(false)

  const { data, isLoading, isError } = useGetMyProjects({
    pageable: { page: 0, size: 20, sort: ['createdAt,desc'] },
  })

  // Предположение по вложенности конверта ApiResponse — см. комментарий выше.
  const projects: ProjectResponse[] = data?.data.data?.content ?? []

  return (
    <AppShell>
      <div className="mx-auto max-w-5xl space-y-6">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-xl font-semibold text-neutral-900 dark:text-white">Проекты</h1>
            <p className="text-sm text-neutral-500 dark:text-neutral-400">
              Все ваши железнодорожные проекты в одном месте
            </p>
          </div>
          <Button onClick={() => setIsFormOpen(true)}>+ Новый проект</Button>
        </div>

        <div className="overflow-hidden rounded-2xl border border-neutral-200 bg-white dark:border-neutral-800 dark:bg-neutral-900">
          {isLoading && (
            <div className="p-8 text-center text-sm text-neutral-500 dark:text-neutral-400">
              Загрузка…
            </div>
          )}

          {isError && (
            <div className="p-8 text-center text-sm text-red-500">
              Не удалось загрузить проекты
            </div>
          )}

          {!isLoading && !isError && projects.length === 0 && (
            <div className="p-8 text-center text-sm text-neutral-500 dark:text-neutral-400">
              Проектов пока нет — создайте первый
            </div>
          )}

          {!isLoading && !isError && projects.length > 0 && (
            <table className="w-full text-left text-sm">
              <thead className="border-b border-neutral-200 text-xs uppercase tracking-wide text-neutral-500 dark:border-neutral-800 dark:text-neutral-400">
                <tr>
                  <th className="px-5 py-3 font-medium">Название</th>
                  <th className="px-5 py-3 font-medium">Статус</th>
                  <th className="px-5 py-3 font-medium">Создан</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-neutral-100 dark:divide-neutral-800">
                {projects.map((project) => (
                  <tr
                    key={project.id}
                    className="transition-colors hover:bg-neutral-50 dark:hover:bg-neutral-800/50"
                  >
                    <td className="px-5 py-3 font-medium text-neutral-900 dark:text-white">
                      {project.name}
                    </td>
                    <td className="px-5 py-3">
                      <StatusBadge status={project.status} />
                    </td>
                    <td className="px-5 py-3 text-neutral-500 dark:text-neutral-400">
                      {formatDate(project.createdAt)}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </div>
      </div>

      {isFormOpen && (
        <div className="fixed inset-0 z-50 flex justify-end bg-black/30" onClick={() => setIsFormOpen(false)}>
          <div
            className="h-full w-full max-w-md overflow-y-auto bg-white p-6 shadow-xl dark:bg-neutral-900"
            onClick={(e) => e.stopPropagation()}
          >
            <div className="mb-4 flex items-center justify-between">
              <h2 className="text-lg font-semibold text-neutral-900 dark:text-white">Новый проект</h2>
              <button
                type="button"
                onClick={() => setIsFormOpen(false)}
                className="text-neutral-400 hover:text-neutral-700 dark:hover:text-neutral-200"
              >
                ✕
              </button>
            </div>
            <CreateProjectForm onCreated={() => setIsFormOpen(false)} />
          </div>
        </div>
      )}
    </AppShell>
  )
}