#include <ESP32Servo.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ==============================================================================
// HERRAMIENTA DE MEDICIÓN DE RANGO (WIDE RANGE) Y CALIBRACIÓN ANGULAR DE SERVOS
// Brazo Robótico 5 DOF - Taller de Proyecto II (Grupo J1)
// ==============================================================================
// Propósito:
// 1. Probar servomotores individualmente para hallar su rango útil ("wide range")
//    físico real (típicamente 500 us a 2500 us en micro-servos como MG90S/SG90).
// 2. Registrar pares de calibración (ancho de pulso en us vs ángulo medido en grados).
// 3. Generar e imprimir automáticamente la tabla Look-Up Table (LUT) en código C
//    lista para copiar y pegar en `configuracion.c` (LUT_POR_SERVO) o `firmware.ino`.
// ==============================================================================

const int SERVO_COUNT = 5;
const int servoPins[SERVO_COUNT]    = {18,     19,       21,     22,       23};
const char* servoNames[SERVO_COUNT] = {"Base", "Hombro", "Codo", "Muñeca", "Pinza"};

// Rango físico extendido admisible (MG90S soportan normalmente ~500 a ~2500 us)
const int ABS_MIN_US = 450;
const int ABS_MAX_US = 2550;

// Estado de servos
Servo servos[SERVO_COUNT];
int currentPulse[SERVO_COUNT];
bool servoAttached[SERVO_COUNT];
int activeServo = 0;

// Modo de operación:
// true  = Solo el servo activo está energizado (los demás en detach para evitar zumbidos y consumo)
// false = Todos los servos energizados manteniendo su posición actual
bool modoSoloActivo = true;

// Estructura para registrar puntos de calibración de cada servo
struct PuntoCalib {
  int us;
  float deg;
};

const int MAX_PUNTOS = 15;
PuntoCalib puntosCalib[SERVO_COUNT][MAX_PUNTOS];
int cantidadPuntos[SERVO_COUNT] = {0, 0, 0, 0, 0};

// Declaraciones de funciones
void imprimirMenu();
void mostrarEstado();
void fijarPulso(int idx, int targetUs, bool suave = false);
void setModoActivo(int idx);
void registrarPunto(int idx, int us, float deg);
void imprimirTablaLUT(int idx);
void imprimirTodasLasTablas();
void barridoPrueba(int idx);
void testWiggle(int idx);
void procesarComandoLinea(String linea);

void setup() {
  // Desactivar detector de caídas de tensión (brownout) para soportar
  // los picos de arranque de los servos
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(500);

  // Asignar temporizadores PWM para el ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (int i = 0; i < SERVO_COUNT; i++) {
    servos[i].setPeriodHertz(50);
    currentPulse[i] = 1500; // Centro por defecto
    servoAttached[i] = false;
  }

  // Inicializar en servo 0 (Base)
  setModoActivo(0);

  Serial.println("\n\n=============================================================");
  Serial.println("  CALIBRADOR DE RANGO Y ÁNGULOS DE SERVOS (ESP32 - MG90S)");
  Serial.println("=============================================================");
  imprimirMenu();
  mostrarEstado();
}

void setModoActivo(int idx) {
  activeServo = idx;
  if (modoSoloActivo) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      if (i == idx) {
        if (!servoAttached[i]) {
          servos[i].attach(servoPins[i], ABS_MIN_US, ABS_MAX_US);
          servos[i].writeMicroseconds(currentPulse[i]);
          servoAttached[i] = true;
        }
      } else {
        if (servoAttached[i]) {
          servos[i].detach();
          servoAttached[i] = false;
        }
      }
    }
  } else {
    for (int i = 0; i < SERVO_COUNT; i++) {
      if (!servoAttached[i]) {
        servos[i].attach(servoPins[i], ABS_MIN_US, ABS_MAX_US);
        servos[i].writeMicroseconds(currentPulse[i]);
        servoAttached[i] = true;
      }
    }
  }
}

