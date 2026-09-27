package api.common.exception;

import org.springframework.http.HttpStatus;

public class ResourceNotFoundException extends BusinessException {

    private static final String DEFAULT_CODE = "RESOURCE_NOT_FOUND";

    public ResourceNotFoundException(String resourceName, Object identifier) {
        super(
                String.format("Resource '%s' not found by identifier: %s", resourceName, identifier),
                DEFAULT_CODE,
                HttpStatus.NOT_FOUND
        );
    }
    // Обычный конструктор для кастомного сообщения
    public ResourceNotFoundException(String message) {
        super(message, DEFAULT_CODE, HttpStatus.NOT_FOUND);
    }

    // Конструктор для кастомного сообщения и специфичного кода ошибки (например, "USER_NOT_FOUND")
    public ResourceNotFoundException(String message, String errorCode) {
        super(message, errorCode, HttpStatus.NOT_FOUND);
    }
}
