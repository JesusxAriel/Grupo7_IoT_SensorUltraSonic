================================================================================
                    SISTEMA DE SEMÁFORO ULTRASONICO - ESP32 / ARDUINO
================================================================================

--------------------------------------------------------------------------------
1. ARQUITECTURA Y RELACIÓN ENTRE ARCHIVOS Y CLASES
--------------------------------------------------------------------------------

El proyecto está diseñado bajo los principios de programación orientada a objetos (POO),
modularidad, encapsulamiento y arquitectura basada en máquinas de estado/no bloqueante
usando la función `millis()` de Arduino.

A continuación se detalla cómo se relacionan las clases y archivos:

                    ┌────────────────────────┐
                    │       main.cpp         │
                    │   (Punto de entrada)   │
                    └──────────┬─────────────┘
                               │
            ┌──────────────────┴──────────────────┐
            ▼                                     ▼
   ┌──────────────────┐                  ┌──────────────────┐
   │ UltrasonicSensor │                  │     Semaforo     │
   └──────────────────┘                  └────────┬─────────┘
                                                  │ (Contiene 3 instancias)
                                                  ▼
                                           ┌─────────────┐
                                           │     Led     │
                                           └─────────────┘

1. **Clase Led (Led.h / Led.cpp)**:
   - Representa el componente de hardware más básico (un LED individual).
   - Encapsula el control digital del pin (HIGH/LOW) y la lógica de parpadeo no bloqueante.
   - Modos soportados: Apagado, Sólido (Encendido) y Parpadeo (a frecuencia personalizada en Hz).
   - **Relación**: Es una clase base o componente fundamental. No depende de ningún otro archivo local.

2. **Clase Semaforo (Semaforo.h / Semaforo.cpp)**:
   - Actúa como una fachada o controlador de alto nivel que compone 3 objetos de la clase `Led`
     (verde, amarillo y rojo).
   - Gestiona los estados del semáforo según rangos de distancia (`Cerca`, `Medio`, `Lejos`, `Error`).
   - Traduce los cambios de rango en órdenes específicas para los LEDs (encender, apagar, parpadear a X Hz).
   - **Relación**: Depende de `Led.h` (composición). Contiene las instancias `verde_`, `amarillo_` y `rojo_`.

3. **Clase UltrasonicSensor (UltrasonicSensor.h / UltrasonicSensor.cpp)**:
   - Encapsula la interacción con el sensor ultrasónico HC-SR04 (pines TRIG y ECHO).
   - Realiza mediciones periódicas sin bloquear la ejecución global mediante temporización interna.
   - Procesa la duración del pulso `pulseIn`, calcula la distancia en centímetros y valida rangos válidos (2-400 cm).
   - **Relación**: Es un módulo independiente que no conoce la existencia del semáforo.

4. **Archivo Principal (main.cpp)**:
   - Modulo de orquestación principal.
   - Define la configuración de pines hardware (`Pines::TRIG`, `Pines::ECHO`, `Pines::VERDE`, etc.).
   - Instancia globalmente `sensor` y `semaforo`.
   - En `setup()`: Inicializa el puerto serie e inicializa los periféricos (`sensor.begin()`, `semaforo.begin()`).
   - En `loop()`:
     a) Llama a `sensor.update()` y `semaforo.update()` para mantener activas las máquinas de estado sin demoras (no-blocking).
     b) Obtiene la distancia calculada (`sensor.getDistanceCM()`).
     c) Envía la distancia a `semaforo.procesarDistancia(distancia)` para actualizar los estados visuales si corresponde.
     d) Imprime periódicamente información de monitoreo y alertas por el puerto serie (115200 baudios).


================================================================================
2. CÓDIGO FUENTE DE TODOS LOS ARCHIVOS DEL PROYECTO
================================================================================

