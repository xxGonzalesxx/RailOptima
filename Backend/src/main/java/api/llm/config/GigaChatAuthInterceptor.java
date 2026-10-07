package api.llm.config;

import org.springframework.http.HttpRequest;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.http.client.SimpleClientHttpRequestFactory;
import org.springframework.http.MediaType;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.client.RestClient;
import org.springframework.lang.NonNull;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import java.util.Map;
import java.util.UUID;

public class GigaChatAuthInterceptor implements ClientHttpRequestInterceptor {

    private final String authKey;
    private final String scope;
    private String cachedAccessToken;
    private long tokenExpiryTime = 0;

    public GigaChatAuthInterceptor(String authKey, String scope) {
        this.authKey = authKey;
        this.scope = scope;
    }

    @Override
    @NonNull
    public ClientHttpResponse intercept(@NonNull HttpRequest request, @NonNull byte[] body, @NonNull ClientHttpRequestExecution execution) throws IOException {
        if (cachedAccessToken == null || System.currentTimeMillis() >= tokenExpiryTime) {
            refreshAccessToken();
        }
        request.getHeaders().setBearerAuth(cachedAccessToken);
        return execution.execute(request, body);
    }

    private synchronized void refreshAccessToken() {
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

            RestClient authClient = RestClient.builder()
                    .requestFactory(requestFactory)
                    .baseUrl("https://ngw.devices.sberbank.ru:9443/api/v2")
                    .build();

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

            if (response != null && response.containsKey("access_token")) {
                this.cachedAccessToken = (String) response.get("access_token");
                this.tokenExpiryTime = System.currentTimeMillis() + (25 * 60 * 1000);
            } else {
                throw new RuntimeException("Сбербанк прислал пустой ответ при генерации токена");
            }
        } catch (Exception e) {
            throw new RuntimeException("Сбой Сбер OAuth шлюза: " + e.getMessage(), e);
        }
    }
}
