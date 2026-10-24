/*
 * FABRIK v3: la versión final.
 * Implementación J1.
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
 * @param T El objetivo en el plano, medido desde el hombro (cm).
 * @param cadena La CadenaVirtual de la muñeca y la pinza elegidas.
 * @param theta1_ini Hombro de partida (grados del modelo).
 * @param theta2_ini Codo de partida (grados del modelo).
 * @param traza Si guarda cada iteración en Ejecucion.traza, que después se libera con free.
 * @return La Ejecucion.
 */
Ejecucion iterar(Vector2D T, CadenaVirtual cadena, double theta1_ini, double theta2_ini, bool traza);

/**
 * Pasos 0 a 3 y control final con la muñeca y la pinza dadas, probando los planos en orden.
 * @param objetivo El punto (x, y, z) en cm.
 * @param theta3 Ángulo de la muñeca (grados del modelo).
 * @param theta4 Ángulo de la pinza (grados del modelo).
 * @param theta1_ini Hombro de partida (grados del modelo).
 * @param theta2_ini Codo de partida (grados del modelo).
 * @return La Solucion.
 */
Solucion resolver(const Vector3D objetivo, double theta3, double theta4, double theta1_ini, double theta2_ini);

#ifdef __cplusplus
}
#endif

#endif // FABRIK_H