void fijarPulso(int idx, int targetUs, bool suave) {
  targetUs = constrain(targetUs, ABS_MIN_US, ABS_MAX_US);

  if (!servoAttached[idx]) {
    servos[idx].attach(servoPins[idx], ABS_MIN_US, ABS_MAX_US);
    servoAttached[idx] = true;
  }

  if (suave && abs(targetUs - currentPulse[idx]) > 30) {
    int paso = (targetUs > currentPulse[idx]) ? 5 : -5;
    while (abs(targetUs - currentPulse[idx]) > 5) {
      currentPulse[idx] += paso;
      servos[idx].writeMicroseconds(currentPulse[idx]);
      delay(3);
    }
  }

  currentPulse[idx] = targetUs;
  servos[idx].writeMicroseconds(currentPulse[idx]);

  // Alerta si se aproxima a zonas extremas
  if (currentPulse[idx] <= 550 || currentPulse[idx] >= 2450) {
    Serial.printf(">> Servo %d (%s): %d us  [¡ATENCIÓN: Cerca de límite físico! Escucha si zumba]\n",
                  idx, servoNames[idx], currentPulse[idx]);
  } else {
    Serial.printf(">> Servo %d (%s): %d us\n", idx, servoNames[idx], currentPulse[idx]);
  }
}

void imprimirMenu() {
  Serial.println("-------------------------------------------------------------");
  Serial.println("COMANDOS DISPONIBLES (Monitor Serie - 115200 baud):");
  Serial.println("  '0' .. '4'      : Seleccionar servo (0:Base, 1:Hombro, 2:Codo, 3:Muñeca, 4:Pinza)");
  Serial.println("  '+' / '-'       : Ajuste fino (+/- 10 us)");
  Serial.println("  'u' / 'd'       : Ajuste medio (+/- 50 us)");
  Serial.println("  ']' / '['       : Ajuste rápido (+/- 100 us)");
  Serial.println("  'c'             : Ir a centro neutral (1500 us)");
  Serial.println("  '<numero>'      : Ir directamente a ese pulso en us (ej: 1250, 600, 2400)");
  Serial.println("  'a <angulo>'    : Asignar ángulo medido al pulso actual (ej: a 0, a 45, a 90)");
  Serial.println("  't'             : Imprimir tabla LUT formateada en C para este servo");
  Serial.println("  'T'             : Imprimir tablas LUT de TODOS los servos");
  Serial.println("  'l'             : Listar puntos calibrados del servo actual");
  Serial.println("  'del'           : Borrar puntos calibrados del servo actual");
  Serial.println("  'b'             : Barrido suave automático de prueba (cualquier tecla frena)");
  Serial.println("  'w'             : Wiggle suave de identificación");
  Serial.println("  'm'             : Alternar modo (Solo servo activo vs Todos conectados)");
  Serial.println("  'h' / '?'       : Mostrar este menú de ayuda");
  Serial.println("-------------------------------------------------------------");
}

void mostrarEstado() {
  Serial.println("\n=============================================================");
  Serial.printf("SERVO SELECCIONADO: [%d] %s (Pin GPIO %d)\n", 
                activeServo, servoNames[activeServo], servoPins[activeServo]);
  Serial.printf("  Pulso actual    : %d us\n", currentPulse[activeServo]);
  Serial.printf("  Modo energía    : %s\n", modoSoloActivo ? "SOLO ACTIVO (los demás libres)" : "TODOS ENERGIZADOS");
  Serial.printf("  Puntos guardados: %d\n", cantidadPuntos[activeServo]);
  if (cantidadPuntos[activeServo] > 0) {
    Serial.print("  Puntos actuales : ");
    for (int i = 0; i < cantidadPuntos[activeServo]; i++) {
      Serial.printf("[%d us -> %.1f°] ", puntosCalib[activeServo][i].us, puntosCalib[activeServo][i].deg);
    }
    Serial.println();
  }
  Serial.println("=============================================================");
}

void registrarPunto(int idx, int us, float deg) {
  // Verificar si ya existe este pulso para actualizarlo
  for (int i = 0; i < cantidadPuntos[idx]; i++) {
    if (puntosCalib[idx][i].us == us) {
      puntosCalib[idx][i].deg = deg;
      Serial.printf("[ACTUALIZADO] Servo %d (%s): %d us = %.1f°\n", idx, servoNames[idx], us, deg);
      return;
    }
  }

  if (cantidadPuntos[idx] >= MAX_PUNTOS) {
    Serial.println("[ERROR] Límite de puntos alcanzado para este servo. Usa 'del' si querés reiniciar.");
    return;
  }

  puntosCalib[idx][cantidadPuntos[idx]].us = us;
  puntosCalib[idx][cantidadPuntos[idx]].deg = deg;
  cantidadPuntos[idx]++;

  // Ordenar puntos por pulso ascendente (Bubble Sort simple)
  for (int i = 0; i < cantidadPuntos[idx] - 1; i++) {
    for (int j = 0; j < cantidadPuntos[idx] - i - 1; j++) {
      if (puntosCalib[idx][j].us > puntosCalib[idx][j + 1].us) {
        PuntoCalib temp = puntosCalib[idx][j];
        puntosCalib[idx][j] = puntosCalib[idx][j + 1];
        puntosCalib[idx][j + 1] = temp;
      }
    }
  }

  Serial.printf("[REGISTRADO] Servo %d (%s): %d us = %.1f° (Total: %d puntos)\n",
                idx, servoNames[idx], us, deg, cantidadPuntos[idx]);
}

