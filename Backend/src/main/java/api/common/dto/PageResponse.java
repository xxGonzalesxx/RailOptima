
package api.common.dto;

import java.util.List;

public record PageResponse<T>(
        List<T> content,
        int pageNumber,
        int pageSize,
        long totalElements,
        int totalPages,
        boolean last
) {
    public static <T> PageResponse<T> from(
            List<T> content,
            long totalElements,
            int pageNumber,
            int pageSize
    ) {
        int totalPages = pageSize > 0
                ? (int) ((totalElements + pageSize - 1L) / pageSize)
                : 0;

        boolean last = totalPages == 0 || pageNumber >= totalPages - 1;

        return new PageResponse<>(
                content,
                pageNumber,
                pageSize,
                totalElements,
                totalPages,
                last
        );
    }
}
