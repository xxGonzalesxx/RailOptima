package api.llm.controller;

import api.common.dto.ApiResponse;
import api.llm.dto.AskRequest;
import api.llm.dto.AskResponse;
import api.llm.service.LlmService;
import jakarta.annotation.security.PermitAll;
import jakarta.ws.rs.Produces;
import jakarta.validation.Valid;
import jakarta.ws.rs.Consumes;
import jakarta.ws.rs.POST;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.core.MediaType;
import org.eclipse.microprofile.openapi.annotations.parameters.RequestBody;

@Path("/api/v1/llm")
@Produces(MediaType.APPLICATION_JSON)
@Consumes(MediaType.APPLICATION_JSON)
@PermitAll
public class LlmController {

    private final LlmService llmService;

    public LlmController(LlmService llmService) {
        this.llmService = llmService;
    }

    @POST
    @Path("/ask")
    public ApiResponse<AskResponse> ask(@Valid @RequestBody AskRequest request) {
        String answer = llmService.ask(request.question());
        return ApiResponse.success(new AskResponse(answer));
    }
}