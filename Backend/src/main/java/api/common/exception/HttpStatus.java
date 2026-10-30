package api.common.exception;

public enum HttpStatus {
    BAD_REQUEST(400),
    UNAUTHORIZED(401),
    FORBIDDEN(403),
    NOT_FOUND(404),
    CONFLICT(409),
    UNPROCESSABLE_ENTITY(422),
    INTERNAL_SERVER_ERROR(500),
    BAD_GATEWAY(502);

    private final int code;

    HttpStatus(int code) {
        this.code = code;
    }

    public int code() {
        return code;
    }
}