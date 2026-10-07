package api.llm.service;

import api.llm.config.GigaChatAuthInterceptor;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.MediaType;
import org.springframework.http.client.SimpleClientHttpRequestFactory;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestClient;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import java.util.List;
import java.util.Map;

@Service
public class LlmService {

    private final RestClient gigaChatClient;

    public LlmService(
            @Value("${gigachat.auth-key}") String gigachatAuthKey,
            @Value("${gigachat.scope}") String gigachatScope) {

        try {
            SSLContext sslContext = SSLContext.getInstance("TLS");
            sslContext.init(null, new TrustManager[]{new X509TrustManager() {
                @Override
                public X509Certificate[] getAcceptedIssuers() {
                    return new X509Certificate[0];
                }
                @Override
                public void checkClientTrusted(X509Certificate[] certs, String authType) {}
                @Override
                public void checkServerTrusted(X509Certificate[] certs, String authType) {}
            }}, new SecureRandom());

            HttpsURLConnection.setDefaultHostnameVerifier((hostname, session) -> true);

            SimpleClientHttpRequestFactory requestFactory = new SimpleClientHttpRequestFactory() {
                @Override
                protected void prepareConnection(HttpURLConnection connection, String httpMethod) throws IOException {
                    if (connection instanceof HttpsURLConnection httpsConnection) {
                        httpsConnection.setSSLSocketFactory(sslContext.getSocketFactory());
                    }
                    super.prepareConnection(connection, httpMethod);
                }
            };

            this.gigaChatClient = RestClient.builder()
                    .baseUrl("https://api.giga.chat")
                    .requestFactory(requestFactory)
                    .requestInterceptor(new GigaChatAuthInterceptor(gigachatAuthKey, gigachatScope))
                    .build();

        } catch (Exception e) {
            throw new RuntimeException("Failed to initialize GigaChat client", e);
        }
    }

    @SuppressWarnings("unchecked")
    public String ask(String question) {
        try {
            Map<String, Object> requestBody = Map.of(
                    "model", "GigaChat-3-Ultra",
                    "messages", List.of(
                            Map.of("role", "user", "content", question)
                    )
            );

            Map<String, Object> response = gigaChatClient.post()
                    .uri("/v1/chat/completions")
                    .contentType(MediaType.APPLICATION_JSON)
                    .body(requestBody)
                    .retrieve()
                    .body(Map.class);

            if (response != null && response.containsKey("choices")) {
                List<Map<String, Object>> choices = (List<Map<String, Object>>) response.get("choices");
                if (!choices.isEmpty()) {
                    Map<String, Object> choice = choices.get(0);
                    Map<String, Object> message = (Map<String, Object>) choice.get("message");
                    return (String) message.get("content");
                }
            }
            return "Не удалось прочитать 'choices' из ответа Сбера.";

        } catch (Exception e) {
            // Возвращаем текст реальной ошибки прямо наружу!
            return "Детали сбоя в Java-коде: " + e.getMessage();
        }
    }
}
