package api.project.controller;

import api.common.dto.ApiResponse;
import api.common.dto.PageResponse;
import api.project.dto.ProjectCreateRequest;
import api.project.dto.ProjectResponse;
import api.project.dto.ProjectUpdateRequest;
import api.project.service.ProjectService;
import jakarta.validation.Valid;
import lombok.RequiredArgsConstructor;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.data.domain.Sort;
import org.springframework.data.web.PageableDefault;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.web.bind.annotation.*;

import java.util.UUID;

@RestController
@RequestMapping("/api/v1/projects")
@RequiredArgsConstructor
public class ProjectController {

    private final ProjectService projectService;

    @PostMapping
    public ResponseEntity<ApiResponse<ProjectResponse>> create(
            @Valid @RequestBody ProjectCreateRequest request,
            Authentication authentication
    ) {
        ProjectResponse response = projectService.createProject(request, authentication.getName());
        return ResponseEntity.status(HttpStatus.CREATED).body(ApiResponse.success(response));
    }

    @GetMapping
    public ResponseEntity<ApiResponse<PageResponse<ProjectResponse>>> getMyProjects(
            Authentication authentication,
            @PageableDefault(size = 20, sort = "createdAt", direction = Sort.Direction.DESC) Pageable pageable
    ) {
        Page<ProjectResponse> page = projectService.getMyProjects(authentication.getName(), pageable);
        return ResponseEntity.ok(ApiResponse.success(PageResponse.from(page)));
    }

    @GetMapping("/{id}")
    public ResponseEntity<ApiResponse<ProjectResponse>> getOne(
            @PathVariable UUID id,
            Authentication authentication
    ) {
        ProjectResponse response = projectService.getProject(id, authentication.getName());
        return ResponseEntity.ok(ApiResponse.success(response));
    }

    @PutMapping("/{id}")
    public ResponseEntity<ApiResponse<ProjectResponse>> update(
            @PathVariable UUID id,
            @Valid @RequestBody ProjectUpdateRequest request,
            Authentication authentication
    ) {
        ProjectResponse response = projectService.updateProject(id, request, authentication.getName());
        return ResponseEntity.ok(ApiResponse.success(response));
    }

    @DeleteMapping("/{id}")
    public ResponseEntity<Void> delete(
            @PathVariable UUID id,
            Authentication authentication
    ) {
        projectService.deleteProject(id, authentication.getName());
        return ResponseEntity.noContent().build();
    }
}