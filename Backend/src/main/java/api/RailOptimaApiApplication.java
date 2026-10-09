package api;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.context.properties.ConfigurationPropertiesScan;

@SpringBootApplication
@ConfigurationPropertiesScan
public class RailOptimaApiApplication {
     static void main(String[] args) {
         SpringApplication.run(RailOptimaApiApplication.class, args);
    }
}

