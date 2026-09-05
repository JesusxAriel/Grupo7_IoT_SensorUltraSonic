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

UltrasonicSensor sensor(Pines::TRIG, Pines::ECHO);
Semaforo semaforo(Pines::VERDE, Pines::AMARILLO, Pines::ROJO);

// Control de tiempo para imprimir por puerto serie sin saturar
unsigned long lastPrintMs = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 250; // Imprime cada 250 ms

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