void imprimirTablaLUT(int idx) {
  int n = cantidadPuntos[idx];
  Serial.println("\n// -------------------------------------------------------------");
  Serial.printf("// Tabla de Calibración Servo %d (%s) - %d puntos\n", idx, servoNames[idx], n);
  Serial.println("// -------------------------------------------------------------");

  if (n < 2) {
    Serial.println("// [AVISO] Se necesitan al menos 2 puntos (mínimo y máximo) para formar una LUT válida.");
    Serial.println("// Puedes registrar puntos con 'a <grados>' en diferentes pulsos.");
  }

  Serial.printf("const PuntoCalibracion LUT_%s[%d] = {\n", servoNames[idx], max(n, 1));
  for (int i = 0; i < n; i++) {
    Serial.printf("    {%d, %.1ff}%s", 
                  puntosCalib[idx][i].us, 
                  puntosCalib[idx][i].deg, 
                  (i == n - 1) ? "" : ",\n");
  }
  Serial.println("\n};\n");
}

void imprimirTodasLasTablas() {
  Serial.println("\n/****************************************************************");
  Serial.println(" * TABLAS LUT_POR_SERVO PARA configuracion.c");
  Serial.println(" ****************************************************************/");
  for (int i = 0; i < SERVO_COUNT; i++) {
    imprimirTablaLUT(i);
  }

  Serial.println("// =============================================================");
  Serial.println("// Bloque para pegar directamente en configuracion.c:");
  Serial.println("// =============================================================");
  Serial.println("const PuntoCalibracion LUT_POR_SERVO[5][7] = {");
  for (int i = 0; i < SERVO_COUNT; i++) {
    Serial.printf("    { // [%d] %s\n", i, servoNames[i]);
    if (cantidadPuntos[i] == 0) {
      Serial.println("        // (Sin calibrar)");
    } else {
      for (int p = 0; p < cantidadPuntos[i]; p++) {
        Serial.printf("        {%d, %.1ff}%s\n", 
                      puntosCalib[i][p].us, 
                      puntosCalib[i][p].deg, 
                      (p == cantidadPuntos[i] - 1) ? "" : ",");
      }
    }
    Serial.printf("    }%s\n", (i == SERVO_COUNT - 1) ? "" : ",");
  }
  Serial.println("};\n");
}

void barridoPrueba(int idx) {
  Serial.printf("\n>>> Iniciando barrido suave en servo %d (%s)...\n", idx, servoNames[idx]);
  Serial.println(">>> Envía cualquier tecla por Serial para DETENER.");

  int pulsoInicio = currentPulse[idx];
  int minB = max(pulsoInicio - 400, ABS_MIN_US);
  int maxB = min(pulsoInicio + 400, ABS_MAX_US);

  while (Serial.available() == 0) {
    for (int p = pulsoInicio; p <= maxB; p += 10) {
      if (Serial.available() > 0) break;
      fijarPulso(idx, p, false);
      delay(20);
    }
    for (int p = maxB; p >= minB; p -= 10) {
      if (Serial.available() > 0) break;
      fijarPulso(idx, p, false);
      delay(20);
    }
    for (int p = minB; p <= pulsoInicio; p += 10) {
      if (Serial.available() > 0) break;
      fijarPulso(idx, p, false);
      delay(20);
    }
    delay(500);
  }

  // Limpiar buffer serie
  while (Serial.available() > 0) Serial.read();
  fijarPulso(idx, pulsoInicio, true);
  Serial.println(">>> Barrido detenido.");
}

void testWiggle(int idx) {
  Serial.printf("Wiggle suave en servo %d (%s)...\n", idx, servoNames[idx]);
  int centro = currentPulse[idx];
  for (int d = 0; d <= 60; d += 6) {
    servos[idx].writeMicroseconds(centro + d);
    delay(15);
  }
  for (int d = 60; d >= -60; d -= 6) {
    servos[idx].writeMicroseconds(centro + d);
    delay(15);
  }
  for (int d = -60; d <= 0; d += 6) {
    servos[idx].writeMicroseconds(centro + d);
    delay(15);
  }
  servos[idx].writeMicroseconds(centro);
}

