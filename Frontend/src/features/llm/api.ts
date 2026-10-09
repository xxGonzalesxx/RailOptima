import { customInstance } from '@/lib/axios'

export interface AskRequest {
  question: string
}

export interface AskResponse {
  answer: string
}

export interface ApiResponse<T> {
  success: boolean
  message: string
  data: T
  timestamp: string
  path: string | null
}

export function askLlm(payload: AskRequest) {
  return customInstance<ApiResponse<AskResponse>>('/api/v1/llm/ask', {
    method: 'POST',
    body: JSON.stringify(payload),
  })
}