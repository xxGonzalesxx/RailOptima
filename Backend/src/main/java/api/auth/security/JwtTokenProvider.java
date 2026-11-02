
package api.auth.security;

import api.auth.model.User;
import io.smallrye.jwt.build.Jwt;
import jakarta.enterprise.context.ApplicationScoped;
import org.eclipse.microprofile.config.inject.ConfigProperty;

import java.util.Set;

@ApplicationScoped
public class JwtTokenProvider {

    @ConfigProperty(name = "smallrye.jwt.new-token.issuer")
    String issuer;

    @ConfigProperty(name = "smallrye.jwt.new-token.lifespan", defaultValue = "7200")
    long lifespan;

    public String generateToken(User user) {
        return Jwt.issuer(issuer)
                .subject(user.getEmail())
                .claim("userId", user.getId().toString())
                .groups(Set.of(user.getRole().name()))
                .expiresIn(lifespan)
                .sign();
    }
}
