package api.llm.controller;

import api.common.dto.ApiResponse;
import api.llm.dto.AskRequest;
import api.llm.dto.AskResponse;
import api.llm.service.LlmService;
import jakarta.validation.Valid;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/v1/llm")
public class LlmController {

    private final LlmService llmService;

    public LlmController(LlmService llmService) {
        this.llmService = llmService;
    }

    @PostMapping("/ask")
    public ApiResponse<AskResponse> ask(@Valid @RequestBody AskRequest request) {
        String answer = llmService.ask(request.question());
        return ApiResponse.success(new AskResponse(answer));
    }
}