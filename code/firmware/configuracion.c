#include "configuracion.h"
#include "numpy.h" 
#include "algebra_lineal.h" 
#include <string.h>      // Para strcmp

// --- 1. CONFIGURACIÓN GENERAL --- //
const double RADIO_ESFERA = 1.0; 
const Vector3D T_BASE_GLOBAL = {0.0, 0.0, 0.0}; 

// --- Matriz de Colisiones ---
const int MATRIZ_COLISIONES[5][5] = {
    { 0, 0, 0, 0, 1}, // Eslabón 0
    { 0, 0, 1, 0, 1}, // Eslabón 1
    { 0, 1, 0, 0, 0}, // Eslabón 2
    { 0, 0, 0, 0, 0}, // Eslabón 3
    { 1, 1, 0, 0, 0}  // Eslabón 4
};

// Fijamos los ángulos de prueba 
const int ANGULO_FIJO_ESLABON_0 = 0;
const int ANGULO_FIJO_ESLABON_1 = 0; 
const int ANGULO_FIJO_ESLABON_2 = 0; 
const int ANGULO_FIJO_ESLABON_3 = 0;
const int ANGULO_FIJO_ESLABON_4 = 0;

// --- 2. VECTORES DE DESPLAZAMIENTO --- //
const Vector3D SALTO_0 = {0.0, 0.0, 3.0};
const Vector3D SALTO_1 = {3.3, 0.0, 0.0};
const Vector3D SALTO_2 = {4.0, 0.5, 0.0};
const Vector3D SALTO_3 = {0.0, 0.0, -2.0}; 
const Vector3D SALTO_4 = {0.0, 0.0, 2.0}; 

const Vector3D *SALTO_ESLABONES[5] = {&SALTO_0, &SALTO_1, &SALTO_2, &SALTO_3, &SALTO_4};

// --- 3. CREACIÓN DE ESFERAS LOCALES ---

// esf_0 = np.array([ [0.0, 0.0, 0.0] ])
const EsferaE0 esf_0 = {
    {0.0, 0.0, 0.0}
};

// esf_1 = np.array([ [0.0, 0.0, 0.0], [2, 0, 0.0] ])
const EsferaE1 esf_1 = {
    {0.0, 0.0, 0.0},
    {2.0, 0.0, 0.0}
};

// esf_2 = np.array([ [0.0, 0.0, 0.0], [3.0, 0.0, 0.0], [5.0, 0.0, 0.0] ])
const EsferaE2 esf_2 = {
    {0.0, 0.0, 0.0},
    {2.0, 0.5, 0.0},
    {4.0, 0.5, 0.0}
};

// esf_3 = np.array([[0.0, 0.0, 0.0]])
const EsferaE3 esf_3 = {
    {0.0, 0.0, 0.0}
};

// esf_4 = np.array([ [0, 0.0, 0.0], [0, -2.0, 0.0] ])
const EsferaE4 esf_4 = {
    {0.0, 0.0, 0.0},
    {0.0, -2.0, 0.0}
};

// Lista de esferas locales
const void *esferas_local[5] = {
    (const void *)&esf_0, 
    (const void *)&esf_1, 
    (const void *)&esf_2, 
    (const void *)&esf_3, 
    (const void *)&esf_4
};


// --- 4. Vectores OFFSET ---
Matrix4x4 T_E0; 
Matrix4x4 T_E1; 
Matrix4x4 T_E2; 
Matrix4x4 T_E3; 
Matrix4x4 T_E4; 

const Matrix4x4 *OFFSET_ESLABON_EN_JUNTA[5] = {&T_E0, &T_E1, &T_E2, &T_E3, &T_E4};

// Implementación de la Función de Inicialización (requerida por C)
void inicializar_matrices_offset() {
    // T_E0 = np.eye(4)
    np_eye(T_E0, 4);

    // T_E1 = matriz_traslacion(0, 2.3, 1.5)
    matriz_traslacion(T_E1, 0.0, 2.3, 1.5);

    // T_E2 = np.eye(4)
    np_eye(T_E2, 4);

    // T_E3 = matriz_traslacion(0, 2.3, 1.5)
    matriz_traslacion(T_E3, 0.0, 2.3, 1.5);

    // T_E4 = matriz_traslacion(0, 2.2, 0.0)
    matriz_traslacion(T_E4, 0.0, 2.2, 0.0);
}


