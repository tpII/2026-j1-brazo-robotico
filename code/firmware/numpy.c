#include "numpy.h"
#include <math.h>

// Implementación de funciones trigonométricas básicas (usando math.h)
double np_cos(double angulo_rad) {
    return cos(angulo_rad);
}

double np_sin(double angulo_rad) {
    return sin(angulo_rad);
}

// Implementación de la matriz identidad
void np_eye(Matrix4x4 M, int size) {
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

// Simula np.identity (equivalente a np_eye)
void np_identity(Matrix4x4 T, int size) {
    np_eye(T, size);
}

// Simula np.append (añadir el '1' homogéneo)
void np_append(Vector4D p_local_homogeneo, const Vector3D coordenada_local, double value) {
    p_local_homogeneo[0] = coordenada_local[0];
    p_local_homogeneo[1] = coordenada_local[1];
    p_local_homogeneo[2] = coordenada_local[2];
    p_local_homogeneo[3] = value;
}

// Simula la multiplicación matricial (vector = matriz @ vector)
void np_matmul(Vector4D p_global_homogeneo, const Matrix4x4 matriz_transformada, const Vector4D p_local_homogeneo) {
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