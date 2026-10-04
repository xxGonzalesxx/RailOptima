import { z } from 'zod'

export const loginSchema = z.object({
  email: z.string().email('Некорректный email'),
  password: z.string().min(1, 'Введите пароль'),
})
export type LoginFormData = z.infer<typeof loginSchema>

export const registerSchema = z.object({
  email: z.string().email('Некорректный email'),
  password: z
    .string()
    .min(6, 'Минимум 6 символов')
    .max(100, 'Максимум 100 символов'),
})
export type RegisterFormData = z.infer<typeof registerSchema>