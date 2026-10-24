/*
 * FABRIK v3: la versión final.
 *
 * En el plano del brazo, cada iteración hace:
 * 1. forward: p5 sobre el objetivo y el codo a l2 de p5;
 * 2. regla del codo (pole): si el codo quedó del lado prohibido de la recta hombro -> objetivo,
 *    se lo espeja respecto de esa recta;
 * 3. backward: el codo a l1 del hombro, con el hombro recortado a su rango, y la punta a l2
 *    del codo, con el giro relativo theta2 + alpha recortado a [-180° + alpha, alpha].
 * Corta por epsilon (llegó), por eta (p5 dejó de moverse) o por k_max, en ese orden.
 * Implementación J1.
 */
#include "fabrik.h"
#include "configuracion.h"
#include <math.h>
#include <stdlib.h> // Para malloc

Ejecucion iterar(Vector2D T, CadenaVirtual cadena, double theta1_ini, double theta2_ini, bool traza) {
    /* Paso 2: itera en el plano desde la pose (theta1, theta2) hasta el objetivo T. */
    double rangos[3][2];
    rangos_fabrik(rangos);
    double lo_hombro = rangos[1][0], hi_hombro = rangos[1][1];
    double lo_codo = rangos[2][0], hi_codo = rangos[2][1];

    double l1 = cadena.l1, l2 = cadena.l2, alfa = cadena.alfa;
    Vector2D E = por(l1, u2(theta1_ini));
    Vector2D P = sumar(E, por(l2, u2(theta1_ini + theta2_ini + alfa)));
    double error = norma(restar(P, T));
    int k = 0, recortes = 0;
    const char *motivo = NULL;
    double mejor_error = 0.0, mejor_theta1 = 0.0, mejor_theta2 = 0.0;
    double theta1 = theta1_ini, rel = theta2_ini + alfa;
    bool recorte;
    Paso *pasos = traza ? (Paso *)malloc((K_MAX + 1) * sizeof(Paso)) : NULL;
    if (pasos != NULL) {
        Paso inicio = {0, {NAN, NAN}, {NAN, NAN}, E, P, error};
        pasos[0] = inicio;
    }

    while (error > EPSILON && k < K_MAX) {
        k++;
        Vector2D P_anterior = P;
        // forward
        P = T;
        E = sumar(P, dividir(por(l2, restar(E, P)), norma(restar(E, P))));
        Vector2D codo_forward = E, codo_espejado = {NAN, NAN};
        // regla del codo
        if (T.r * E.h - T.h * E.r < 0) {
            Vector2D u = dividir(T, norma(T));
            E = restar(por(2 * producto_escalar(E, u), u), E);
            codo_espejado = E;
        }
        // backward, con los límites del hombro y del codo
        E = dividir(por(l1, E), norma(E));
        theta1 = ajustar_angulo(ang(E), lo_hombro, hi_hombro, &recorte);
        recortes += recorte;
        E = por(l1, u2(theta1));
        Vector2D direccion = dividir(restar(P, E), norma(restar(P, E)));
        rel = ajustar_angulo(ang(direccion) - theta1, lo_codo + alfa, hi_codo + alfa, &recorte);
        recortes += recorte;
        P = sumar(E, por(l2, u2(theta1 + rel)));

        error = norma(restar(P, T));
        if (k == 1 || error < mejor_error) {
            mejor_error = error;
            mejor_theta1 = theta1;
            mejor_theta2 = rel - alfa;
        }
        if (pasos != NULL) {
            Paso paso = {k, codo_forward, codo_espejado, E, P, error};
            pasos[k] = paso;
        }
        if (error <= EPSILON) {
            motivo = "eps";
            break;
        }
        if (norma(restar(P, P_anterior)) < ETA) {
            motivo = "eta";
            break;
        }
    }
    if (motivo == NULL) {  // el while terminó sin break
        motivo = error > EPSILON ? "kmax" : "eps";
    }

    if (k == 0) {
        mejor_error = error;
        mejor_theta1 = theta1;
        mejor_theta2 = rel - alfa;
    }
    Ejecucion ejecucion = {theta1, rel - alfa, error, k, motivo, recortes,
                           mejor_error, mejor_theta1, mejor_theta2, pasos};
    return ejecucion;
}

Solucion resolver(const Vector3D objetivo, double theta3, double theta4, double theta1_ini, double theta2_ini) {
    /* Pasos 0 a 3 y control final con la muñeca y la pinza dadas, probando los planos en orden. */
    return resolver_planos(iterar, objetivo, theta3, theta4, theta1_ini, theta2_ini);
}
