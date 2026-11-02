
package api.llm.config;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import jakarta.enterprise.context.ApplicationScoped;
import jakarta.inject.Inject;

import java.net.URI;
import java.net.URLEncoder;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.UUID;

@ApplicationScoped
public class GigaChatTokenProvider {

    private static final String OAUTH_URL =
            "https://ngw.devices.sberbank.ru:9443/api/v2/oauth";

    private static final long TOKEN_REFRESH_MARGIN_MS = 60_000L;
    private static final long DEFAULT_TOKEN_LIFETIME_MS = 25 * 60_000L;

    @Inject
    GigaChatConfig config;

    @Inject
    ObjectMapper mapper;

    @Inject
    GigaChatHttpClientFactory httpClientFactory;

    private volatile String cachedAccessToken;
    private volatile long tokenExpiryTime;

    public String getAccessToken() {
        if (cachedAccessToken == null
                || System.currentTimeMillis() >= tokenExpiryTime) {
            refreshAccessToken();
        }

        return cachedAccessToken;
    }

    private synchronized void refreshAccessToken() {
        if (cachedAccessToken != null
                && System.currentTimeMillis() < tokenExpiryTime) {
            return;
        }

        try {
            String body = "scope=" + URLEncoder.encode(
                    config.scope(),
                    StandardCharsets.UTF_8
            );

            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(OAUTH_URL))
                    .timeout(Duration.ofSeconds(20))
                    .header("Authorization", "Basic " + config.authKey())
                    .header("RqUID", UUID.randomUUID().toString())
                    .header("Content-Type", "application/x-www-form-urlencoded")
                    .header("Accept", "application/json")
                    .POST(HttpRequest.BodyPublishers.ofString(body))
                    .build();

            HttpResponse<String> response = httpClientFactory.getClient().send(
                    request,
                    HttpResponse.BodyHandlers.ofString()
            );

            if (response.statusCode() < 200
                    || response.statusCode() >= 300) {
                throw new IllegalStateException(
                        "GigaChat OAuth вернул HTTP "
                                + response.statusCode()
                );
            }

            JsonNode json = mapper.readTree(response.body());
            JsonNode accessToken = json.get("access_token");

            if (accessToken == null
                    || !accessToken.isTextual()
                    || accessToken.asText().isBlank()) {
                throw new IllegalStateException(
                        "GigaChat OAuth не вернул access_token"
                );
            }

            long now = System.currentTimeMillis();
            long expiryTime = json.has("expires_at")
                    ? json.get("expires_at").asLong()
                    : now + DEFAULT_TOKEN_LIFETIME_MS;

            cachedAccessToken = accessToken.asText();
            tokenExpiryTime = Math.max(
                    now,
                    expiryTime - TOKEN_REFRESH_MARGIN_MS
            );

        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new IllegalStateException(
                    "Получение токена GigaChat было прервано", e
            );
        } catch (Exception e) {
            if (e instanceof IllegalStateException ise) {
                throw ise;
            }

            throw new IllegalStateException(
                    "Не удалось получить токен GigaChat OAuth", e
            );
        }
    }
}
