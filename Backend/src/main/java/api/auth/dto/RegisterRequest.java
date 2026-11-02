
package api.auth.dto;

import jakarta.validation.constraints.Email;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

public record RegisterRequest(
        @NotBlank(message = "Email не может быть пустым")
        @Email(message = "Некорректный формат email")
        @Size(max = 150, message = "Email не должен превышать 150 символов")
        String email,

        @NotBlank(message = "Пароль не может быть пустым")
        @Size(
                min = 6,
                max = 100,
                message = "Пароль должен содержать от 6 до 100 символов"
        )
        String password
) {}
