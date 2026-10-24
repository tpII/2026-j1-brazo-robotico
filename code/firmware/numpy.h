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

// Declaraciones de funciones que simulan NumPy
double np_cos(double angulo_rad);
double np_sin(double angulo_rad);

/**
 * Simula np.eye(size) o np.identity(size).
 * Inicializa la matriz M como la matriz identidad de tamaño 'size'.
 */
void np_eye(Matrix4x4 M, int size);
void np_identity(Matrix4x4 T, int size);

/**
 * Simula np.append(coordenada_local, 1).
 * Crea un vector homogéneo 4D a partir de un vector 3D y un valor escalar (1).
 */
void np_append(Vector4D p_local_homogeneo, const Vector3D coordenada_local, double value);

/**
 * Simula la multiplicación matricial vector @ matriz.
 * Multiplica matriz_transformada (4x4) por p_local_homogeneo (4x1).
 */
void np_matmul(Vector4D p_global_homogeneo, const Matrix4x4 matriz_transformada, const Vector4D p_local_homogeneo);

void np_matmul_4x4_4x4(Matrix4x4 result, const Matrix4x4 A, const Matrix4x4 B);

#ifdef __cplusplus
}
#endif

#endif // NUMPY_H