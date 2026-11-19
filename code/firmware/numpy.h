/*
 * Los tipos de vectores y matrices y las funciones de NumPy que usa la cinemática.
 *
 * Archivo de 2025 (grupo J5, code/C/main/numpy.h). En la simulación se usa NumPy.
 */
#ifndef NUMPY_H
#define NUMPY_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// Definiciones de tipos (basadas en los arrays que simulan NumPy)
typedef double Matrix4x4[4][4];
typedef double Vector3D[3];
typedef double Vector4D[4];

/**
 * El coseno, como np.cos.
 *
 * Args:
 *     angulo_rad: ángulo en radianes.
 *
 * Returns:
 *     El coseno del ángulo.
 */
double np_cos(double angulo_rad);

/**
 * El seno, como np.sin.
 *
 * Args:
 *     angulo_rad: ángulo en radianes.
 *
 * Returns:
 *     El seno del ángulo.
 */
double np_sin(double angulo_rad);

/**
 * La matriz identidad, como np.eye(size).
 *
 * Args:
 *     M: donde se escribe la matriz.
 *     size: cuántas filas y columnas se llenan; 4 para una Matrix4x4.
 */
void np_eye(Matrix4x4 M, int size);

/**
 * La matriz identidad, como np.identity(size); es lo mismo que np_eye.
 *
 * Args:
 *     T: donde se escribe la matriz.
 *     size: cuántas filas y columnas se llenan; 4 para una Matrix4x4.
 */
void np_identity(Matrix4x4 T, int size);

/**
 * Un punto 3D con una cuarta coordenada, como np.append(coordenada_local, value).
 *
 * Args:
 *     p_local_homogeneo: donde se escribe el vector (x, y, z, value).
 *     coordenada_local: el punto (x, y, z).
 *     value: la cuarta coordenada; 1 para un punto en coordenadas homogéneas.
 */
void np_append(Vector4D p_local_homogeneo, const Vector3D coordenada_local, double value);

/**
 * Una matriz 4x4 por un vector de cuatro coordenadas, como matriz_transformada @ p_local_homogeneo.
 *
 * Args:
 *     p_global_homogeneo: donde se escribe el resultado.
 *     matriz_transformada: la matriz 4x4.
 *     p_local_homogeneo: el vector de cuatro coordenadas.
 */
void np_matmul(Vector4D p_global_homogeneo, const Matrix4x4 matriz_transformada, const Vector4D p_local_homogeneo);

/**
 * El producto de dos matrices 4x4, como A @ B.
 *
 * Args:
 *     result: donde se escribe A @ B.
 *     A: la matriz de la izquierda.
 *     B: la matriz de la derecha.
 */
void np_matmul_4x4_4x4(Matrix4x4 result, const Matrix4x4 A, const Matrix4x4 B);

#ifdef __cplusplus
}
#endif

#endif // NUMPY_H
