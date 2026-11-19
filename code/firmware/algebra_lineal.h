/*
 * Rotaciones y traslaciones 4x4 en coordenadas homogéneas.
 *
 * Archivo de 2025 (grupo J5, code/C/main/algebra_lineal.h). En la simulación es algebra_lineal.py.
 */
#ifndef ALGEBRA_LINEAL_H
#define ALGEBRA_LINEAL_H

#include "numpy.h" // Incluye tipos como Matrix4x4, Vector3D, Vector4D y placeholders de NumPy

#ifdef __cplusplus
extern "C" {
#endif

// --- Definición de macro math.radians ---
#define math_radians(grados) ((grados) * (M_PI / 180.0))

// --- Funciones de Álgebra Lineal traducidas ---

/**
 * Rotación 4x4 alrededor del eje X.
 *
 * Args:
 *     R: donde se escribe la matriz homogénea 4x4 de la rotación.
 *     angulo_grados: ángulo en grados, positivo según la regla de la mano derecha.
 */
void matriz_rotacion_x(Matrix4x4 R, double angulo_grados);

/**
 * Rotación 4x4 alrededor del eje Y.
 *
 * Args:
 *     R: donde se escribe la matriz homogénea 4x4 de la rotación.
 *     angulo_grados: ángulo en grados, positivo según la regla de la mano derecha.
 */
void matriz_rotacion_y(Matrix4x4 R, double angulo_grados);

/**
 * Rotación 4x4 alrededor del eje Z.
 *
 * Args:
 *     R: donde se escribe la matriz homogénea 4x4 de la rotación.
 *     angulo_grados: ángulo en grados, positivo según la regla de la mano derecha.
 */
void matriz_rotacion_z(Matrix4x4 R, double angulo_grados);

/**
 * Genera la matriz de traslación 4x4.
 *
 * Args:
 *     T: donde se escribe la matriz homogénea 4x4 que le suma (tx, ty, tz) a un punto.
 *     tx: traslación en x (cm).
 *     ty: traslación en y (cm).
 *     tz: traslación en z (cm).
 */
void matriz_traslacion(Matrix4x4 T, double tx, double ty, double tz);

/**
 * Aplica la matriz de transformación a una coordenada 3D local.
 *
 * Args:
 *     resultado: donde se escribe el punto (x, y, z) en el marco global.
 *     matriz_transformada: matriz homogénea 4x4 que lleva del marco local al global.
 *     coordenada_local: el punto (x, y, z) en el marco local.
 */
void aplicar_transformada(Vector3D resultado, const Matrix4x4 matriz_transformada, const Vector3D coordenada_local);

#ifdef __cplusplus
}
#endif

#endif // ALGEBRA_LINEAL_H
