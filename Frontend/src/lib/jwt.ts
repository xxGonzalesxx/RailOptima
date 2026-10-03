/**
 * Безопасное хранилище JWT-токена.
 * Токен хранится в замыкании модуля — недоступен из window.
 */

let accessToken: string | null = null

export const tokenStore = {
  get: (): string | null => accessToken,
  set: (token: string | null): void => {
    accessToken = token
  },
  clear: (): void => {
    accessToken = null
  },
  has: (): boolean => accessToken !== null,
}

