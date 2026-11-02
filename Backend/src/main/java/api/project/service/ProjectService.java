
package api.project.service;

import api.auth.repository.UserRepository;
import api.common.dto.PageResponse;
import api.common.exception.ForbiddenException;
import api.common.exception.ResourceNotFoundException;
import api.project.dto.ProjectCreateRequest;
import api.project.dto.ProjectResponse;
import api.project.dto.ProjectUpdateRequest;
import api.project.model.Project;
import api.project.model.ProjectStatus;
import api.project.repository.ProjectRepository;
import io.quarkus.hibernate.reactive.panache.common.WithSession;
import io.quarkus.hibernate.reactive.panache.common.WithTransaction;
import io.quarkus.panache.common.Page;
import io.quarkus.panache.common.Sort;
import io.smallrye.mutiny.Uni;
import jakarta.enterprise.context.ApplicationScoped;
import jakarta.inject.Inject;
import org.jboss.logging.Logger;

import java.util.List;
import java.util.UUID;

@ApplicationScoped
public class ProjectService {

    private static final Logger log = Logger.getLogger(ProjectService.class);

    @Inject
    ProjectRepository projectRepository;

    @Inject
    UserRepository userRepository;

    @WithTransaction
    public Uni<ProjectResponse> createProject(
            ProjectCreateRequest request,
            String userEmail
    ) {
        return resolveUserId(userEmail)
                .chain(ownerId -> {
                    Project project = new Project();
                    project.setName(request.name());
                    project.setDescription(request.description());
                    project.setStatus(ProjectStatus.CREATED);
                    project.setOwnerId(ownerId);

                    return projectRepository.persist(project)
                            .replaceWith(() -> {
                                log.infof(
                                        "Project '%s' created by user %s",
                                        project.getName(),
                                        ownerId
                                );
                                return toResponse(project);
                            });
                });
    }

    @WithSession
    public Uni<PageResponse<ProjectResponse>> getMyProjects(
            String userEmail,
            int pageIndex,
            int pageSize
    ) {
        if (pageIndex < 0 || pageSize < 1) {
            return Uni.createFrom().failure(
                    new IllegalArgumentException("Некорректные параметры пагинации")
            );
        }

        return resolveUserId(userEmail)
                .chain(ownerId -> {
                    var query = projectRepository.find(
                            "ownerId",
                            Sort.by("createdAt").descending(),
                            ownerId
                    );

                    query.page(Page.of(pageIndex, pageSize));

                    return query.list()
                            .chain(projects -> query.count()
                                    .map(total -> {
                                        List<ProjectResponse> content = projects.stream()
                                                .map(this::toResponse)
                                                .toList();

                                        return PageResponse.from(
                                                content,
                                                total,
                                                pageIndex,
                                                pageSize
                                        );
                                    }));
                });
    }

    @WithSession
    public Uni<ProjectResponse> getProject(
            UUID projectId,
            String userEmail
    ) {
        return resolveUserId(userEmail)
                .chain(ownerId -> findOwnedProject(projectId, ownerId))
                .map(this::toResponse);
    }

    @WithTransaction
    public Uni<ProjectResponse> updateProject(
            UUID projectId,
            ProjectUpdateRequest request,
            String userEmail
    ) {
        return resolveUserId(userEmail)
                .chain(ownerId -> findOwnedProject(projectId, ownerId)
                        .map(project -> {
                            project.setName(request.name());
                            project.setDescription(request.description());
                            project.setStatus(request.status());

                            log.infof(
                                    "Project %s updated by user %s",
                                    projectId,
                                    ownerId
                            );

                            return toResponse(project);
                        }));
    }


    @WithTransaction
    public Uni<Void> deleteProject(UUID projectId, String userEmail) {
        return resolveUserId(userEmail)
                .chain(ownerId -> findOwnedProject(projectId, ownerId)
                        .call(project -> projectRepository.delete(project))
                        .invoke(() -> log.infof(
                                "Project %s deleted by user %s",
                                projectId,
                                ownerId
                        )))
                .replaceWithVoid();
    }


    private Uni<Project> findOwnedProject(
            UUID projectId,
            UUID currentUserId
    ) {
        return projectRepository.findById(projectId)
                .onItem().ifNull().failWith(
                        () -> new ResourceNotFoundException("Project", projectId)
                )
                .map(project -> {
                    if (!project.getOwnerId().equals(currentUserId)) {
                        throw new ForbiddenException(
                                "У вас нет доступа к этому проекту"
                        );
                    }
                    return project;
                });
    }

    private Uni<UUID> resolveUserId(String email) {
        return userRepository.findByEmail(email)
                .onItem().transformToUni(optionalUser -> {
                    if (optionalUser.isEmpty()) {
                        return Uni.createFrom().failure(
                                new ResourceNotFoundException("User", email)
                        );
                    }

                    return Uni.createFrom().item(
                            optionalUser.get().getId()
                    );
                });
    }

    private ProjectResponse toResponse(Project project) {
        return new ProjectResponse(
                project.getId(),
                project.getName(),
                project.getDescription(),
                project.getStatus(),
                project.getOwnerId(),
                project.getCreatedAt(),
                project.getUpdatedAt()
        );
    }
}
