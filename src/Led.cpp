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