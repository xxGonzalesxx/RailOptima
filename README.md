# 🚂 RailOptima

[![Java](https://img.shields.io/badge/Java-25_LTS-orange)](https://openjdk.org)
[![Spring Boot](https://img.shields.io/badge/Spring_Boot-4.1.1-brightgreen)](https://spring.io)
[![Gradle](https://img.shields.io/badge/Gradle-9.8-02303A)](https://gradle.org)
[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-18--alpine-336791)](https://postgresql.org)
[![React](https://img.shields.io/badge/React-19-61DAFB)](https://react.dev)
[![Vite](https://img.shields.io/badge/Vite-8-646CFF)](https://vite.dev)

Монорепозиторий проекта **RailOptima** — высокопроизводительной платформы для управления железнодорожными проектами. Включает REST API (Spring Boot), веб-интерфейс (React + Vite) и задел под Unity-клиент (3D-визуализация).

---

## 📁 Структура репозитория

```text
RailOptima/
├── Backend/                    # Spring Boot приложение (Java 25)
│   └── src/main/
│       ├── java/api/           # Исходный код (auth, project, common)
│       └── resources/          # application.yml + Flyway-миграции
├── Frontend/                   # React + Vite + TypeScript
│   ├── src/
│   │   ├── api/generated/      # Orval-клиент (генерируется, не коммитится)
│   │   ├── lib/                # axios-мутатор, jwt-хранилище
│   │   ├── components/         # UI-компоненты (пусто — заполняется)
│   │   ├── pages/              # Страницы (пусто — заполняется)
│   │   ├── hooks/              # Кастомные хуки (пусто — заполняется)
│   │   ├── services/           # Бизнес-логика (пусто — заполняется)
│   │   ├── types/              # Общие типы (пусто — заполняется)
│   │   └── assets/
│   ├── orval.config.ts         # Генерация API-клиента из OpenAPI
│   └── vite.config.ts          # Alias @/ → src/, прокси /api → :8080
├── build.gradle.kts            # Корневой Gradle (sourceSets на Backend/src)
├── settings.gradle.kts
├── gradlew / gradlew.bat
├── gradle/                     # Wrapper + libs.versions.toml
├── docker-compose.yml          # PostgreSQL 18
├── .env                        # Секреты (не в git)
├── .env.example                # Шаблон
└── README.md
```

> **Gradle в корне репозитория.** Все команды бэкенда (`./gradlew bootRun`, `./gradlew build`) выполняются **из корня** `RailOptima/`, а не из `Backend/`.

---

## 🛠️ Технологический стек

| Компонент | Технология | Версия |
| :--- | :--- | :--- |
| Бэкенд-платформа | Java (LTS) | **25** |
| Фреймворк | Spring Boot | **4.1.1** |
| Сборка | Gradle (Kotlin DSL) | **9.8** |
| БД | PostgreSQL | **18-alpine** |
| Миграции | Flyway | (из Spring Boot BOM) |
| JWT | JJWT | **0.13.0** |
| OpenAPI | springdoc-openapi | **3.1.1** |
| Веб-клиент | React + TypeScript | **19 / 6.0** |
| Сборщик фронта | Vite | **8** |
| Генератор API-клиента | Orval | **8.37** |
| Пакетный менеджер | pnpm | **12** |
| Десктоп-клиент | Unity + C# | 2022 LTS (в планах) |

---

## 🚀 Быстрый старт

### Требования

- **JDK 25** (`java -version` → 25.x)
- **Docker** + **Docker Compose**
- **pnpm** (для фронта)

### 1. Клонировать репозиторий

```bash
git clone <repo-url> RailOptima
cd RailOptima
```

### 2. Настроить окружение

```bash
cp .env.example .env
# Открой .env и при необходимости поменяй POSTGRES_PASSWORD, JWT_SECRET, DB_*
```

`.env` используется **и Docker Compose** (для Postgres), **и Spring Boot** (для datasource, JWT, CORS).

### 3. Поднять PostgreSQL

```bash
docker compose up -d
docker compose ps   # railoptima-db должен быть Up
```

### 4. Запустить бэкенд

```bash
./gradlew bootRun
```

Проверка:
```bash
curl http://localhost:8080/actuator/health
# {"status":"UP"}
```

Swagger UI: <http://localhost:8080/swagger-ui.html>

Flyway автоматически применит миграции `V1__init.sql` → `V3__add_projects_owner_fk.sql` при первом запуске.

### 5. Запустить фронтенд

```bash
cd Frontend
pnpm install
pnpm dev
```

Открыть: <http://localhost:3000>

Vite проксирует `/api/*` → `http://localhost:8080` (см. `Frontend/vite.config.ts`).

---

## 📜 Команды

### Бэкенд (из корня репозитория)

| Команда | Действие |
| :--- | :--- |
| `./gradlew build` | Собрать jar + прогнать тесты |
| `./gradlew clean build` | Полная пересборка |
| `./gradlew bootRun` | Запустить приложение |
| `./gradlew test` | Только тесты |

### Frontend (из `Frontend/`)

| Команда | Действие |
| :--- | :--- |
| `pnpm install` | Установить зависимости |
| `pnpm dev` | Dev-сервер на `:3000` |
| `pnpm build` | Production-сборка в `dist/` |
| `pnpm preview` | Локальный просмотр `dist/` |
| `pnpm lint` | Oxlint |
| `pnpm api:generate` | Пересобрать API-клиент через Orval |

### Docker

| Команда | Действие |
| :--- | :--- |
| `docker compose up -d` | Запустить Postgres |
| `docker compose down` | Остановить |
| `docker compose down -v` | Остановить + удалить volume (данные БД) |
| `docker compose logs -f postgres` | Логи БД |

---

## 🔄 Генерация API-клиента (Orval)

Orval читает OpenAPI-спеку и генерирует типизированный React Query-клиент в `Frontend/src/api/generated/`.

**Способ 1 — через файл `swagger.json`** (текущий):

```bash
# Бэкенд должен быть запущен
curl -o Frontend/swagger.json http://localhost:8080/v3/api-docs

cd Frontend
pnpm api:generate
```

**Способ 2 — напрямую с URL** — поменять в `orval.config.ts`:

```ts
input: { target: 'http://localhost:8080/v3/api-docs' },
```

**Важно:** `Frontend/src/api/generated/` **не коммитится** (в `.gitignore`). Каждый разработчик генерирует клиент локально после `pnpm install`.

---

## 🏛️ Архитектура

**Модульный монолит** с изоляцией контекстов. Каждый модуль (`auth`, `project`, `common`) независим, общается через плоские DTO (Java Records) и UUID.

```text
┌────────────────────────────────────────────┐
│              КЛИЕНТЫ                        │
│  React (v19)          Unity (C#)           │
└──────────────┬─────────────────────────────┘
               │ REST API + JWT
               ▼
┌────────────────────────────────────────────┐
│         БЭКЕНД (Spring Boot 4)             │
│                                            │
│  ┌────────┐  ┌─────────┐  ┌──────────┐    │
│  │  AUTH  │  │ PROJECT │  │   LLM    │    │
│  └────────┘  └─────────┘  └──────────┘    │
│  ┌────────────────────────────────────┐   │
│  │ COMMON (ApiResponse, Exceptions)    │   │
│  └────────────────────────────────────┘   │
└──────┬──────────────────────┬──────────────┘
       │                      │
       ▼                      ▼
  PostgreSQL 18         Внешний LLM API
```

### Реализовано

**Модуль `COMMON`**
- `ApiResponse<T>` и `PageResponse<T>` — иммутабельные Java Records, timestamp в UTC ISO-8601
- `GlobalExceptionHandler` — централизованная обработка ошибок с корректными HTTP-статусами
- Иерархия исключений: `ConflictException` (409), `ForbiddenException` (403), `ResourceNotFoundException` (404), `ValidationException` (400)
- `SecurityConfig` с CORS, BCrypt (rounds=12)

**Модули `AUTH` + `PROJECT`**
- Stateless JWT (JJWT 0.13.0) — UUID пользователя зашит в токен, без похода в БД на каждый запрос
- JPA-сущности `User`, `Project` с индексами под пагинацию (`idx_user_email`, `idx_project_owner`, `idx_project_created_at`)
- DTO-рекорды: `RegisterRequest`, `LoginRequest`, `ProjectCreateRequest`, `ProjectUpdateRequest` и др.
- Flyway-миграции `V1`–`V3`

### В планах

1. `ProjectService` и полный CRUD с проверкой прав собственности
2. Мониторинг через Actuator
3. Модуль `LLM` (DeepSeek API / локальный Ollama)
4. Локализация ошибок валидации (`validationErrors` в ответах)
5. Облачная миграция БД (Neon / Supabase / Yandex Cloud) с `sslmode=require`

---

## 🔐 Безопасность

- **`.env` не коммитится** (в `.gitignore`), в репозитории только `.env.example`
- Секреты: `POSTGRES_PASSWORD`, `JWT_SECRET`, `DB_*`
- JWT stateless, BCrypt для паролей
- CORS настроен через `CORS_ALLOWED_ORIGINS` в `.env`

**Перед деплоем** обязательно сгенерировать новые секреты:
```bash
openssl rand -base64 32   # JWT_SECRET
openssl rand -hex 16      # POSTGRES_PASSWORD
```

---

## 🧹 Что игнорируется Git

- `build/` — артефакты Gradle
- `.gradle/` — кэш Gradle
- `.idea/`, `*.iml` — IntelliJ
- `Frontend/node_modules/`, `Frontend/dist/`
- `Frontend/swagger.json`
- `Frontend/src/api/generated/`
- `.env`

---

## 📌 Соглашения

- **Java-код** — только в `Backend/src/main/java/api/...`
- **Gradle-задачи** — запускаются из корня
- **Фронтенд-команды** — из `Frontend/`
- **DTO** — Java Records, не классы
- **Даты** — только `Instant` (UTC)
- **UUID** — версия 7 (time-ordered)

---

## 📄 Лицензия

Приватный проект. Все права защищены.