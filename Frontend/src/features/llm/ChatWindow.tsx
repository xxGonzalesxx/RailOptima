import { useEffect, useRef, useState, type FormEvent } from 'react'
import { Button } from '@/components/ui/Button'
import { useAskLlm } from './useAskLlm'

interface ChatMessage {
  role: 'user' | 'assistant'
  content: string
}

export function ChatWindow() {
  const [messages, setMessages] = useState<ChatMessage[]>([])
  const [input, setInput] = useState('')
  const bottomRef = useRef<HTMLDivElement>(null)
  const { mutate, isPending, error } = useAskLlm()

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: 'smooth' })
  }, [messages, isPending])

  function handleSend(e: FormEvent) {
    e.preventDefault()
    const question = input.trim()
    if (!question || isPending) return

    setMessages((prev) => [...prev, { role: 'user', content: question }])
    setInput('')

    mutate(
      { question },
      {
        onSuccess: (res) => {
          setMessages((prev) => [...prev, { role: 'assistant', content: res.data.answer }])
        },
      },
    )
  }

  const errorMessage = error?.response?.data?.message ?? (error ? 'Не удалось получить ответ' : null)

  return (
    <div className="flex h-[480px] w-[360px] flex-col overflow-hidden rounded-xl border border-white/15 bg-black/90 shadow-xl">
      <div className="border-b border-white/10 px-4 py-3 text-sm font-semibold text-white">
        Ассистент RailOptima
      </div>

      <div className="flex-1 space-y-2 overflow-y-auto px-3 py-3">
        {messages.length === 0 && !isPending && (
          <p className="m-auto text-center text-sm text-white/40">Задайте вопрос</p>
        )}

        {messages.map((m, i) => (
          <div
            key={i}
            className={`max-w-[80%] rounded-xl px-3 py-2 text-sm leading-relaxed whitespace-pre-wrap ${
              m.role === 'user'
                ? 'ml-auto rounded-br-sm bg-red-500 text-white'
                : 'rounded-bl-sm bg-white/10 text-white/90'
            }`}
          >
            {m.content}
          </div>
        ))}

        {isPending && (
          <div className="rounded-xl rounded-bl-sm bg-white/10 px-3 py-2 text-sm text-white/50 italic">
            Печатает…
          </div>
        )}

        {errorMessage && <p className="text-center text-sm text-red-400">{errorMessage}</p>}

        <div ref={bottomRef} />
      </div>

      <form onSubmit={handleSend} className="flex gap-2 border-t border-white/10 p-3">
        <input
          value={input}
          onChange={(e) => setInput(e.target.value)}
          placeholder="Введите сообщение…"
          disabled={isPending}
          maxLength={2000}
          className="flex-1 rounded-lg border border-white/15 bg-white/5 px-3 py-2 text-sm text-white placeholder-white/30 outline-none transition focus:border-red-400/50 focus:bg-white/10"
        />
        <Button type="submit" loading={isPending} disabled={!input.trim()} className="w-auto px-4">
          Отправить
        </Button>
      </form>
    </div>
  )
}