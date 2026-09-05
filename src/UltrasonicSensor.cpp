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