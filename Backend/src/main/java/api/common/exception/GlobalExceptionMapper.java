
package api.common.exception;

import api.common.dto.ApiResponse;
import jakarta.persistence.OptimisticLockException;
import jakarta.ws.rs.WebApplicationException;
import jakarta.ws.rs.core.Response;
import jakarta.ws.rs.ext.ExceptionMapper;
import jakarta.ws.rs.ext.Provider;
import org.jboss.logging.Logger;

import java.util.HashMap;
import java.util.Map;

@Provider
public class GlobalExceptionMapper implements ExceptionMapper<Throwable> {

    private static final Logger log =
            Logger.getLogger(GlobalExceptionMapper.class);

    @Override
    public Response toResponse(Throwable ex) {

        // 1. Ошибки валидации приложения
        if (ex instanceof ValidationException ve) {
            log.warnf("Validation error: %s", ve.getMessage());

            return Response.status(Response.Status.BAD_REQUEST)
                    .entity(ApiResponse.validationError(
                            ve.getMessage(),
                            ve.getValidationErrors(),
                            null
                    ))
                    .build();
        }

        // 2. Остальные бизнес-ошибки
        if (ex instanceof BusinessException be) {
            log.warnf(
                    "Business error [%s]: %s",
                    be.getErrorCode(),
                    be.getMessage()
            );

            return Response.status(be.getStatus().code())
                    .entity(ApiResponse.validationError(
                            be.getMessage(),
                            Map.of("errorCode", be.getErrorCode()),
                            null
                    ))
                    .build();
        }

        // 3. Ошибки валидации Jakarta Bean Validation (@Valid)
        if (ex instanceof jakarta.validation.ConstraintViolationException cve) {
            Map<String, String> errors = new HashMap<>();

            cve.getConstraintViolations().forEach(violation -> {
                String field = violation.getPropertyPath().toString();

                if (field.contains(".")) {
                    field = field.substring(field.lastIndexOf('.') + 1);
                }

                errors.put(field, violation.getMessage());
            });

            return Response.status(Response.Status.BAD_REQUEST)
                    .entity(ApiResponse.validationError(
                            "Ошибка валидации полей",
                            errors,
                            null
                    ))
                    .build();
        }

        // 4. Сохраняем исходный HTTP-ответ стандартных JAX-RS-исключений
        if (ex instanceof WebApplicationException wae) {
            return wae.getResponse();
        }

        // 5. Оптимистичные блокировки, включая обёрнутые исключения
        if (hasCause(ex, OptimisticLockException.class)) {
            log.warn("Optimistic lock conflict");

            return Response.status(Response.Status.CONFLICT)
                    .entity(ApiResponse.validationError(
                            "Данные устарели, обновите страницу",
                            Map.of("errorCode", "CONFLICT_STALE_DATA"),
                            null
                    ))
                    .build();
        }

        // 6. Ошибки состояния приложения
        // Не считаем любую IllegalStateException ошибкой GigaChat.
        // Для интеграции с GigaChat следует использовать отдельное исключение.

        // 7. Неожиданные ошибки сервера
        log.error("Unexpected system error", ex);

        return Response.status(Response.Status.INTERNAL_SERVER_ERROR)
                .entity(ApiResponse.error(
                        "На сервере произошла внутренняя ошибка",
                        null
                ))
                .build();
    }

    private boolean hasCause(
            Throwable throwable,
            Class<? extends Throwable> exceptionType
    ) {
        Throwable current = throwable;

        // Защита от циклической цепочки причин
        for (int i = 0; current != null && i < 100; i++) {
            if (exceptionType.isInstance(current)) {
                return true;
            }

            Throwable next = current.getCause();

            if (next == current) {
                break;
            }

            current = next;
        }

        return false;
    }
}
