#include <ESP32Servo.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

Servo testServo;
const int SERVO_PIN = 18; // GPIO 18 (cable naranja/amarillo)
const int LED_PIN = 2;    // LED integrado en la placa

void setup() {
  // Desactivar temporalmente el detector de caídas de tensión (brownout)
  // por si el USB de la PC tiene micro-caídas al encender el motor
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.println("\n==============================================");
  Serial.println("--- TEST SEGURO MG90S: MOVIMIENTO ULTRA SUAVE ---");
  Serial.println("==============================================");

  // Período 50 Hz estándar
  testServo.setPeriodHertz(50);
  
  // Attach idéntico al firmware del brazo (SERVO_MIN_US y SERVO_MAX_US de configuracion.c)
  testServo.attach(SERVO_PIN, 1000, 2000);

  Serial.println("Colocando en posición central (1500 us)...");
  testServo.writeMicroseconds(1500);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  Serial.println("Iniciando barrido continuo tipo limpiaparabrisas...");
}

void loop() {
  // Barrido hacia un lado (1500 -> 1800 us) con pasos diminutos para no exigir corriente
  digitalWrite(LED_PIN, HIGH);
  for (int us = 1500; us <= 1800; us += 5) {
    testServo.writeMicroseconds(us);
    delay(20);
  }
  Serial.print("+");
  digitalWrite(LED_PIN, LOW);
  delay(400);

  // Barrido hacia el otro lado (1800 -> 1200 us)
  digitalWrite(LED_PIN, HIGH);
  for (int us = 1800; us >= 1200; us -= 5) {
    testServo.writeMicroseconds(us);
    delay(20);
  }
  Serial.print("-");
  digitalWrite(LED_PIN, LOW);
  delay(400);

  // Regreso al centro (1200 -> 1500 us)
  for (int us = 1200; us <= 1500; us += 5) {
    testServo.writeMicroseconds(us);
    delay(20);
  }
  Serial.println(" [Ciclo OK]");
  delay(500);
}
