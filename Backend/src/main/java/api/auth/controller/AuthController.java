
package api.auth.controller;

import api.auth.dto.AuthResponse;
import api.auth.dto.LoginRequest;
import api.auth.dto.RegisterRequest;
import api.auth.service.AuthService;
import api.common.dto.ApiResponse;
import io.smallrye.mutiny.Uni;
import jakarta.annotation.security.PermitAll;
import jakarta.inject.Inject;
import jakarta.validation.Valid;
import jakarta.ws.rs.Consumes;
import jakarta.ws.rs.POST;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.Produces;
import jakarta.ws.rs.core.MediaType;
import jakarta.ws.rs.core.Response;

@Path("/api/v1/auth")
@Produces(MediaType.APPLICATION_JSON)
@Consumes(MediaType.APPLICATION_JSON)
@PermitAll
public class AuthController {

    @Inject
    AuthService authService;

    @POST
    @Path("/register")
    public Uni<Response> register(@Valid RegisterRequest request) {
        return authService.register(request)
                .map(response -> Response.status(Response.Status.CREATED)
                        .entity(ApiResponse.success(response))
                        .build());
    }

    @POST
    @Path("/login")
    public Uni<ApiResponse<AuthResponse>> login(@Valid LoginRequest request) {
        return authService.login(request)
                .map(ApiResponse::success);
    }
}
