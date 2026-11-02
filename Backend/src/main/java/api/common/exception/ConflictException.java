package api.common.exception;

public class ConflictException extends BusinessException {

    // Дефолтный конструктор: сразу жестко зашивает статус 409
    public ConflictException(String message) {
        super(message, "CONFLICT", HttpStatus.CONFLICT);
    }

    // Конструктор с кастомным кодом (например, "EMAIL_ALREADY_EXISTS")
    public ConflictException(String message, String errorCode) {
        super(message, errorCode, HttpStatus.CONFLICT);
    }
}
