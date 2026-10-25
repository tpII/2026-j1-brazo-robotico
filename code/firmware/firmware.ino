// --- NUEVAS INCLUSIONES PARA WEB SERVER Y JSON ---
#include <ArduinoJson.h> // ¡Asegúrate de instalar esta librería!
#include <WebServer.h>
#include <WiFi.h>

// --- CONFIGURACIÓN DE RED ---
const char *ssid = "Personal-58C-2.4GHz";
const char *password = "9114DFC58C";
const IPAddress localIP(192, 168, 1, 184); // IP Fija sugerida para el ESP32
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);

// Objeto del Servidor en el puerto 80
WebServer server(80);

#include <ESP32Servo.h>
#include <cmath>
#include <stdlib.h> // Necesario para 'free' en el setup
#include <string.h>

// --- INCLUSIÓN DE MÓDULOS DE CINEMÁTICA ---
#include "algebra_lineal.h"
#include "cinematica.h"
#include "configuracion.h"
#include "numpy.h"

// ================== PINES Y CONFIGURACIÓN HARDWARE ==================

// ----------- Pines de Control -----------
const int BTN_NEXT = 13;
const int BTN_PREV = 14;
const int POT_PIN = 34;

// ----------- Configuración de Servos (5 Eslabones) -----------
const int SERVO_COUNT = 5;
const int servoPins[SERVO_COUNT] = {18, 19, 21, 22, 23};

// CON ESTO SE PUEDE CONTROLAR LOS VALORES FISICOS
const int servoMinUs[SERVO_COUNT] = {400, 400, 400, 400, 400};
const int servoMaxUs[SERVO_COUNT] = {2600, 1800, 2600, 2000, 2600};
// --- PULSOS DE HOME PARA CADA SERVO (FÁCILMENTE CONFIGURABLES) ---
int servoHomePulse[SERVO_COUNT] = {
    400,  // Servo 0
    1400, // Servo 1
    1200, // Servo 2
    960,  // Servo 3
    400   // Servo 4
};

// ================== CONSTANTES DE CONTROL ==================
const float ALPHA = 0.25f; // suavizado exponencial ADC
const int DEADZONE_US = 150;
const int SYNC_TOLERANCE_US = 200;
const int DEBOUNCE_MS = 50;
const int STEP_SIZE_US = 20;

// ---- Tabla de calibración (us -> grados) ----
struct CalibPoint {
  int us;
  float deg;
};

CalibPoint LUT_GENERICO[] = {
    {400, 0.0}, // Inicio
    {767, 30.0},   {1134, 60.0},
    {1501, 90.0}, // Punto central (similar al 1500 original)
    {1868, 120.0}, {2235, 150.0},
    {2600, 180.0} // Final
};

const int LUT_SIZE = sizeof(LUT_GENERICO) / sizeof(LUT_GENERICO[0]);

CalibPoint LUT1[] = {
    {400, 0.0}, // Inicio
    {800, 30.0},   {1000, 60.0},
    {1100, 90.0}, // Punto central (similar al 1500 original)
    {1200, 120.0}, {1500, 150.0},
    {1800, 180.0} // Final
};

const int LUT_SIZE1 = sizeof(LUT1) / sizeof(LUT1[0]);

CalibPoint LUT2[] = {
    {400, 180.0}, // Inicio
    {767, 150.0}, {1134, 120.0},
    {1501, 90.0}, // Punto central (similar al 1500 original)
    {1868, 60.0}, {2235, 30.0},
    {2600, 0.0} // Final
};

const int LUT_SIZE2 = sizeof(LUT2) / sizeof(LUT2[0]);

CalibPoint LUT3[] = {
    {400, 0.0}, // Inicio
    {500, 30.0},   {600, 60.0},
    {900, 90.0}, // Punto central: (400 + 2100) / 2 = 1250 us
    {1300, 120.0}, {1700, 150.0},
    {2000, 180.0} // Final ajustado a 2100 us
};

const int LUT_SIZE3 = sizeof(LUT3) / sizeof(LUT3[0]);

// ================== ESTADO GLOBAL DEL ROBOT ==================
Servo servos[SERVO_COUNT];
float filteredAdc = 0.0f;
int currentPulse[SERVO_COUNT];
double angulos_grados[SERVO_COUNT] = {0.0};

// Índice del servo actualmente controlado por el potenciómetro
int activeServo = 0;

// Variables de estado para el Debounce
int lastRawNext = HIGH;
int lastStableNext = HIGH;
unsigned long lastDebounceNext = 0;
int lastRawPrev = HIGH;
int lastStablePrev = HIGH;
unsigned long lastDebouncePrev = 0;

