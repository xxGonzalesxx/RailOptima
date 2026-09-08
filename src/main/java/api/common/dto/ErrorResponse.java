package api.common.dto;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.time.LocalDateTime;
import java.util.Map;

@Data
@Builder
@AllArgsConstructor
@NoArgsConstructor
public class ErrorResponse {

    // Код ошибки (например, "NOT_FOUND")
    private String error;

    // Текст ошибки
    private String message;

    // HTTP-статус (404, 400, 500)
    private int status;

    // Время ошибки
    private LocalDateTime timestamp;

    // URL, где произошла ошибка
    private String path;

    // Ошибки валидации (поле → сообщение)
    private Map<String, String> validationErrors;

}
