import { ThemeToggle } from '@/components/ui/ThemeToggle';

export function Topbar() {
  return (
    <header className="flex h-16 shrink-0 items-center justify-between border-b border-neutral-200 bg-white px-6 dark:border-neutral-800 dark:bg-neutral-900">
      <div className="flex items-center gap-2 rounded-lg bg-neutral-100 px-3 py-1.5 text-sm text-neutral-500 dark:bg-neutral-800 dark:text-neutral-400 md:w-80">
        <svg viewBox="0 0 24 24" fill="none" className="h-4 w-4 shrink-0">
          <circle cx="11" cy="11" r="7" stroke="currentColor" strokeWidth="1.6" />
          <path d="m20 20-3-3" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" />
        </svg>
        <span className="truncate">Поиск проектов…</span>
      </div>

      <div className="flex items-center gap-3">
        <ThemeToggle />
        <div className="h-8 w-8 rounded-full bg-gradient-to-br from-indigo-500 to-violet-600" />
      </div>
    </header>
  );
}