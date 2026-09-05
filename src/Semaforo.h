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