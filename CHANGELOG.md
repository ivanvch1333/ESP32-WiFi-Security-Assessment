# Registro de Cambios (CHANGELOG)

Todos los cambios notables en este proyecto serán documentados en este archivo.

## [15.1] - 2026-09-10
### Añadido
- Interfaz web con persistencia de mensajes vía `sessionStorage`.
- Sistema multi-plantilla para portales cautivos: MikroTik, pfSense, Restaurante, Parque y Hotel.
- Interfaz gráfica en pantalla OLED SSD1306 con 4 vistas rotativas (Información general, telemetría, estado de red y estadísticas).
- Pulsador en GPIO 14 para alternar páginas en la pantalla OLED de forma manual y pausa de rotación automática.
- Canal de notificación y alertas por Telegram mediante API Bot.
- Sistema de archivos SPIFFS para registro persistente de accesos y credenciales.
- Panel de configuración administrativo en el puerto TCP 81 (`http://8.8.8.8:81`).

### Optimizado
- Gestión de estabilidad eliminando watchdog timers bloqueantes.
- Buffer y colas seguras para el envío de paquetes de red y telemetría.
- DNS Server cautivo con redirección automática para todos los dominios (`* -> 8.8.8.8`).

## [10.0] - 2026-07-15
### Añadido
- Servidor web asíncrono con `ESPAsyncWebServer`.
- Integración básica de telemetría y pruebas perimetrales en laboratorio.

## [1.0] - 2026-05-30
### Añadido
- Estructura inicial del proyecto de titulación en el ISTE.
- Prototipo base de punto de acceso en ESP32 con captura de solicitudes HTTP.
