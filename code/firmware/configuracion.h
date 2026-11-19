/*
 * Datos del brazo y parámetros del firmware.
 *
 * Archivo de 2025 (grupo J5, code/C/main/configuracion.h). En la simulación es configuracion.py.
 */
#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include "algebra_lineal.h" // Incluye tipos como Matrix4x4, Vector3D, Vector4D y numpy.h
#include <stdbool.h>       // Para usar bool (Traduciendo el True/False)

#ifdef __cplusplus
extern "C" {
#endif

// --- 1. CONFIGURACIÓN GENERAL --- //
extern const Vector3D T_BASE_GLOBAL; // posición de la base en el marco global


// Para abajo es implementado por el grupo J1.

// --- 2. PISO Y TOLERANCIA --- //
extern const double PLANO_PISO_Z;
extern const double TOL_COLISION;

// --- 3. SERVOS MG90S --- //
extern const int SERVO_MIN_US[5];
extern const int SERVO_MAX_US[5];
extern const int SERVO_HOME_US[5];

// Un punto de una tabla de calibración: el pulso y los grados del servo.
typedef struct {
    int us;
    double grados;
} PuntoCalibracion;

extern const PuntoCalibracion LUT_POR_SERVO[5][5];

extern const int SIGNO_MODELO[5];
extern const double DESVIO_MODELO[5];

// --- 4. FABRIK Y BÚSQUEDA --- //
extern const double EPSILON;
extern const double ETA;
extern const int K_MAX;
extern const double PASO_BUSQUEDA;
extern const double MUNECA_NEUTRA;
extern const double PINZA_NEUTRA;

// --- 5. MOVIMIENTO --- //
extern const double DT;
extern const double VEL_MAX_ARTICULACION;
extern const double VEL_MAX_PUNTO;
extern const double T_RAMPA;

// --- 6 y 7. MEDIDAS DEL BRAZO REAL --- //
// Posición del final de cada eslabón en su marco local (cm).
extern const Vector3D SALTO_0;
extern const Vector3D SALTO_1;
extern const Vector3D SALTO_2;
extern const Vector3D SALTO_3;
extern const Vector3D SALTO_4;

extern const Vector3D *SALTO_ESLABONES[5];

// --- 8. ESFERAS DE COLISIÓN --- //
extern const double RADIO_ESFERA;
extern const int CANTIDAD_ESFERAS[5];           // cuántas esferas tiene cada eslabón
extern const Vector3D esferas_local[5][5];      // los centros locales, CANTIDAD_ESFERAS[i] en el eslabón i
extern const int MATRIZ_COLISIONES[5][5];
extern const double TOPE_CODO;
extern const double CAJA_PALA[3][2];

#ifdef __cplusplus
}
#endif

#endif // CONFIGURACION_H
