ALTER TABLE projects
    ADD CONSTRAINT fk_projects_owner
        FOREIGN KEY (owner_id) REFERENCES users(id)
            ON DELETE RESTRICT;