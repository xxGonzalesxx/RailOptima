package api.auth.security;

import api.auth.model.Role;
import io.jsonwebtoken.Claims;
import io.jsonwebtoken.ExpiredJwtException;
import io.jsonwebtoken.MalformedJwtException;
import io.jsonwebtoken.UnsupportedJwtException;
import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.security.Keys;
import io.jsonwebtoken.security.SignatureException;
import jakarta.annotation.PostConstruct;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import javax.crypto.SecretKey;
import java.nio.charset.StandardCharsets;
import java.util.Date;
import java.util.UUID;

@Component
@Slf4j
public class JwtTokenProvider {

    @Value("${jwt.secret}")
    private String secret;

    @Value("${jwt.expiration:86400000}")
    private long expirationMs;

    private SecretKey signingKey;

    @PostConstruct
    public void init() {
        // Проверка длины секрета при старте
        if (secret == null || secret.length() < 32) {
            log.error("JWT секрет должен быть минимум 32 символа! Текущая длина: {}",
                    secret != null ? secret.length() : 0);
            throw new IllegalStateException("JWT секрет слишком короткий. Минимум 32 символа.");
        }
        this.signingKey = Keys.hmacShaKeyFor(secret.getBytes(StandardCharsets.UTF_8));
        log.info("JWT провайдер успешно инициализирован");
    }

    // === ГЕНЕРАЦИЯ ТОКЕНА ===
    public String generateToken(UUID userId, String email, Role role) {
        Date now = new Date();
        Date expiryDate = new Date(now.getTime() + expirationMs);

        return Jwts.builder()
                .subject(email)
                .claim("userId", userId.toString())
                .claim("role", role.name())
                .issuedAt(now)
                .expiration(expiryDate)
                .signWith(signingKey)
                .compact();
    }

    // === ВАЛИДАЦИЯ ТОКЕНА (с разными типами ошибок) ===
    public TokenValidationResult validateToken(String token) {
        try {
            Jwts.parser().verifyWith(signingKey).build().parseSignedClaims(token);
            return new TokenValidationResult(true, null);
        } catch (SignatureException e) {
            log.error("Неверная подпись JWT: {}", e.getMessage());
            return new TokenValidationResult(false, "INVALID_SIGNATURE");
        } catch (MalformedJwtException e) {
            log.error("Неверный формат JWT: {}", e.getMessage());
            return new TokenValidationResult(false, "MALFORMED");
        } catch (ExpiredJwtException e) {
            log.error("JWT истёк: {}", e.getMessage());
            return new TokenValidationResult(false, "EXPIRED");
        } catch (UnsupportedJwtException e) {
            log.error("Неподдерживаемый JWT: {}", e.getMessage());
            return new TokenValidationResult(false, "UNSUPPORTED");
        } catch (IllegalArgumentException e) {
            log.error("JWT claims пуст: {}", e.getMessage());
            return new TokenValidationResult(false, "EMPTY_CLAIMS");
        }
    }

    // === ИЗВЛЕЧЕНИЕ ДАННЫХ ИЗ ТОКЕНА ===
    private Claims extractAllClaims(String token) {
        return Jwts.parser()
                .verifyWith(signingKey)
                .build()
                .parseSignedClaims(token)
                .getPayload();
    }

    public String getEmailFromToken(String token) {
        return extractAllClaims(token).getSubject();
    }

    public UUID getUserIdFromToken(String token) {
        String userIdStr = extractAllClaims(token).get("userId", String.class);
        return UUID.fromString(userIdStr);
    }

    public String getRoleFromToken(String token) {
        return extractAllClaims(token).get("role", String.class);
    }

    // === РЕЗУЛЬТАТ ВАЛИДАЦИИ ===
    public record TokenValidationResult(boolean valid, String reason) {}
}