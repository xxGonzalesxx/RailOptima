import { useMutation } from '@tanstack/react-query'
import type { AxiosError } from 'axios'
import { askLlm, type AskRequest, type AskResponse, type ApiResponse } from './api'

export function useAskLlm() {
  return useMutation<ApiResponse<AskResponse>, AxiosError<ApiResponse<null>>, AskRequest>({
    mutationFn: askLlm,
  })
}