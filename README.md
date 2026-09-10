# 📡 Evaluador WiFi ESP32 - Proyecto Académico

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32-Arduino-blue)](https://www.espressif.com/)
[![Version](https://img.shields.io/badge/version-15.1-brightgreen)]()

## 📋 Descripción

**Evaluador WiFi ESP32** es una herramienta educativa desarrollada como proyecto de titulación para el análisis y evaluación de redes inalámbricas en entornos controlados. El sistema permite demostrar conceptos de ciberseguridad mediante un portal cautivo educativo y un panel de administración centralizado.

### Proyecto de Titulación
- **Autor:** Valle Chérrez Klever Iván
- **Institución:** Instituto Superior Tecnológico Universitario España (ISTE)
- **Carrera:** Sistemas de Información y Ciberseguridad
- **Tutor:** Ing. Marco Polo Rodrigo Silva Segovia
- **Año:** 2026

---

## ⚠️ Aviso Académico

> **Este proyecto es exclusivamente para fines educativos y de investigación en seguridad de la información.** Su uso está destinado a laboratorios controlados y bajo autorización expresa. El autor y la institución no asumen responsabilidad por el uso indebido fuera del ámbito académico.

---

## ✨ Características Principales

- 🌐 **Portal Cautivo Multi-Plantilla:** 5 plantillas personalizables (MikroTik, pfSense, Restaurante, Parque, Hotel).
- 🖥️ **Panel de Administración Web:** Gestión de parámetros de red, cambio de plantillas, vista de registros y descargas en tiempo real (`http://8.8.8.8:81`).
- 💾 **Almacenamiento Local SPIFFS:** Persistencia segura de credenciales e historial en memoria flash no volátil.
- 📲 **Telemetría y Notificaciones Telegram:** Envío de eventos en tiempo real a través de bot de Telegram.
- 📟 **Interfaz Visual OLED SSD1306:** Monitoreo en vivo de IP, SSID, clientes conectados, estado de red y rotación manual/automática mediante pulsador GPIO 14.
- 📡 **Escaneo y Análisis de Red:** Escáner de redes Wi-Fi perimetrales integrado.
- ⌨️ **Consola Interactiva por Monitor Serial:** Configuración y diagnóstico a 115200 baudios.

---

## 🛠️ Hardware Requerido

| Componente | Especificación / Modelo | Pines / Conexión |
|------------|-------------------------|------------------|
| Microcontrolador | ESP32-WROOM-32 (NodeMCU / DevKit v1) | Micro-USB / 3.3V - 5V |
| Pantalla | OLED SSD1306 0.96" (128x64) I2C | SDA → GPIO 21, SCL → GPIO 22 |
| Pulsador / Botón | Botón momentáneo normalmente abierto | GPIO 14 (Pull-Up interno) a GND |
| Alimentación | Cable micro-USB / Fuente 5V 1A / Powerbank | Vin / GND |

---

## 📂 Estructura del Repositorio

```text
Evaluador-WiFi-ESP32/
│
├── src/
│   └── EvaluadorWiFi.ino           # Código fuente principal de la aplicación
│
├── docs/
│   ├── manual_usuario.md           # Guía completa de uso y administración web
│   ├── guia_instalacion.md         # Paso a paso para Arduino IDE y PlatformIO
│   ├── guia_anexos.md              # Documentación y evidencias para memoria técnica
│   ├── cronograma.md               # Cronograma y fases de desarrollo del proyecto
│   └── imagenes/                   # Diagramas de arquitectura y capturas
│
├── hardware/
│   ├── esquematico.md              # Diagrama de conexiones y esquemático
│   ├── lista_componentes.md        # Lista de materiales (BOM)
│   └── conexiones_pines.md         # Tabla y mapeo detallado de pines GPIO
│
├── .gitignore                      # Configuración de exclusiones Git
├── LICENSE                         # Licencia MIT
├── README.md                       # Documentación principal
├── CHANGELOG.md                    # Historial de versiones y cambios
└── platformio.ini                  # Configuración de compilación PlatformIO
```

---

## 🚀 Instalación y Puesta en Marcha

### Opción 1: Arduino IDE (Recomendado)
1. Instalar **Arduino IDE** (v1.8.19 o v2.x).
2. Agregar el soporte para placas ESP32 mediante el Gestor de Tarjetas (`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`).
3. Instalar las librerías necesarias:
   - `ESPAsyncWebServer`
   - `AsyncTCP`
   - `ArduinoJson` (v6.x)
   - `U8g2`
4. Abrir `src/EvaluadorWiFi.ino`, seleccionar la placa **ESP32 Dev Module** y el puerto COM asignado.
5. Compilar y subir el firmware.

### Opción 2: PlatformIO
```bash
# Clonar el repositorio
git clone https://github.com/ivanvch1333/ESP32-WiFi-Security-Assessment.git
cd Evaluador-WiFi-ESP32

# Compilar y subir
pio run --target upload

# Abrir monitor serial
pio device monitor -b 115200
```

---

## 🔑 Credenciales por Defecto

- **Red WiFi AP Generada:** `WiFi-Segura` (Sin contraseña)
- **IP del Portal Cautivo:** `http://8.8.8.8` (DNS Spoofing automático)
- **Panel Administrativo:** `http://8.8.8.8:81`
- **Usuario Admin:** `admin`
- **Contraseña Admin:** `admin123`

---

## 📄 Licencia

Este proyecto está bajo la Licencia [MIT](LICENSE) - consulte el archivo para más detalles.