--- ARCHIVO: main.cpp ---
```cpp
#include <Arduino.h>
#include "UltrasonicSensor.h"
#include "Semaforo.h"

namespace Pines {
    constexpr uint8_t TRIG = 27;
    constexpr uint8_t ECHO = 26;
    constexpr uint8_t VERDE = 13;
    constexpr uint8_t AMARILLO = 12;
    constexpr uint8_t ROJO = 14;
}

UltrasonicSensor sensor(Pines::TRIG, Pines::ECHO, 350);
Semaforo semaforo(Pines::VERDE, Pines::AMARILLO, Pines::ROJO);

// Control de tiempo para imprimir por puerto serie sin saturar
unsigned long lastPrintMs = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 350; // Imprime cada 350 ms

void setup() {
    Serial.begin(115200);
    sensor.begin();
    semaforo.begin();
    Serial.println("\n--- Sistema de Semaforo Ultrasonico Iniciado ---");
}

void loop() {
    // Avance de máquinas de estado
    sensor.update();
    semaforo.update();

    // Obtener distancia del sensor
    float distancia = sensor.getDistanceCM();

    // Actualización orientada a eventos
    semaforo.procesarDistancia(distancia);

    // Impresión en puerto serie
    if (millis() - lastPrintMs >= PRINT_INTERVAL_MS) {
        lastPrintMs = millis();

        // Si la distancia es negativa o inválida (fuera de rango 2-400 cm o timeout)
        if (distancia < 0 || distancia < 2.0f || distancia > 400.0f) {
            Serial.println("[ALERTA] Lectura invalida / Error de Sensor: ¡Parpadeando los 3 LEDs a la vez!");
        } else {
            Serial.print("Distancia objeto: ");
            Serial.print(distancia, 1);
            Serial.println(" cm");
        }
    }
}
```

--- ARCHIVO: Led.h ---
```cpp
#pragma once
#include <Arduino.h>

class Led {
private:
    uint8_t pin_;
    enum class Modo { Apagado, Solido, Parpadeo };
    Modo modo_ = Modo::Apagado;

    float frecuenciaHz_ = 0.0f;
    unsigned long intervaloMs_ = 0;
    unsigned long ultimoCambioMs_ = 0;
    bool estadoPin_ = false;

public:
    explicit Led(uint8_t pin);
    
    void begin();
    void turnOff();
    void turnOn();
    void blink(float hz);
    void update();
};
```

--- ARCHIVO: Led.cpp ---
```cpp
#include "Led.h"

Led::Led(uint8_t pin) : pin_(pin) {}

void Led::begin() {
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
}

void Led::turnOff() {
    modo_ = Modo::Apagado;
    estadoPin_ = false;
    digitalWrite(pin_, LOW);
}

void Led::turnOn() {
    modo_ = Modo::Solido;
    estadoPin_ = true;
    digitalWrite(pin_, HIGH);
}

void Led::blink(float hz) {
    if (hz <= 0.0f) {
        turnOff();
        return;
    }

    unsigned long nuevoIntervalo = static_cast<unsigned long>(1000.0f / (hz * 2.0f));

    // Solo reinicia si el modo o la frecuencia cambiaron realmente (idempotente)
    if (modo_ != Modo::Parpadeo || frecuenciaHz_ != hz) {
        modo_ = Modo::Parpadeo;
        frecuenciaHz_ = hz;
        intervaloMs_ = nuevoIntervalo;
        ultimoCambioMs_ = millis();
        estadoPin_ = true;
        digitalWrite(pin_, HIGH);
    }
}

void Led::update() {
    if (modo_ != Modo::Parpadeo) return;

    unsigned long tiempoActual = millis();
    if (tiempoActual - ultimoCambioMs_ >= intervaloMs_) {
        ultimoCambioMs_ = tiempoActual;
        estadoPin_ = !estadoPin_;
        digitalWrite(pin_, estadoPin_);
    }
}
```

--- ARCHIVO: Semaforo.h ---
```cpp
#pragma once
#include "Led.h"

class Semaforo {
private:
    Led verde_;
    Led amarillo_;
    Led rojo_;

    enum class RangoDistancia { Indefinido, Error, Cerca, Medio, Lejos };
    RangoDistancia rangoActual_ = RangoDistancia::Indefinido;

public:
    Semaforo(uint8_t pinVerde, uint8_t pinAmarillo, uint8_t pinRojo);

    void begin();
    void update();
    void procesarDistancia(float distanciaCm);
};
```

