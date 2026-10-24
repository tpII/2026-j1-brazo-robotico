/*
 * Este archivo es común para todas las versiones del algoritmo de FABRIK.
 * Implementación J1.
 */
#ifndef PLANO_H
#define PLANO_H

#include "numpy.h" // Para Vector3D
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Un vector (r, h) del plano del brazo. Es una estructura para poder devolverlo y copiarlo con =.
typedef struct {
    double r;
    double h;
} Vector2D;

// La cadena hombro, codo, p5 dentro del plano.
typedef struct {
    double l1;        // distancia del hombro al codo (cm)
    double l2;        // distancia del codo a p5, dentro del plano (cm)
    double alfa;      // ángulo fijo entre el segmento codo-p5 y el antebrazo (grados)
    double d;         // distancia de p5 al plano (cm)
    double z_hombro;  // altura del hombro (cm)
} CadenaVirtual;

// Una fila de la traza de la iteración. Los puntos son (r, h) del plano, medidos desde el hombro.
typedef struct {
    int k;                   // número de iteración
    Vector2D codo_forward;   // el codo después del forward (NAN en la fila 0)
    Vector2D codo_espejado;  // el codo después de la regla del codo, o NAN si no se espejó
    Vector2D codo;           // el codo después del backward
    Vector2D punta;          // la punta después del backward
    double error;            // distancia de la punta al objetivo (cm)
} Paso;

// Resultado de una ejecución de la iteración en un plano.
typedef struct {
    double theta1;        // ángulo del hombro al terminar (grados del modelo)
    double theta2;        // ángulo del codo al terminar (grados del modelo)
    double error;         // distancia final de la punta al objetivo (cm)
    int iteraciones;      // iteraciones hechas
    const char *motivo;   // por qué cortó: "eps", "eta" o "kmax"
    int recortes;         // cuántas veces se recortó un ángulo a su rango
    double mejor_error;   // el error de la iteración más cercana al objetivo
    double mejor_theta1;  // el hombro de esa iteración
    double mejor_theta2;  // el codo de esa iteración
    Paso *traza;          // los Paso de cada iteración (iteraciones + 1), si se pidieron; si no, NULL
} Ejecucion;

// Lo que pasó en un plano candidato.
typedef struct {
    double theta0;        // ángulo de la base de ese plano (grados del modelo)
    double r_c;           // distancia horizontal del objetivo dentro del plano (cm)
    bool alcanzable;      // si pasó el Paso 0 dentro del plano
    Ejecucion ejecucion;  // la Ejecucion de la iteración, si se iteró
    double angulos[5];    // (theta0, theta1, theta2, theta3, theta4) al terminar la ejecución
    double error;         // distancia final de la punta al objetivo (cm)
    bool valida;          // si pasó el control final
} SolucionPlano;

// Resultado de probar los planos en orden para una muñeca y una pinza.
typedef struct {
    bool valida;              // si algún plano dio una pose válida
    double angulos[5];        // la pose del primer plano válido, o NAN
    double error;             // el error de esa pose (cm), o NAN
    int n_planos;             // cuántos planos se probaron, de 0 a 2
    SolucionPlano planos[2];  // la SolucionPlano de cada plano probado
} Solucion;

// La función de iteración de una versión de FABRIK (iterar de fabrik.c, por ejemplo).
typedef Ejecucion (*FuncionIterar)(Vector2D T, CadenaVirtual cadena, double theta1_ini, double theta2_ini,
                                   bool traza);

/** a + b. */
Vector2D sumar(Vector2D a, Vector2D b);

/** a - b. */
Vector2D restar(Vector2D a, Vector2D b);

/** k * v. */
Vector2D por(double k, Vector2D v);

/** v / k. */
Vector2D dividir(Vector2D v, double k);

/** La norma de v. */
double norma(Vector2D v);

/** El producto escalar entre a y b. */
double producto_escalar(Vector2D a, Vector2D b);

/**
 * Vector unitario del plano a g grados.
 * @param g Ángulo en grados, medido desde el eje r.
 * @return El vector (cos g, sin g).
 */
Vector2D u2(double g);

/**
 * Ángulo de un vector del plano, en grados.
 * @param v Vector (r, h).
 * @return atan2(h, r) en grados, entre -180 y 180.
 */
double ang(Vector2D v);

/**
 * Cadena virtual para una muñeca y una pinza dadas.
 * Se calcula con la cinemática directa en la pose cero.
 * @param theta3 Ángulo de la muñeca.
 * @param theta4 Ángulo de la pinza.
 * @return La CadenaVirtual.
 */
