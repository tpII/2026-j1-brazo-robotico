/*
 * Pulsos, grados del servo y grados del modelo, con las tablas de main.ino.
 * Implementación J1.
 */
#ifndef SERVOS_H
#define SERVOS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Grados del servo i para un pulso, como getAngleForServo de main.ino.
 * Interpola en la tabla de calibración del servo y recorta en los extremos de la tabla.
 * @param i Índice del servo (0 a 4).
 * @param us Ancho de pulso en us.
 * @return Los grados del servo, de 0 a 180.
 */
double pulso_a_servo(int i, int us);

/**
 * Grados del modelo a partir de grados del servo: theta = signo * s + desvío.
 * @param i Índice del servo (0 a 4).
 * @param s Grados del servo.
 * @return Los grados del modelo.
 */
double servo_a_modelo(int i, double s);

/**
 * Rango del servo i en grados del servo, con los pulsos que permite main.ino.
 * @param i Índice del servo (0 a 4).
 * @param rango (Salida) El mínimo y el máximo.
 */
void rango_servo(int i, double rango[2]);

/**
 * Rango del servo i en grados del modelo.
 * @param i Índice del servo (0 a 4).
 * @param rango (Salida) El mínimo y el máximo.
 */
void rango_modelo(int i, double rango[2]);

#ifdef __cplusplus
}
#endif

#endif // SERVOS_H
