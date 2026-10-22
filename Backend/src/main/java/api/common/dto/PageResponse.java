package api.common.dto;

import io.quarkus.hibernate.orm.panache.PanacheQuery;
import io.quarkus.panache.common.Page;
import java.util.List;

public record PageResponse<T>(
        List<T> content,        // Список элементов на текущей странице (например, проекты)
        int pageNumber,         // Номер текущей страницы (начиная с 0)
        int pageSize,           // Количество элементов на странице
        long totalElements,     // Всего элементов в базе данных
        int totalPages,         // Всего страниц доступно
        boolean last            // Является ли эта страница последней?
) {
    public static <T> PageResponse<T> from(PanacheQuery<T> query, Page page) {
        query.page(page);
        List<T> content = query.list();
        long totalElements = query.count();
        int totalPages = query.pageCount();
        int pageNumber = page.index;
        int pageSize = page.size;

        return new PageResponse<>(
                content,
                pageNumber,
                pageSize,
                totalElements,
                totalPages,
                pageNumber >= totalPages - 1
        );
    }
}