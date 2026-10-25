/*
 * Pulsos, grados del servo y grados del modelo, con las tablas de firmware.ino.
 *
 * Equivale a getAngleForServo, home() y la conversión de signos de firmware.ino.
 * Implementación J1.
 */
#include "servos.h"
#include "configuracion.h"
#include <math.h>

double pulso_a_servo(int i, int us) {
  /* Grados del servo i para un pulso, como getAngleForServo de firmware.ino. */
  const PuntoCalibracion *lut = LUT_POR_SERVO[i];
  int n = sizeof(LUT_POR_SERVO[i]) / sizeof(LUT_POR_SERVO[i][0]);
  if (us <= lut[0].us) {
    return lut[0].grados;
  }
  if (us >= lut[n - 1].us) {
    return lut[n - 1].grados;
  }
  for (int k = 0; k < n - 1; k++) {
    int x0 = lut[k].us, x1 = lut[k + 1].us;
    double y0 = lut[k].grados, y1 = lut[k + 1].grados;
    if (us < x1) {
      double t = (double)(us - x0) / (x1 - x0);
      return y0 + t * (y1 - y0);
    }
  }
  return NAN; // inalcanzable con una tabla ordenada, como en getAngleForServo
}

double servo_a_pulso(int i, double s) {
  /* Pulso que da s grados en el servo i. */
  const PuntoCalibracion *lut = LUT_POR_SERVO[i];
  int n = sizeof(LUT_POR_SERVO[i]) / sizeof(LUT_POR_SERVO[i][0]);
  double minimo = lut[0].grados, maximo = lut[0].grados;
  for (int k = 1; k < n; k++) {
    minimo = fmin(minimo, lut[k].grados);
    maximo = fmax(maximo, lut[k].grados);
  }
  s = fmin(fmax(s, minimo), maximo);
  for (int k = 0; k < n - 1; k++) {
    int x0 = lut[k].us, x1 = lut[k + 1].us;
    double y0 = lut[k].grados, y1 = lut[k + 1].grados;
    if (fmin(y0, y1) <= s && s <= fmax(y0, y1)) {
      return x0 + (s - y0) * (x1 - x0) / (y1 - y0);
    }
  }
  return NAN;
}

double servo_a_modelo(int i, double s) {
  /* Grados del modelo a partir de grados del servo: theta = signo * s + desvío.
   */
  return SIGNO_MODELO[i] * s + DESVIO_MODELO[i];
}

double modelo_a_servo(int i, double theta) {
  /* Grados del servo a partir de grados del modelo, la inversa de servo_a_modelo. */
  return (theta - DESVIO_MODELO[i]) * SIGNO_MODELO[i];
}

void rango_servo(int i, double rango[2]) {
  /* Rango del servo i en grados del servo, con los pulsos que permite firmware.ino.
   */
  double a = pulso_a_servo(i, SERVO_MIN_US[i]);
  double b = pulso_a_servo(i, SERVO_MAX_US[i]);
  rango[0] = fmin(a, b);
  rango[1] = fmax(a, b);
}

void rango_modelo(int i, double rango[2]) {
  /* Rango del servo i en grados del modelo. */
  double servo[2];
  rango_servo(i, servo);
  double a = servo_a_modelo(i, servo[0]);
  double b = servo_a_modelo(i, servo[1]);
  rango[0] = fmin(a, b);
  rango[1] = fmax(a, b);
}

void home_modelo(double pose[5]) {
  /* La pose del home del firmware (home() de firmware.ino) en grados del modelo. */
  for (int i = 0; i < 5; i++) {
    int us = SERVO_HOME_US[i];
    if (us < SERVO_MIN_US[i]) {
      us = SERVO_MIN_US[i];
    }
    if (us > SERVO_MAX_US[i]) {
      us = SERVO_MAX_US[i];
    }
    pose[i] = servo_a_modelo(i, pulso_a_servo(i, us));
  }
}

void pulsos_de(const double pose[5], int pulsos[5]) {
  /* Los cinco pulsos enteros para una pose del modelo. */
  for (int i = 0; i < 5; i++) {
    int us = (int)floor(servo_a_pulso(i, modelo_a_servo(i, pose[i])) + 0.5);
    if (us < SERVO_MIN_US[i]) {
      us = SERVO_MIN_US[i];
    }
    if (us > SERVO_MAX_US[i]) {
      us = SERVO_MAX_US[i];
    }
    pulsos[i] = us;
  }
}
