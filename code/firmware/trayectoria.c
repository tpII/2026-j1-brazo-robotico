/*
 * Movimiento seguro de una pose a otra.
 *
 * El camino es la recta en ángulos del modelo, theta(s) = a + s*(b - a) con s de 0 a 1, así todas las
 * articulaciones arrancan y terminan juntas. La duración es la menor que cumple:
 * - ninguna articulación gira más de VEL_MAX_ARTICULACION (°/s);
 * - ningún punto del brazo (p1 a p5 y los centros de las esferas) va más rápido que VEL_MAX_PUNTO (cm/s);
 * - el movimiento arranca y frena con rampas de T_RAMPA segundos.
 * Se toma un paso cada DT, y después se verifica paso por paso que ninguno pase los límites.
 * Implementación J1.
 */
#include "trayectoria.h"
#include "cinematica.h"
#include "colisiones.h"
#include "configuracion.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h> // Para malloc y free
#include <string.h> // Para memcpy

void puntos_vigilados(const double pose[5], Vector3D puntos[14]) {
    /* Los puntos del brazo cuya velocidad se limita: p1 a p5 y los 9 centros de las esferas. */
    Geometria g;
    cinematica_directa(pose, true, &g);
    int n = 0;
    for (int i = 1; i < 6; i++) {
        memcpy(puntos[n++], g.puntos[i], sizeof(Vector3D));
    }
    for (int i = 0; i < 5; i++) {
        for (int e = 0; e < CANTIDAD_ESFERAS[i]; e++) {
            memcpy(puntos[n++], g.esferas[i][e], sizeof(Vector3D));
        }
    }
}

static double fraccion(double t, double duracion) {
    /* Fracción del camino recorrida en el instante t. */
    double velocidad = 1.0 / (duracion - T_RAMPA);
    double aceleracion = velocidad / T_RAMPA;
    if (t < T_RAMPA) {
        return 0.5 * aceleracion * t * t;
    }
    if (t <= duracion - T_RAMPA) {
        return velocidad * (t - T_RAMPA / 2);
    }
    double resto = duracion - t;
    return 1.0 - 0.5 * aceleracion * (resto * resto);
}

static Pose *generar_pasos(const double a[5], const double b[5], int cantidad) {
    /* Los pasos de a a b, uno cada DT. */
    Pose *pasos = (Pose *)malloc(cantidad * sizeof(Pose));
    if (pasos == NULL) {
        return NULL;
    }
    double duracion = cantidad * DT;
    for (int k = 1; k < cantidad; k++) {
        double s = fraccion(k * DT, duracion);
        for (int i = 0; i < 5; i++) {
            pasos[k - 1][i] = a[i] + s * (b[i] - a[i]);
        }
    }
    memcpy(pasos[cantidad - 1], b, sizeof(Pose));
    return pasos;
}

static bool respeta_limites(const double a[5], Pose *pasos, int n_pasos) {
    /* Si ningún paso gira una articulación ni mueve un punto más de lo permitido en DT. */
    double max_angulo = VEL_MAX_ARTICULACION * DT + 1e-9;
    double max_punto = VEL_MAX_PUNTO * DT + 1e-9;
    double anterior[5];
    Vector3D puntos_anteriores[14], puntos[14];
    memcpy(anterior, a, sizeof(anterior));
    puntos_vigilados(a, puntos_anteriores);
    for (int k = 0; k < n_pasos; k++) {
        puntos_vigilados(pasos[k], puntos);
        for (int i = 0; i < 5; i++) {
            if (fabs(pasos[k][i] - anterior[i]) > max_angulo) {
                return false;
            }
        }
        for (int p = 0; p < 14; p++) {
            if (np_linalg_norm(puntos[p], puntos_anteriores[p]) > max_punto) {
                return false;
            }
        }
        memcpy(anterior, pasos[k], sizeof(anterior));
        memcpy(puntos_anteriores, puntos, sizeof(puntos));
    }
    return true;
}

Trayectoria planificar(const double a[5], const double b[5]) {
    /* Pasos de 20 ms de la pose a a la pose b, dentro de los límites de velocidad y con rampas. */
    Trayectoria trayectoria = {NULL, 0, 0.0};
    double giro_maximo = 0.0;
    for (int i = 0; i < 5; i++) {
        giro_maximo = fmax(giro_maximo, fabs(b[i] - a[i]));
    }
    if (giro_maximo < 1e-9) {
        trayectoria.pasos = (Pose *)malloc(sizeof(Pose));
        if (trayectoria.pasos != NULL) {
            memcpy(trayectoria.pasos[0], b, sizeof(Pose));
            trayectoria.n_pasos = 1;
        }
        return trayectoria;
    }

    // Mayor desplazamiento de un punto por unidad de tiempo, con una muestra por grado como mínimo.
    int muestras = (int)ceil(giro_maximo);
    if (muestras < 10) {
        muestras = 10;
    }
    double recorrido = 0.0;
    Vector3D anteriores[14], puntos[14];
    puntos_vigilados(a, anteriores);
    for (int j = 1; j <= muestras; j++) {
        double pose[5];
        for (int i = 0; i < 5; i++) {
            pose[i] = a[i] + ((double)j / muestras) * (b[i] - a[i]);
        }
        puntos_vigilados(pose, puntos);
        double mayor = 0.0;
        for (int p = 0; p < 14; p++) {
            mayor = fmax(mayor, np_linalg_norm(puntos[p], anteriores[p]));
        }
        recorrido = fmax(recorrido, mayor * muestras);
        memcpy(anteriores, puntos, sizeof(puntos));
    }

    double duracion = fmax(2 * T_RAMPA, T_RAMPA + fmax(giro_maximo / VEL_MAX_ARTICULACION,
                                                      recorrido / VEL_MAX_PUNTO));
    int cantidad = (int)ceil(duracion / DT - 1e-9);
    while (true) {
        Pose *pasos = generar_pasos(a, b, cantidad);
        if (pasos == NULL) {
            return trayectoria;
        }
        if (respeta_limites(a, pasos, cantidad)) {
            trayectoria.pasos = pasos;
            trayectoria.n_pasos = cantidad;
            trayectoria.duracion = cantidad * DT;
            return trayectoria;
        }
        free(pasos);
        int mas_lento = (int)ceil(cantidad * 1.05);   // más lento, sin perder el múltiplo de DT
        cantidad = mas_lento > cantidad + 1 ? mas_lento : cantidad + 1;
    }
}

int primer_choque(Pose *pasos, int n_pasos) {
    /* El primer paso que choca. */
    for (int i = 0; i < n_pasos; i++) {
        Geometria g;
        cinematica_directa(pasos[i], true, &g);
        if (detectar_colisiones(&g).hay_choque) {
            return i;
        }
    }
    return -1;
}
