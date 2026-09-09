package api.project.model;

import jakarta.persistence.*;
import lombok.*;
import org.hibernate.annotations.CreationTimestamp;
import org.hibernate.annotations.UpdateTimestamp;

import java.time.Instant;
import java.util.UUID;

@Entity
@Table(name = "projects", indexes = {
        // Оптимизируем поиск проектов конкретного пользователя
        @Index(name = "idx_project_owner", columnList = "owner_id"),
        // Оптимизируем сортировку по дате создания
        @Index(name = "idx_project_created_at", columnList = "created_at")
})
@Getter
@Setter
@Builder
@NoArgsConstructor
@AllArgsConstructor
public class Project {

    @Id
    @GeneratedValue(strategy = GenerationType.UUID)
    private UUID id;

    @Column(nullable = false, length = 100)
    private String name;

    @Column(length = 500)
    private String description;

    @Enumerated(EnumType.STRING)
    @Column(nullable = false, length = 30)
    private ProjectStatus status;

    /**
     * Связь модулей. Мы не подтягиваем сюда тяжелую сущность User из модуля AUTH.
     * Мы храним только его UUID. Это обеспечивает слабую связность (Loose Coupling),
     * что позволит легко распилить монолит на микросервисы в будущем.
     */
    @Column(name = "owner_id", nullable = false)
    private UUID ownerId;

    @CreationTimestamp
    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt;

    @UpdateTimestamp
    @Column(name = "updated_at", nullable = false)
    private Instant updatedAt;
}
