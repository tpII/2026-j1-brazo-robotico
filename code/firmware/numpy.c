/*
 * Los tipos de vectores y matrices y las funciones de NumPy que usa la cinemática.
 *
 * Archivo de 2025 (grupo J5, code/C/main/numpy.c). En la simulación se usa NumPy.
 */
#include "numpy.h"
#include <math.h>

double np_cos(double angulo_rad) {
    /* El coseno, como np.cos. */
    return cos(angulo_rad);
}

double np_sin(double angulo_rad) {
    /* El seno, como np.sin. */
    return sin(angulo_rad);
}

void np_eye(Matrix4x4 M, int size) {
    /* La matriz identidad, como np.eye(size). */
    int i, j;
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            if (i == j) {
                M[i][j] = 1.0;
            } else {
                M[i][j] = 0.0;
            }
        }
    }
}

void np_identity(Matrix4x4 T, int size) {
    /* La matriz identidad, como np.identity(size); es lo mismo que np_eye. */
    np_eye(T, size);
}

void np_append(Vector4D p_local_homogeneo, const Vector3D coordenada_local, double value) {
    /* Un punto 3D con una cuarta coordenada, como np.append(coordenada_local, value). */
    p_local_homogeneo[0] = coordenada_local[0];
    p_local_homogeneo[1] = coordenada_local[1];
    p_local_homogeneo[2] = coordenada_local[2];
    p_local_homogeneo[3] = value;
}

void np_matmul(Vector4D p_global_homogeneo, const Matrix4x4 matriz_transformada, const Vector4D p_local_homogeneo) {
    /* Una matriz 4x4 por un vector de cuatro coordenadas, como matriz_transformada @ p_local_homogeneo. */
    int i, j;
    // Multiplicación: p_global[i] = Sum(M[i][j] * p_local[j])
    for (i = 0; i < 4; i++) {
        p_global_homogeneo[i] = 0.0;
        for (j = 0; j < 4; j++) {
            p_global_homogeneo[i] += matriz_transformada[i][j] * p_local_homogeneo[j];
        }
    }
}

void np_matmul_4x4_4x4(Matrix4x4 result, const Matrix4x4 A, const Matrix4x4 B) {
    /* El producto de dos matrices 4x4, como A @ B. */
    int i, j, k;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            result[i][j] = 0.0;
            for (k = 0; k < 4; k++) {
                result[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}
