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