
package api.llm.client;

import api.llm.config.GigaChatConfig;
import api.llm.config.GigaChatHttpClientFactory;
import api.llm.config.GigaChatTokenProvider;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import jakarta.enterprise.context.ApplicationScoped;
import jakarta.inject.Inject;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.List;
import java.util.Map;

@ApplicationScoped
public class GigaChatClient {

    @Inject
    GigaChatConfig config;

    @Inject
    GigaChatTokenProvider tokenProvider;

    @Inject
    ObjectMapper mapper;

    @Inject
    GigaChatHttpClientFactory httpClientFactory;

    public String chat(String prompt) {
        try {
            String jsonBody = mapper.writeValueAsString(Map.of(
                    "model", config.model(),
                    "messages", List.of(
                            Map.of("role", "user", "content", prompt)
                    )
            ));

            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(config.apiUrl() + "/chat/completions"))
                    .timeout(Duration.ofSeconds(60))
                    .header("Authorization", "Bearer " + tokenProvider.getAccessToken())
                    .header("Content-Type", "application/json")
                    .header("Accept", "application/json")
                    .POST(HttpRequest.BodyPublishers.ofString(jsonBody))
                    .build();

            HttpResponse<String> response = httpClientFactory.getClient().send(
                    request,
                    HttpResponse.BodyHandlers.ofString()
            );

            if (response.statusCode() < 200 || response.statusCode() >= 300) {
                throw new IllegalStateException(
                        "GigaChat API вернул HTTP " + response.statusCode()
                );
            }

            JsonNode json = mapper.readTree(response.body());
            JsonNode content = json.at("/choices/0/message/content");

            if (content.isMissingNode()
                    || !content.isTextual()
                    || content.asText().isBlank()) {
                throw new IllegalStateException(
                        "GigaChat API вернул ответ без текста"
                );
            }

            return content.asText();

        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new IllegalStateException(
                    "Вызов GigaChat был прерван", e
            );
        } catch (Exception e) {
            if (e instanceof IllegalStateException ise) {
                throw ise;
            }
            throw new IllegalStateException(
                    "Ошибка при вызове GigaChat", e
            );
        }
    }
}
