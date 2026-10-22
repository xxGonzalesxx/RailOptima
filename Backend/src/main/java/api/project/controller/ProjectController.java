package api.project.controller;

import api.common.dto.ApiResponse;
import api.common.dto.PageResponse;
import api.project.dto.ProjectCreateRequest;
import api.project.dto.ProjectResponse;
import api.project.dto.ProjectUpdateRequest;
import api.project.service.ProjectService;
import jakarta.annotation.security.RolesAllowed;
import jakarta.inject.Inject;
import jakarta.validation.Valid;
import jakarta.ws.rs.*;
import jakarta.ws.rs.core.MediaType;
import jakarta.ws.rs.core.Response;
import org.eclipse.microprofile.jwt.JsonWebToken;

import java.util.UUID;

@Path("/api/v1/projects")
@Produces(MediaType.APPLICATION_JSON)
@Consumes(MediaType.APPLICATION_JSON)
@RolesAllowed("USER")
public class ProjectController {

    @Inject
    ProjectService projectService;

    @Inject
    JsonWebToken jwt; // Вместо Spring Authentication

    @POST
    public Response create(@Valid ProjectCreateRequest request) {
        // jwt.getName() вернет subject (id пользователя), как Authentication.getName() в Spring
        ProjectResponse response = projectService.createProject(request, jwt.getName());

        return Response.status(Response.Status.CREATED)
                .entity(ApiResponse.success(response))
                .build();
    }

    @GET
    public ApiResponse<PageResponse<ProjectResponse>> getMyProjects(
            @QueryParam("page") @DefaultValue("0") int pageIndex,
            @QueryParam("size") @DefaultValue("20") int pageSize
    ) {
        PageResponse<ProjectResponse> response = projectService.getMyProjects(jwt.getName(), pageIndex, pageSize);
        return ApiResponse.success(response);
    }

    @GET
    @Path("/{id}")
    public ApiResponse<ProjectResponse> getOne(@PathParam("id") UUID id) {
        ProjectResponse response = projectService.getProject(id, jwt.getName());
        return ApiResponse.success(response);
    }

    @PUT
    @Path("/{id}")
    public ApiResponse<ProjectResponse> update(
            @PathParam("id") UUID id,
            @Valid ProjectUpdateRequest request
    ) {
        ProjectResponse response = projectService.updateProject(id, request, jwt.getName());
        return ApiResponse.success(response);
    }

    @DELETE
    @Path("/{id}")
    public Response delete(@PathParam("id") UUID id) {
        projectService.deleteProject(id, jwt.getName());
        return Response.noContent().build(); // Аналог ResponseEntity.noContent()
    }
}