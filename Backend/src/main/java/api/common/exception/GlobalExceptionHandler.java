package api.common.exception;

import api.common.dto.ApiResponse;
import jakarta.servlet.http.HttpServletRequest;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.orm.ObjectOptimisticLockingFailureException;
import org.springframework.validation.FieldError;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

import java.util.HashMap;
import java.util.Map;

@Slf4j
@RestControllerAdvice
public class GlobalExceptionHandler {

    // --- 1. Обработка 404 (ResourceNotFoundException) ---
    @ExceptionHandler(ResourceNotFoundException.class)
    public ResponseEntity<ApiResponse<Map<String, String>>> handleNotFound(ResourceNotFoundException ex, HttpServletRequest request) {
        log.warn("Resource not found: {}", ex.getMessage());
        return buildErrorResponse(ex.getMessage(), "NOT_FOUND", HttpStatus.NOT_FOUND, request);
    }

    // --- 2. Обработка 400 (Кастомная Валидация из Сервисов) ---
    @ExceptionHandler(ValidationException.class)
    public ResponseEntity<ApiResponse<Map<String, String>>> handleValidation(ValidationException ex, HttpServletRequest request) {
        log.warn("Validation error: {}", ex.getMessage());

        ApiResponse<Map<String, String>> response = ApiResponse.validationError(
                ex.getMessage(),
                ex.getValidationErrors(),
                request.getRequestURI()
        );
        return ResponseEntity.badRequest().body(response);
    }

    // --- 3. Обработка 400 от встроенной валидации Spring (@Valid) ---
    @ExceptionHandler(MethodArgumentNotValidException.class)
    public ResponseEntity<ApiResponse<Map<String, String>>> handleMethodArgumentNotValid(MethodArgumentNotValidException ex, HttpServletRequest request) {
        Map<String, String> errors = new HashMap<>();
        ex.getBindingResult().getAllErrors().forEach(error -> {
            if (error instanceof FieldError fieldError) {
                errors.put(fieldError.getField(), fieldError.getDefaultMessage());
            } else {
                errors.put("global", error.getDefaultMessage());
            }
        });

        log.warn("Spring validation failed on path {}: {}", request.getRequestURI(), errors);

        ApiResponse<Map<String, String>> response = ApiResponse.validationError(
                "Ошибка валидации полей",
                errors,
                request.getRequestURI()
        );
        return ResponseEntity.badRequest().body(response);
    }

    // --- 4. Обработка 409 (Конфликт данных) ---
    @ExceptionHandler(ConflictException.class)
    public ResponseEntity<ApiResponse<Map<String, String>>> handleConflict(ConflictException ex, HttpServletRequest request) {
        log.warn("Conflict: {}", ex.getMessage());
        return buildErrorResponse(ex.getMessage(), "CONFLICT", HttpStatus.CONFLICT, request);
    }

    // --- 5. Обработка 403 (Доступ запрещён) ---
    @ExceptionHandler(ForbiddenException.class)
    public ResponseEntity<ApiResponse<Map<String, String>>> handleForbidden(ForbiddenException ex, HttpServletRequest request) {
        log.warn("Forbidden: {}", ex.getMessage());
        return buildErrorResponse(ex.getMessage(), "FORBIDDEN", HttpStatus.FORBIDDEN, request);
    }

    // --- 6. Обработка всех остальных BusinessException (Динамический статус из исключения) ---
    @ExceptionHandler(BusinessException.class)
    public ResponseEntity<ApiResponse<Map<String, String>>> handleBusiness(BusinessException ex, HttpServletRequest request) {
        log.warn("Business error [{}]: {}", ex.getErrorCode(), ex.getMessage());

        // Так как мы добавили поле HttpStatus status в класс BusinessException, берем его оттуда
        HttpStatus status = ex.getStatus() != null ? ex.getStatus() : HttpStatus.BAD_REQUEST;
        return buildErrorResponse(ex.getMessage(), ex.getErrorCode(), status, request);
    }

    // --- 7. Обработка всех неожиданных системных ошибок (500) ---
    @ExceptionHandler(Exception.class)
    public ResponseEntity<ApiResponse<Void>> handleGeneric(Exception ex, HttpServletRequest request) {
        log.error("Unexpected system error on path: {}", request.getRequestURI(), ex);

        ApiResponse<Void> response = ApiResponse.error(
                "На сервере произошла внутренняя ошибка. Пожалуйста, попробуйте позже.",
                request.getRequestURI()
        );
        return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(response);
    }

    // --- Приватный вспомогательный метод для сборки ответа (DRY) ---
    private ResponseEntity<ApiResponse<Map<String, String>>> buildErrorResponse(
            String message, String errorCode, HttpStatus status, HttpServletRequest request) {

        ApiResponse<Map<String, String>> response = ApiResponse.validationError(
                message,
                Map.of("errorCode", errorCode),
                request.getRequestURI()
        );
        return ResponseEntity.status(status).body(response);
    }

    @ExceptionHandler(ObjectOptimisticLockingFailureException.class)
    public ResponseEntity<ApiResponse<Map<String, String>>> handleOptimisticLock(
            ObjectOptimisticLockingFailureException ex, HttpServletRequest request) {
        log.warn("Optimistic lock conflict on path {}", request.getRequestURI());
        return buildErrorResponse(
                "Проект был изменён другим пользователем, обновите данные и попробуйте снова",
                "CONFLICT_STALE_DATA",
                HttpStatus.CONFLICT,
                request
        );
    }
}
