import type { ButtonHTMLAttributes } from 'react'

interface ButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  loading?: boolean
  variant?: 'primary' | 'secondary'
}

export function Button({
  loading,
  variant = 'primary',
  children,
  className = '',
  disabled,
  ...props
}: ButtonProps) {
  const base = 'w-full rounded-lg py-2.5 text-sm font-semibold transition disabled:opacity-50'
  const variants = {
    primary: 'bg-red-500 text-white hover:bg-red-400',
    secondary: 'border border-white/15 text-white/80 hover:border-white/30',
  }
  return (
    <button className={`${base} ${variants[variant]} ${className}`} disabled={disabled || loading} {...props}>
      {loading ? 'Загрузка…' : children}
    </button>
  )
}