CadenaVirtual cadena_virtual(double theta3, double theta4);

/**
 * Paso 1: los planos que dejan p5 sobre el objetivo, primero el que apunta hacia él.
 * Descarta los que dejan la base fuera de su rango. Si rho < |d| no hay ninguno.
 * @param objetivo El punto (x, y, z) en cm.
 * @param d Distancia de p5 al plano, de la cadena virtual (cm).
 * @param planos (Salida) Hasta dos pares (theta0, r_c).
 * @return Cuántos planos hay, de 0 a 2.
 */
int planos_candidatos(const Vector3D objetivo, double d, double planos[2][2]);

/**
 * El objetivo dentro del plano, medido desde el hombro: T = (r_c, z - z_hombro).
 * @param objetivo El punto (x, y, z) en cm.
 * @param r_c Distancia horizontal del objetivo dentro del plano (cm).
 * @param cadena La CadenaVirtual.
 * @return El vector T.
 */
Vector2D objetivo_en_plano(const Vector3D objetivo, double r_c, CadenaVirtual cadena);

/**
 * Paso 0 dentro del plano: ||T|| <= l1 + l2.
 * @param T El objetivo en el plano, desde el hombro.
 * @param cadena La CadenaVirtual.
 * @return true si la cadena estirada llega.
 */
bool alcanzable(Vector2D T, CadenaVirtual cadena);

/**
 * Recorte angular.
 * El ángulo se lleva a [lo, lo + 360) y, si cae afuera, va al tope más cercano medido en ángulo.
 * @param a Ángulo en grados.
 * @param lo Tope mínimo en grados.
 * @param hi Tope máximo en grados.
 * @param recorte (Salida) Si hubo que recortar.
 * @return El ángulo en [lo, hi].
 */
double ajustar_angulo(double a, double lo, double hi, bool *recorte);

/**
 * Si un ángulo está en [lo, hi], tomado módulo 360, con una tolerancia de 1e-6 grados.
 * @param a Ángulo en grados.
 * @param lo Tope mínimo en grados.
 * @param hi Tope máximo en grados.
 * @return true si está en el rango.
 */
bool en_rango(double a, double lo, double hi);

/**
 * Los rangos que usa FABRIK: base, hombro y codo, en grados del modelo.
 * @param rangos (Salida) Tres pares (mínimo, máximo).
 */
void rangos_fabrik(double rangos[3][2]);

/**
 * Control final: error <= epsilon, ángulos en rango y p5 a epsilon o menos del objetivo.
 * @param objetivo El punto (x, y, z) en cm.
 * @param angulos La pose (theta0, theta1, theta2, theta3, theta4) en grados del modelo.
 * @param error El error que dio la iteración (cm).
 * @return true si la pose pasa el control.
 */
bool es_valida(const Vector3D objetivo, const double angulos[5], double error);

/**
 * Paso 0, iteración y control final en un plano.
 * @param iterar La función de iteración de una versión de FABRIK (iterar de fabrik.c, por ejemplo).
 * @param objetivo El punto (x, y, z) en cm.
 * @param theta0 Ángulo de la base del plano (grados del modelo).
 * @param r_c Distancia horizontal del objetivo dentro del plano (cm).
 * @param cadena La CadenaVirtual.
 * @param theta3 Ángulo de la muñeca (grados del modelo).
 * @param theta4 Ángulo de la pinza (grados del modelo).
 * @param theta1_ini Hombro de partida (grados del modelo).
 * @param theta2_ini Codo de partida (grados del modelo).
 * @return La SolucionPlano.
 */
SolucionPlano resolver_plano(FuncionIterar iterar, const Vector3D objetivo, double theta0, double r_c,
                             CadenaVirtual cadena, double theta3, double theta4, double theta1_ini,
                             double theta2_ini);

/**
 * Prueba los planos candidatos en orden y se queda con el primero válido.
 * @param iterar La función de iteración de una versión de FABRIK.
 * @param objetivo El punto (x, y, z) en cm.
 * @param theta3 Ángulo de la muñeca (grados del modelo).
 * @param theta4 Ángulo de la pinza (grados del modelo).
 * @param theta1_ini Hombro de partida (grados del modelo).
 * @param theta2_ini Codo de partida (grados del modelo).
 * @return La Solucion.
 */
Solucion resolver_planos(FuncionIterar iterar, const Vector3D objetivo, double theta3, double theta4,
                         double theta1_ini, double theta2_ini);

#ifdef __cplusplus
}
#endif

#endif // PLANO_H
