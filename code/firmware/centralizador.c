/*
 * Módulo central de lógica.
 *
 * Guarda el estado del brazo, atiende los pedidos de objetivo, avanza un paso del movimiento cada
 * vez que se lo llama (cada 20 ms) y arma la telemetría que lee la página del gemelo digital.
 * - Un pedido mientras el brazo se mueve se rechaza con «ocupado»; «Detener» frena el brazo
 *   en el paso en que está.
 * - Un objetivo con valores que no son números finitos, o debajo del piso, se rechaza sin buscar:
 *   p5 no tiene esfera, así que las esferas no lo detectarían.
 * El loop() de firmware.ino lo llama y les manda los pulsos a los servos.
 * Implementación J1.
 */
#include "centralizador.h"
#include "cinematica.h"
#include "colisiones.h"
#include "configuracion.h"
#include "plano.h"
#include "servos.h"
#include <math.h>
#include <stdarg.h> // Para va_list
#include <stdio.h>  // Para vsnprintf
#include <stdlib.h> // Para free
#include <string.h> // Para memcpy y strcmp

// Un JSON que se va escribiendo en un texto de tamaño fijo.
typedef struct {
    char *texto;
    size_t largo;  // el tamaño de texto
    size_t usado;  // lo escrito hasta ahora, o lo que haría falta si no entra
} Json;

static void escribir(Json *j, const char *formato, ...) {
    /* Agrega texto al JSON, como printf. */
    va_list argumentos;
    va_start(argumentos, formato);
    size_t libre = j->usado < j->largo ? j->largo - j->usado : 0;
    int n = vsnprintf(libre > 0 ? j->texto + j->usado : NULL, libre, formato, argumentos);
    va_end(argumentos);
    if (n > 0) {
        j->usado += (size_t)n;
    }
}

static void numero(Json *j, double valor) {
    /* Un número, o null si no es finito: NAN hace de None. */
    if (isfinite(valor)) {
        escribir(j, "%.10g", valor);
    } else {
        escribir(j, "null");
    }
}

static void numeros(Json *j, const double *valores, int n) {
    /* Una lista de números. */
    escribir(j, "[");
    for (int k = 0; k < n; k++) {
        if (k > 0) {
            escribir(j, ", ");
        }
        numero(j, valores[k]);
    }
    escribir(j, "]");
}

static void lista_puntos(Json *j, const Geometria *g) {
    /* Los puntos p0 a p5. */
    escribir(j, "[");
    for (int k = 0; k < 6; k++) {
        if (k > 0) {
            escribir(j, ", ");
        }
        numeros(j, g->puntos[k], 3);
    }
    escribir(j, "]");
}

static void lista_marcos(Json *j, const Geometria *g) {
    /* Los marcos de los cinco eslabones, de tres filas de cuatro números. */
    escribir(j, "[");
    for (int i = 0; i < 5; i++) {
        escribir(j, i > 0 ? ", [" : "[");
        for (int f = 0; f < 3; f++) {
            if (f > 0) {
                escribir(j, ", ");
            }
            numeros(j, g->marcos[i][f], 4);
        }
        escribir(j, "]");
    }
    escribir(j, "]");
}

static void lista_esferas(Json *j, const Geometria *g) {
    /* Los centros de las esferas, una lista por eslabón. */
    escribir(j, "[");
    for (int i = 0; i < 5; i++) {
        escribir(j, i > 0 ? ", [" : "[");
        for (int e = 0; e < CANTIDAD_ESFERAS[i]; e++) {
            if (e > 0) {
                escribir(j, ", ");
            }
            numeros(j, g->esferas[i][e], 3);
        }
        escribir(j, "]");
    }
    escribir(j, "]");
}

static void centros(Json *j, const Geometria *g, const Colision *choque) {
    /* Las coordenadas de las esferas que chocan; sin choque, ninguna. */
    escribir(j, "[");
    for (int k = 0; choque != NULL && k < choque->n_esferas; k++) {
        if (k > 0) {
            escribir(j, ", ");
        }
        numeros(j, g->esferas[choque->esferas[k].eslabon][choque->esferas[k].esfera], 3);
    }
    escribir(j, "]");
}

