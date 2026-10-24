#include "algebra_lineal.h"
#include "numpy.h"
#include <stdio.h>

// El archivo numpy.h ya incluye math.h (implícitamente) y los typedefs.

// --- Funciones de Álgebra Lineal ---

void matriz_rotacion_x(Matrix4x4 R, double angulo_grados) {
    /* Rotación 4x4 alrededor del eje X. */
    double angulo_rad = math_radians(angulo_grados);
    double c = np_cos(angulo_rad);
    double s = np_sin(angulo_rad);

    R[0][0] = 1; R[0][1] = 0; R[0][2] = 0;  R[0][3] = 0;
    R[1][0] = 0; R[1][1] = c; R[1][2] = -s; R[1][3] = 0;
    R[2][0] = 0; R[2][1] = s; R[2][2] = c;  R[2][3] = 0;
    R[3][0] = 0; R[3][1] = 0; R[3][2] = 0;  R[3][3] = 1;
}

void matriz_rotacion_y(Matrix4x4 R, double angulo_grados) {
    /* Rotación 4x4 alrededor del eje Y. */
    double angulo_rad = math_radians(angulo_grados);
    double c = np_cos(angulo_rad);
    double s = np_sin(angulo_rad);

    R[0][0] = c;  R[0][1] = 0; R[0][2] = s; R[0][3] = 0;
    R[1][0] = 0;  R[1][1] = 1; R[1][2] = 0; R[1][3] = 0;
    R[2][0] = -s; R[2][1] = 0; R[2][2] = c; R[2][3] = 0;
    R[3][0] = 0;  R[3][1] = 0; R[3][2] = 0; R[3][3] = 1;
}

void matriz_rotacion_z(Matrix4x4 R, double angulo_grados) {
    /* Rotación 4x4 alrededor del eje Z. */
    double angulo_rad = math_radians(angulo_grados);
    double c = np_cos(angulo_rad);
    double s = np_sin(angulo_rad);

    R[0][0] = c; R[0][1] = -s; R[0][2] = 0; R[0][3] = 0;
    R[1][0] = s; R[1][1] = c;  R[1][2] = 0; R[1][3] = 0;
    R[2][0] = 0; R[2][1] = 0;  R[2][2] = 1; R[2][3] = 0;
    R[3][0] = 0; R[3][1] = 0;  R[3][2] = 0; R[3][3] = 1;
}

void matriz_traslacion(Matrix4x4 T, double tx, double ty, double tz) {
    /* Genera la matriz de traslación 4x4. */
    
    // T = np.identity(4)
    np_identity(T, 4);

    T[0][3] = tx; // T[0, 3] = tx
    T[1][3] = ty; // T[1, 3] = ty
    T[2][3] = tz; // T[2, 3] = tz
}

void aplicar_transformada(Vector3D resultado, const Matrix4x4 matriz_transformada, const Vector3D coordenada_local) {
    /* Aplica la matriz de transformación a una coordenada 3D local. */
    
    // Convertir a coordenadas homogéneas [x, y, z, 1]
    Vector4D p_local_homogeneo;
    np_append(p_local_homogeneo, coordenada_local, 1.0);

    // p_global_homogeneo = matriz_transformada @ p_local_homogeneo
    Vector4D p_global_homogeneo;
    np_matmul(p_global_homogeneo, matriz_transformada, p_local_homogeneo);

    // Devolver las coordenadas 3D globales [x, y, z]
    resultado[0] = p_global_homogeneo[0];
    resultado[1] = p_global_homogeneo[1];
    resultado[2] = p_global_homogeneo[2];
}