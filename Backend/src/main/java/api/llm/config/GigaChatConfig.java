package api.llm.config;

import io.smallrye.config.ConfigMapping;
import io.smallrye.config.WithDefault;

@ConfigMapping(prefix = "gigachat")
public interface GigaChatConfig {

    String authKey(); // Из application.yml: gigachat.auth-key

    @WithDefault("https://gigachat.devices.sberbank.ru/api/v1")
    String apiUrl(); // gigachat.api-url

    @WithDefault("GIGACHAT_API_PERS")
    String scope(); // gigachat.scope

    @WithDefault("GigaChat")
    String model(); // gigachat.model
}