static void indices(Json *j, const Colision *choque) {
    /* Las esferas que chocan, como pares [eslabón, esfera]; sin choque, ninguna. */
    escribir(j, "[");
    for (int k = 0; choque != NULL && k < choque->n_esferas; k++) {
        escribir(j, "%s[%d, %d]", k > 0 ? ", " : "", choque->esferas[k].eslabon, choque->esferas[k].esfera);
    }
    escribir(j, "]");
}

static void dibujo(Json *j, const double pose[5], const Colision *choque) {
    /* Lo que la página necesita para dibujar una pose, sin las llaves, para poder agregarle claves. */
    Geometria g;
    cinematica_directa(pose, true, &g);
    escribir(j, "\"angles\": ");
    numeros(j, pose, 5);
    escribir(j, ", \"puntos\": ");
    lista_puntos(j, &g);
    escribir(j, ", \"marcos\": ");
    lista_marcos(j, &g);
    escribir(j, ", \"spheres\": ");
    lista_esferas(j, &g);
    escribir(j, ", \"colliding_centers\": ");
    centros(j, &g, choque);
    escribir(j, ", \"esferas_en_choque\": ");
    indices(j, choque);
}

// La cadena estirada de «fuera de alcance» no tiene en cuenta los límites, así que se marcan los
// ángulos que salen del rango. En «bloqueado por límites», FABRIK ya recortó el hombro y el codo en
// cada paso: el que quedó en un tope es el que no deja llegar. Una pose que choca respeta los
// rangos y no marca ninguno.
static void servos_fuera_de_rango(Json *j, const char *estado, const double pose[5]) {
    /* Para cada servo, si el ángulo que hacía falta para llegar queda fuera de su rango. */
    escribir(j, "[");
    for (int i = 0; i < 5; i++) {
        double rango[2];
        rango_modelo(i, rango);
        bool marca = false;
        if (strcmp(estado, "fuera_de_alcance") == 0) {
            marca = !en_rango(pose[i], rango[0], rango[1]);
        } else if (strcmp(estado, "bloqueado_por_limites") == 0) {
            marca = (i == 1 || i == 2) && fmin(fabs(pose[i] - rango[0]), fabs(pose[i] - rango[1])) < 1e-6;
        }
        escribir(j, "%s%s", i > 0 ? ", " : "", marca ? "true" : "false");
    }
    escribir(j, "]");
}

static void metricas(Json *j, const Centralizador *c) {
    /* Los contadores de la última búsqueda para la página, o null. */
    if (!c->hay_resultado) {
        escribir(j, "null");
        return;
    }
    const Resultado *r = &c->resultado;
    escribir(j, "{\"posiciones\": %d, \"ejecuciones\": %d, \"iteraciones\": %d, \"iteraciones_totales\": %d, "
                "\"tiempo_ms\": ",
             r->posiciones, r->ejecuciones, r->iteraciones, r->iteraciones_totales);
    numero(j, round(r->tiempo_ms * 1000) / 1000);
    escribir(j, "}");
}

static double error_cm(const Centralizador *c) {
    /* El error de la última búsqueda en cm, o NAN. */
    return c->hay_resultado ? c->resultado.error : NAN;
}

void iniciar(Centralizador *c) {
    /* El brazo en el home del firmware, como setup(). */
    home_modelo(c->angulos);
    memcpy(c->pulsos, SERVO_HOME_US, sizeof(c->pulsos));
    c->estado = "quieto";
    c->hay_objetivo = false;
    c->hay_resultado = false;
    c->indice = 0;
}

const char *pedir_objetivo(Centralizador *c, double x, double y, double z, char *respuesta, size_t largo) {
    /* Busca una pose para el objetivo y, si llega, empieza a moverse. */
    if (!isfinite(x) || !isfinite(y) || !isfinite(z)) {
        return "Los valores de x, y y z tienen que ser números.";
    }
    if (z < PLANO_PISO_Z) {
        return "El objetivo está debajo del piso (z < 0).";
    }
    Json j = {respuesta, largo, 0};
    if (strcmp(c->estado, "moviendo") == 0) {
        escribir(&j, "{\"resultado\": \"ocupado\", \"error_cm\": null, \"metricas\": null}");
        return NULL;
    }
    c->hay_objetivo = true;
    c->objetivo[0] = x;
    c->objetivo[1] = y;
    c->objetivo[2] = z;
    if (c->hay_resultado) {
        free(c->resultado.pasos);
    }
    c->resultado = buscar(c->objetivo, c->angulos, true);
    c->hay_resultado = true;
    c->indice = 0;
    c->estado = strcmp(c->resultado.estado, "ok") == 0 ? "moviendo" : "quieto";
    escribir(&j, "{\"resultado\": \"%s\", \"error_cm\": ", c->resultado.estado);
    numero(&j, error_cm(c));
    escribir(&j, ", \"metricas\": ");
    metricas(&j, c);
    escribir(&j, "}");
    return NULL;
}

