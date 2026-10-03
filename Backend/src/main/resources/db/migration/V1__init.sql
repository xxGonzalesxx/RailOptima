CREATE TABLE users (
                       id          UUID PRIMARY KEY,
                       email       VARCHAR(150) NOT NULL,
                       password    VARCHAR(100) NOT NULL,
                       role        VARCHAR(20)  NOT NULL,
                       created_at  TIMESTAMP(6) WITH TIME ZONE NOT NULL,
                       updated_at  TIMESTAMP(6) WITH TIME ZONE NOT NULL
);

CREATE UNIQUE INDEX idx_user_email ON users (email);

CREATE TABLE projects (
                          id           UUID PRIMARY KEY,
                          name         VARCHAR(100) NOT NULL,
                          description  VARCHAR(500),
                          status       VARCHAR(30)  NOT NULL,
                          owner_id     UUID NOT NULL,
                          created_at   TIMESTAMP(6) WITH TIME ZONE NOT NULL,
                          updated_at   TIMESTAMP(6) WITH TIME ZONE NOT NULL,

                          CONSTRAINT projects_status_check
                              CHECK (status IN ('CREATED', 'IN_PROGRESS', 'COMPLETED'))
);

CREATE INDEX idx_project_owner_created ON projects (owner_id, created_at);