# PROYECTO: SISTEMA DE CONTROL DE PROXIMIDAD POR ULTRASONIDO (ESP32)

## DESCRIPCIÓN
Sistema no bloqueante basado en máquinas de estado para medir distancia mediante un sensor **HC-SR04** y controlar la respuesta visual de un semáforo de 3 LEDs. Todas las operaciones de temporización utilizan `millis()`.

## ESTRUCTURA DEL PROYECTO

```text
├── Led.h
├── Led.cpp
├── UltrasonicSensor.h
├── UltrasonicSensor.cpp
├── Semaforo.h
├── Semaforo.cpp
└── main.cpp
```

---

## ARCHIVOS DEL CÓDIGO

### `Led.h`

```cpp
#pragma once
#include <Arduino.h>

/**
 * @brief Controlador no bloqueante para diodos LED.
 * Maneja modos de encendido sólido, apagado y parpadeo mediante frecuencias en Hertz (Hz).
 */
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

---

### `Led.cpp`

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

    // Calcula el tiempo de alternancia en milisegundos (medio período)
    unsigned long nuevoIntervalo = static_cast<unsigned long>(1000.0f / (hz * 2.0f));

    // Garantiza idempotencia: solo reinicia el temporizador si cambia el modo o la frecuencia
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

---

### `UltrasonicSensor.h`

```cpp
#pragma once
#include <Arduino.h>

/**
 * @brief Controlador para sensor de ultrasonido (HC-SR04).
 * Realiza muestreos en una cadencia fija (100 ms) sin congelar la ejecución.
 */
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
    
    /**
     * @return Distancia en centímetros (2.0 a 400.0 cm) o -1.0f en caso de error/fuera de rango.
     */
    float getDistanceCM() const;
};
```

---

### `UltrasonicSensor.cpp`

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

    // Timeout asignado a 25.000 µs (aprox. 400 cm máximo) para evitar cuelgues
    unsigned long duracionUs = pulseIn(pinEcho_, HIGH, 25000);

    if (duracionUs == 0) {
        ultimaDistancia_ = -1.0f;
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

---

### `Semaforo.h`

```cpp
#pragma once
#include "Led.h"

/**
 * @brief Módulo encapsulador para la gestión del semáforo de 3 colores.
 * Traduce lecturas absolutas en rangos operacionales de respuesta.
 */
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

---

### `Semaforo.cpp`

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

    // 1. Evaluación de rangos
    if (distanciaCm < 0.0f) {
        nuevoRango = RangoDistancia::Error;
    } else if (distanciaCm <= 10.0f) {
        nuevoRango = RangoDistancia::Cerca;
    } else if (distanciaCm <= 25.0f) {
        nuevoRango = RangoDistancia::Medio;
    } else {
        nuevoRango = RangoDistancia::Lejos;
    }

    // 2. Control de cambios de estado (evita re-ejecución de escrituras si el rango no cambia)
    if (nuevoRango == rangoActual_) return;

    rangoActual_ = nuevoRango;

    // 3. Actuación según el nuevo rango determinado
    switch (rangoActual_) {
        case RangoDistancia::Cerca:
            rojo_.blink(4.0f);  // Rojo parpadea a 4 Hz
            amarillo_.turnOff();
            verde_.turnOff();
            break;

        case RangoDistancia::Medio:
            rojo_.turnOff();
            amarillo_.turnOn(); // Amarillo sólido
            verde_.turnOff();
            break;

        case RangoDistancia::Lejos:
            rojo_.turnOff();
            amarillo_.turnOff();
            verde_.turnOn();    // Verde sólido
            break;

        case RangoDistancia::Error:
        default:
            // Alerta global: Parpadeo de los 3 LEDs a 5 Hz
            rojo_.blink(5.0f);
            amarillo_.blink(5.0f);
            verde_.blink(5.0f);
            break;
    }
}
```

---

### `main.cpp`

```cpp

#include <Arduino.h>
#include "UltrasonicSensor.h"
#include "Semaforo.h"

// =====================================================
// CONFIGURACIÓN DE PINES - ESP32
// =====================================================
namespace Pines {
    constexpr uint8_t TRIG = 27;
    constexpr uint8_t ECHO = 26;

    constexpr uint8_t VERDE = 13;
    constexpr uint8_t AMARILLO = 12;
    constexpr uint8_t ROJO = 14;
}

// =====================================================
// INSTANCIACIÓN DE COMPONENTES
// =====================================================
UltrasonicSensor sensor(Pines::TRIG, Pines::ECHO);
Semaforo semaforo(Pines::VERDE, Pines::AMARILLO, Pines::ROJO);

// =====================================================
// CONTROL DE IMPRESIÓN POR PUERTO SERIAL
// =====================================================
unsigned long lastPrintMs = 0;

// Imprimir información cada 250 ms
constexpr unsigned long PRINT_INTERVAL_MS = 250;

// =====================================================
// SETUP
// =====================================================
void setup() {
    Serial.begin(115200);

    // Inicializar sensor ultrasónico
    sensor.begin();

    // Inicializar semáforo
    semaforo.begin();

    Serial.println();
    Serial.println("--- Sistema de Semaforo Ultrasonico Iniciado ---");
    Serial.println("Sensor HC-SR04 listo.");
    Serial.println("Semaforo listo.");
    Serial.println();
}

// =====================================================
// LOOP PRINCIPAL
// =====================================================
void loop() {

    // -------------------------------------------------
    // 1. Actualizar máquinas de estado
    // -------------------------------------------------
    sensor.update();
    semaforo.update();

    // -------------------------------------------------
    // 2. Obtener distancia medida por el sensor
    // -------------------------------------------------
    float distancia = sensor.getDistanceCM();

    // -------------------------------------------------
    // 3. Procesar distancia y actualizar semáforo
    // -------------------------------------------------
    semaforo.procesarDistancia(distancia);

    // -------------------------------------------------
    // 4. Mostrar información por el Monitor Serial
    // -------------------------------------------------
    if (millis() - lastPrintMs >= PRINT_INTERVAL_MS) {

        lastPrintMs = millis();

        // Verificar si la lectura es inválida
        if (distancia < 0.0f ||
            distancia < 2.0f ||
            distancia > 400.0f) {

            Serial.println(
                "[ALERTA] Lectura invalida / Error de Sensor: "
                "¡Parpadeando los 3 LEDs a la vez!"
            );

        } else {

            Serial.print("Distancia objeto: ");
            Serial.print(distancia, 1);
            Serial.println(" cm");
        }
    }
}
```