# ⚙️ Guía de Instalación y Carga de Firmware

## Requisitos Previos
- Cable de datos Micro-USB de buena calidad.
- Módulo ESP32-WROOM-32.
- Pantalla OLED I2C 0.96" SSD1306 (128x64).
- Computadora con Windows 10/11, Linux o macOS.

---

## Opción A: Instalación mediante Arduino IDE

### Paso 1: Descarga e Instalación de Arduino IDE
Descargar la última versión desde el sitio oficial de [Arduino](https://www.arduino.cc/en/software).

### Paso 2: Instalación del Core ESP32
1. En Arduino IDE, ir a **Archivo > Preferencias**.
2. En el campo *Gestor de URLs Adicionales de Tarjetas*, agregar:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Ir a **Herramientas > Placa > Gestor de Tarjetas**, buscar `esp32` e instalar **esp32 by Espressif Systems**.

### Paso 3: Instalación de Librerías
Abrir el Gestor de Librerías (**Herramientas > Administrar Bibliotecas**) e instalar:
- `ArduinoJson` (versión 6.21.x)
- `U8g2` (versión 2.35.x)
- `ESPAsyncWebServer` y `AsyncTCP` (descargar e instalar desde archivos .ZIP de GitHub si no aparecen en el gestor oficial).

### Paso 4: Configuración de la Placa
En el menú **Herramientas**, seleccionar:
- **Placa:** "ESP32 Dev Module"
- **Flash Size:** "4MB (32Mb)"
- **Partition Scheme:** "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
- **Upload Speed:** "921600"
- **Puerto:** Seleccionar el puerto COM correspondiente a la placa conectada.

### Paso 5: Compilación y Carga
1. Abrir `src/EvaluadorWiFi.ino`.
2. Hacer clic en **Subir** (ícono de flecha).
3. Si el monitor muestra `Connecting...___...`, mantener presionado el botón **BOOT/IO0** en el ESP32 hasta que inicie la escritura.

---

## Opción B: Instalación mediante PlatformIO (VS Code)

1. Instalar la extensión **PlatformIO IDE** en Visual Studio Code.
2. Abrir la carpeta `Evaluador-WiFi-ESP32` en VS Code.
3. PlatformIO descargará automáticamente las dependencias especificadas en `platformio.ini`.
4. Conectar el ESP32 por USB.
5. Ejecutar la tarea de subida mediante la barra inferior o la terminal:
   ```bash
   pio run --target upload
   ```
