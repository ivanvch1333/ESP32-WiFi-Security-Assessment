# 📖 Manual de Usuario - Evaluador WiFi ESP32

## 1. Introducción
El **Evaluador WiFi ESP32** es un dispositivo embebido diseñado para la evaluación y demostración de mecanismos de autenticación y portales cautivos en redes inalámbricas dentro de entornos académicos y de laboratorio.

---

## 2. Puesta en Operación
1. **Conexión eléctrica:** Conectar el ESP32 a un puerto USB o fuente de 5V DC (mínimo 1A).
2. **Arranque inicial:**
   - La pantalla OLED mostrará la pantalla de bienvenida con los datos del autor, tutor y la institución (ISTE).
   - Se iniciará el punto de acceso inalámbrico (AP) por defecto: `WiFi-Segura`.
   - La IP configurada por defecto para la puerta de enlace es `8.8.8.8`.

---

## 3. Navegación en Pantalla OLED SSD1306
El sistema cuenta con 4 vistas de información en pantalla:
- **Página 1 (General):** Título del proyecto, autor e institución.
- **Página 2 (Estado AP):** SSID actual, canal de operación, clientes conectados e IP del AP.
- **Página 3 (Telemetría & Conectividad):** Estado de conexión Wi-Fi estación (STA) y Telegram.
- **Página 4 (Métricas):** Total de eventos registrados y estado de la cola de almacenamiento.

> **Control mediante Pulsador (GPIO 14):**
> - Presionar el pulsador avanza a la siguiente página y pausa la rotación automática durante 10 segundos.
> - Tras 10 segundos sin actividad, la rotación automática se reanuda de forma transparente.

---

## 4. Panel de Administración Web
Para acceder a la consola administrativa:
1. Conectarse a la red WiFi generada por el ESP32 (`WiFi-Segura`) o a través de la IP asignada en modo estación.
2. Abrir un navegador e ingresar a: `http://8.8.8.8:81`
3. Ingresar las credenciales de administrador:
   - **Usuario:** `admin`
   - **Contraseña:** `admin123`

### Funciones del Panel Administrativo:
- **Selección de Plantilla:** Cambiar entre MikroTik, pfSense, Restaurante, Parque y Hotel.
- **Configuración de AP:** Modificar el nombre de la red (SSID), canal y ocultación.
- **Gestión de Registros:** Visualizar en tabla las credenciales/registros capturados, descargar el archivo de logs o limpiar la memoria SPIFFS.
- **Configuración de Telegram:** Actualizar el Token del bot y el Chat ID de destino.

---

## 5. Interfaz de Comandos por Consola Serial
A través del monitor serial a **115200 baudios**, están disponibles los siguientes comandos:
- `help` : Muestra el menú de ayuda y comandos disponibles.
- `status` : Muestra el estado del sistema, clientes y memoria libre.
- `scan` : Realiza un escaneo de redes Wi-Fi perimetrales.
- `clear` : Limpia los registros guardados en SPIFFS.
- `restart` : Reinicia el microcontrolador ESP32.
