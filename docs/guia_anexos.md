# 📑 Guía de Anexos del Proyecto de Titulación

## Anexo 1: Arquitectura del Sistema
El sistema se compone de una arquitectura en capas que integra hardware de bajo costo con servicios de red embebidos:
1. **Capa Física:** Microcontrolador ESP32-WROOM-32 con interfaz de telemetría OLED y botón de control.
2. **Capa de Red:** Punto de acceso autónomo con servidor DNS Cautivo (Spoofing) y servidor HTTP asíncrono.
3. **Capa de Aplicación:** Motor de renderizado dinámico de plantillas HTML/CSS y API REST administrativa.
4. **Capa de Telemetría:** Pasarela hacia API Telegram para notificación remota de eventos.

Los diagramas visuales detallados se encuentran disponibles en la carpeta:
- `docs/imagenes/Arquitectura proyecto EVIL Portal.drawio.png`
- `docs/imagenes/Arquitectura proyecto EVIL Portal2.png`
- `docs/imagenes/Arquitectura proyecto EVIL Portal3.png`

---

## Anexo 2: Matriz de Pruebas y Validación
| ID Prueba | Descripción | Criterio de Éxito | Estado |
|-----------|-------------|-------------------|--------|
| PR-01 | Inicialización del AP en canal 6 | Red visible con SSID configurado | Superado |
| PR-02 | Captura y redirección DNS Cautivo | Peticiones HTTP redirigidas a 8.8.8.8 | Superado |
| PR-03 | Renderizado de las 5 plantillas | Visualización correcta en Android e iOS | Superado |
| PR-04 | Escritura en memoria SPIFFS | Persistencia de datos tras reinicio | Superado |
| PR-05 | Alertas vía Telegram API | Mensaje recibido en menos de 3 segundos | Superado |
| PR-06 | Navegación de páginas en OLED | Cambio de página instantáneo con GPIO 14 | Superado |

---

## Anexo 3: Estándares y Buenas Prácticas de Laboratorio
- Todas las pruebas deben efectuarse en un entorno radioeléctrico cerrado o caja de Faraday/laboratorio asignado.
- No utilizar credenciales reales ni afectar redes operativas de terceros sin consentimiento formal.
