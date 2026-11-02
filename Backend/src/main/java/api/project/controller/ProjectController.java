
package api.project.controller;

import api.common.dto.ApiResponse;
import api.common.dto.PageResponse;
import api.project.dto.ProjectCreateRequest;
import api.project.dto.ProjectResponse;
import api.project.dto.ProjectUpdateRequest;
import api.project.service.ProjectService;
import io.smallrye.mutiny.Uni;
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
    JsonWebToken jwt;

    @POST
    public Uni<Response> create(@Valid ProjectCreateRequest request) {
        return projectService.createProject(request, jwt.getName())
                .map(response -> Response.status(Response.Status.CREATED)
                        .entity(ApiResponse.success(response))
                        .build());
    }

    @GET
    public Uni<ApiResponse<PageResponse<ProjectResponse>>> getMyProjects(
            @QueryParam("page") @DefaultValue("0") int pageIndex,
            @QueryParam("size") @DefaultValue("20") int pageSize
    ) {
        return projectService.getMyProjects(jwt.getName(), pageIndex, pageSize)
                .map(ApiResponse::success);
    }

    @GET
    @Path("/{id}")
    public Uni<ApiResponse<ProjectResponse>> getOne(
            @PathParam("id") UUID id
    ) {
        return projectService.getProject(id, jwt.getName())
                .map(ApiResponse::success);
    }

    @PUT
    @Path("/{id}")
    public Uni<ApiResponse<ProjectResponse>> update(
            @PathParam("id") UUID id,
            @Valid ProjectUpdateRequest request
    ) {
        return projectService.updateProject(id, request, jwt.getName())
                .map(ApiResponse::success);
    }

    @DELETE
    @Path("/{id}")
    public Uni<Response> delete(@PathParam("id") UUID id) {
        return projectService.deleteProject(id, jwt.getName())
                .replaceWith(Response.noContent().build());
    }
}
