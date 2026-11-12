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

// --- 4. PISO Y TOLERANCIA (cinematica.c: plano_z = 0.0 y tol = 1e-9) --- //
const double PLANO_PISO_Z = 0.0;
const double TOL_COLISION = 1e-9;

// --- 5. SERVOS MG90S --- //
// Los horns se montan con los servos a 1500 us y el brazo en L: el brazo vertical; el antebrazo, la
// muñeca y la pinza horizontales y en línea, y la base a 90 grados del frente. Los pulsos y las
// tablas de J5 en firmware.ino quedan con los SG90.
const int SERVO_MIN_US[5] = {550, 550, 550, 550, 500};
const int SERVO_MAX_US[5] = {2450, 2450, 2500, 2450, 2400};
// El mismo home que con los SG90: (0, 140, -114.6, 4.5, 0) grados del modelo.
const int SERVO_HOME_US[5] = {550, 2006, 1199, 1550, 500};

// Tablas de calibración (us, grados del servo) medidas en el brazo, de 45 en 45 grados a partir de la L,
// que son 90 grados del servo. La del codo va al revés, como LUT2 de J5. En la pinza los 0 grados se
// midieron con 450 us, pero ESP32Servo no manda pulsos de menos de 500 us y ese punto queda en 500.
const PuntoCalibracion LUT_POR_SERVO[5][5] = {
    {{550, 0.0}, {1000, 45.0}, {1500, 90.0}, {2050, 135.0}, {2450, 180.0}}, // base
    {{550, 0.0}, {1000, 45.0}, {1500, 90.0}, {1950, 135.0}, {2450, 180.0}}, // hombro
    {{550, 180.0}, {950, 135.0}, {1500, 90.0}, {2050, 45.0}, {2500, 0.0}},  // codo
    {{550, 0.0}, {1050, 45.0}, {1500, 90.0}, {2000, 135.0}, {2450, 180.0}}, // muñeca
    {{500, 0.0}, {900, 45.0}, {1500, 90.0}, {2000, 135.0}, {2400, 180.0}}   // pinza
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

// --- 8. PUNTA DE LA PINZA (de los STL) --- //
const Vector3D SALTO_4_MEDIDO = {0.0, -4.33, 0.0}; // la mano es un ArmFront: del eje del servo 4 al final de la bandeja

// --- 9. MEDIDAS DEL BRAZO REAL (de los STL de las piezas y de la hoja de datos del MG90S) --- //
const Vector3D SALTO_0_MEDIDO = {0.0, 0.0, 4.58};  // la base giratoria a 3.29 cm de la mesa y el hombro 1.29 cm más arriba
const Vector3D SALTO_1_MEDIDO = {6.25, 0.0, 0.0};  // Arm.stl con el tramo del medio 1.3 cm más largo, como el armado
const Vector3D SALTO_2_MEDIDO = {5.89, 0.0, 0.3};  // hasta el horn de la muñeca, cuyo eje queda 0.3 cm al costado
const Vector3D SALTO_3_MEDIDO = {0.0, 0.0, -1.29}; // la base de la mano es un Swing: su fondo y el eje del servo 4

// Los saltos que usa la cinemática de J1 en lugar de SALTO_ESLABONES.
const Vector3D *SALTO_ESLABONES_MEDIDOS[5] = {&SALTO_0_MEDIDO, &SALTO_1_MEDIDO, &SALTO_2_MEDIDO,
                                              &SALTO_3_MEDIDO, &SALTO_4_MEDIDO};

// --- 10. ESFERAS DE COLISIÓN (de las medidas del brazo) --- //
// Una esfera en cada articulación y otra cada 1.5 cm como máximo, sobre el salto de cada eslabón.
// En la pala la última queda a un radio de la punta, así la esfera no sobresale.
const double RADIO_ESFERA_MEDIDO = 1.5;
const int CANTIDAD_ESFERAS[5] = {4, 5, 4, 1, 3};
const Vector3D ESFERAS_MEDIDAS[5][5] = {
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.145}, {0.0, 0.0, 2.29}, {0.0, 0.0, 3.435}},
    {{0.0, 0.0, 0.0}, {1.25, 0.0, 0.0}, {2.5, 0.0, 0.0}, {3.75, 0.0, 0.0}, {5.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {1.4725, 0.0, 0.075}, {2.945, 0.0, 0.15}, {4.4175, 0.0, 0.225}},
    {{0.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.0, -1.415, 0.0}, {0.0, -2.83, 0.0}}
};

// Los pares contiguos y el (2, 4) no se revisan: sus esferas se superponen en todas las poses (la
// base de la mano mide 1.29 cm y deja la pala junto al antebrazo). El choque del brazo con el
// antebrazo lo cubre el tope del codo.
const int MATRIZ_COLISIONES_MEDIDA[5][5] = {
    { 0, 0, 1, 1, 1}, // Eslabón 0
    { 0, 0, 0, 1, 1}, // Eslabón 1
    { 1, 0, 0, 0, 0}, // Eslabón 2
    { 1, 1, 0, 0, 0}, // Eslabón 3
    { 1, 1, 0, 0, 0}  // Eslabón 4
};

// Con el codo más cerrado que esto (grados del modelo), el antebrazo toca el brazo.
const double TOPE_CODO = -139.0;

// Caja de la pala en su marco (cm): x es el grosor, y el largo y z el ancho del ArmFront de la mano.
// Contra la mesa se revisan sus 8 esquinas.
const double CAJA_PALA[3][2] = {{-0.85, 0.75}, {-4.38, 0.82}, {-2.14, 1.56}};