--- ARCHIVO: Semaforo.cpp ---
```cpp
#include "Semaforo.h"

Semaforo::Semaforo(uint8_t pinVerde, uint8_t pinAmarillo, uint8_t pinRojo)
    : verde_(pinVerde), amarillo_(pinAmarillo), rojo_(pinRojo) {}

void Semaforo::begin() {
    verde_.begin();
    amarillo_.begin();
    rojo_.begin();
}

void Semaforo::update() {
    verde_.update();
    amarillo_.update();
    rojo_.update();
}

void Semaforo::procesarDistancia(float distanciaCm) {
    RangoDistancia nuevoRango;

    // 1. Clasificación del rango
    if (distanciaCm < 0.0f) {
        nuevoRango = RangoDistancia::Error;
    } else if (distanciaCm <= 30.0f) {
        nuevoRango = RangoDistancia::Cerca;
    } else if (distanciaCm <= 80.0f) {
        nuevoRango = RangoDistancia::Medio;
    } else {
        nuevoRango = RangoDistancia::Lejos;
    }

    // 2. Si el rango no ha cambiado, no tocamos la configuración de los LEDs
    if (nuevoRango == rangoActual_) return;

    rangoActual_ = nuevoRango;

    // 3. Aplicación de estados según el nuevo rango
    switch (rangoActual_) {
        case RangoDistancia::Cerca:
            rojo_.blink(6.0f); // Parpadeo rápido en rojo
            amarillo_.turnOff();
            verde_.turnOff();
            break;

        case RangoDistancia::Medio:
            rojo_.turnOff();
            amarillo_.blink(2.0f); // Parpadeo lento en amarillo
            verde_.turnOff();
            break;

        case RangoDistancia::Lejos:
            rojo_.turnOff();
            amarillo_.turnOff();
            verde_.turnOn(); // Verde sólido
            break;

        case RangoDistancia::Error:
        default:
            // Los 3 LEDs parpadean rápido a 5 Hz demostrando error crítico
            rojo_.blink(5.0f);
            amarillo_.blink(5.0f);
            verde_.blink(5.0f);
            break;
    }
}
```

--- ARCHIVO: UltrasonicSensor.h ---
```cpp
#pragma once
#include <Arduino.h>

class UltrasonicSensor {
private:
    uint8_t pinTrig_;
    uint8_t pinEcho_;
    unsigned long ultimoMuestreoMs_ = 0;
    unsigned long intervaloMuestreoMs_ = 100;
    float ultimaDistancia_ = -1.0f;

    void realizarMedicion();

public:
    UltrasonicSensor(uint8_t pinTrig, uint8_t pinEcho, unsigned long intervaloMs = 100);

    void begin();
    void update();
    float getDistanceCM() const;
};
```

--- ARCHIVO: UltrasonicSensor.cpp ---
```cpp
#include "UltrasonicSensor.h"

UltrasonicSensor::UltrasonicSensor(uint8_t pinTrig, uint8_t pinEcho, unsigned long intervaloMs)
    : pinTrig_(pinTrig), pinEcho_(pinEcho), intervaloMuestreoMs_(intervaloMs) {}

void UltrasonicSensor::begin() {
    pinMode(pinTrig_, OUTPUT);
    pinMode(pinEcho_, INPUT);
    digitalWrite(pinTrig_, LOW);
}

void UltrasonicSensor::update() {
    unsigned long tiempoActual = millis();
    if (tiempoActual - ultimoMuestreoMs_ >= intervaloMuestreoMs_) {
        ultimoMuestreoMs_ = tiempoActual;
        realizarMedicion();
    }
}

void UltrasonicSensor::realizarMedicion() {
    digitalWrite(pinTrig_, LOW);
    delayMicroseconds(2);
    digitalWrite(pinTrig_, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinTrig_, LOW);

    // Timeout de 25000 us (~400 cm máximo)
    unsigned long duracionUs = pulseIn(pinEcho_, HIGH, 25000);

    if (duracionUs == 0) {
        ultimaDistancia_ = -1.0f; // Error o fuera de rango
    } else {
        float distancia = (duracionUs * 0.0343f) / 2.0f;
        if (distancia >= 2.0f && distancia <= 400.0f) {
            ultimaDistancia_ = distancia;
        } else {
            ultimaDistancia_ = -1.0f;
        }
    }
}

float UltrasonicSensor::getDistanceCM() const {
    return ultimaDistancia_;
}
```