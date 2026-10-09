plugins {
    java
    id("org.springframework.boot") version "4.1.1"
}

group = "com.railoptima"
version = "0.1.0-SNAPSHOT"

java {
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(25))
    }
}

configurations {
    compileOnly {
        extendsFrom(configurations.annotationProcessor.get())
    }
}

repositories {
    mavenCentral()
}

val jjwtVersion = "0.13.0"
val lombokVersion = "1.18.48"
val springdocVersion = "3.1.1"

dependencies {
    // Встроенный нативный BOM управления зависимостями Spring Boot 4.x
    implementation(platform("org.springframework.boot:spring-boot-dependencies:4.1.1"))

    // Встроенный нативный BOM управления зависимостями Spring AI 2.x
    implementation(platform("org.springframework.ai:spring-ai-bom:2.0.1"))

    // Web & Validation
    implementation("org.springframework.boot:spring-boot-starter-web")
    implementation("org.springframework.boot:spring-boot-starter-validation")
    implementation("org.springframework.boot:spring-boot-starter-actuator")

    // Security
    implementation("org.springframework.boot:spring-boot-starter-security")

    // JWT (JJWT)
    implementation("io.jsonwebtoken:jjwt-api:$jjwtVersion")
    runtimeOnly("io.jsonwebtoken:jjwt-impl:$jjwtVersion")
    runtimeOnly("io.jsonwebtoken:jjwt-jackson:$jjwtVersion")

    // OpenAPI / Swagger
    implementation("org.springdoc:springdoc-openapi-starter-webmvc-ui:$springdocVersion")

    // Database & Migrations
    implementation("org.springframework.boot:spring-boot-starter-data-jpa")
    runtimeOnly("org.postgresql:postgresql")
    implementation("org.springframework.boot:spring-boot-starter-flyway")
    implementation("org.flywaydb:flyway-database-postgresql")

    // Lombok (Добавлен также в тест-контекст, чтобы у Gradle 9.x не было ворнингов при сборке тестов)
    compileOnly("org.projectlombok:lombok:$lombokVersion")
    annotationProcessor("org.projectlombok:lombok:$lombokVersion")
    testCompileOnly("org.projectlombok:lombok:$lombokVersion")
    testAnnotationProcessor("org.projectlombok:lombok:$lombokVersion")

    // Test
    testImplementation("org.springframework.boot:spring-boot-starter-test")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

sourceSets {
    main {
        java.srcDirs("Backend/src/main/java")
        resources.srcDirs("Backend/src/main/resources")
    }
    test {
        java.srcDirs("Backend/src/test/java")
        resources.srcDirs("Backend/src/test/resources")
    }
}

tasks.withType<Test> {
    useJUnitPlatform()
}

tasks.withType<org.springframework.boot.gradle.tasks.run.BootRun> {
    val envFile = rootProject.file(".env")
    if (envFile.exists()) {
        envFile.readLines().forEach { line ->
            if (line.isNotBlank() && !line.startsWith("#") && line.contains("=")) {
                val (key, value) = line.split("=", limit = 2)
                environment(key.trim(), value.trim())
            }
        }
    }
}
