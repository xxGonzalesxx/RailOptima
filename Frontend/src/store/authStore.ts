import { create } from 'zustand'
import { tokenStore } from '@/lib/jwt'

interface AuthState {
  isAuthenticated: boolean
  login: (token: string) => void
  logout: () => void
}

export const useAuthStore = create<AuthState>((set) => ({
  isAuthenticated: tokenStore.has(),
  login: (token: string) => {
    tokenStore.set(token)
    set({ isAuthenticated: true })
  },
  logout: () => {
    tokenStore.clear()
    set({ isAuthenticated: false })
  },
}))