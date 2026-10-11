import { customInstance } from '@/lib/axios'

export interface AskRequest {
  question: string
}

 interface AskResponse {
  answer: string
}

export interface ApiResponse<T> {
  success: boolean
  message: string
  data: T
  timestamp: string
  path: string | null
}

export interface AskLlmResult {
  data: ApiResponse<AskResponse>
  status: number
  headers: Headers
}

export function askLlm(payload: AskRequest) {
  return customInstance<AskLlmResult>('/api/v1/llm/ask', {
    method: 'POST',
    body: JSON.stringify(payload),
  })
}