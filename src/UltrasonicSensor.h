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