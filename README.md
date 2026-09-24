# 🚂 RailOptima Launcher

> Web-лаунчер для умного проектирования железных дорог

[![React](https://img.shields.io/badge/React-19.3-61DAFB?logo=react&logoColor=white)](https://react.dev)
[![TypeScript](https://img.shields.io/badge/TypeScript-6.0-3178C6?logo=typescript&logoColor=white)](https://www.typescriptlang.org)
[![Vite](https://img.shields.io/badge/Vite-8.3-646CFF?logo=vite&logoColor=white)](https://vitejs.dev)
[![Tailwind](https://img.shields.io/badge/Tailwind-4.3-06B6D4?logo=tailwindcss&logoColor=white)](https://tailwindcss.com)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)

---

## 📖 О проекте

**RailOptima Launcher** — веб-приложение для инженеров-проектировщиков железных дорог. Позволяет:

- 🗂️ Управлять проектами (создание, просмотр, редактирование)
- 📊 Смотреть дашборд с аналитикой
- 💬 Общаться с ИИ-ассистентом по нормативам (СНиП/ГОСТ)
- 🔗 Синхронизировать данные с десктоп-клиентом на Unreal Engine

**Стек:** React 19 + TypeScript + Vite 8 + Tailwind v4.

---

## ⚡ Быстрый старт

### ⚠️ ВАЖНО

**После `git clone` обязательно выполните `pnpm install`** — без этого проект **не запустится**. Git не хранит `node_modules/` — зависимости устанавливаются отдельно.

---

### 1️⃣ Требования

| Инструмент | Версия | Как проверить |
| :--- | :--- | :--- |
| **Node.js** | 22+ | `node --version` |
| **pnpm** | 9+ | `pnpm --version` |
| **Git** | 2.30+ | `git --version` |

**Если Node.js не установлен:**

```bash
# Linux / macOS
curl -fsSL https://fnm.vercel.app/install | bash
source ~/.bashrc
fnm install 22
fnm use 22

# Windows
# Скачайте с https://nodejs.org
Если pnpm не установлен:

bash
corepack enable
corepack prepare pnpm@latest --activate
2️⃣ Клонирование
bash
git clone https://github.com/xxGonzalesxx/railoptima-launcher.git
cd railoptima-launcher
3️⃣ Установка зависимостей
Это обязательный шаг! ⚠️

bash
pnpm install
Что произойдёт:

📥 Скачаются ~500 МБ зависимостей (React, Vite, Tailwind и др.)

📁 Создастся node_modules/

🔒 Установятся точные версии из pnpm-lock.yaml

⏱️ Время: 30-60 секунд

Если нужно одобрить сборку esbuild:

bash
pnpm approve-builds
# Стрелками выберите esbuild → Space → Enter
4️⃣ Запуск
bash
pnpm dev
Откройте в браузере: http://localhost:5173

Готово! 🎉

🛠 Доступные команды
Команда	Что делает	Когда использовать
pnpm dev	Запускает dev-сервер с HMR	Во время разработки
pnpm build	Собирает production-билд в dist/	Перед деплоем
pnpm preview	Просмотр production-билда локально	После build
pnpm lint	Проверяет код через Oxlint	Перед коммитом
pnpm test	Запускает тесты (Vitest)	Перед коммитом
📦 Стек и зависимости
🎨 Runtime (dependencies)
Пакет	Версия	Назначение
react	^19.3.0	UI-библиотека
react-dom	^19.3.0	React для браузера
@tanstack/react-router	^1.170.39	Типизированная маршрутизация
@tanstack/react-query	^5.103.2	Кэш запросов и синхронизация с API
zustand	^5.0.15	Лёгкий state-менеджер
react-hook-form	^7.88.0	Работа с формами
zod	^4.6.5	Валидация данных
axios	^1.20.0	HTTP-клиент
🛠 Dev-зависимости (devDependencies)
Пакет	Версия	Назначение
vite	^8.3.0	Сборщик нового поколения (Rolldown)
@vitejs/plugin-react	^6.1.1	React-плагин для Vite
typescript	~6.0.2	Язык + компилятор
oxlint	^1.85.0	Быстрый линтер на Rust
tailwindcss	^4.3.3	CSS-фреймворк
@tailwindcss/vite	^4.3.3	Плагин Tailwind для Vite
orval	^8.37.0	Генератор API-клиента из Swagger
vitest	^5.0.1	Быстрый тест-раннер
@testing-library/react	^16.3.3	Тесты React-компонентов
@types/node	^24.13.5	Типы Node.js
@types/react	^19.3.0	Типы React
@types/react-dom	^19.3.0	Типы React DOM
🔄 Полная команда установки
Если нужно установить всё с нуля:

bash
# Runtime
pnpm add react react-dom \
  @tanstack/react-router @tanstack/react-query \
  zustand react-hook-form zod axios

# Dev
pnpm add -D vite @vitejs/plugin-react typescript oxlint \
  tailwindcss @tailwindcss/vite \
  orval vitest @testing-library/react \
  @types/node @types/react @types/react-dom
После этого:

bash
pnpm approve-builds
pnpm dev
📁 Структура проекта
text
railoptima-launcher/
├── public/                 # Статические файлы (favicon, robots.txt)
├── src/                    # Исходный код
│   ├── main.tsx            # Точка входа
│   ├── App.tsx             # Корневой компонент
│   ├── index.css           # Глобальные стили + Tailwind
│   └── vite-env.d.ts       # Типы Vite
├── index.html              # HTML-шаблон
├── package.json            # Манифест проекта
├── pnpm-lock.yaml          # Точные версии зависимостей
├── tsconfig.json           # Конфиг TypeScript
├── vite.config.ts          # Конфиг Vite
├── .oxlintrc.json          # Конфиг линтера
├── .gitignore              # Что НЕ коммитить
└── README.md               # Этот файл
⚠️ Обратите внимание:

node_modules/ не хранится в Git — устанавливается через pnpm install

dist/ не хранится в Git — создаётся через pnpm build

pnpm-lock.yaml в Git — обеспечивает одинаковые версии у всех

🎯 Стек — почему именно это
Технология	Почему выбрана
React 19.3	Актуальный, с React Compiler
TypeScript 6.0	Стабильный (не 7.0 — там ломается Oxlint)
Vite 8.3	Rolldown внутри — 10-30× быстрее сборки
Oxlint 1.85	В 50-100× быстрее ESLint
Tailwind v4.3	CSS-first, движок Oxide
TanStack Router	End-to-end типизация
TanStack Query v5	Кэш «из коробки»
Orval 8.37	Генерация API из Swagger бэкенда
Zustand 5	Простой, быстрый state
Zod 4	Валидация + типы
🔗 Связка с Backend
Backend (RailOptima API) — отдельный проект на Java 25 + Spring Boot 4.1.

Компонент	URL в dev	URL в prod
Frontend	http://localhost:3000	https://app.railoptima.ru
Backend	http://localhost:8080	https://api.railoptima.ru
Прокси в dev: Vite автоматически проксирует /api/* → http://localhost:8080.

Генерация API-клиента (Orval):

bash
# Убедитесь, что backend запущен на :8080
pnpm orval
Сгенерированные типы и хуки появятся в src/api/generated/.

🐛 Решение проблем
❌ pnpm: команда не найдена
bash
corepack enable
corepack prepare pnpm@latest --activate
❌ Cannot find module 'vite'
Забыли установить зависимости:

bash
pnpm install
❌ Error: Cannot find module ...
Удалить node_modules и переустановить:

bash
rm -rf node_modules pnpm-lock.yaml
pnpm install
❌ ERR_PNPM_IGNORED_BUILDS
Одобрить сборку:

bash
pnpm approve-builds
# Выбрать esbuild стрелками → Space → Enter
❌ Порт 5173 занят
Vite автоматически предложит другой порт. Или:

bash
pnpm dev --port 3000
❌ Ошибки TypeScript
Проверьте версию:

bash
pnpm list typescript
# Должно быть 6.0.3
Если 7.0 — откатитесь:

bash
pnpm add -D typescript@~6.0.2
Причина: TS 7.0 ломает Oxlint и другие тулы.