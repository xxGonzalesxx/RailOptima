import { forwardRef, type InputHTMLAttributes } from 'react'

interface TextFieldProps extends InputHTMLAttributes<HTMLInputElement> {
  label: string
  error?: string
}

export const TextField = forwardRef<HTMLInputElement, TextFieldProps>(
  ({ label, error, id, name, ...props }, ref) => {
    const fieldId = id ?? name
    return (
      <div>
        <label htmlFor={fieldId} className="mb-1 block text-sm font-medium text-white/70">
          {label}
        </label>
        <input
          ref={ref}
          id={fieldId}
          name={name}
          className="w-full rounded-lg border border-white/15 bg-white/5 px-3 py-2 text-white placeholder-white/30 outline-none transition focus:border-red-400/50 focus:bg-white/10"
          {...props}
        />
        {error && <p className="mt-1 text-sm text-red-400">{error}</p>}
      </div>
    )
  },
)
TextField.displayName = 'TextField'