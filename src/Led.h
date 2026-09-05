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