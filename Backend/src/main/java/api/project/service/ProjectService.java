package api.project.service;

import api.auth.repository.UserRepository;
import api.common.exception.ForbiddenException;
import api.common.exception.ResourceNotFoundException;
import api.project.dto.ProjectCreateRequest;
import api.project.dto.ProjectResponse;
import api.project.dto.ProjectUpdateRequest;
import api.project.model.Project;
import api.project.model.ProjectStatus;
import api.project.repository.ProjectRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.UUID;

@Service
@RequiredArgsConstructor
@Slf4j
public class ProjectService {

    private final ProjectRepository projectRepository;
    private final UserRepository userRepository;

    @Transactional
    public ProjectResponse createProject(ProjectCreateRequest request, String userEmail) {
        UUID ownerId = resolveUserId(userEmail);

        Project project = Project.builder()
                .name(request.name())
                .description(request.description())
                .status(ProjectStatus.CREATED)
                .ownerId(ownerId)
                .build();

        Project saved = projectRepository.saveAndFlush(project);  // ← flush сразу
        log.info("Project '{}' created by user {}", saved.getName(), ownerId);

        return toResponse(saved);
    }

    @Transactional(readOnly = true)
    public Page<ProjectResponse> getMyProjects(String userEmail, Pageable pageable) {
        UUID ownerId = resolveUserId(userEmail);
        return projectRepository.findAllByOwnerId(ownerId, pageable)
                .map(this::toResponse);
    }

    @Transactional(readOnly = true)
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

        Project updated = projectRepository.saveAndFlush(project);
        log.info("Project {} updated by user {}", projectId, currentUserId);

        return toResponse(updated);
    }

    @Transactional
    public void deleteProject(UUID projectId, String userEmail) {
        UUID currentUserId = resolveUserId(userEmail);
        Project project = findOwnedProject(projectId, currentUserId);

        projectRepository.delete(project);
        log.info("Project {} deleted by user {}", projectId, currentUserId);
    }

    // --- private helpers ---

    private Project findOwnedProject(UUID projectId, UUID currentUserId) {
        Project project = projectRepository.findById(projectId)
                .orElseThrow(() -> new ResourceNotFoundException("Project", projectId));

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