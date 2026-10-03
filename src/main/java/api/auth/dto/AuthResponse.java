package api.auth.dto;

import api.auth.model.Role;

import java.util.UUID;

public record AuthResponse(
        String token,
        UUID userId,
        String email,
        Role role
) {}
