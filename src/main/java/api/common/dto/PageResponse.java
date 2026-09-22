package api.common.dto;

import org.springframework.data.domain.Page;

import java.util.List;

public record PageResponse<T>(
        List<T> content,        // Список элементов на текущей странице (например, проекты)
        int pageNumber,         // Номер текущей страницы (начиная с 0)
        int pageSize,           // Количество элементов на странице
        long totalElements,     // Всего элементов в базе данных
        int totalPages,         // Всего страниц доступно
        boolean last            // Является ли эта страница последней?
) {
    public static <T> PageResponse<T> from(Page<T> page) {
        return new PageResponse<>(
                page.getContent(),
                page.getNumber(),
                page.getSize(),
                page.getTotalElements(),
                page.getTotalPages(),
                page.isLast()
        );
    }
}