void procesarComandoLinea(String linea) {
  linea.trim();
  if (linea.length() == 0) return;

  // 1. Comando directo de ángulo: 'a <numero>', 'ang <numero>', 'deg <numero>'
  if (linea.startsWith("a ") || linea.startsWith("A ") || 
      linea.startsWith("ang ") || linea.startsWith("deg ")) {
    int espacio = linea.indexOf(' ');
    float deg = linea.substring(espacio + 1).toFloat();
    registrarPunto(activeServo, currentPulse[activeServo], deg);
    return;
  }

  // 2. Borrar puntos del servo activo
  if (linea.equalsIgnoreCase("del") || linea.equalsIgnoreCase("borrar") || linea.equalsIgnoreCase("clear")) {
    cantidadPuntos[activeServo] = 0;
    Serial.printf("[OK] Puntos de calibración borrados para servo %d (%s).\n", 
                  activeServo, servoNames[activeServo]);
    return;
  }

  // 3. Imprimir tabla
  if (linea.equals("t") || linea.equalsIgnoreCase("tabla")) {
    imprimirTablaLUT(activeServo);
    return;
  }
  if (linea.equals("T") || linea.equalsIgnoreCase("tablas") || linea.equalsIgnoreCase("all")) {
    imprimirTodasLasTablas();
    return;
  }

  // 4. Listar puntos
  if (linea.equalsIgnoreCase("l") || linea.equalsIgnoreCase("listar") || linea.equalsIgnoreCase("p")) {
    mostrarEstado();
    return;
  }

  // 5. Si el usuario ingresó un número directo (ej: 1420 o 800)
  bool esNumero = true;
  for (unsigned int i = 0; i < linea.length(); i++) {
    if (!isDigit(linea[i]) && !(i == 0 && linea[i] == '-')) {
      esNumero = false;
      break;
    }
  }

  if (esNumero) {
    int valor = linea.toInt();
    if (valor >= ABS_MIN_US && valor <= ABS_MAX_US) {
      fijarPulso(activeServo, valor, true);
    } else {
      Serial.printf("[AVISO] Pulso fuera de rango admisible (%d a %d us): %d us\n",
                    ABS_MIN_US, ABS_MAX_US, valor);
    }
    return;
  }

  // Si no coincidió con comandos de línea, procesar por caracteres
  for (unsigned int i = 0; i < linea.length(); i++) {
    char c = linea[i];
    if (c >= '0' && c <= '4') {
      setModoActivo(c - '0');
      mostrarEstado();
    }
    else if (c == '+') {
      fijarPulso(activeServo, currentPulse[activeServo] + 10);
    }
    else if (c == '-') {
      fijarPulso(activeServo, currentPulse[activeServo] - 10);
    }
    else if (c == 'u' || c == 'U') {
      fijarPulso(activeServo, currentPulse[activeServo] + 50);
    }
    else if (c == 'd' || c == 'D') {
      fijarPulso(activeServo, currentPulse[activeServo] - 50);
    }
    else if (c == ']') {
      fijarPulso(activeServo, currentPulse[activeServo] + 100);
    }
    else if (c == '[') {
      fijarPulso(activeServo, currentPulse[activeServo] - 100);
    }
    else if (c == 'c' || c == 'C') {
      fijarPulso(activeServo, 1500, true);
    }
    else if (c == 'b' || c == 'B') {
      barridoPrueba(activeServo);
    }
    else if (c == 'w' || c == 'W') {
      testWiggle(activeServo);
    }
    else if (c == 't') {
      imprimirTablaLUT(activeServo);
    }
    else if (c == 'T') {
      imprimirTodasLasTablas();
    }
    else if (c == 'm' || c == 'M') {
      modoSoloActivo = !modoSoloActivo;
      setModoActivo(activeServo);
      Serial.printf("[MODO] %s\n", modoSoloActivo ? "SOLO ACTIVO" : "TODOS ACTIVOS");
    }
    else if (c == 'h' || c == 'H' || c == '?') {
      imprimirMenu();
      mostrarEstado();
    }
  }
}

void loop() {
  if (Serial.available() > 0) {
    String entrada = Serial.readStringUntil('\n');
    procesarComandoLinea(entrada);
  }
}

