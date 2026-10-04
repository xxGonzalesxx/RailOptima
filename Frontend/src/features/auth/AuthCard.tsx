import type { ReactNode } from 'react'

export function AuthCard({
  title,
  children,
  footer,
}: {
  title: string
  children: ReactNode
  footer?: ReactNode
}) {
  return (
    <div className="flex min-h-screen items-center justify-center bg-[#07070a] px-6">
      <div className="w-full max-w-sm rounded-2xl border border-white/10 bg-white/[0.03] p-8 backdrop-blur-xl">
        <h1 className="font-[var(--font-display)] text-2xl font-bold text-white">{title}</h1>
        <div className="mt-6 space-y-4">{children}</div>
        {footer && <div className="mt-6 text-center text-sm text-white/50">{footer}</div>}
      </div>
    </div>
  )
}