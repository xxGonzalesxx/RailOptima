package api.project.dto;

import api.project.model.ProjectStatus;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Size;

public record ProjectUpdateRequest(
        @NotBlank(message = "Название проекта не может быть пустым")
        @Size(max = 100, message = "Название проекта не должно превышать 100 символов")
        String name,

        @Size(max = 500, message = "Описание проекта не должно превышать 500 символов")
        String description,

        @NotNull(message = "Статус проекта не может быть пустым")
        ProjectStatus status
) {}
