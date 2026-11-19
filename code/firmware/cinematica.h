/*
 * Cinemática directa del brazo: puntos, marcos y esferas para cinco ángulos del modelo.
 *
 * Archivo de 2025 (grupo J5, code/C/main/cinematica.h). En la simulación es cinematica.py.
 */
#ifndef CINEMATICA_H
#define CINEMATICA_H

#include "numpy.h"         // Para Matrix4x4, Vector3D, Vector4D y funciones np_*
#include "algebra_lineal.h" // Para las declaraciones de matriz_rotacion_* y aplicar_transformada
#include <stdbool.h>       // Para usar bool

#ifdef __cplusplus
extern "C" {
#endif


// Para abajo es implementado por el grupo J1.

// Posición del brazo para unos ángulos dados.
typedef struct {
    Vector3D puntos[6];      // p0 a p5
    Vector3D esferas[5][5];  // centros globales de las esferas (CANTIDAD_ESFERAS[i] en el eslabón i), si se calcularon
    double marcos[5][3][4];  // rotación y origen de cada eslabón: las tres primeras filas de M
} Geometria;

/**
 * Puntos, marcos y esferas del brazo para cinco ángulos del modelo.
 *
 * Para cada eslabón i: M = M_acum * (Rfix_i * Rz(theta_i)) y después M_acum = M * T(SALTO_i).
 *
 * Args:
 *     angulos: los cinco ángulos del modelo, en grados.
 *     con_esferas: con false no calcula las esferas, para los usos que solo necesitan los puntos
 *         (la cadena virtual y el control final de FABRIK).
 *     g: donde se escribe la Geometria de la pose.
 */
void cinematica_directa(const double angulos[5], bool con_esferas, Geometria *g);

/**
 * Distancia entre dos puntos 3D, como np.linalg.norm(a - b).
 *
 * Es de J5 (cinematica.c); se declara acá para usarla en los otros módulos.
 *
 * Args:
 *     a: el primer punto.
 *     b: el segundo punto.
 *
 * Returns:
 *     La distancia entre a y b.
 */
double np_linalg_norm(const double *a, const double *b);

#ifdef __cplusplus
}
#endif

#endif // CINEMATICA_H
