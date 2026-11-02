package api.llm.config;

import jakarta.enterprise.context.ApplicationScoped;

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import java.net.http.HttpClient;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import java.time.Duration;

@ApplicationScoped
public class GigaChatHttpClientFactory {

    private final HttpClient httpClient;

    public GigaChatHttpClientFactory() {
        try {
            X509TrustManager trustAllManager = new X509TrustManager() {
                @Override
                public X509Certificate[] getAcceptedIssuers() {
                    return new X509Certificate[0];
                }

                @Override
                public void checkClientTrusted(
                        X509Certificate[] certs, String authType) {
                }

                @Override
                public void checkServerTrusted(
                        X509Certificate[] certs, String authType) {
                }
            };

            SSLContext sslContext = SSLContext.getInstance("TLS");
            sslContext.init(
                    null,
                    new TrustManager[]{trustAllManager},
                    new SecureRandom()
            );

            this.httpClient = HttpClient.newBuilder()
                    .sslContext(sslContext)
                    .connectTimeout(Duration.ofSeconds(10))
                    .build();

        } catch (Exception e) {
            throw new IllegalStateException(
                    "Не удалось создать GigaChat HTTP Client", e
            );
        }
    }

    public HttpClient getClient() {
        return httpClient;
    }
}