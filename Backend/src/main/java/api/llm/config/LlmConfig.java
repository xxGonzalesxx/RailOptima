package api.llm.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.client.SimpleClientHttpRequestFactory;
import org.springframework.web.client.RestClient;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;

@Configuration
public class LlmConfig {

    // ВНИМАНИЕ: доверяем любому сертификату, потому что у Сбера/GigaChat
    // цепочка подписана УЦ Минцифры, которого обычно нет в стандартном
    // JVM-truststore. Правильное решение для прода — импортировать
    // российский корневой сертификат Минцифры в truststore и убрать этот
    // trust-all TrustManager. Для разработки — оставляем как есть.
    @Bean
    public SimpleClientHttpRequestFactory gigaChatRequestFactory() throws Exception {
        SSLContext sslContext = SSLContext.getInstance("TLS");
        sslContext.init(null, new TrustManager[]{new X509TrustManager() {
            public X509Certificate[] getAcceptedIssuers() { return new X509Certificate[0]; }
            public void checkClientTrusted(X509Certificate[] certs, String authType) {}
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
        requestFactory.setConnectTimeout(10_000);
        requestFactory.setReadTimeout(30_000);
        return requestFactory;
    }

    @Bean
    public RestClient gigaChatClient(
            SimpleClientHttpRequestFactory gigaChatRequestFactory,
            @Value("${gigachat.auth-key}") String authKey,
            @Value("${gigachat.scope}") String scope) {

        RestClient.Builder authClientBuilder = RestClient.builder()
                .requestFactory(gigaChatRequestFactory);

        return RestClient.builder()
                .baseUrl("https://api.giga.chat")
                .requestFactory(gigaChatRequestFactory)
                .requestInterceptor(new GigaChatAuthInterceptor(authKey, scope, authClientBuilder))
                .build();
    }
}