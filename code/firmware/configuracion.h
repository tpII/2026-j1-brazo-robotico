#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include "algebra_lineal.h" // Incluye tipos como Matrix4x4, Vector3D, Vector4D y numpy.h
#include <stdbool.h>       // Para usar bool (Traduciendo el True/False)

#ifdef __cplusplus
extern "C" {
#endif

// --- Nuevas Definiciones de Tipos para Esferas ---
// Tipos específicos para la dimensión de cada array de esferas.
typedef double EsferaE0[1][3];
typedef double EsferaE1[2][3];
typedef double EsferaE2[3][3];
typedef double EsferaE3[1][3];
typedef double EsferaE4[2][3];

// --- 1. CONFIGURACIÓN GENERAL --- //
extern const double RADIO_ESFERA; 
extern const Vector3D T_BASE_GLOBAL; 

// --- Matriz de Colisiones ---
extern const int MATRIZ_COLISIONES[5][5];

// Fijamos los ángulos de prueba 
extern const int ANGULO_FIJO_ESLABON_0;
extern const int ANGULO_FIJO_ESLABON_1;
extern const int ANGULO_FIJO_ESLABON_2;
extern const int ANGULO_FIJO_ESLABON_3;
extern const int ANGULO_FIJO_ESLABON_4;

// Lista de límites angulares
extern const double min_angulos[5];
extern const double max_angulos[5];

// --- 2. VECTORES DE DESPLAZAMIENTO --- //
extern const Vector3D SALTO_0;
extern const Vector3D SALTO_1;
extern const Vector3D SALTO_2;
extern const Vector3D SALTO_3;
extern const Vector3D SALTO_4;

extern const Vector3D *SALTO_ESLABONES[5];

// --- 3. CREACIÓN DE ESFERAS LOCALES ---
extern const EsferaE0 esf_0;
extern const EsferaE1 esf_1;
extern const EsferaE2 esf_2;
extern const EsferaE3 esf_3;
extern const EsferaE4 esf_4;

// Lista de punteros a los arrays de esferas locales (se usa void* para el tipo genérico)
extern const void *esferas_local[5];


// --- 4. Vectores OFFSET ---
extern Matrix4x4 T_E0; 
extern Matrix4x4 T_E1; 
extern Matrix4x4 T_E2; 
extern Matrix4x4 T_E3; 
extern Matrix4x4 T_E4; 

extern const Matrix4x4 *OFFSET_ESLABON_EN_JUNTA[5]; 

// --- Inicialización y Simulación ---
void inicializar_matrices_offset(); // Necesaria para inicializar T_Ex matrices
// extern const bool MODO_DINAMICO;

// // --- Función de Vista ---
// void set_view(void *ax, const char *plane);


// Para abajo es implementado por el grupo J1.

// --- 3. ESFERAS LOCALES --- //
// Cuántas esferas tiene cada eslabón: las filas de esf_0 a esf_4.
extern const int CANTIDAD_ESFERAS[5];

// --- 4. PISO Y TOLERANCIA (cinematica.c) --- //
extern const double PLANO_PISO_Z;
extern const double TOL_COLISION;

// --- 5. SERVOS MG90S --- //
extern const int SERVO_MIN_US[5];
extern const int SERVO_MAX_US[5];
extern const int SERVO_HOME_US[5];

// Un punto de una tabla de calibración: el pulso y los grados del servo.
typedef struct {
    int us;
    double grados;
} PuntoCalibracion;

extern const PuntoCalibracion LUT_POR_SERVO[5][7];

extern const int SIGNO_MODELO[5];
extern const double DESVIO_MODELO[5];

// --- 6. FABRIK Y BÚSQUEDA --- //
extern const double EPSILON;
extern const double ETA;
extern const int K_MAX;
extern const double PASO_BUSQUEDA;
extern const double MUNECA_NEUTRA;
extern const double PINZA_NEUTRA;

// --- 7. MOVIMIENTO --- //
extern const double DT;
extern const double VEL_MAX_ARTICULACION;
extern const double VEL_MAX_PUNTO;
extern const double T_RAMPA;

// --- 8. PUNTA DE LA PINZA (medida en el brazo real) --- //
extern const Vector3D SALTO_4_MEDIDO;

// --- 9. MEDIDAS DEL BRAZO REAL (los valores pueden tener cierto error) --- //
extern const Vector3D SALTO_0_MEDIDO;
extern const Vector3D SALTO_1_MEDIDO;
extern const Vector3D SALTO_2_MEDIDO;
extern const Vector3D SALTO_3_MEDIDO;

extern const Vector3D *SALTO_ESLABONES_MEDIDOS[5];

#ifdef __cplusplus
}
#endif

#endif // CONFIGURACION_H