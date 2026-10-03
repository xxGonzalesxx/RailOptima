package api.auth.security;

import api.auth.security.JwtTokenProvider.TokenValidationResult;
import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.jspecify.annotations.NonNull;
import org.springframework.security.authentication.UsernamePasswordAuthenticationToken;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.security.core.userdetails.UserDetails;
import org.springframework.security.core.userdetails.UserDetailsService;
import org.springframework.security.web.authentication.WebAuthenticationDetailsSource;
import org.springframework.stereotype.Component;
import org.springframework.web.filter.OncePerRequestFilter;

import java.io.IOException;
import java.util.List;

@Component
@RequiredArgsConstructor
@Slf4j
public class JwtAuthenticationFilter extends OncePerRequestFilter {

    private final JwtTokenProvider jwtTokenProvider;
    private final UserDetailsService userDetailsService;

    @Override
    protected void doFilterInternal(
            HttpServletRequest request,
            @NonNull HttpServletResponse response,
            @NonNull FilterChain filterChain
    ) throws ServletException, IOException {

        final String path = request.getRequestURI();

        // Пропускаем эндпоинты авторизации
        if (path.startsWith("/api/v1/auth/") || path.equals("/health") || path.startsWith("/actuator/health")) {
            filterChain.doFilter(request, response);
            return;
        }

        String token = extractToken(request);

        if (token != null) {
            TokenValidationResult validation = jwtTokenProvider.validateToken(token);

            if (validation.valid() && SecurityContextHolder.getContext().getAuthentication() == null) {
                try {
                    String email = jwtTokenProvider.getEmailFromToken(token);
                    String role = jwtTokenProvider.getRoleFromToken(token);

                    if (email != null && role != null) {
                        UserDetails userDetails = userDetailsService.loadUserByUsername(email);

                        var authorities = List.of(new SimpleGrantedAuthority("ROLE_" + role));

                        var authentication = new UsernamePasswordAuthenticationToken(
                                userDetails,
                                null,
                                authorities
                        );

                        authentication.setDetails(new WebAuthenticationDetailsSource().buildDetails(request));
                        SecurityContextHolder.getContext().setAuthentication(authentication);
                        log.debug("Аутентифицирован пользователь: {} с ролью: {}", email, role);
                    }
                } catch (Exception e) {
                    log.error("Ошибка аутентификации: {}", e.getMessage());
                }
            } else if (!validation.valid()) {
                log.debug("Невалидный JWT токен для URI: {}, причина: {}", request.getRequestURI(), validation.reason());
            }
        }

        filterChain.doFilter(request, response);
    }

    private String extractToken(HttpServletRequest request) {
        String bearerToken = request.getHeader("Authorization");
        if (bearerToken != null && bearerToken.startsWith("Bearer ")) {
            return bearerToken.substring(7);
        }
        return null;
    }
}