#ifndef ALGEBRA_LINEAL_H
#define ALGEBRA_LINEAL_H

#include "numpy.h" // Incluye tipos como Matrix4x4, Vector3D, Vector4D y placeholders de NumPy

#ifdef __cplusplus
extern "C" {
#endif

// --- Definición de macro math.radians ---
#define math_radians(grados) ((grados) * (M_PI / 180.0))

// --- Funciones de Álgebra Lineal traducidas ---

void matriz_rotacion_x(Matrix4x4 R, double angulo_grados);
void matriz_rotacion_y(Matrix4x4 R, double angulo_grados);
void matriz_rotacion_z(Matrix4x4 R, double angulo_grados);
void matriz_traslacion(Matrix4x4 T, double tx, double ty, double tz);
void aplicar_transformada(Vector3D resultado, const Matrix4x4 matriz_transformada, const Vector3D coordenada_local);

#ifdef __cplusplus
}
#endif

#endif // ALGEBRA_LINEAL_H