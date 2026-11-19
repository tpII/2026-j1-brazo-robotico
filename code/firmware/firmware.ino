// Firmware del brazo: el servidor HTTP, los servos y el loop() que da un paso del movimiento cada DT.
// Archivo de 2025 (grupo J5, code/C/main/main.ino). En la simulación es servidor.py.
#include <WebServer.h>
#include <WiFi.h>

// --- CONFIGURACIÓN DE RED ---
const char *ssid = "Guest@Home";
const char *password = "anatipica";

// Objeto del Servidor en el puerto 80
WebServer server(80);

#include <ESP32Servo.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <stdlib.h> // Para strtod

// --- CONFIGURACIÓN DEL BRAZO ---
#include "configuracion.h"

// ================== PINES Y CONFIGURACIÓN HARDWARE ==================

// ----------- Configuración de Servos (5 Eslabones) -----------
const int SERVO_COUNT = 5;
const int servoPins[SERVO_COUNT] = {18, 19, 21, 22, 23};
// Los pulsos de los MG90S (mínimo, máximo y home) y sus tablas de calibración están en
// configuracion.c.

// ================== ESTADO GLOBAL DEL ROBOT ==================
Servo servos[SERVO_COUNT];
int currentPulse[SERVO_COUNT];

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
  /* Manda un JSON con Access-Control-Allow-Origin, como hacía J5 en /data: la
     página se abre desde la compu, que es otra dirección, y sin ese permiso el
     navegador no la deja leer la respuesta. */
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
  if (telemetria(&centralizador, telemetria_json, sizeof(telemetria_json)) >=
      sizeof(telemetria_json)) {
    responder_json(
        500, "{\"error\": \"La telemetría no entra en telemetria_json.\"}");
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
  const char *error =
      pedir_objetivo(&centralizador, x, y, z, respuesta, sizeof(respuesta));
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
  snprintf(respuesta, sizeof(respuesta), "{\"estado\": \"%s\"}",
           centralizador.estado);
  responder_json(200, respuesta);
}

void manejar_home() {
  /* POST /home: vuelve al home, si el camino no choca. */
  char respuesta[128];
  const char *error = pedir_home(&centralizador, respuesta, sizeof(respuesta));
  if (error != NULL) {
    responder_error(error);
    return;
  }
  Serial.printf("Home: %s\n", respuesta);
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

void ir_a_home() {
  /* Lleva los servos a los pulsos del home de configuracion.c, de a uno. */
  for (int i = 0; i < SERVO_COUNT; i++) {
    servos[i].writeMicroseconds(SERVO_HOME_US[i]);
    currentPulse[i] = SERVO_HOME_US[i];
    Serial.printf("Servo %d -> %d us\n", i, SERVO_HOME_US[i]);
    delay(300); // deja que ese servo llegue antes de pasar al siguiente (de J5)
  }
}

// ================== SETUP ==================

void setup() {
  // Desactivar detector de caídas de tensión (brownout) para soportar
  // los picos de corriente de los 5 servos metálicos MG90S
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(400);

  // Habilitar todos los temporizadores PWM para los servos MG90S
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // --- Conexión Wi-Fi (de J5) ---
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado.");
  Serial.print("IP del servidor: ");
  Serial.println(WiFi.localIP());

  // --- Servidor web (/ como en J5) ---
  // La página es code/index.html: como en 2025, se abre con doble clic y le
  // pide los datos a esta placa, a la IP que tiene escrita en
  // code/web/conexion.js.
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain",
                "ESP32 Robot Arm Server. Endpoint /data para JSON.");
  });
  server.on("/data", HTTP_GET, manejar_data);
  server.on("/set", HTTP_POST, manejar_set);
  server.on("/detener", HTTP_POST, manejar_detener);
  server.on("/home", HTTP_POST, manejar_home);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");

  // --- Servos y home ---
  // Con los pulsos de los MG90S de configuracion.c.
  for (int i = 0; i < SERVO_COUNT; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], SERVO_MIN_US[i], SERVO_MAX_US[i]);
  }
  ir_a_home();
  iniciar(&centralizador);
  ultimo_paso = millis();

  Serial.print("Listo. En code/web/conexion.js, ESP32 = http://");
  Serial.println(WiFi.localIP());
}

// ================== LOOP ==================

void loop() {
  // Atiende un pedido de la web, si hay uno.
  server.handleClient();

  // Un paso del movimiento cada DT. Si se atrasa no recupera el tiempo: entre
  // dos pasos siempre hay por lo menos DT, así el brazo nunca va más rápido que
  // lo planificado.
  unsigned long ahora = millis();
  if (ahora - ultimo_paso >= DT * 1000) {
    avanzar(&centralizador);
    mover_servos();
    ultimo_paso = ahora;
  }
}