// ================== UTILITARIOS ==================

float getAngleForServo(int servo_idx, int us) {

  // 1. Declaración de Punteros (Deben estar fuera del if/else para ser
  // accesibles)
  const CalibPoint *local_lut;
  int local_lut_size;

  // 2. Selección de la LUT (Corregida la lógica)
  if (servo_idx == 2) {
    // Servo 2 usa LUT2 (Invertida)
    local_lut = LUT2;
    local_lut_size = LUT_SIZE2;

  } else if (servo_idx == 1) {
    // Servo 1 usa LUT1 (Calibración específica)
    local_lut = LUT1;
    local_lut_size = LUT_SIZE1;

  } else if (servo_idx == 3) {
    // Servo 1 usa LUT1 (Calibración específica)
    local_lut = LUT3;
    local_lut_size = LUT_SIZE3;

  } else {
    // Todos los demás (0, 3, 4) usan la LUT original
    local_lut = LUT_GENERICO;
    local_lut_size = LUT_SIZE;
  }

  // --- Lógica de Interpolación (Unificada) ---

  // 3. Chequeo de límites (Extrapolación)
  if (us <= local_lut[0].us) {
    return local_lut[0].deg;
  }
  if (us >= local_lut[local_lut_size - 1].us) {
    return local_lut[local_lut_size - 1].deg;
  }

  // 4. Búsqueda por interpolación lineal
  for (int i = 0; i < local_lut_size - 1; i++) {
    // La condición de parada puede ser: si el pulso 'us' es menor
    // que el siguiente punto (x1), entonces está en este segmento.
    if (us < local_lut[i + 1].us) {
      int x0 = local_lut[i].us;
      int x1 = local_lut[i + 1].us;
      float y0 = local_lut[i].deg;
      float y1 = local_lut[i + 1].deg;

      // Cálculo de interpolación lineal: y = y0 + t * (y1 - y0)
      float t = (float)(us - x0) / (float)(x1 - x0);
      return y0 + t * (y1 - y0);
    }
  }

  return NAN; // Debería ser inalcanzable si la LUT está completa y ordenada
}

// Función de Antirrebote (Debounce)
bool fallingEdgeDebounced(int pin, int &lastRaw, int &lastStable,
                          unsigned long &tLast) {
  int raw = digitalRead(pin);
  unsigned long now = millis();

  if (raw != lastRaw) {
    lastRaw = raw;
    tLast = now;
  }
  if ((now - tLast) > DEBOUNCE_MS) {
    if (raw != lastStable) {
      lastStable = raw;
      if (lastStable == LOW)
        return true; // flanco de bajada (pulso a GND)
    }
  }
  return false;
}

// ================== FUNCIÓN DE AYUDA DE KINEMÁTICA ==================

// NOTA: Esta función DEBE liberar la memoria de los resultados devueltos
// después de usarlos. En el handler JSON, construiremos el JSON y luego
// liberaremos la memoria.
bool obtener_pose_global(EsferasGlobales esferas_resultado,
                         ColisionSet *centros_resultado) {

  // Ejecutar Sistema de Colisión con la pose global actual
  return calcular_configuracion_modular(
      angulos_grados, // Usa la pose actual validada
      esferas_local, RADIO_ESFERA, esferas_resultado, centros_resultado);
}

// ================== HANDLER WEB ==================

int get_sphere_row_count(int eslabon_idx) {
  if (eslabon_idx == 0)
    return 1;
  if (eslabon_idx == 1)
    return 2;
  if (eslabon_idx == 2)
    return 3;
  if (eslabon_idx == 3)
    return 1;
  if (eslabon_idx == 4)
    return 2;
  return 0; // Error
}

