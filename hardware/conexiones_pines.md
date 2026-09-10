# 📍 Tabla de Mapeo de Pines GPIO

| Pin ESP32 | Función del Sistema | Modo / Configuración | Conexión Destino |
|-----------|---------------------|----------------------|------------------|
| **GPIO 21** | I2C Data (SDA) | Salida / Entrada Bidireccional | Pin `SDA` de Pantalla OLED |
| **GPIO 22** | I2C Clock (SCL) | Salida de Reloj I2C | Pin `SCL` de Pantalla OLED |
| **GPIO 14** | Botón de Navegación | Entrada Digital (`INPUT_PULLUP`) | Terminal 1 del Pulsador (Terminal 2 a GND) |
| **3V3** | Alimentación 3.3V | Salida de Alimentación | Pin `VCC` de Pantalla OLED |
| **GND** | Tierra común | Referencia 0V | Pin `GND` de Pantalla OLED y Botón |
| **Vin (5V)** | Entrada de Voltaje | Entrada de Alimentación externa | Conector USB / Fuente externa 5V |

---

## Notas de Hardware
- Los pines **GPIO 6 a 11** están reservados internamente para la memoria SPI Flash integrada y **no deben utilizarse**.
- El pin **GPIO 14** está libre de restricciones de arranque (strapping pin), lo que garantiza un boot seguro sin interferir con el estado del microcontrolador al encender.
