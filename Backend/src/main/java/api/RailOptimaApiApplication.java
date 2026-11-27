package api;

import io.quarkus.runtime.Quarkus;
import io.quarkus.runtime.annotations.QuarkusMain;

@QuarkusMain
public class RailOptimaApiApplication {
     static void main(String[] args) {
         Quarkus.run(args);
         System.out.print("Swagger-UI:http://localhost:8080/q/swagger-ui");
    }
}