void handleDataRequest() {
  // 1. Ejecutar Cinemática para obtener la pose actual
  EsferasGlobales esferas_globales_temp;
  ColisionSet centros_colisionantes;

  // Inicializar punteros a NULL (CRÍTICO)
  centros_colisionantes.coords = NULL;
  centros_colisionantes.count = 0;
  for (int j = 0; j < SERVO_COUNT; j++)
    esferas_globales_temp[j] = NULL;

  bool colision_detectada =
      obtener_pose_global(esferas_globales_temp, &centros_colisionantes);

  // 2. Crear Documento JSON (Tamaño estimado)
  // El tamaño depende del número de esferas: 5 eslabones * (2 o 3 esferas por
  // eslabón) * 3 coordenadas
  const size_t CAPACITY =
      JSON_OBJECT_SIZE(
          4) + // 4 campos principales (angles, collision, spheres, centers)
      JSON_ARRAY_SIZE(SERVO_COUNT) +     // Array de ángulos (5 doubles)
      SERVO_COUNT * JSON_ARRAY_SIZE(5) + // 5 eslabones
      300;                               // Buffer de seguridad

  DynamicJsonDocument doc(CAPACITY);

  // --- 3. Llenar el JSON ---

  // a) Ángulos
  JsonArray anglesArray = doc.createNestedArray("angles");
  for (int i = 0; i < SERVO_COUNT; i++) {
    anglesArray.add(angulos_grados[i]);
  }

  // b) Estado de Colisión
  doc["collision_detected"] = colision_detectada;
  doc["active_servo_index"] = activeServo;

  // c) Centros Colisionantes (si los hay)
  JsonArray collidingCentersArray = doc.createNestedArray("colliding_centers");
  if (centros_colisionantes.count > 0 && centros_colisionantes.coords != NULL) {
    // Los centros colisionantes son una lista aplanada de coordenadas (x1, y1,
    // z1, x2, y2, z2...) La lista centros_colisionantes.coords contiene (x, y,
    // z) de cada centro
    for (int i = 0; i < centros_colisionantes.count; i++) {
      JsonArray center = collidingCentersArray.createNestedArray();
      center.add(centros_colisionantes.coords[i][0]);
      center.add(centros_colisionantes.coords[i][1]);
      center.add(centros_colisionantes.coords[i][2]);
    }
  }

  // d) Esferas Globales (La pose completa)
  doc["sphere_radius"] = RADIO_ESFERA;
  JsonArray spheresArray = doc.createNestedArray("spheres");

  for (int i = 0; i < SERVO_COUNT; i++) {
    JsonArray eslabon = spheresArray.createNestedArray();

    // --- Solución al Error 1: Usar helper para obtener rows_i ---
    int rows_i = get_sphere_row_count(i);

    if (esferas_globales_temp[i] != NULL) {

      // --- Solución al Error 2: Casting explícito a Vector3D* ---
      const Vector3D *esf_global_i = (const Vector3D *)esferas_globales_temp[i];

      for (size_t k = 0; k < rows_i; k++) {
        JsonArray sphereCenter = eslabon.createNestedArray();

        // Ahora la indexación es correcta: esf_global_i[k] es el Vector3D
        // (double[3])
        sphereCenter.add(esf_global_i[k][0]); // x
        sphereCenter.add(esf_global_i[k][1]); // y
        sphereCenter.add(esf_global_i[k][2]); // z
      }
    }
  }

  // 4. Serializar JSON a String
  String response;
  serializeJson(doc, response);

  // 5. Enviar Respuesta HTTP
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);

  // 6. Liberar Memoria (CRÍTICO)
  if (centros_colisionantes.coords != NULL)
    free(centros_colisionantes.coords);
  for (int j = 0; j < SERVO_COUNT; j++) {
    if (esferas_globales_temp[j] != NULL)
      free((void *)esferas_globales_temp[j]);
  }
}

// ======================================================
// =========== FUNCIÓN PARA LLEVAR A POSICIÓN HOME =======
// ======================================================
void home() {
  Serial.println("\n--- Moviendo todos los servos a POSICIÓN HOME ---");

  for (int i = 0; i < SERVO_COUNT; i++) {

    int pulse = servoHomePulse[i];

    // Asegurar que cae dentro del rango del servo
    if (pulse < servoMinUs[i])
      pulse = servoMinUs[i];
    if (pulse > servoMaxUs[i])
      pulse = servoMaxUs[i];

    // Convertir a ángulo usando tu LUT
    float ang = getAngleForServo(i, pulse);

    if (i == 3) {
      ang -= 90.0f; // Aplica offset: 0..180 -> -90..90
    }

    if (i == 0 || i == 2) {
      ang = -1 * ang; // Tu corrección especial
    }

    // Guardar en el estado global
    currentPulse[i] = pulse;
    angulos_grados[i] = ang;

    // Mover el servo
    servos[i].writeMicroseconds(pulse);
    Serial.println("--- POSICIÓN HOME PROCESOOOO ---\n");
    Serial.printf("Servo %d -> HomePulse=%dus | Ang=%.1f°\n", i, pulse, ang);
    delay(300); // deja que ese servo llegue antes de pasar al siguiente
  }

  Serial.println("--- POSICIÓN HOME COMPLETADA ---\n");
}

// Para abajo es implementado por el grupo J1.

#include "centralizador.h"

// La búsqueda necesita más pila que los 8 KB que el core le da a loop().
#ifdef SET_LOOP_TASK_STACK_SIZE
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#else
#error "Hace falta el core del ESP32 2.0.0 o más nuevo, para darle 16 KB de pila a loop()"
#endif

