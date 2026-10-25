/*
 * Pulsos, grados del servo y grados del modelo, con las tablas de firmware.ino.
 * Implementación J1.
 */
#ifndef SERVOS_H
#define SERVOS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Grados del servo i para un pulso, como getAngleForServo de firmware.ino.
 * Interpola en la tabla de calibración del servo y recorta en los extremos de la tabla.
 * @param i Índice del servo (0 a 4).
 * @param us Ancho de pulso en us.
 * @return Los grados del servo, de 0 a 180.
 */
double pulso_a_servo(int i, int us);

/**
 * Pulso que da s grados en el servo i.
 * Usa la misma tabla que pulso_a_servo, al revés.
 * @param i Índice del servo (0 a 4).
 * @param s Grados del servo; se recorta al rango de la tabla.
 * @return El pulso en us, sin redondear.
 */
double servo_a_pulso(int i, double s);

/**
 * Grados del modelo a partir de grados del servo: theta = signo * s + desvío.
 * @param i Índice del servo (0 a 4).
 * @param s Grados del servo.
 * @return Los grados del modelo.
 */
double servo_a_modelo(int i, double s);

/**
 * Grados del servo a partir de grados del modelo, la inversa de servo_a_modelo.
 * @param i Índice del servo (0 a 4).
 * @param theta Grados del modelo.
 * @return Los grados del servo.
 */
double modelo_a_servo(int i, double theta);

/**
 * Rango del servo i en grados del servo, con los pulsos que permite firmware.ino.
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

/**
 * La pose del home del firmware (home() de firmware.ino) en grados del modelo.
 * @param pose (Salida) Los cinco ángulos.
 */
void home_modelo(double pose[5]);

/**
 * Los cinco pulsos enteros para una pose del modelo.
 * Redondea con los medios hacia arriba y recorta a los pulsos de firmware.ino.
 * @param pose Los cinco ángulos del modelo, en grados.
 * @param pulsos (Salida) Los cinco pulsos en us.
 */
void pulsos_de(const double pose[5], int pulsos[5]);

#ifdef __cplusplus
}
#endif

#endif // SERVOS_H
