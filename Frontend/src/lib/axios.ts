// Frontend/src/lib/axios.ts
import axios, { type AxiosError, type AxiosRequestConfig, type InternalAxiosRequestConfig } from 'axios'
import { tokenStore } from './jwt'

export const api = axios.create({
  baseURL: '/api/v1',
  headers: { 'Content-Type': 'application/json' },
  timeout: 30_000,
})

api.interceptors.request.use(
  (config: InternalAxiosRequestConfig) => {
    const token = tokenStore.get()
    if (token) config.headers.Authorization = `Bearer ${token}`
    return config
  },
  (error) => Promise.reject(error),
)

api.interceptors.response.use(
  (response) => response,
  (error: AxiosError) => {
    if (error.response?.status === 401) tokenStore.clear()
    return Promise.reject(error)
  },
)

/**
 * Orval-мутатор: fetch-подобная сигнатура, возвращает распакованные данные.
 */
export const customInstance = async <T>(
  url: string,
  options?: RequestInit & { params?: Record<string, unknown> },
): Promise<T> => {
  const config: AxiosRequestConfig = {
    url,
    method: (options?.method as AxiosRequestConfig['method']) ?? 'GET',
    headers: options?.headers as AxiosRequestConfig['headers'],
    data: options?.body ? JSON.parse(options.body as string) : undefined,
    params: options?.params,
    signal: options?.signal ?? undefined,
  }

  const response = await api.request<T>(config)
  return response.data
}

export default customInstance