// Para abajo es implementado por el grupo J1.

// --- 3. ESFERAS LOCALES --- //
const int CANTIDAD_ESFERAS[5] = {1, 2, 3, 1, 2};

// --- 4. PISO Y TOLERANCIA (cinematica.c: plano_z = 0.0 y tol = 1e-9) --- //
const double PLANO_PISO_Z = 0.0;
const double TOL_COLISION = 1e-9;

// --- 5. SERVOS (firmware.ino) --- //
const int SERVO_MIN_US[5] = {400, 400, 400, 400, 400};
const int SERVO_MAX_US[5] = {2600, 1800, 2600, 2000, 2600};
const int SERVO_HOME_US[5] = {400, 1400, 1200, 960, 400};

// Tablas de calibración (us, grados del servo), una por servo como en getAngleForServo. Son las LUT
// de firmware.ino; van en un solo arreglo porque esos nombres ya existen allá.
const PuntoCalibracion LUT_POR_SERVO[5][7] = {
    {{400, 0.0}, {767, 30.0}, {1134, 60.0}, {1501, 90.0}, {1868, 120.0}, {2235, 150.0}, {2600, 180.0}}, // LUT_GENERICO
    {{400, 0.0}, {800, 30.0}, {1000, 60.0}, {1100, 90.0}, {1200, 120.0}, {1500, 150.0}, {1800, 180.0}}, // LUT1
    {{400, 180.0}, {767, 150.0}, {1134, 120.0}, {1501, 90.0}, {1868, 60.0}, {2235, 30.0}, {2600, 0.0}}, // LUT2
    {{400, 0.0}, {500, 30.0}, {600, 60.0}, {900, 90.0}, {1300, 120.0}, {1700, 150.0}, {2000, 180.0}},   // LUT3
    {{400, 0.0}, {767, 30.0}, {1134, 60.0}, {1501, 90.0}, {1868, 120.0}, {2235, 150.0}, {2600, 180.0}}  // LUT_GENERICO
};

// Grados del modelo: theta = signo * s + desvío (home() y loop() de firmware.ino).
const int SIGNO_MODELO[5] = {-1, 1, -1, 1, 1};
const double DESVIO_MODELO[5] = {0.0, 0.0, 0.0, -90.0, 0.0};

// --- 6. FABRIK Y BÚSQUEDA --- //
const double EPSILON = 0.5;          // cm: el error que se acepta
const double ETA = 1e-4;             // cm: p5 dejó de moverse
const int K_MAX = 20;                // iteraciones como máximo
const double PASO_BUSQUEDA = 10.0;   // grados entre posiciones de la muñeca y de la pinza
const double MUNECA_NEUTRA = 0.0;
const double PINZA_NEUTRA = 90.0;    // alineada con el antebrazo en el modelo

// --- 7. MOVIMIENTO --- //
const double DT = 0.02;                    // s entre pasos
const double VEL_MAX_ARTICULACION = 60.0;  // °/s
const double VEL_MAX_PUNTO = 10.0;         // cm/s, para p1 a p5 y los centros de las esferas
const double T_RAMPA = 0.3;                // s para arrancar y para frenar

// --- 8. PUNTA DE LA PINZA (medida en el brazo real) --- //
const Vector3D SALTO_4_MEDIDO = {0.0, -4.3, 0.0};

// --- 9. MEDIDAS DEL BRAZO REAL (los valores pueden tener cierto error) --- //
const Vector3D SALTO_0_MEDIDO = {0.0, 0.0, 4.0};
const Vector3D SALTO_1_MEDIDO = {6.9, 0.0, 0.0};
const Vector3D SALTO_2_MEDIDO = {4.5, 0.5, 0.0};
const Vector3D SALTO_3_MEDIDO = {0.0, 0.0, -3.2};

// Los saltos que usa la cinemática de J1 en lugar de SALTO_ESLABONES.
const Vector3D *SALTO_ESLABONES_MEDIDOS[5] = {&SALTO_0_MEDIDO, &SALTO_1_MEDIDO, &SALTO_2_MEDIDO,
                                              &SALTO_3_MEDIDO, &SALTO_4_MEDIDO};
