import { useForm } from 'react-hook-form'
import { zodResolver } from '@hookform/resolvers/zod'
import { useQueryClient } from '@tanstack/react-query'
import {
  useCreate,
  getGetMyProjectsQueryKey,
} from '@/api/generated/project-controller/project-controller'
import { createProjectSchema, type CreateProjectFormData } from './schemas'
import { TextField } from '@/components/ui/TextField'
import { Button } from '@/components/ui/Button'

export function CreateProjectForm({ onCreated }: { onCreated?: () => void }) {
  const queryClient = useQueryClient()
  const {
    register,
    handleSubmit,
    reset,
    formState: { errors },
  } = useForm<CreateProjectFormData>({ resolver: zodResolver(createProjectSchema) })
  const mutation = useCreate()

  const onSubmit = (data: CreateProjectFormData) => {
    mutation.mutate(
      { data },
      {
        onSuccess: () => {
          queryClient.invalidateQueries({ queryKey: getGetMyProjectsQueryKey() })
          reset()
          onCreated?.()
        },
      },
    )
  }

  return (
    <form onSubmit={handleSubmit(onSubmit)} className="space-y-4 rounded-2xl border border-white/10 bg-white/[0.03] p-6">
      <h2 className="text-lg font-semibold text-white">Новый проект</h2>

      <TextField label="Название" {...register('name')} error={errors.name?.message} />
      <TextField label="Описание (необязательно)" {...register('description')} error={errors.description?.message} />

      {mutation.isError && <p className="text-sm text-red-400">Не удалось создать проект</p>}

      <Button type="submit" loading={mutation.isPending}>
        Создать
      </Button>
    </form>
  )
}