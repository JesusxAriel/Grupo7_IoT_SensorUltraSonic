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
    } else if (distanciaCm <= 10.0f) {
        nuevoRango = RangoDistancia::Cerca;
    } else if (distanciaCm <= 25.0f) {
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
            rojo_.blink(4.0f); // Parpadeo rápido en rojo
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