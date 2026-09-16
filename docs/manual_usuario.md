# 📖 Manual de Usuario - Evaluador WiFi ESP32

## 1. Introducción
El **Evaluador WiFi ESP32** es un dispositivo embebido diseñado para la evaluación y demostración de mecanismos de autenticación y portales cautivos en redes inalámbricas dentro de entornos académicos y de laboratorio.

---

## 2. Puesta en Operación
1. **Conexión eléctrica:** Conectar el ESP32 a un puerto USB o fuente de 5V DC (mínimo 1A).
2. **Arranque inicial:**
   - La pantalla OLED mostrará la pantalla de bienvenida (Splash) con la institución (ISTE), nombre del proyecto (`ESP32 Wifi- Security`), autor, SSID y canal.
   - Se iniciará el punto de acceso inalámbrico (AP) por defecto: `MikroTik-HotSpot`.
   - La IP configurada por defecto para la puerta de enlace es `8.8.8.8`.

---

## 3. Navegación en Pantalla OLED SSD1306 (128x64)
La pantalla OLED cuenta con **4 páginas de información** accesibles de forma automática o mediante el pulsador conectado en el pin **GPIO 14**:

| Página | Nombre | Contenido Visualizado |
|:---:|:---|:---|
| **0** | **Splash / Inicio** | Institución (`ISTE`), Título (`ESP32 Wifi- Security`), Autor (`Ivan Valle`), SSID AP y Canal. |
| **1** | **Estadísticas** | Clientes conectados, Total de capturas, Canal actual, Estado Deauth (`ON/OFF`), Conexión STA (`OK/NO`), Tamaño de cola de mensajes. |
| **2** | **Credenciales** | Historial de las **últimas 4 capturas** almacenadas localmente en la memoria SPIFFS. |
| **3** | **Sistema** | SSID del AP Falso, SSID de la Red Objetivo, Estado de Telegram (`ON/OFF`), Cola de envío, Dirección IP (`8.8.8.8`). |

> 📌 **Control mediante Pulsador (GPIO 14):**
> - Cada pulsación avanza manualmente a la siguiente página (0 → 1 → 2 → 3 → 0) y pausa la rotación automática durante 10 segundos.
> - Al capturarse una nueva credencial, la pantalla salta automáticamente para mostrar el aviso de **¡NUEVA CAPTURA!** durante 4 segundos.

---

## 4. Panel de Administración Web (`http://8.8.8.8:81`)
Para acceder a la consola administrativa de configuración en caliente:
1. Conectarse a la red WiFi generada por el ESP32 (`MikroTik-HotSpot`) o acceder mediante la IP en modo estación (STA).
2. Abrir el navegador e ingresar a la URL: `http://8.8.8.8:81`
3. Ingresar las credenciales de administrador:
   - **Usuario:** `admin`
   - **Contraseña:** `admin123`

### Funcionalidades del Panel Web:
- **Configuración del AP Falso:** Modificar el nombre de red (SSID) y contraseña (dejar vacío para red abierta).
- **Configuración de Red Objetivo:** Definir el SSID y la dirección MAC/BSSID de la red a evaluar.
- **Configuración de Telegram:** Actualizar el Bot Token, Chat ID y activar/desactivar el envío de alertas.
- **Conexión de Red STA:** Configurar la red Wi-Fi y contraseña a la que se conectará el ESP32 para acceder a Internet y enviar notificaciones.
- **Selección de Plantilla de Portal Cautivo:** Elegir entre las 5 plantillas disponibles:
  1. *MikroTik HotSpot*
  2. *pfSense Captive Portal*
  3. *Restaurante / Cafetería*
  4. *Parque / Red Pública*
  5. *Hotel Premium Access*
- **Opciones de Red y Ataque:** Ajustar el canal de radio (1 al 11) y activar/desactivar el ataque de desautenticación (Deauth).
- **Estadísticas en Tiempo Real:** Métricas de capturas totales, clientes en vivo, canal, estado de deauth, estado de conexión STA y tamaño de cola.
- **Escáner de Redes Wi-Fi:** Escaneo de redes perimetrales en tiempo real con detección de canal, RSSI y nivel de señal.
- **Acciones del Sistema:** Botón de **Guardar** configuración, **Reiniciar** el ESP32 y **Borrar** credenciales (limpieza de memoria SPIFFS).

> ⚠️ *Nota sobre visualización de credenciales:* Las credenciales capturadas no se muestran en la tabla web por seguridad; su recepción y consulta se realiza en tiempo real a través del **Bot de Telegram**, la **Pantalla OLED (Pág. 2)** y la **Consola Serial**.

---

## 5. Interfaz de Comandos por Monitor Serial (115200 baudios)
El microcontrolador provee una consola de control interactiva mediante el puerto serie:

| Comando | Descripción |
|---|---|
| `help` | Muestra el listado de comandos disponibles. |
| `stats` | Muestra las estadísticas completas del sistema (Clientes, Capturas, Canal, Deauth, STA, Cola). |
| `list` | Lista todas las credenciales capturadas almacenadas en la memoria SPIFFS (`creds.txt`). |
| `scan` | Ejecuta un escaneo de redes Wi-Fi en el área y muestra SSID, BSSID y Canal. |
| `channel` | Muestra el canal de radiofrecuencia actual. |
| `deauth on` | Activa el ataque de desautenticación (Deauth). |
| `deauth off` | Desactiva el ataque de desautenticación. |
| `clear` | Borra el archivo de credenciales de la memoria flash SPIFFS y reinicia contadores. |
| `showconfig` | Muestra la configuración actual de SSID, Canal, Plantilla de Portal y Deauth. |
| `oled 0-3` | Cambia de forma manual la página activa en la pantalla OLED (ej. `oled 1`). |
