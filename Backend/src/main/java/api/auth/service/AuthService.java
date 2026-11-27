
package api.auth.service;

import api.auth.dto.AuthResponse;
import api.auth.dto.LoginRequest;
import api.auth.dto.RegisterRequest;
import api.auth.model.Role;
import api.auth.model.User;
import api.auth.repository.UserRepository;
import api.auth.security.JwtTokenProvider;
import api.common.exception.ConflictException;
import api.common.exception.UnauthorizedException;
import io.quarkus.elytron.security.common.BcryptUtil;
import io.quarkus.hibernate.reactive.panache.common.WithSession;
import io.quarkus.hibernate.reactive.panache.common.WithTransaction;
import io.smallrye.mutiny.Uni;
import jakarta.enterprise.context.ApplicationScoped;
import jakarta.inject.Inject;
import org.jboss.logging.Logger;

@ApplicationScoped
public class AuthService {

    private static final Logger log = Logger.getLogger(AuthService.class);

    @Inject
    UserRepository userRepository;

    @Inject
    JwtTokenProvider jwtTokenProvider;

    @WithTransaction
    public Uni<AuthResponse> register(RegisterRequest request) {
        return userRepository.existsByEmail(request.email())
                .chain(exists -> {
                    if (exists) {
                        return Uni.createFrom().failure(
                                new ConflictException(
                                        "Пользователь с таким email уже зарегистрирован",
                                        "EMAIL_ALREADY_EXISTS"
                                )
                        );
                    }

                    return Uni.createFrom()
                            .item(() -> {
                                char[] pwd = request.password().toCharArray();
                                try {
                                    return BcryptUtil.bcryptHash(pwd, 12);
                                } finally {
                                    java.util.Arrays.fill(pwd, '\0');
                                }
                            })
                            .chain(hashedPassword -> {
                                User user = User.builder()
                                        .email(request.email())
                                        .password(hashedPassword)
                                        .role(Role.USER)
                                        .build();

                                return userRepository.persist(user)
                                        .replaceWith(() -> {
                                            String token = jwtTokenProvider.generateToken(user);
                                            log.info("New user registered");
                                            return new AuthResponse(
                                                    token,
                                                    user.getId(),
                                                    user.getEmail(),
                                                    user.getRole()
                                            );
                                        });
                            });
                });
    }

    @WithSession
    public Uni<AuthResponse> login(LoginRequest request) {
        return userRepository.findByEmail(request.email())
                .onItem().transformToUni(optionalUser -> {
                    if (optionalUser.isEmpty()) {
                        return Uni.createFrom().failure(
                                new UnauthorizedException("Неверный email или пароль")
                        );
                    }

                    User user = optionalUser.get();

                    return Uni.createFrom()
                            .item(() -> {
                                char[] pwd = request.password().toCharArray();
                                try {
                                    return BcryptUtil.matches(pwd, user.getPassword());
                                } finally {
                                    java.util.Arrays.fill(pwd, '\0');
                                }
                            })
                            .chain(passwordMatches -> {
                                if (!passwordMatches) {
                                    return Uni.createFrom().failure(
                                            new UnauthorizedException("Неверный email или пароль")
                                    );
                                }

                                String token = jwtTokenProvider.generateToken(user);
                                log.info("User logged in");

                                return Uni.createFrom().item(
                                        new AuthResponse(
                                                token,
                                                user.getId(),
                                                user.getEmail(),
                                                user.getRole()
                                        )
                                );
                            });
                });
    }
}
