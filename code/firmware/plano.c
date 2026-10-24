/*
 * Este archivo es común para todas las versiones del algoritmo de FABRIK.
 *
 * Implementación de la cadena virtual, el Paso 0, el Paso 1 (planos candidatos), el recorte
 * angular, el control final y la resolución de los planos en orden. Cada versión pone su propio
 * paso de iteración.
 * Implementación J1.
 */
#include "plano.h"
#include "cinematica.h"
#include "configuracion.h"
#include "servos.h"
#include <math.h>
#include <string.h> // Para memcpy

static const double D2R = M_PI / 180;

Vector2D sumar(Vector2D a, Vector2D b) {
    Vector2D v = {a.r + b.r, a.h + b.h};
    return v;
}

Vector2D restar(Vector2D a, Vector2D b) {
    Vector2D v = {a.r - b.r, a.h - b.h};
    return v;
}

Vector2D por(double k, Vector2D v) {
    Vector2D w = {k * v.r, k * v.h};
    return w;
}

Vector2D dividir(Vector2D v, double k) {
    Vector2D w = {v.r / k, v.h / k};
    return w;
}

double norma(Vector2D v) {
    return sqrt(v.r * v.r + v.h * v.h);
}

double producto_escalar(Vector2D a, Vector2D b) {
    return a.r * b.r + a.h * b.h;
}

Vector2D u2(double g) {
    /* Vector unitario del plano a g grados. */
    Vector2D v = {cos(g * D2R), sin(g * D2R)};
    return v;
}

double ang(Vector2D v) {
    /* Ángulo de un vector del plano, en grados. */
    return atan2(v.h, v.r) / D2R;
}

static double modulo(double a, double b) {
    /* a módulo b, entre 0 y b, para b > 0. */
    double m = fmod(a, b);
    return m < 0 ? m + b : m;
}

static double norm180(double a) {
    /* Lleva un ángulo en grados a [-180, 180). */
    return modulo(a + 180, 360) - 180;
}

CadenaVirtual cadena_virtual(double theta3, double theta4) {
    /* Cadena virtual para una muñeca y una pinza dadas. */
    double pose_cero[5] = {0.0, 0.0, 0.0, theta3, theta4};
    Vector3D p[6];
    cinematica_directa(pose_cero, p);
    Vector2D v = {p[5][0] - p[2][0], p[5][2] - p[2][2]};
    CadenaVirtual cadena = {.l1 = np_linalg_norm(p[2], p[1]), .l2 = hypot(v.r, v.h), .alfa = ang(v),
                            .d = p[5][1], .z_hombro = p[1][2]};
    return cadena;
}

int planos_candidatos(const Vector3D objetivo, double d, double planos[2][2]) {
    /* Paso 1: los planos que dejan p5 sobre el objetivo, primero el que apunta hacia él. */
    double x = objetivo[0], y = objetivo[1];
    double rho = hypot(x, y);
    if (rho < fabs(d)) {
        return 0;
    }
    double rango[2];
    rango_modelo(0, rango);
    int signos[2] = {1, -1};
    int n = 0;
    for (int i = 0; i < 2; i++) {
        double r_c = signos[i] * sqrt(rho * rho - d * d);
        double theta0 = norm180(atan2(y, x) / D2R - atan2(d, r_c) / D2R);
        if (en_rango(theta0, rango[0], rango[1])) {
            planos[n][0] = theta0;
            planos[n][1] = r_c;
            n++;
        }
    }
    return n;
}

Vector2D objetivo_en_plano(const Vector3D objetivo, double r_c, CadenaVirtual cadena) {
    /* El objetivo dentro del plano, medido desde el hombro: T = (r_c, z - z_hombro). */
    Vector2D T = {r_c, objetivo[2] - cadena.z_hombro};
    return T;
}

bool alcanzable(Vector2D T, CadenaVirtual cadena) {
    /* Paso 0 dentro del plano: ||T|| <= l1 + l2. */
    return norma(T) <= cadena.l1 + cadena.l2;
}

double ajustar_angulo(double a, double lo, double hi, bool *recorte) {
    /* Recorte angular. */
    a = lo + modulo(a - lo, 360.0);
    if (a <= hi + 1e-9) {
        *recorte = false;
        return a;
    }
    *recorte = true;
    return (a - hi) <= (lo + 360.0 - a) ? hi : lo;
}

bool en_rango(double a, double lo, double hi) {
    /* Si un ángulo está en [lo, hi], tomado módulo 360. */
    double tol = 1e-6;  // tolerancia en grados
    a = lo + modulo(a - lo, 360.0);
    return a <= hi + tol || a >= lo + 360.0 - tol;
}

void rangos_fabrik(double rangos[3][2]) {
    /* Los rangos que usa FABRIK: base, hombro y codo, en grados del modelo. */
    for (int i = 0; i < 3; i++) {
        rango_modelo(i, rangos[i]);
    }
}

bool es_valida(const Vector3D objetivo, const double angulos[5], double error) {
    /* Control final: error <= epsilon, ángulos en rango y p5 a epsilon o menos del objetivo. */
    if (error > EPSILON) {
        return false;
    }
    double rangos[3][2];
    rangos_fabrik(rangos);
    for (int i = 0; i < 3; i++) {
        if (!en_rango(angulos[i], rangos[i][0], rangos[i][1])) {
            return false;
        }
    }
    Vector3D p[6];
    cinematica_directa(angulos, p);
    return np_linalg_norm(p[5], objetivo) <= EPSILON + 1e-9;
}

SolucionPlano resolver_plano(FuncionIterar iterar, const Vector3D objetivo, double theta0, double r_c,
                             CadenaVirtual cadena, double theta3, double theta4, double theta1_ini,
                             double theta2_ini) {
    /* Paso 0, iteración y control final en un plano. */
    Vector2D T = objetivo_en_plano(objetivo, r_c, cadena);
    if (!alcanzable(T, cadena)) {
        SolucionPlano solucion = {.theta0 = theta0, .r_c = r_c, .alcanzable = false};
        return solucion;
    }
    Ejecucion ejecucion = iterar(T, cadena, theta1_ini, theta2_ini, false);
    double theta1 = ejecucion.theta1, theta2 = ejecucion.theta2, error = ejecucion.error;
    SolucionPlano solucion = {theta0, r_c, true, ejecucion, {theta0, theta1, theta2, theta3, theta4}, error, false};
    solucion.valida = es_valida(objetivo, solucion.angulos, error);
    return solucion;
}

Solucion resolver_planos(FuncionIterar iterar, const Vector3D objetivo, double theta3, double theta4,
                         double theta1_ini, double theta2_ini) {
    /* Prueba los planos candidatos en orden y se queda con el primero válido. */
    CadenaVirtual cadena = cadena_virtual(theta3, theta4);
    double candidatos[2][2];
    int n = planos_candidatos(objetivo, cadena.d, candidatos);
    Solucion resultado = {.valida = false, .angulos = {NAN, NAN, NAN, NAN, NAN}, .error = NAN, .n_planos = 0};
    for (int i = 0; i < n; i++) {
        SolucionPlano solucion = resolver_plano(iterar, objetivo, candidatos[i][0], candidatos[i][1], cadena,
                                                theta3, theta4, theta1_ini, theta2_ini);
        resultado.planos[resultado.n_planos++] = solucion;
        if (solucion.valida) {
            resultado.valida = true;
            memcpy(resultado.angulos, solucion.angulos, sizeof(resultado.angulos));
            resultado.error = solucion.error;
            return resultado;
        }
    }
    return resultado;
}
