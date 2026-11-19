/*
 * Datos del brazo y parámetros del firmware.
 *
 * Archivo de 2025 (grupo J5, code/C/main/configuracion.c). En la simulación es configuracion.py.
 */
#include "configuracion.h"

// --- 1. CONFIGURACIÓN GENERAL --- //
const Vector3D T_BASE_GLOBAL = {0.0, 0.0, 0.0};


// Para abajo es implementado por el grupo J1.

// --- 2. PISO Y TOLERANCIA --- //
// Las rotaciones fijas de montaje de esta sección de configuracion.py están escritas en
// cinematica_directa (cinematica.c).
const double PLANO_PISO_Z = 0.0;
const double TOL_COLISION = 1e-9;

// --- 3. SERVOS MG90S --- //
// Los horns se montan con los servos a 1500 us y el brazo en L: el brazo vertical; el antebrazo, la
// muñeca y la pinza horizontales y en línea, y la base a 90 grados del frente. La pinza baja hasta
// 450 us. La ESP32Servo del gestor de librerías no manda pulsos de menos de 500 us (MIN_PULSE_WIDTH
// en ESP32Servo.h), así que el firmware se tiene que compilar con ese mínimo en 450 o menos.
const int SERVO_MIN_US[5] = {550, 550, 550, 550, 450};
const int SERVO_MAX_US[5] = {2450, 2450, 2500, 2450, 2400};
// El mismo home que con los SG90: (0, 140, -114.6, 4.5, 0) grados del modelo.
const int SERVO_HOME_US[5] = {550, 2006, 1199, 1550, 450};

// Tablas de calibración (us, grados del servo) medidas en el brazo, de 45 en 45 grados a partir de la L,
// que son 90 grados del servo. La del codo va al revés: con más pulso, menos grados.
const PuntoCalibracion LUT_POR_SERVO[5][5] = {
    {{550, 0.0}, {1000, 45.0}, {1500, 90.0}, {2050, 135.0}, {2450, 180.0}}, // base
    {{550, 0.0}, {1000, 45.0}, {1500, 90.0}, {1950, 135.0}, {2450, 180.0}}, // hombro
    {{550, 180.0}, {950, 135.0}, {1500, 90.0}, {2050, 45.0}, {2500, 0.0}},  // codo
    {{550, 0.0}, {1050, 45.0}, {1500, 90.0}, {2000, 135.0}, {2450, 180.0}}, // muñeca
    {{450, 0.0}, {900, 45.0}, {1500, 90.0}, {2000, 135.0}, {2400, 180.0}}   // pinza
};

// Grados del modelo: theta = signo * s + desvío (servo_a_modelo, en servos.c).
const int SIGNO_MODELO[5] = {-1, 1, -1, 1, 1};
const double DESVIO_MODELO[5] = {0.0, 0.0, 0.0, -90.0, 0.0};

// --- 4. FABRIK Y BÚSQUEDA --- //
const double EPSILON = 0.5;          // cm: el error que se acepta
const double ETA = 1e-4;             // cm: p5 dejó de moverse
const int K_MAX = 20;                // iteraciones como máximo
const double PASO_BUSQUEDA = 10.0;   // grados entre posiciones de la muñeca y de la pinza
const double MUNECA_NEUTRA = 0.0;
const double PINZA_NEUTRA = 90.0;    // alineada con el antebrazo en el modelo

// --- 5. MOVIMIENTO --- //
const double DT = 0.02;                    // s entre pasos
const double VEL_MAX_ARTICULACION = 60.0;  // °/s
const double VEL_MAX_PUNTO = 10.0;         // cm/s, para p1 a p5 y los centros de las esferas
const double T_RAMPA = 0.3;                // s para arrancar y para frenar

// --- 6. PUNTA DE LA PINZA (de los STL) --- //
// La mano es un ArmFront: del eje del servo 4 al final de la bandeja.
const Vector3D SALTO_4 = {0.0, -4.33, 0.0};

// --- 7. MEDIDAS DEL BRAZO REAL (de los STL de las piezas y de la hoja de datos del MG90S) --- //
// Posición del final de cada eslabón en su marco local (cm).
// El fondo de la base giratoria queda a 3.29 cm de la mesa (caja del soporte, tapa del eje del
// MG90S y horn) y el eje del hombro, 1.29 cm más arriba.
const Vector3D SALTO_0 = {0.0, 0.0, 4.58};
// Arm.stl mide 4.95 cm entre ejes; el brazo armado tiene el tramo del medio 1.3 cm más largo.
const Vector3D SALTO_1 = {6.25, 0.0, 0.0};
// Del codo al horn de la muñeca, con el servo contra la pared de atrás de la caja. El eje de la
// muñeca queda 0.5 cm al costado (sobre el eje del codo) porque está cerca de una punta del servo.
const Vector3D SALTO_2 = {5.89, 0.0, 0.5};
// La base de la mano es otro Swing, con el eje del servo 4 a 1.29 cm de su fondo.
const Vector3D SALTO_3 = {0.0, 0.0, -1.29};

const Vector3D *SALTO_ESLABONES[5] = {&SALTO_0, &SALTO_1, &SALTO_2, &SALTO_3, &SALTO_4};

// --- 8. ESFERAS DE COLISIÓN (de las medidas del brazo) --- //
// Una esfera en cada articulación y otra cada 1.5 cm como máximo, sobre el salto de cada eslabón.
// En la pala la última queda a un radio de la punta, así la esfera no sobresale.
const double RADIO_ESFERA = 1.5;
const int CANTIDAD_ESFERAS[5] = {4, 5, 4, 1, 3};
const Vector3D esferas_local[5][5] = {
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.145}, {0.0, 0.0, 2.29}, {0.0, 0.0, 3.435}},
    {{0.0, 0.0, 0.0}, {1.25, 0.0, 0.0}, {2.5, 0.0, 0.0}, {3.75, 0.0, 0.0}, {5.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {1.4725, 0.0, 0.125}, {2.945, 0.0, 0.25}, {4.4175, 0.0, 0.375}},
    {{0.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.0, -1.415, 0.0}, {0.0, -2.83, 0.0}},
};

// MATRIZ_COLISIONES[i][j] = 1 indica que se revisa el choque entre los eslabones i y j. Los pares
// contiguos y el (2, 4) no se revisan: sus esferas se superponen en todas las poses (la base de la
// mano mide 1.29 cm y deja la pala junto al antebrazo). El choque del brazo con el antebrazo lo
// cubre el tope del codo.
const int MATRIZ_COLISIONES[5][5] = {
    {0, 0, 1, 1, 1}, // Eslabón 0
    {0, 0, 0, 1, 1}, // Eslabón 1
    {1, 0, 0, 0, 0}, // Eslabón 2
    {1, 1, 0, 0, 0}, // Eslabón 3
    {1, 1, 0, 0, 0}  // Eslabón 4
};

// Con el codo más cerrado que esto (grados del modelo), el antebrazo toca el brazo.
const double TOPE_CODO = -139.0;

// Caja de la pala en su marco (cm): x es el grosor, y el largo y z el ancho del ArmFront de la mano.
// Contra la mesa se revisan sus 8 esquinas.
const double CAJA_PALA[3][2] = {{-0.80, 0.80}, {-4.33, 0.80}, {-2.19, 1.56}};
