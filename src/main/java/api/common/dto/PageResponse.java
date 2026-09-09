package api.common.dto;

import java.util.List;

public record PageResponse<T>(
        List<T> content,        // Список элементов на текущей странице (например, проекты)
        int pageNumber,         // Номер текущей страницы (начиная с 0)
        int pageSize,           // Количество элементов на странице
        long totalElements,     // Всего элементов в базе данных
        int totalPages,         // Всего страниц доступно
        boolean last            // Является ли эта страница последней?
) {}
