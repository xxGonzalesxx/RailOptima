package api.project.dto;

import api.project.model.ProjectStatus;

import java.time.Instant;
import java.util.UUID;

public record ProjectResponse(
        UUID id,
        String name,
        String description,
        ProjectStatus status,
        UUID ownerId,       // ID пользователя-создателя из модуля AUTH
        Instant createdAt,  // Время создания в UTC (ISO-8601)
        Instant updatedAt   // Время последнего изменения
) {}
