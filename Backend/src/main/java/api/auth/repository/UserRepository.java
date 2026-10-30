
package api.auth.repository;

import api.auth.model.User;
import io.quarkus.hibernate.reactive.panache.PanacheRepositoryBase;
import io.smallrye.mutiny.Uni;
import jakarta.enterprise.context.ApplicationScoped;

import java.util.Optional;
import java.util.UUID;

@ApplicationScoped
public class UserRepository
        implements PanacheRepositoryBase<User, UUID> {

    public Uni<Optional<User>> findByEmail(String email) {
        return find("email", email)
                .firstResult()
                .map(Optional::ofNullable);
    }

    public Uni<Boolean> existsByEmail(String email) {
        return count("email", email)
                .map(count -> count > 0);
    }
}