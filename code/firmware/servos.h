/*
 * Pulsos, grados del servo y grados del modelo, con las tablas de configuracion.c.
 *
 * En la simulación es servos.py. Implementación J1.
 */
#ifndef SERVOS_H
#define SERVOS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Grados del servo i para un pulso.
 *
 * Interpola en la tabla de calibración del servo y recorta en los extremos de la tabla.
 *
 * Args:
 *     i: índice del servo (0 a 4).
 *     us: ancho de pulso en us.
 *
 * Returns:
 *     Los grados del servo, de 0 a 180.
 */
double pulso_a_servo(int i, int us);

/**
 * Pulso que da s grados en el servo i.
 *
 * Usa la misma tabla que pulso_a_servo, al revés.
 *
 * Args:
 *     i: índice del servo (0 a 4).
 *     s: grados del servo; se recorta al rango de la tabla.
 *
 * Returns:
 *     El pulso en us, sin redondear.
 */
double servo_a_pulso(int i, double s);

/**
 * Grados del modelo a partir de grados del servo: theta = signo * s + desvío.
 *
 * Args:
 *     i: índice del servo (0 a 4).
 *     s: grados del servo.
 *
 * Returns:
 *     Los grados del modelo.
 */
double servo_a_modelo(int i, double s);

/**
 * Grados del servo a partir de grados del modelo, la inversa de servo_a_modelo.
 *
 * Args:
 *     i: índice del servo (0 a 4).
 *     theta: grados del modelo.
 *
 * Returns:
 *     Los grados del servo.
 */
double modelo_a_servo(int i, double theta);

/**
 * Rango del servo i en grados del servo, con los pulsos que permite configuracion.c.
 *
 * Args:
 *     i: índice del servo (0 a 4).
 *     rango: donde se escriben el mínimo y el máximo.
 */
void rango_servo(int i, double rango[2]);

/**
 * Rango del servo i en grados del modelo.
 *
 * Args:
 *     i: índice del servo (0 a 4).
 *     rango: donde se escriben el mínimo y el máximo.
 */
void rango_modelo(int i, double rango[2]);

/**
 * La pose del home del firmware (SERVO_HOME_US) en grados del modelo.
 *
 * Args:
 *     pose: donde se escriben los cinco ángulos.
 */
void home_modelo(double pose[5]);

/**
 * Los cinco pulsos enteros para una pose del modelo.
 *
 * Redondea con los medios hacia arriba y recorta a los pulsos de configuracion.c.
 *
 * Args:
 *     pose: los cinco ángulos del modelo, en grados.
 *     pulsos: donde se escriben los cinco pulsos en us.
 */
void pulsos_de(const double pose[5], int pulsos[5]);

#ifdef __cplusplus
}
#endif

#endif // SERVOS_H
