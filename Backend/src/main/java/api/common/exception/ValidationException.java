package api.common.exception;

import lombok.Getter;
import java.util.Map;

@Getter
public class ValidationException extends BusinessException {

    private final Map<String, String> validationErrors;

    // Автоматически передаем статус 400 (BAD_REQUEST) в родительский класс
    public ValidationException(String message, Map<String, String> validationErrors) {
        super(message, "VALIDATION_ERROR", HttpStatus.BAD_REQUEST);
        this.validationErrors = validationErrors;
    }

    public ValidationException(String message) {
        this(message, Map.of());
    }
}
