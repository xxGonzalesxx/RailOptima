package api.common.constant;

public final class ApiConstants {

    private ApiConstants() {
        throw new UnsupportedOperationException("Это утилитный класс, его нельзя создавать");
    }

    // Базовые пути
    public static final String BASE_PATH = "/api/v1";

    // Заголовки авторизации
    public static final String HEADER_AUTHORIZATION = "Authorization";
    public static final String HEADER_BEARER_PREFIX = "Bearer ";

    // Пагинация
    public static final int DEFAULT_PAGE_SIZE = 20;
    public static final int MAX_PAGE_SIZE = 100;

    // Сообщения — теперь на русском
    public static final String MSG_SUCCESS = "Операция успешно завершена";
    public static final String MSG_UNAUTHORIZED = "Необходима авторизация";
    public static final String MSG_FORBIDDEN = "Доступ запрещён";
}