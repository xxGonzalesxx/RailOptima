package api.project.repository;

import api.project.model.Project;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.UUID;

@Repository
public interface ProjectRepository extends JpaRepository<Project, UUID> {

    /**
     * Поиск всех проектов конкретного пользователя с поддержкой пагинации.
     * Запрос отработает мгновенно благодаря составному индексу idx_project_owner.
     */
    Page<Project> findAllByOwnerId(UUID ownerId, Pageable pageable);
}
