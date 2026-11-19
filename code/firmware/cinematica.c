/*
 * Cinemática directa del brazo: puntos, marcos y esferas para cinco ángulos del modelo.
 *
 * Archivo de 2025 (grupo J5, code/C/main/cinematica.c). En la simulación es cinematica.py.
 */
#include "cinematica.h"
#include "configuracion.h"
#include "algebra_lineal.h"
#include "numpy.h"


double np_linalg_norm(const double *a, const double *b) {
    /* Distancia entre dos puntos 3D, como np.linalg.norm(a - b). */
    // Asume vectores 3D (índices 0, 1, 2)
    double dx = a[0] - b[0];
    double dy = a[1] - b[1];
    double dz = a[2] - b[2];
    return sqrt(dx * dx + dy * dy + dz * dz);
}


// Para abajo es implementado por el grupo J1.

void cinematica_directa(const double angulos[5], bool con_esferas, Geometria *g) {
    /* Puntos, marcos y esferas del brazo para cinco ángulos del modelo. */

    // Para cada eslabón i: M = M_acum * (Rfix_i * Rz(theta_i)) y después M_acum = M * T(SALTO_i).
    // Rfix son las rotaciones fijas de montaje de J5 (ROTACIONES_FIJAS en configuracion.py). Se
    // arman en la primera llamada.
    static Matrix4x4 Rfix[5];
    static bool rfix_armadas = false;
    if (!rfix_armadas) {
        np_eye(Rfix[0], 4);
        matriz_rotacion_x(Rfix[1], 90.0);
        np_eye(Rfix[2], 4);
        matriz_rotacion_y(Rfix[3], -90.0);
        matriz_rotacion_y(Rfix[4], 90.0);
        rfix_armadas = true;
    }
    Matrix4x4 M_acum, M, R_junta, M_temp, T_salto;
    matriz_traslacion(M_acum, T_BASE_GLOBAL[0], T_BASE_GLOBAL[1], T_BASE_GLOBAL[2]);

    for (int i = 0; i < 5; i++) {
        matriz_rotacion_z(R_junta, angulos[i]);
        np_matmul_4x4_4x4(M_temp, Rfix[i], R_junta);
        np_matmul_4x4_4x4(M, M_acum, M_temp);
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 4; k++) {
                g->marcos[i][j][k] = M[j][k];
            }
            g->puntos[i][j] = M[j][3];
        }
        if (con_esferas) {
            for (int e = 0; e < CANTIDAD_ESFERAS[i]; e++) {
                aplicar_transformada(g->esferas[i][e], M, esferas_local[i][e]);
            }
        }
        const double *salto = *SALTO_ESLABONES[i];
        matriz_traslacion(T_salto, salto[0], salto[1], salto[2]);
        np_matmul_4x4_4x4(M_acum, M, T_salto);
    }
    for (int j = 0; j < 3; j++) {
        g->puntos[5][j] = M_acum[j][3];
    }
}
