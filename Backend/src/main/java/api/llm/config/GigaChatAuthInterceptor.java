package api.llm.config;

import org.jspecify.annotations.NonNull;
import org.springframework.http.HttpRequest;
import org.springframework.http.MediaType;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.client.RestClient;

import java.io.IOException;
import java.util.Map;
import java.util.UUID;

public class GigaChatAuthInterceptor implements ClientHttpRequestInterceptor {

    private final String authKey;
    private final String scope;
    private final RestClient authClient;

    private volatile String cachedAccessToken;
    private volatile long tokenExpiryTime = 0;

    public GigaChatAuthInterceptor(String authKey, String scope, RestClient.Builder authClientBuilder) {
        this.authKey = authKey;
        this.scope = scope;
        this.authClient = authClientBuilder
                .baseUrl("https://ngw.devices.sberbank.ru:9443/api/v2")
                .build();
    }

    @Override
    @NonNull
    public ClientHttpResponse intercept(@NonNull HttpRequest request, byte @NonNull [] body,
                                        @NonNull ClientHttpRequestExecution execution) throws IOException {
        if (cachedAccessToken == null || System.currentTimeMillis() >= tokenExpiryTime) {
            refreshAccessToken();
        }
        request.getHeaders().setBearerAuth(cachedAccessToken);
        return execution.execute(request, body);
    }

    private synchronized void refreshAccessToken() {
        if (cachedAccessToken != null && System.currentTimeMillis() < tokenExpiryTime) {
            return; // другой поток уже обновил, пока мы ждали на synchronized
        }
        try {
            MultiValueMap<String, String> formData = new LinkedMultiValueMap<>();
            formData.add("scope", scope);

            @SuppressWarnings("unchecked")
            Map<String, Object> response = authClient.post()
                    .uri("/oauth")
                    .header("Authorization", "Basic " + authKey)
                    .header("RqUID", UUID.randomUUID().toString())
                    .contentType(MediaType.APPLICATION_FORM_URLENCODED)
                    .body(formData)
                    .retrieve()
                    .body(Map.class);

            if (response == null || !response.containsKey("access_token")) {
                throw new IllegalStateException("GigaChat OAuth вернул пустой ответ");
            }

            this.cachedAccessToken = (String) response.get("access_token");

            Object expiresAt = response.get("expires_at");
            this.tokenExpiryTime = (expiresAt instanceof Number n)
                    ? n.longValue() - 60_000
                    : System.currentTimeMillis() + (25 * 60 * 1000);

        } catch (Exception e) {
            throw new IllegalStateException("Не удалось получить токен GigaChat OAuth", e);
        }
    }
}