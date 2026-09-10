# 🔌 Esquemático y Diagrama de Conexiones

## Diagrama de Bloques del Hardware

```text
               +-----------------------------------+
               |           ESP32 DevKit            |
               |                                   |
               |  [3V3] ----------------> VCC OLED |
               |  [GND] ----------------> GND OLED |
               |  [GPIO 21] ------------> SDA OLED |
               |  [GPIO 22] ------------> SCL OLED |
               |                                   |
               |  [GPIO 14] ------------> Botón/SW |
               |                          (a GND)  |
               |                                   |
               |  [Vin / 5V] <---------- Micro-USB |
               +-----------------------------------+
```

---

## Detalle del Circuito

1. **Interfaz I2C (Pantalla OLED SSD1306):**
   - El bus I2C del ESP32 utiliza por defecto los pines **GPIO 21 (SDA)** y **GPIO 22 (SCL)**.
   - La pantalla opera con niveles lógicos de 3.3V suministrados directamente desde el pin `3V3` del ESP32.

2. **Pulsador de Navegación de Pantalla:**
   - Conectado entre **GPIO 14** y **GND**.
   - Se utiliza la resistencia interna de Pull-Up (`INPUT_PULLUP`) configurada en el firmware, por lo que no se requiere resistencia física externa.
