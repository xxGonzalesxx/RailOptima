import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'
import path from 'node:path'

export default defineConfig({
  plugins: [
    react(),
    tailwindcss(),
  ],

  resolve: {
    alias: {
      '@': path.resolve(import.meta.dirname, './src'),  
    },
  },

  server: {
    port: 3000,

    proxy: {
      '/api': {
        // В продакшене здесь будет https://api.railoptima.ru
        // В разработке — локальный backend на :8080
        target: 'http://localhost:8080',

        // Меняет заголовок Host в запросе на localhost:8080
        // Нужно, если backend проверяет Host (защита от подмены)
        changeOrigin: true,

        // Отключает проверку SSL (для self-signed сертификатов)
        secure: false,
      },
    },
  },
})