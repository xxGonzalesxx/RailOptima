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
import io.quarkus.panache.common.Page;
import io.quarkus.panache.common.Sort;
import jakarta.enterprise.context.ApplicationScoped;
import jakarta.inject.Inject;
import jakarta.transaction.Transactional;
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

    @Transactional
    public ProjectResponse createProject(ProjectCreateRequest request, String userEmail) {
        UUID ownerId = resolveUserId(userEmail);

        Project project = new Project();
        project.setName(request.name());
        project.setDescription(request.description());
        project.setStatus(ProjectStatus.CREATED);
        project.setOwnerId(ownerId);

        projectRepository.persist(project);
        log.infof("Project '%s' created by user %s", project.getName(), ownerId);

        return toResponse(project);
    }

    @Transactional
    public PageResponse<ProjectResponse> getMyProjects(String userEmail, int pageIndex, int pageSize) {
        UUID ownerId = resolveUserId(userEmail);

        // PanacheQuery для выборки с пагинацией
        var query = projectRepository.find("ownerId", Sort.by("createdAt").descending(), ownerId);

        query.page(Page.of(pageIndex, pageSize));

        List<ProjectResponse> content = query.list().stream()
                .map(this::toResponse)
                .toList();

        long totalElements = query.count();

        return PageResponse.from(content, totalElements, pageIndex, pageSize);
    }

    @Transactional
    public ProjectResponse getProject(UUID projectId, String userEmail) {
        UUID currentUserId = resolveUserId(userEmail);
        Project project = findOwnedProject(projectId, currentUserId);
        return toResponse(project);
    }

    @Transactional
    public ProjectResponse updateProject(UUID projectId, ProjectUpdateRequest request, String userEmail) {
        UUID currentUserId = resolveUserId(userEmail);
        Project project = findOwnedProject(projectId, currentUserId);

        project.setName(request.name());
        project.setDescription(request.description());
        project.setStatus(request.status());

        // В Panache сущность обновляется автоматически, если она в контексте (managed).
        // Явно вызывать save не нужно, но для надежности:
        projectRepository.persist(project);
        log.infof("Project %s updated by user %s", projectId, currentUserId);

        return toResponse(project);
    }

    @Transactional
    public void deleteProject(UUID projectId, String userEmail) {
        UUID currentUserId = resolveUserId(userEmail);
        Project project = findOwnedProject(projectId, currentUserId);

        projectRepository.delete(project);
        log.infof("Project %s deleted by user %s", projectId, currentUserId);
    }

    // --- private helpers ---

    private Project findOwnedProject(UUID projectId, UUID currentUserId) {
        Project project = projectRepository.findById(projectId);
        if (project == null) {
            throw new ResourceNotFoundException("Project", projectId);
        }

        if (!project.getOwnerId().equals(currentUserId)) {
            throw new ForbiddenException("У вас нет доступа к этому проекту");
        }
        return project;
    }

    private UUID resolveUserId(String email) {
        return userRepository.findByEmail(email)
                .orElseThrow(() -> new ResourceNotFoundException("User", email))
                .getId();
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