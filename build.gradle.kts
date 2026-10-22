
plugins {
    java
    id("io.quarkus") version "3.40.1"
    id("io.freefair.lombok") version "9.8.0"
}

group = "com.railoptima"
version = "0.1.0-SNAPSHOT"

java {
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(25))
    }
}
dependencies {
    // Основная платформа Quarkus BOM
    implementation(platform("io.quarkus.platform:quarkus-bom:3.40.1"))

    // Core REST & Валидация
    implementation("io.quarkus:quarkus-rest")
    implementation("io.quarkus:quarkus-rest-jackson")
    implementation("io.quarkus:quarkus-hibernate-validator")

    // REST Client для GigaChat (свой клиент с OAuth)
    implementation("io.quarkus:quarkus-rest-client")
    implementation("io.quarkus:quarkus-rest-client-jackson")

    // Мониторинг
    implementation("io.quarkus:quarkus-smallrye-health")
    implementation("io.quarkus:quarkus-micrometer-registry-prometheus")

    // Security & JWT (нативный SmallRye)
    implementation("io.quarkus:quarkus-security")
    implementation("io.quarkus:quarkus-smallrye-jwt")
    implementation("io.quarkus:quarkus-smallrye-jwt-build")
    implementation("io.quarkus:quarkus-elytron-security-common")

    // OpenAPI / Swagger UI
    implementation("io.quarkus:quarkus-smallrye-openapi")

    // Persistence
    implementation("io.quarkus:quarkus-hibernate-orm-panache")
    implementation("io.quarkus:quarkus-jdbc-postgresql")
    implementation("io.quarkus:quarkus-flyway")

}

sourceSets {
    main {
        java.srcDirs(file("Backend/src/main/java"))
        resources.srcDirs(file("Backend/src/main/resources"))
    }
    test {
        java.srcDirs(file("Backend/src/test/java"))
        resources.srcDirs(file("Backend/src/test/resources"))
    }
}

// Ленивая конфигурация компиляции
tasks.withType<JavaCompile>().configureEach {
    options.encoding = "UTF-8"
    options.compilerArgs.add("-Xlint:deprecation")
}

// Ленивая конфигурация тестов (не ломает Configuration Cache)
tasks.withType<Test>().configureEach {
    useJUnitPlatform()
    enabled = false
}