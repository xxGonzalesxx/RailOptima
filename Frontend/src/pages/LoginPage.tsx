import { useForm } from 'react-hook-form'
import { zodResolver } from '@hookform/resolvers/zod'
import { Link, useNavigate } from '@tanstack/react-router'
import { useLogin } from '@/api/generated/auth-controller/auth-controller'
import { useAuthStore } from '@/store/authStore'
import { loginSchema, type LoginFormData } from '@/features/auth/schemas'
import { AuthCard } from '@/features/auth/AuthCard'
import { TextField } from '@/components/ui/TextField'
import { Button } from '@/components/ui/Button'

export function LoginPage() {
  const navigate = useNavigate()
  const login = useAuthStore((s) => s.login)
  const {
    register,
    handleSubmit,
    formState: { errors },
  } = useForm<LoginFormData>({ resolver: zodResolver(loginSchema) })
  const mutation = useLogin()

  const onSubmit = (data: LoginFormData) => {
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
      title="Вход в RailOptima"
      footer={
        <>
          Нет аккаунта?{' '}
          <Link to="/register" className="text-red-400 hover:text-red-300">
            Зарегистрироваться
          </Link>
        </>
      }
    >
      <form onSubmit={handleSubmit(onSubmit)} className="space-y-4">
        <TextField label="Email" type="email" {...register('email')} error={errors.email?.message} />
        <TextField label="Пароль" type="password" {...register('password')} error={errors.password?.message} />
        {mutation.isError && <p className="text-sm text-red-400">Неверный email или пароль</p>}
        <Button type="submit" loading={mutation.isPending}>
          Войти
        </Button>
      </form>
    </AuthCard>
  )
}