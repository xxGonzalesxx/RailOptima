package api.project.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

public record ProjectUpdateRequest(
        @NotBlank(message = "Название проекта не может быть пустым")
        @Size(max = 100, message = "Название проекта не должно превышать 100 символов")
        String name,

        @Size(max = 500, message = "Описание проекта не должно превышать 500 символов")
        String description,

        @NotBlank(message = "Статус проекта не может быть пустым")
        String status // Например: CREATED, IN_PROGRESS, COMPLETED
) {}
