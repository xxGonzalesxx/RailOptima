package api.llm.service;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestClient;
import org.springframework.web.client.RestClientException;

import java.util.List;
import java.util.Map;

@Service
public class LlmService {

    private static final Logger log = LoggerFactory.getLogger(LlmService.class);

    private final RestClient gigaChatClient;
    private final String model;

    public LlmService(RestClient gigaChatClient, @Value("${gigachat.model}") String model) {
        this.gigaChatClient = gigaChatClient;
        this.model = model;
    }

    @SuppressWarnings("unchecked")
    public String ask(String question) {
        Map<String, Object> requestBody = Map.of(
                "model", model,
                "messages", List.of(Map.of("role", "user", "content", question))
        );

        Map<String, Object> response;
        try {
            response = gigaChatClient.post()
                    .uri("/v1/chat/completions")
                    .contentType(MediaType.APPLICATION_JSON)
                    .body(requestBody)
                    .retrieve()
                    .body(Map.class);
        } catch (RestClientException e) {
            log.error("Ошибка при обращении к GigaChat", e);
            throw new IllegalStateException("GigaChat временно недоступен, попробуйте позже");
        }

        if (response == null || !response.containsKey("choices")) {
            log.warn("Неожиданный формат ответа GigaChat: {}", response);
            throw new IllegalStateException("GigaChat вернул ответ в неожиданном формате");
        }

        List<Map<String, Object>> choices = (List<Map<String, Object>>) response.get("choices");
        if (choices.isEmpty()) {
            throw new IllegalStateException("GigaChat вернул пустой список choices");
        }

        Map<String, Object> message = (Map<String, Object>) choices.get(0).get("message");
        return (String) message.get("content");
    }
}