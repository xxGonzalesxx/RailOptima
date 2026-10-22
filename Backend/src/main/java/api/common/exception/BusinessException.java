package api.common.exception;

import jakarta.ws.rs.core.Response;
import lombok.Getter;

@Getter
public class BusinessException extends RuntimeException {

    private final String errorCode;
    private final Response.Status status;

    public BusinessException(String message) {
        this(message, "BUSINESS_ERROR", Response.Status.BAD_REQUEST);
    }

    public BusinessException(String message, String errorCode, Response.Status status) {
        super(message);
        this.errorCode = errorCode;
        this.status = status;
    }

    public BusinessException(String message, String errorCode, Response.Status status, Throwable cause) {
        super(message, cause);
        this.errorCode = errorCode;
        this.status = status;
    }
}