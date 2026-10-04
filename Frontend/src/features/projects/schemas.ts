import { z } from 'zod'

export const createProjectSchema = z.object({
  name: z.string().min(1, 'Введите название').max(100, 'Максимум 100 символов'),
  description: z.string().max(500, 'Максимум 500 символов').optional(),
})
export type CreateProjectFormData = z.infer<typeof createProjectSchema>