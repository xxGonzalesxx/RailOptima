package api.common.dto;

import java.time.Instant;

public record ApiResponse<T>(
        boolean success,
        String message,
        T data,
        Instant timestamp,
        String path
) {
    public ApiResponse {
        if (timestamp == null) {
            timestamp = Instant.now();
        }
    }
    // 1. Для успешных ответов с данными
    public static <T> ApiResponse<T> success(T data) {
        return new ApiResponse<>(true, "Операция успешно завершена", data, null, null);
    }

    // 2. Для обычных ошибок (404 Not Found, 401 Unauthorized и т.д.)
    public static <T> ApiResponse<T> error(String message, String path) {
        return new ApiResponse<>(false, message, null, null, path);
    }

    // Передаем карту ошибок (поле -> текст) прямо в поле data!
    public static <T> ApiResponse<T> validationError(String message, T validationErrors, String path) {
        return new ApiResponse<>(false, message, validationErrors, null, path);
    }
}
