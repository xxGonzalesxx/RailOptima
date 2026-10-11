import { useMutation } from '@tanstack/react-query'
import type { AxiosError } from 'axios'
import { askLlm, type AskRequest, type AskLlmResult, type ApiResponse } from './api'

export function useAskLlm() {
  return useMutation<AskLlmResult, AxiosError<ApiResponse<null>>, AskRequest>({
    mutationFn: askLlm,
  })
}