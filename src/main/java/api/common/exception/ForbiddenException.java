package api.common.exception;

import org.springframework.http.HttpStatus;

public class ForbiddenException extends BusinessException {

    // Сразу жестко фиксируем статус 403 FORBIDDEN
    public ForbiddenException(String message) {
        super(message, "FORBIDDEN", HttpStatus.FORBIDDEN);
    }

    // Дополнительный конструктор для передачи детального кода ошибки
    public ForbiddenException(String message, String errorCode) {
        super(message, errorCode, HttpStatus.FORBIDDEN);
    }
}
