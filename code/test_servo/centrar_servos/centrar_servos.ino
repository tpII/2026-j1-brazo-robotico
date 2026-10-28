#include <ESP32Servo.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ==============================================================================
// UTILIDAD DE CENTRADO Y CALIBRACIÓN ELECTRÓNICA PARA SERVOS MG90S
// Brazo Robótico 5 DOF - Taller de Proyecto II (Grupo J1)
// ==============================================================================
// Permite mover cada servo a:
// 1. Pulso de CENTRO (1500 us / 90°): con los cinco ahí se montan los horns con
//    el brazo en L (ver poseCentro).
// 2. Pulso HOME del firmware, para comprobar que el brazo llega al home.
// 3. Pequeños movimientos de verificación (Wiggle) para confirmar qué servo es.
// Los pulsos son los de configuracion.c del firmware: 1000 a 2000 us según el
// datasheet del MG90S.
// ==============================================================================

const int SERVO_COUNT = 5;
const int servoPins[SERVO_COUNT]    = {18,     19,       21,     22,       23};
const char* servoNames[SERVO_COUNT] = {"Base", "Hombro", "Codo", "Muñeca", "Pinza"};

// Pulsos de Home del firmware del brazo (SERVO_HOME_US de configuracion.c)
const int servoHomeUs[SERVO_COUNT] = {
    1000, // Servo 0: Base (0°)
    1778, // Servo 1: Hombro (140° en modelo)
    1363, // Servo 2: Codo (-114.6° en modelo)
    1525, // Servo 3: Muñeca (+4.5° en modelo)
    1000  // Servo 4: Pinza (Cerrada)
};

const int servoMinUs[SERVO_COUNT] = {1000, 1000, 1000, 1000, 1000};
const int servoMaxUs[SERVO_COUNT] = {2000, 2000, 2000, 2000, 2000};

// Pose de cada eslabón con el servo a 1500 us, para montar el horn (brazo en L),
// y hacia dónde tiene que moverse al ajustar el pulso
const char* poseCentro[SERVO_COUNT] = {
    "Brazo a 90° del frente; con '-' gira hacia el frente",
    "Brazo principal vertical; con '+' se inclina hacia atrás",
    "Antebrazo horizontal, en escuadra con el brazo; con '-' se dobla más",
    "Muñeca en línea con el antebrazo",
    "Pinza en línea con el antebrazo; con '-' se dobla hacia el home"
};

// Guía visual de la pose de home de cada eslabón
const char* poseDescripcion[SERVO_COUNT] = {
    "Base centrada apuntando hacia el frente (0°)",
    "Brazo principal inclinado hacia atrás/arriba (140°)",
    "Antebrazo doblado hacia adelante (-114.6°)",
    "Muñeca alineada/horizontal en eje con el antebrazo (+4.5°)",
    "Pinza totalmente CERRADA"
};

Servo servos[SERVO_COUNT];
int currentPulse[SERVO_COUNT];
int activeServo = 0;

void mostrarEstadoServo(int idx);

void imprimirMenu() {
  Serial.println("\n=============================================================");
  Serial.println("   HERRAMIENTA DE CENTRADO ELECTRÓNICO (SERVOS MG90S)");
  Serial.println("=============================================================");
  Serial.println("Comandos disponibles en el Monitor Serie (115200 baud):");
  Serial.println("  '0' a '4'   : Selecciona el servo (0:Base, 1:Hombro, 2:Codo, 3:Muñeca, 4:Pinza)");
  Serial.println("  'h' / 'H'   : Mueve TODOS los servos a su posición HOME del firmware");
  Serial.println("  'c' / 'C'   : Coloca el servo activo en CENTRO (1500 us / 90°), para montar el horn");
  Serial.println("  'w' / 'W'   : Movimiento suave (Wiggle) para identificar visualmente el servo");
  Serial.println("  '+' / '-'   : Ajuste fino manual del pulso (+/- 25 us)");
  Serial.println("  'm' / '?'   : Mostrar este menú");
  Serial.println("=============================================================");
  mostrarEstadoServo(activeServo);
}

