
package api.llm.service;

import api.llm.client.GigaChatClient;
import jakarta.enterprise.context.ApplicationScoped;
import jakarta.inject.Inject;
import org.jboss.logging.Logger;

@ApplicationScoped
public class LlmService {

    private static final Logger log = Logger.getLogger(LlmService.class);

    @Inject
    GigaChatClient gigaChatClient;

    public String ask(String question) {
        try {
            return gigaChatClient.chat(question);
        } catch (IllegalStateException e) {
            log.error("Ошибка при обращении к GigaChat", e);
            throw new IllegalStateException(
                    "GigaChat временно недоступен, попробуйте позже",
                    e
            );
        }
    }
}
