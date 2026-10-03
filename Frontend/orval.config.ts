import { defineConfig } from 'orval'

export default defineConfig({
  railoptima: {
    input: {
      target: './swagger.json',
    },
    output: {
      mode: 'tags-split',
      target: './src/api/generated/endpoints.ts',
      schemas: './src/api/generated/models',
      client: 'react-query',
      mock: false,
      override: {
        mutator: {
          path: './src/lib/axios.ts',
          name: 'customInstance',
        },
        // ⬇️ ВАЖНО: указать какие операции — mutation
        operations: {
          // POST-запросы → useMutation
          register: { query: { useQuery: false, useMutation: true } },
          login:    { query: { useQuery: false, useMutation: true } },
          create:   { query: { useQuery: false, useMutation: true } },
          update:   { query: { useQuery: false, useMutation: true } },
          delete:   { query: { useQuery: false, useMutation: true } },
          // GET-запросы → useQuery (остаются по умолчанию)
        },
        query: {
          useQuery: true,
          useMutation: true,
          signal: true,
        },
      },
    },
  },
})