void mostrarEstadoServo(int idx) {
  Serial.println("\n-------------------------------------------------------------");
  Serial.printf(">> SERVO ACTIVO: [%d] %s (Pin GPIO %d)\n", idx, servoNames[idx], servoPins[idx]);
  Serial.printf("   Pulso actual : %d us\n", currentPulse[idx]);
  Serial.printf("   Pulso Home   : %d us  [Rango seguro: %d .. %d us]\n", 
                servoHomeUs[idx], servoMinUs[idx], servoMaxUs[idx]);
  Serial.printf("   MONTAJE A 1500 us : %s\n", poseCentro[idx]);
  Serial.printf("   EN HOME           : %s\n", poseDescripcion[idx]);
  Serial.println("-------------------------------------------------------------");
}

void moverAHome(int idx) {
  currentPulse[idx] = servoHomeUs[idx];
  servos[idx].writeMicroseconds(currentPulse[idx]);
  Serial.printf("[OK] Servo %d (%s) fijado en HOME (%d us)\n", idx, servoNames[idx], currentPulse[idx]);
}

void moverTodosAHome() {
  Serial.println("\n>>> Moviendo TODOS los servos a posición HOME secuencialmente...");
  for (int i = 0; i < SERVO_COUNT; i++) {
    moverAHome(i);
    delay(350); // Pausa de 350ms para evitar picos simultáneos de corriente
  }
  Serial.println(">>> ¡Listo! Todos los servos están clavados en HOME para montar los horns.");
}

void testWiggle(int idx) {
  Serial.printf("Haciendo oscilación suave en servo %d (%s) para verificar...\n", idx, servoNames[idx]);
  int centro = currentPulse[idx];
  for (int d = 0; d <= 80; d += 5) {
    servos[idx].writeMicroseconds(centro + d);
    delay(15);
  }
  for (int d = 80; d >= -80; d -= 5) {
    servos[idx].writeMicroseconds(centro + d);
    delay(15);
  }
  for (int d = -80; d <= 0; d += 5) {
    servos[idx].writeMicroseconds(centro + d);
    delay(15);
  }
  servos[idx].writeMicroseconds(centro);
  Serial.println("Fin de oscilación.");
}

void setup() {
  // Desactivar detector de caídas de tensión (brownout) para soportar
  // los picos de arranque de los servos MG90S
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(500);

  // Asignar temporizadores PWM para el ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Inicializar y acoplar los 5 servos
  for (int i = 0; i < SERVO_COUNT; i++) {
    servos[i].setPeriodHertz(50); // 50 Hz estándar
    servos[i].attach(servoPins[i], servoMinUs[i], servoMaxUs[i]);
    currentPulse[i] = servoHomeUs[i];
  }

  // Clavar todos en Home al arrancar
  moverTodosAHome();
  imprimirMenu();
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();

    // Descartar saltos de línea
    if (c == '\r' || c == '\n') return;

    if (c >= '0' && c <= '4') {
      activeServo = c - '0';
      moverAHome(activeServo);
      mostrarEstadoServo(activeServo);
    }
    else if (c == 'h' || c == 'H') {
      moverTodosAHome();
    }
    else if (c == 'c' || c == 'C') {
      currentPulse[activeServo] = 1500;
      servos[activeServo].writeMicroseconds(1500);
      Serial.printf("[!] Servo %d (%s) colocado en CENTRO ABSOLUTO (1500 us / 90°)\n",
                    activeServo, servoNames[activeServo]);
    }
    else if (c == 'w' || c == 'W') {
      testWiggle(activeServo);
    }
    else if (c == '+') {
      currentPulse[activeServo] = min(currentPulse[activeServo] + 25, servoMaxUs[activeServo]);
      servos[activeServo].writeMicroseconds(currentPulse[activeServo]);
      Serial.printf("Pulso Servo %d: %d us\n", activeServo, currentPulse[activeServo]);
    }
    else if (c == '-') {
      currentPulse[activeServo] = max(currentPulse[activeServo] - 25, servoMinUs[activeServo]);
      servos[activeServo].writeMicroseconds(currentPulse[activeServo]);
      Serial.printf("Pulso Servo %d: %d us\n", activeServo, currentPulse[activeServo]);
    }
    else if (c == 'm' || c == 'M' || c == '?') {
      imprimirMenu();
    }
  }
}
