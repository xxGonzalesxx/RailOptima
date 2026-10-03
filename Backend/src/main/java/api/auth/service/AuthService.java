package api.auth.service;

import api.auth.dto.AuthResponse;
import api.auth.dto.LoginRequest;
import api.auth.dto.RegisterRequest;
import api.auth.model.User;
import api.auth.repository.UserRepository;
import api.auth.security.JwtTokenProvider;
import api.common.exception.ConflictException;
import api.common.exception.UnauthorizedException;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@RequiredArgsConstructor
@Slf4j
public class AuthService {

    private final UserRepository userRepository;
    private final PasswordEncoder passwordEncoder;
    private final JwtTokenProvider jwtTokenProvider;

    @Transactional
    public AuthResponse register(RegisterRequest request) {
        if (userRepository.existsByEmail(request.email())) {
            throw new ConflictException("Пользователь с таким email уже зарегистрирован", "EMAIL_ALREADY_EXISTS");
        }

        User user = User.builder()
                .email(request.email())
                .password(passwordEncoder.encode(request.password()))
                .build();

        User saved = userRepository.saveAndFlush(user);
        log.info("New user registered: {}", saved.getEmail());

        String token = jwtTokenProvider.generateToken(saved.getId(), saved.getEmail(), saved.getRole());
        return new AuthResponse(token, saved.getId(), saved.getEmail(), saved.getRole());
    }

    @Transactional(readOnly = true)
    public AuthResponse login(LoginRequest request) {
        User user = userRepository.findByEmail(request.email())
                .orElseThrow(() -> new UnauthorizedException("Неверный email или пароль"));

        if (!passwordEncoder.matches(request.password(), user.getPassword())) {
            throw new UnauthorizedException("Неверный email или пароль");
        }

        String token = jwtTokenProvider.generateToken(user.getId(), user.getEmail(), user.getRole());
        log.info("User logged in: {}", user.getEmail());

        return new AuthResponse(token, user.getId(), user.getEmail(), user.getRole());
    }
}