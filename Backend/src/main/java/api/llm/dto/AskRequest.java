package api.llm.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

public record AskRequest(
        @NotBlank(message = "Вопрос не может быть пустым")
        @Size(max = 2000, message = "Максимум 2000 символов")
        String question
) {}