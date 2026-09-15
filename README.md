# Sistema de Control de Proximidad por Ultrasonido (ESP32) 

Este repositorio contiene la implementación y documentación del **Sistema de Control de Proximidad por Ultrasonido**, desarrollado sobre una tarjeta **ESP32** como parte de la **Práctica 1** de la asignatura **SIS-234 (Ingeniería de Sistemas)**.

El objetivo del proyecto es medir distancias en tiempo real usando un sensor ultrasónico **HC-SR04** y clasificar la proximidad de un objeto mediante un indicador visual tipo semáforo compuesto por 3 LEDs (Rojo, Naranja/Amarillo y Verde).

---

## 📁 Estructura de la carpeta `doc/`

Toda la documentación técnica del proyecto se encuentra centralizada en la carpeta [`doc/`](./doc/):

* **[`doc/reporte-desarrollo.md`](./doc/reporte-desarrollo.md):** 
  * Es el informe técnico principal redactado bajo la metodología **BMAD** (*Requirement Analysis → System Design → Implementation → Testing*).
  * Contiene los requerimientos funcionales y no funcionales, diagramas de bloques, máquina de estados, código fuente explicado, tabla de pruebas físicas (TC-1 a TC-16) y conclusiones.
  * **Cumplimiento con la Práctica 1:** Este reporte cumple punto por punto con la guía de la práctica, abarcando la clasificación en 3 rangos de distancia (`Cerca`, `Medio`, `Lejos`), lógica de estado de `Error`, tiempos de respuesta no bloqueantes con `millis()`, frecuencia de muestreo y pruebas de estabilidad.
* **[`doc/diagrama-cableado.html`](./doc/diagrama-cableado.html):** 
  * Es un archivo interactivo/visual que detalla el **diagrama de conexiones del circuito**.
  * Explica explícitamente cómo está conectado el hardware: los pines de disparo (`TRIG`) y eco (`ECHO`) del sensor HC-SR04 a la ESP32, así como los ánodos de los LEDs (Rojo, Naranja, Verde) con sus respectivas resistencias delimitadoras hacia los pines GPIO del microcontrolador y la línea común de tierra (`GND`).

---

## 🛠️ Hardware Utilizado

| Componente | Pin ESP32 | Función / Descripción |
| :--- | :--- | :--- |
| **HC-SR04 Trig** | GPIO 27 | Salida digital (Disparo de pulso 10 µs) |
| **HC-SR04 Echo** | GPIO 26 | Entrada digital (Medición de eco) |
| **LED Rojo** | GPIO 14 | Indicador para Zona Cerca ($d \le 30\text{ cm}$) - Parpadeo a 6 Hz |
| **LED Naranja** | GPIO 12 | Indicador para Zona Media ($30 < d \le 80\text{ cm}$) - Parpadeo a 2 Hz |
| **LED Verde** | GPIO 13 | Indicador para Zona Lejana ($d > 80\text{ cm}$) - Luz sólida |

---

## 💻 Arquitectura de Software

El código está estructurado bajo **Programación Orientada a Objetos (POO)** y es totalmente **no bloqueante** (utiliza `millis()` en lugar de `delay()`):

* **`Led`:** Clase que encapsula el control individual de los LEDs, soportando estados encendido, apagado y parpadeo a una frecuencia configurable en Hz (idempotente).
* **`UltrasonicSensor`:** Clase para el manejo del HC-SR04, realizando muestreos periódicos y convirtiendo el tiempo de vuelo a centímetros.
* **`Semaforo`:** Clase orquestadora que contiene los tres LEDs y maneja la máquina de estados según la distancia procesada.
* **`main.cpp`:** Super-loop principal que coordina el flujo de datos y el reporte por Monitor Serial (115200 baudios).

---

## 👥 Integrantes - Grupo 7

* **Repositorio:** [Grupo7_IoT_SensorUltraSonic](https://github.com/JesusxAriel/Grupo7_IoT_SensorUltraSonic.git)
* **Carrera:** Ingeniería de Sistemas — SIS-234 - Universidad Catolica Boliviana San Pablo
