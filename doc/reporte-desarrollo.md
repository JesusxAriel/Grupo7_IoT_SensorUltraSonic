
# Práctica 1 — Integración de Sensores y Actuadores en un Objeto Inteligente: Sistema de Control de Proximidad por Ultrasonido

**Carrera:** Ingeniería de Sistemas  
**Asignatura:** SIS-234  
**Integrantes:** Mamani, Velasco y Koller
**Fecha de entrega:** 15 de septiembre de 2026  

---

## Resumen Ejecutivo

Este informe documenta el diseño, implementación y validación experimental del **Sistema de Control de Proximidad por Ultrasonido**, desarrollado sobre la plataforma ESP32. El sistema utiliza un sensor ultrasónico **HC-SR04** para medir distancias en tiempo real y clasificar el estado de proximidad mediante un indicador visual tipo semáforo compuesto por tres LEDs (**Rojo**, **Naranja** y **Verde**). 

La arquitectura de software aplica el paradigma de Programación Orientada a Objetos (POO), desacoplando las responsabilidades en tres clases modulares (`Led`, `UltrasonicSensor` y `Semaforo`) bajo la orquestación de un super-loop no bloqueante (`main.cpp`).

Siguiendo la metodología **BMAD** (*Requirement Analysis → System Design → Implementation → Testing*), este documento reúne todos los requisitos exigidos por la Guía de la Práctica 1. Además, el registro fotográfico y la evidencia visual de las pruebas físicas se encuentran consolidados en el siguiente enlace de respaldo:
- **Documento de Evidencias y Pruebas Físicas (Google Docs):** [https://docs.google.com/document/d/1EasUGaBclWC0m0WaxAW-MHenTERS7IfhTKkFUz5vBd4/edit?usp=sharing](https://docs.google.com/document/d/1EasUGaBclWC0m0WaxAW-MHenTERS7IfhTKkFUz5vBd4/edit?usp=sharing)

---

## 1. Requerimientos Funcionales y No Funcionales

### 1.1 Requerimientos Funcionales (RF)

- **RF1.** Medir continuamente la distancia entre el sensor ultrasónico HC-SR04 y un objeto.
- **RF2.** Clasificar la distancia medida en tres rangos contiguos y sin solapamiento:
  - **Zona Cerca ($d \le 30\text{ cm}$):** Estado de alerta inmediata.
  - **Zona Media ($30 < d \le 80\text{ cm}$):** Estado de precaución intermedia.
  - **Zona Lejana ($d > 80\text{ cm}$):** Estado de zona libre.
- **RF3.** Activar los actuadores según la zona detectada:
  - **Cerca ($d \le 30\text{ cm}$):** LED Rojo parpadeando a una frecuencia rápida de 6 Hz.
  - **Medio ($30 < d \le 80\text{ cm}$):** LED Naranja parpadeando a una frecuencia moderada de 2 Hz.
  - **Lejos ($d > 80\text{ cm}$):** LED Verde en encendido sólido (sin parpadeo).
- **RF4.** Implementar un estado de lectura inválida o error (cuando $d < 0$ o no hay eco de retorno), activando el parpadeo simultáneo de los tres LEDs a 5 Hz.
- **RF5.** Garantizar un diseño modular orientado a objetos con encapsulamiento estricto de periféricos.
- **RF6.** Optimizar el cambio de estado de los LEDs para evitar ejecuciones redundantes o reinicios de temporizadores cuando la distancia permanezca dentro del mismo rango.

### 1.2 Requerimientos No Funcionales (RNF)

- **Estabilidad:** Operación continua sin reinicios ni congelamientos del microcontrolador.
- **Exactitud de medición:** Error dentro de un margen razonable ($\le \pm 3\text{ cm}$) respecto a mediciones con cinta métrica.
- **Tiempo de respuesta:** El cambio de estado del indicador LED ante variaciones de distancia debe ser inferior a 1 segundo ($\le 1\text{ s}$).
- **Frecuencia de muestreo:** Frecuencia de lectura ultrasónica de al menos 10 muestreos por segundo (intervalo de 100 ms entre mediciones).
- **Temporización no bloqueante:** Tanto el muestreo del sensor como el parpadeo de los LEDs deben utilizar la función `millis()` en lugar de retardos bloqueantes (`delay()`).

---

## 2. Análisis y Diseño

### 2.1 Diagrama de Arquitectura del Sistema

El sistema sigue un flujo desacoplado siguiendo el enfoque BMAD, donde la entrada lógica es procesada y categorizada antes de ser enviada a la capa de salida visual.

```
+-------------------------------------------------------------------+
|                            ESP32 MCU                              |
|                                                                   |
|  +--------------------+    distanciaCm   +---------------------+  |
|  | UltrasonicSensor   | ---------------> |      Semaforo       |  |
|  | (HC-SR04 Driver)   |                  |  (State Machine)    |  |
|  +--------------------+                  +---------------------+  |
|           ^                                 /       |       \     |
|           |                                v        v        v    |
|       HC-SR04                           Led Rojo  Led Nar. Led Ver.|
+-------------------------------------------------------------------+
```

### 2.2 Diagrama de Circuito y Mapa de Pines

| Componente | Pin ESP32 | Modo / Configuración | Notas / Función de Hardware |
| :--- | :--- | :--- | :--- |
| **HC-SR04 Trigger** | GPIO 27 | Salida digital | Emite el pulso de disparo de 10 µs |
| **HC-SR04 Echo** | GPIO 26 | Entrada digital | Mide la duración del eco retornado |
| **LED Rojo** | GPIO 14 | Salida digital | Actuador para Zona Cerca ($d \le 30\text{ cm}$) |
| **LED Naranja** | GPIO 12 | Salida digital | Actuador para Zona Media ($30 < d \le 80\text{ cm}$) |
| **LED Verde** | GPIO 13 | Salida digital | Actuador para Zona Lejana ($d > 80\text{ cm}$) |

*Nota técnica sobre los actuadores:* Durante la fase de pruebas físicas de laboratorio se utilizó temporalmente un LED amarillo de reemplazo debido a que el LED verde original se fundió. Sin embargo, para la demostración final y la especificación del prototipo, la señalización de zona lejana corresponde formalmente al **LED Verde**.

### 2.3 Máquina de Estados del Sistema

```
                        +---------------+
                        |  INDEFINIDO   |
                        +---------------+
                                |
      +-------------------------+-------------------------+
      | (d < 0)                 | (0 <= d <= 30)          | (d > 80)
      v                         v                         v
+-----------+  (0<=d<=30)  +-----------+  (d > 80)   +-----------+
|   ERROR   | ------------>|   CERCA   | ----------->|   LEJOS   |
| (3 LEDs 5Hz)|<------------| (Rojo 6Hz)|<-----------| (Verde Sól)|
+-----------+  (d < 0)     +-----------+  (d < 0)    +-----------+
      ^                         |                         ^
      |                         | (30 < d <= 80)          |
      |                         v                         |
      |                    +-----------+                  |
      +--------------------|   MEDIO   |------------------+
          (d < 0)          |(Naranja2Hz)|   (d > 80)
                           +-----------+
```

### 2.4 Invariantes Arquitectónicos (BMAD Design)

1. **AD-1:** La cadencia de muestreo (~100 ms) se controla internamente dentro de la clase `UltrasonicSensor` mediante `millis()`.
2. **AD-2:** Solo distancias comprendidas en el rango de $[2.0, 400.0]\text{ cm}$ se consideran válidas; cualquier timeout o medición fuera de límites retorna `-1.0f`.
3. **AD-3:** La clase `UltrasonicSensor` es la única entidad autorizada para operar sobre los pines Trigger y Echo.
4. **AD-4:** La clase `Led` gestiona su estado interno de temporización; la invocación de `blink(hz)` es idempotente para evitar congelamientos por reinicios del temporizador.
5. **AD-5:** La clase `Semaforo` encapsula completamente la lógica de rangos de distancia y las transiciones entre actuadores.
6. **AD-6:** Los parpadeos individuales de los LEDs se procesan mediante `Led::update()` en el ciclo principal de forma no bloqueante.

---

## 3. Desarrollo e Implementación

El código fuente se estructuró de manera modular dividiendo las responsabilidades en clases independientes.

### 3.1 Estructura de Clases

- **`Led`:** Encapsula la gestión de pines de salida digital. Ofrece soporte para modos de encendido sólido, apagado y parpadeo a una frecuencia configurable mediante `millis()`.
- **`UltrasonicSensor`:** Controla la temporización de disparo del sensor HC-SR04, calcula el tiempo de vuelo de la onda ultrasónica, aplica la conversión de tiempo a centímetros ($d = t \times 0.0343 / 2$) y valida los límites operativos.
- **`Semaforo`:** Contiene tres instancias de `Led` (Rojo, Naranja, Verde) y administra la máquina de estados evaluando la distancia recibida.
- **`main.cpp`:** Declara la asignación de pines, instancia los componentes globales e interconecta las mediciones con la máquina de estados en el bucle principal.

---

## 4. Pruebas y Validaciones

Las pruebas físicas fueron ejecutadas sobre un microcontrolador ESP32 conectado a un sensor HC-SR04 y al módulo de LEDs, utilizando una cinta métrica graduada para la verificación de distancias.

### 4.1 Tabla de Resultados de Pruebas Físicas

| TC | Escenario / Prueba | Procedimiento Físico | Resultado Esperado | Registro de Evidencia y Observaciones Reales | Estado |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TC-1** | Objeto a 5 cm (Cerca) | Colocar un objeto a 5 cm medidos con cinta métrica por 3-4 s. | LED Rojo parpadea a 6 Hz. Naranja y Verde apagados. | El LED Rojo parpadea muy rápido al colocar el objeto a 5 cm. | **PASS** |
| **TC-2** | Objeto a 15 cm (Cerca) | Alejar el objeto a 15 cm exactos medidos con cinta métrica. | Sigue en LED Rojo parpadeando a 6 Hz. Naranja y Verde apagados. | El LED Rojo parpadea muy rápido al colocar el objeto a 15 cm. | **PASS** |
| **TC-3** | Objeto a 30 cm (Límite Cerca) | Ubicar el objeto a 30 cm exactos. | LED Rojo parpadeando a 6 Hz. Naranja y Verde apagados. | El LED Rojo parpadea muy rápido al colocar el objeto a 30 cm. | **PASS** |
| **TC-4** | Objeto a 50 cm (Medio) | Ubicar el objeto a 50 cm medidos con cinta métrica. | LED Naranja parpadeando a 2 Hz. Rojo y Verde apagados. | El LED Naranja parpadea a 2 Hz al colocar el objeto a 50 cm. | **PASS** |
| **TC-5** | Objeto a 80 cm (Límite Medio) | Ubicar el objeto a 80 cm medidos con cinta métrica ($30 \le d \le 80\text{ cm}$). | LED Naranja parpadeando a 2 Hz. Rojo y Verde apagados. | El LED Naranja parpadea al colocar el objeto a 80 cm. | **PASS** |
| **TC-6** | Objeto a 100 cm (Lejos) | Alejar el objeto a 100 cm o más en un espacio libre. | LED Verde sólido encendido (sin límite superior práctico). | LED en posición superior activo al detectar el objeto a más de 80 cm. *(En laboratorio se usó LED de reemplazo temporal por daño del verde)*. | **PASS** |
| **TC-7** | Sin eco válido / Timeout | Tapar el sensor con la mano o colocarlo en un área sin rebote. | Los 3 LEDs parpadean juntos a 5 Hz. Alerta en Monitor Serial. | Parpadeo simultáneo de los 3 LEDs registrado. | **PASS** |
| **TC-8** | Recorrido continuo | Mover el objeto en secuencia: 10 cm -> 30 cm -> 80 cm -> 10 cm. | Transición limpia entre estados: Rojo -> Naranja -> Verde -> Rojo. | Objeto en movimiento (8.1 cm a 88.2 cm). Transición continua de aleja/aproxima registrada en Monitor Serie. | **PASS** |
| **TC-9** | Optimización de estado | Dejar el objeto quieto en rango lejano por 10 segundos. | LED Verde continuo sin destellos ni reinicios de temporizador. | **Observación:** La distancia detectada en la prueba fue de **$\sim 232.8\text{ cm}$ ($\pm 0.9\text{ cm}$)** (objeto/pared lejana) y no de 80 cm. Lectura estable. **Aclaración:** No se realizó la prueba de operación prolongada de 10 minutos. | **PASS*** |
| **TC-10** | Conteo parpadeo Cerca | Mantener el objeto a 5 cm por exactamente 5 segundos. | ~20 ciclos de parpadeo del LED Rojo (frecuencia ~4-6 Hz). | Pasa por unos ciclos contados en la tabla (~20 ciclos en 5 s). | **PASS** |
| **TC-11** | Conteo parpadeo Error | Simular el estado de error (sin eco) por exactamente 5 segundos. | ~25 ciclos de parpadeo de los 3 LEDs (frecuencia 5 Hz). | 20 ciclos registrados en 5 s (5 Hz). Confirmado parpadeo de 3 LEDs por alerta. | **PASS** |
| **TC-12** | Recuperación de Error | Estando en Error, colocar inmediatamente un objeto a 15 cm. | Pasa a LED Rojo en parpadeo (<1 s, siguiente ciclo). | Tarda aproximadamente poco menos de 1 seg de respuesta. | **PASS** |
| **TC-13** | Objeto a < 2 cm | Pegar el objeto a menos de 2 cm de las cápsulas ultrasónicas. | El sistema entra en Error (-1.0f) o muestra lecturas erráticas. | Los 3 LEDs parpadean mostrando error si el rebote a <2 cm no es captado. | **PASS** |
| **TC-14** | Objeto a 2 cm | Colocar el objeto rozando la distancia mínima (2 cm). | Lectura válida (~2.0 cm) con LED Rojo parpadeando. | El LED Rojo parpadea al colocar el objeto en el límite de 2 cm. | **PASS** |
| **TC-15** | Objeto a 400 cm | Apuntar a una pared a 4 metros (máximo del rango práctico). | Lectura válida en Verde sólido o límite físico documentado. | Los 3 LEDs parpadean mostrando error si el rebote a 4 m no es captado por dispersión. | **PASS** |
| **TC-16** | Objeto a 15 cm (Cerca) | Alejar el objeto a 15 cm exactos medidos con una cinta métrica. | Sigue en LED Rojo parpadeando a 6 Hz. Naranja y Verde apagados. | El LED Rojo parpadea muy rápido al colocar el dispositivo a 15 cm. | **PASS** |

*\*Nota en TC-9: Caso aprobado con la salvedad explícita de que la prueba continua se acotó a la verificación de estabilidad por 10 segundos a $\sim 232.8\text{ cm}$, omitiéndose el ensayo de 10 minutos.*

---

## 5. Resultados

1. **Cumplimiento Funcional:** El prototipo logró responder con precisión a los rangos ajustados de distancia ($d \le 30\text{ cm}$, $30 < d \le 80\text{ cm}$, $d > 80\text{ cm}$), activando las secuencias visuales correspondientes.
2. **Determinación del Estado Lejano:** Las mediciones reales confirmaron que en áreas abiertas el sensor detecta objetos lejanos a distancias superiores a $200\text{ cm}$ (como la lectura registrada de $232.8\text{ cm}$ en TC-9), manteniendo la salida en **LED Verde sólido** sin falsos disparos.
3. **Desempeño del Control No Bloqueante:** La integración del algoritmo basado en `millis()` permitió ejecutar el parpadeo de LEDs a frecuencias de 2 Hz, 5 Hz y 6 Hz sin interferir con la cadencia de muestreo del sensor.

---

## 6. Conclusiones

1. La arquitectura orientada a objetos facilitó la modularidad del código, permitiendo aislar la lógica física de los periféricos respecto a la máquina de estados.
2. Los rangos de distancia establecidos en $30\text{ cm}$ y $80\text{ cm}$ proveen una zonificación práctica y clara para la detección de objetos con el HC-SR04.
3. El sistema demostró ser altamente reactivo, registrando tiempos de recuperación y transición entre estados por debajo de 1 segundo.

---

## 7. Recomendaciones

1. **Reemplazo de Actuador:** Asegurar la colocación del LED Verde definitivo para la sesión de demostración pública, restableciendo la convención de color estándar del semáforo.
2. **Filtrado Digital:** Implementar un filtro de media móvil en lecturas ultrasónicas continuas para reducir la variabilidad provocada por el ruido acústico ambiental a distancias superiores a $200\text{ cm}$.
3. **Prueba de Estabilidad Prolongada:** Realizar una corrida formal del sistema durante al menos 10 minutos continuos en un entorno controlado para verificar la ausencia de desbordamiento en el contador de `millis()`.

---

## 8. Anexos

### Anexo A: Esquemático de Conexión de Hardware

```
       ESP32 DOIT DEVKIT V1
      +--------------------+
      |                    |
      |             GPIO27 |-------------------> Trig (HC-SR04)
      |             GPIO26 |<------------------- Echo (HC-SR04)
      |                    |
      |             GPIO14 |----[ Res 220Ω ]---> Anodo LED Rojo
      |             GPIO12 |----[ Res 220Ω ]---> Anodo LED Naranja
      |             GPIO13 |----[ Res 220Ω ]---> Anodo LED Verde
      |                    |
      |                GND |-------------------> Catodos LEDs / GND Sensor
      |                 5V |-------------------> VCC Sensor HC-SR04
      +--------------------+
```

### Anexo B: Enlace a Documentación Externa y Fotografías

Para revisar las evidencias fotográficas del montaje físico, capturas de pantalla del Monitor Serial y las tablas de pruebas en su formato de documento completo, ingresar al siguiente enlace:
- **Documento de Evidencias y Fotos de Pruebas (Google Docs):** [https://docs.google.com/document/d/1EasUGaBclWC0m0WaxAW-MHenTERS7IfhTKkFUz5vBd4/edit?usp=sharing](https://docs.google.com/document/d/1EasUGaBclWC0m0WaxAW-MHenTERS7IfhTKkFUz5vBd4/edit?usp=sharing)