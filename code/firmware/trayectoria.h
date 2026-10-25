/*
 * Movimiento seguro de una pose a otra.
 * Implementación J1.
 */
#ifndef TRAYECTORIA_H
#define TRAYECTORIA_H

#include "numpy.h" // Para Vector3D

#ifdef __cplusplus
extern "C" {
#endif

// Una pose: los cinco ángulos del modelo, en grados.
typedef double Pose[5];

// Un movimiento planificado.
typedef struct {
    Pose *pasos;      // una pose cada DT, siendo el último paso la pose final; se libera con free
    int n_pasos;      // cuántos pasos hay
    double duracion;  // duración del movimiento en segundos
} Trayectoria;

/**
 * Los puntos del brazo cuya velocidad se limita: p1 a p5 y los 9 centros de las esferas.
 * @param pose Los cinco ángulos del modelo, en grados.
 * @param puntos (Salida) Los 14 puntos.
 */
void puntos_vigilados(const double pose[5], Vector3D puntos[14]);

/**
 * Pasos de 20 ms de la pose a a la pose b, dentro de los límites de velocidad y con rampas.
 * @param a La pose de partida (cinco ángulos, en grados).
 * @param b La pose de llegada.
 * @return La Trayectoria. Si a y b son la misma pose, tiene un solo paso y dura 0 seg. Si no hay
 *         memoria para los pasos, no tiene ninguno.
 */
Trayectoria planificar(const double a[5], const double b[5]);

/**
 * El primer paso que choca.
 * @param pasos Las poses del camino.
 * @param n_pasos Cuántas son.
 * @return El índice del primer paso que choca, o -1 si el camino está libre.
 */
int primer_choque(Pose *pasos, int n_pasos);

#ifdef __cplusplus
}
#endif

#endif // TRAYECTORIA_H