void avanzar(Centralizador *c) {
    /* Un paso del movimiento. Antes de aplicarlo revisa que no choque. */
    if (strcmp(c->estado, "moviendo") != 0) {
        return;
    }
    const double *paso = c->resultado.pasos[c->indice];
    Geometria g;
    cinematica_directa(paso, true, &g);
    Colision choque = detectar_colisiones(&g);
    if (choque.hay_choque) {  // no debería pasar: la búsqueda ya validó el camino
        c->estado = "detenido";
        c->resultado.estado = "colision";
        c->resultado.hay_silueta = true;
        memcpy(c->resultado.silueta.angulos, paso, sizeof(c->resultado.silueta.angulos));
        c->resultado.silueta.error = NAN;
        c->resultado.silueta.colision = choque;
        return;
    }
    memcpy(c->angulos, paso, sizeof(c->angulos));
    pulsos_de(paso, c->pulsos);
    c->indice++;
    if (c->indice == c->resultado.n_pasos) {
        c->estado = "quieto";
    }
}

void detener(Centralizador *c) {
    /* Frena el brazo en el paso en que está. */
    if (strcmp(c->estado, "moviendo") == 0) {
        c->estado = "detenido";
    }
}

size_t telemetria(const Centralizador *c, char *json, size_t largo) {
    /* El JSON de /data, con las claves de J5 y las nuevas. */
    Json j = {json, largo, 0};
    Geometria g;
    cinematica_directa(c->angulos, true, &g);
    Colision choque = detectar_colisiones(&g);
    const Resultado *r = c->hay_resultado ? &c->resultado : NULL;

    escribir(&j, "{\"angles\": ");
    numeros(&j, c->angulos, 5);
    escribir(&j, ", \"collision_detected\": %s, \"colliding_centers\": ", choque.hay_choque ? "true" : "false");
    centros(&j, &g, &choque);
    escribir(&j, ", \"sphere_radius\": ");
    numero(&j, RADIO_ESFERA);
    escribir(&j, ", \"spheres\": ");
    lista_esferas(&j, &g);
    escribir(&j, ", \"puntos\": ");
    lista_puntos(&j, &g);
    escribir(&j, ", \"marcos\": ");
    lista_marcos(&j, &g);
    escribir(&j, ", \"esferas_en_choque\": ");
    indices(&j, &choque);
    escribir(&j, ", \"estado\": \"%s\", \"pulsos_us\": [%d, %d, %d, %d, %d], \"objetivo\": ", c->estado,
             c->pulsos[0], c->pulsos[1], c->pulsos[2], c->pulsos[3], c->pulsos[4]);
    if (c->hay_objetivo) {
        numeros(&j, c->objetivo, 3);
    } else {
        escribir(&j, "null");
    }
    if (r != NULL) {
        escribir(&j, ", \"resultado\": \"%s\"", r->estado);
    } else {
        escribir(&j, ", \"resultado\": null");
    }
    escribir(&j, ", \"error_cm\": ");
    numero(&j, error_cm(c));
    escribir(&j, ", \"metricas\": ");
    metricas(&j, c);
    escribir(&j, ", \"silueta\": ");
    if (r != NULL && r->hay_silueta) {
        escribir(&j, "{");
        dibujo(&j, r->silueta.angulos, &r->silueta.colision);
        escribir(&j, ", \"error_cm\": ");
        numero(&j, r->silueta.error);
        escribir(&j, ", \"servos_fuera_de_rango\": ");
        servos_fuera_de_rango(&j, r->estado, r->silueta.angulos);
        escribir(&j, "}");
    } else {
        escribir(&j, "null");
    }
    escribir(&j, ", \"pose_final\": ");
    if (strcmp(c->estado, "moviendo") == 0) {
        escribir(&j, "{");
        dibujo(&j, c->resultado.pasos[c->resultado.n_pasos - 1], NULL);
        escribir(&j, "}");
    } else {
        escribir(&j, "null");
    }
    escribir(&j, "}");
    return j.usado;
}
