/*
 * FABRIK v3: la versión final.
 *
 * En la simulación es fabrik.py. Implementación J1.
 */
#ifndef FABRIK_H
#define FABRIK_H

#include "numpy.h" // Para Vector3D
#include "plano.h" // Para Vector2D, CadenaVirtual, Ejecucion y Solucion
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Paso 2: itera en el plano desde la pose (theta1, theta2) hasta el objetivo T.
 *
 * Args:
 *     T: el objetivo en el plano, medido desde el hombro (cm).
 *     cadena: la CadenaVirtual de la muñeca y la pinza elegidas.
 *     theta1_ini: hombro de partida (grados del modelo).
 *     theta2_ini: codo de partida (grados del modelo).
 *     traza: si guarda cada iteración en Ejecucion.traza, que después se libera con free.
 *
 * Returns:
 *     La Ejecucion.
 */
Ejecucion iterar(Vector2D T, CadenaVirtual cadena, double theta1_ini, double theta2_ini, bool traza);

/**
 * Pasos 0 a 3 y control final con la muñeca y la pinza dadas, probando los planos en orden.
 *
 * Args:
 *     objetivo: el punto (x, y, z) en cm.
 *     theta3: ángulo de la muñeca (grados del modelo).
 *     theta4: ángulo de la pinza (grados del modelo).
 *     theta1_ini: hombro de partida (grados del modelo).
 *     theta2_ini: codo de partida (grados del modelo).
 *
 * Returns:
 *     La Solucion.
 */
Solucion resolver(const Vector3D objetivo, double theta3, double theta4, double theta1_ini, double theta2_ini);

#ifdef __cplusplus
}
#endif

#endif // FABRIK_H
