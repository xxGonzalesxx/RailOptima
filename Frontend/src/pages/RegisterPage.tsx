import { useForm } from 'react-hook-form'
import { zodResolver } from '@hookform/resolvers/zod'
import { Link, useNavigate } from '@tanstack/react-router'
import { useRegister } from '@/api/generated/auth-controller/auth-controller'
import { useAuthStore } from '@/store/authStore'
import { registerSchema, type RegisterFormData } from '@/features/auth/schemas'
import { AuthCard } from '@/features/auth/AuthCard'
import { TextField } from '@/components/ui/TextField'
import { Button } from '@/components/ui/Button'

export function RegisterPage() {
  const navigate = useNavigate()
  const login = useAuthStore((s) => s.login)
  const {
    register,
    handleSubmit,
    formState: { errors },
  } = useForm<RegisterFormData>({ resolver: zodResolver(registerSchema) })
  const mutation = useRegister()

  const onSubmit = (data: RegisterFormData) => {
    mutation.mutate(
      { data },
      {
        onSuccess: (response) => {
          const token = response.data?.token
          if (token) {
            login(token)
            navigate({ to: '/dashboard' })
          }
        },
      },
    )
  }

  return (
    <AuthCard
      title="Регистрация в RailOptima"
      footer={
        <>
          Уже есть аккаунт?{' '}
          <Link to="/login" className="text-red-400 hover:text-red-300">
            Войти
          </Link>
        </>
      }
    >
      <form onSubmit={handleSubmit(onSubmit)} className="space-y-4">
        <TextField label="Email" type="email" {...register('email')} error={errors.email?.message} />
        <TextField label="Пароль" type="password" {...register('password')} error={errors.password?.message} />
        {mutation.isError && (
          <p className="text-sm text-red-400">Не удалось зарегистрироваться — возможно, email уже занят</p>
        )}
        <Button type="submit" loading={mutation.isPending}>
          Создать аккаунт
        </Button>
      </form>
    </AuthCard>
  )
}