// El estado del brazo: la pose, el movimiento y la última búsqueda.
Centralizador centralizador;

// La telemetría de /data, que se arma en cada pedido.
char telemetria_json[16384];

// Cuándo se dio el último paso del movimiento (ms).
unsigned long ultimo_paso = 0;

void responder_json(int codigo, const char *json) {
  /* Manda un JSON con Access-Control-Allow-Origin, como /data en J5: la página se abre desde la
     compu, que es otra dirección, y sin ese permiso el navegador no la deja leer la respuesta. */
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(codigo, "application/json", json);
}

void responder_error(const char *mensaje) {
  /* Responde 400 con el motivo del rechazo. */
  char json[160];
  snprintf(json, sizeof(json), "{\"error\": \"%s\"}", mensaje);
  responder_json(400, json);
}

bool leer_numero(const String &texto, double *valor) {
  /* Lee un número de un campo del formulario; false si no es un número. */
  const char *inicio = texto.c_str();
  char *fin;
  *valor = strtod(inicio, &fin);
  while (*fin == ' ') {
    fin++;
  }
  return fin != inicio && *fin == '\0';
}

void manejar_data() {
  /* GET /data: la telemetría. */
  if (telemetria(&centralizador, telemetria_json, sizeof(telemetria_json)) >= sizeof(telemetria_json)) {
    responder_json(500, "{\"error\": \"La telemetría no entra en telemetria_json.\"}");
    return;
  }
  responder_json(200, telemetria_json);
}

void manejar_set() {
  /* POST /set con x, y y z (400 si faltan o no son válidos). */
  if (!server.hasArg("x") || !server.hasArg("y") || !server.hasArg("z")) {
    responder_error("Faltan los campos x, y o z.");
    return;
  }
  double x, y, z;
  if (!leer_numero(server.arg("x"), &x) || !leer_numero(server.arg("y"), &y) ||
      !leer_numero(server.arg("z"), &z)) {
    responder_error("Los valores de x, y y z tienen que ser números.");
    return;
  }
  char respuesta[512];
  const char *error = pedir_objetivo(&centralizador, x, y, z, respuesta, sizeof(respuesta));
  if (error != NULL) {
    responder_error(error);
    return;
  }
  Serial.printf("Objetivo (%.2f, %.2f, %.2f): %s\n", x, y, z, respuesta);
  responder_json(200, respuesta);
}

void manejar_detener() {
  /* POST /detener: frena el brazo. */
  detener(&centralizador);
  char respuesta[64];
  snprintf(respuesta, sizeof(respuesta), "{\"estado\": \"%s\"}", centralizador.estado);
  responder_json(200, respuesta);
}

void mover_servos() {
  /* Le manda a cada servo el pulso del centralizador, si cambió. */
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (centralizador.pulsos[i] != currentPulse[i]) {
      servos[i].writeMicroseconds(centralizador.pulsos[i]);
      currentPulse[i] = centralizador.pulsos[i];
    }
  }
}

// ================== SETUP ==================

void setup() {
  Serial.begin(115200);
  delay(400);

  // --- Conexión Wi-Fi (de J5) ---
  // if (!WiFi.config(localIP, gateway, subnet)) {
  //   Serial.println("Falló la configuración STA");
  // }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado.");
  Serial.print("IP del servidor: ");
  Serial.println(WiFi.localIP());

  // --- Servidor web (/ como en J5) ---
  // La página es code/index.html: como en 2025, se abre con doble clic y le pide los datos a esta
  // placa, a la IP que tiene escrita en code/web/conexion.js.
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain",
                "ESP32 Robot Arm Server. Endpoint /data para JSON.");
  });
  server.on("/data", HTTP_GET, manejar_data);
  server.on("/set", HTTP_POST, manejar_set);
  server.on("/detener", HTTP_POST, manejar_detener);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");

  // --- Servos y home (de J5) ---
  for (int i = 0; i < SERVO_COUNT; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], servoMinUs[i], servoMaxUs[i]);
  }
  home();
  iniciar(&centralizador);
  ultimo_paso = millis();

  Serial.print("Listo. En code/web/conexion.js, ESP32 = http://");
  Serial.println(WiFi.localIP());
}

// ================== LOOP ==================

void loop() {
  // Atiende un pedido de la web, si hay uno.
  server.handleClient();

  // Un paso del movimiento cada DT. Si se atrasa no recupera el tiempo: entre dos pasos siempre
  // hay por lo menos DT, así el brazo nunca va más rápido que lo planificado.
  unsigned long ahora = millis();
  if (ahora - ultimo_paso >= DT * 1000) {
    avanzar(&centralizador);
    mover_servos();
    ultimo_paso = ahora;
  }
}
