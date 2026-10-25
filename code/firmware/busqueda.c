/*
 * La búsqueda de la muñeca y la pinza.
 *
 * El usuario solo da el punto. FABRIK v3 calcula la base, el hombro y el codo, y la búsqueda elige
 * la muñeca y la pinza: primero la posición neutra, después las de la pinza con la muñeca neutra y
 * por último cada posición de la muñeca con todas las de la pinza, de a PASO_BUSQUEDA grados, de la
 * más cercana a la neutra a la más lejana. Se queda con la primera posición que llega.
 *
 * Si una posición tiene la misma cadena virtual que otra que ya falló por alcance o por límites,
 * también falla y se saltea sin contarla. Pasa con la muñeca girada y la pinza alineada: p5, la punta
 * de la pinza, queda sobre el eje de la muñeca y girarla no lo mueve.
 *
 * Con los choques, una pose válida se acepta solo si ni ella ni el camino desde
 * la pose actual chocan. Si choca, se descarta igual que una que no llega: se prueba el otro plano y
 * después la siguiente posición. Una posición con la misma cadena que otra que llegó pero chocaba
 * reusa sus ángulos con su propia muñeca y su propia pinza.
 * Implementación J1.
 */
#include "busqueda.h"
#include "configuracion.h"
#include "fabrik.h"
#include "plano.h"
#include "servos.h"
#include <math.h>
#include <stdlib.h> // Para realloc y free
#include <string.h> // Para memcpy
#include <time.h>   // Para clock

// Cuántos valores de la muñeca o de la pinza se prueban como máximo: con pasos de 10 grados, un
// rango de 360 da 37.
#define MAX_VALORES 37

// Una cadena que llegó pero chocaba, con sus poses válidas: una por plano, dos como máximo.
typedef struct {
    CadenaVirtual cadena;
    int n;                 // cuántas poses válidas hay
    double angulos[2][5];  // las poses
    double error[2];       // el error de cada una (cm)
    int iteraciones[2];    // las iteraciones de cada una
} Chocada;

// Lo que lleva la búsqueda mientras prueba las posiciones.
typedef struct {
    const double *objetivo;   // el punto (x, y, z) en cm
    const double *actual;     // la pose donde está el brazo
    bool revisar_colisiones;
    Resultado resultado;      // con los contadores hasta ahora
    CadenaVirtual *fallidas;  // las cadenas que no llegaron, por alcance o por límites
    int n_fallidas;
    Chocada *chocadas;        // las cadenas que llegaron pero chocaban
    int n_chocadas;
    bool hay_mejor;           // si ya se iteró en algún plano
    double mejor_error;       // el error de la iteración de menor error de todas
    double mejor_pose[5];     // la pose de esa iteración
    bool paso0;               // si alguna posición pasó el Paso 0 en algún plano
    bool hay_silueta;         // si ya hubo una pose válida que chocaba
    Silueta primera_silueta;  // la primera pose válida que chocaba (o el primer paso que chocaba)
} Busqueda;

static int alrededor(double neutra, const double rango[2], double valores[], int maximo) {
    /* Los valores alrededor de la posición neutra, de a PASO_BUSQUEDA grados. */
    double lo = rango[0], hi = rango[1];
    int n = 0;
    for (int k = 1; neutra + k * PASO_BUSQUEDA <= hi + 1e-9 || neutra - k * PASO_BUSQUEDA >= lo - 1e-9; k++) {
        double candidatos[2] = {neutra + k * PASO_BUSQUEDA, neutra - k * PASO_BUSQUEDA};
        for (int c = 0; c < 2; c++) {
            if (lo - 1e-9 <= candidatos[c] && candidatos[c] <= hi + 1e-9 && n < maximo) {
                valores[n++] = candidatos[c];
            }
        }
    }
    return n;
}

static bool misma_cadena(CadenaVirtual a, CadenaVirtual b) {
    /* Si dos cadenas virtuales tienen el mismo l2, alpha y d. */
    return fabs(a.l2 - b.l2) < 1e-9 && fabs(a.alfa - b.alfa) < 1e-9 && fabs(a.d - b.d) < 1e-9;
}

static Colision colision_de(const double pose[5]) {
    /* Los choques de una pose. */
    Geometria g;
    cinematica_directa(pose, true, &g);
    return detectar_colisiones(&g);
}

static bool revisar(const double pose[5], const double actual[5], Silueta *silueta, Trayectoria *trayectoria) {
    /* Revisa los choques de una pose y del camino hasta ella; devuelve true si ninguno choca. */
    Colision choque = colision_de(pose);
    if (choque.hay_choque) {
        memcpy(silueta->angulos, pose, sizeof(silueta->angulos));
        silueta->error = NAN;
        silueta->colision = choque;
        return false;
    }
    *trayectoria = planificar(actual, pose);
    int i = primer_choque(trayectoria->pasos, trayectoria->n_pasos);
    if (i < 0 && trayectoria->n_pasos > 0) {
        return true;
    }
    // La silueta es el paso que choca, o la pose si no hubo memoria para los pasos.
    memcpy(silueta->angulos, i < 0 ? pose : trayectoria->pasos[i], sizeof(silueta->angulos));
    silueta->error = NAN;
    silueta->colision = colision_de(silueta->angulos);
    free(trayectoria->pasos);
    return false;
}

static bool aceptar(Busqueda *b, const double pose[5]) {
    /* Revisa los choques de una pose válida; si se acepta, deja su trayectoria en el resultado. */
    Silueta silueta;
    Trayectoria trayectoria;
    if (revisar(pose, b->actual, &silueta, &trayectoria)) {
        b->resultado.pasos = trayectoria.pasos;
        b->resultado.n_pasos = trayectoria.n_pasos;
        return true;
    }
    if (!b->hay_silueta) {
        b->hay_silueta = true;
        b->primera_silueta = silueta;
    }
    return false;
}

static void llegar(Resultado *r, const double pose[5], double error, int iteraciones) {
    /* Anota la pose que llega. */
    r->estado = "ok";
    memcpy(r->angulos, pose, sizeof(r->angulos));
    r->error = error;
    r->iteraciones = iteraciones;
}

static bool probar_posicion(Busqueda *b, double theta3, double theta4) {
    /* Prueba una posición de la muñeca y la pinza; devuelve true si llega sin chocar. */
    CadenaVirtual cadena = cadena_virtual(theta3, theta4);
    for (int k = 0; k < b->n_fallidas; k++) {
        if (misma_cadena(cadena, b->fallidas[k])) {
            return false;
        }
    }
    b->resultado.posiciones++;
    for (int k = 0; k < b->n_chocadas; k++) {
        Chocada *anterior = &b->chocadas[k];
        if (!misma_cadena(cadena, anterior->cadena)) {
            continue;
        }
        // Misma cadena que una que llegó pero chocaba: mismos ángulos, otro theta4.
        for (int v = 0; v < anterior->n; v++) {
            double pose[5] = {anterior->angulos[v][0], anterior->angulos[v][1], anterior->angulos[v][2], theta3,
                              theta4};
            if (aceptar(b, pose)) {
                llegar(&b->resultado, pose, anterior->error[v], anterior->iteraciones[v]);
                return true;
            }
        }
        return false;
    }

    Chocada validas = {.cadena = cadena, .n = 0};
    double candidatos[2][2];
    int n = planos_candidatos(b->objetivo, cadena.d, candidatos);
    for (int i = 0; i < n; i++) {
        double theta0 = candidatos[i][0], r_c = candidatos[i][1];
        SolucionPlano solucion = resolver_plano(iterar, b->objetivo, theta0, r_c, cadena, theta3, theta4,
                                                b->actual[1], b->actual[2]);
        if (!solucion.alcanzable) {
            continue;
        }
        b->paso0 = true;
        Ejecucion ejecucion = solucion.ejecucion;
        b->resultado.ejecuciones++;
        b->resultado.iteraciones_totales += ejecucion.iteraciones;
        if (!b->hay_mejor || ejecucion.mejor_error < b->mejor_error) {
            double pose[5] = {theta0, ejecucion.mejor_theta1, ejecucion.mejor_theta2, theta3, theta4};
            b->hay_mejor = true;
            b->mejor_error = ejecucion.mejor_error;
            memcpy(b->mejor_pose, pose, sizeof(pose));
        }
        if (!solucion.valida) {
            continue;
        }
        if (!b->revisar_colisiones || aceptar(b, solucion.angulos)) {
            llegar(&b->resultado, solucion.angulos, solucion.error, ejecucion.iteraciones);
            return true;
        }
        memcpy(validas.angulos[validas.n], solucion.angulos, sizeof(solucion.angulos));
        validas.error[validas.n] = solucion.error;
        validas.iteraciones[validas.n] = ejecucion.iteraciones;
        validas.n++;
    }

    // Si falta memoria para anotarla, la cadena se vuelve a probar: tarda más, pero da lo mismo.
    if (validas.n > 0) {
        Chocada *chocadas = (Chocada *)realloc(b->chocadas, (b->n_chocadas + 1) * sizeof(Chocada));
        if (chocadas != NULL) {
            b->chocadas = chocadas;
            b->chocadas[b->n_chocadas++] = validas;
        }
    } else {
        CadenaVirtual *fallidas = (CadenaVirtual *)realloc(b->fallidas, (b->n_fallidas + 1) * sizeof(CadenaVirtual));
        if (fallidas != NULL) {
            b->fallidas = fallidas;
            b->fallidas[b->n_fallidas++] = cadena;
        }
    }
    return false;
}

Resultado buscar(const Vector3D objetivo, const double actual[5], bool revisar_colisiones) {
    /* Busca una pose que llegue al objetivo desde la pose actual, en una sola llamada. */
    clock_t comienzo = clock();
    Busqueda b = {.objetivo = objetivo, .actual = actual, .revisar_colisiones = revisar_colisiones,
                  .resultado = {.angulos = {NAN, NAN, NAN, NAN, NAN}, .error = NAN}};

    // Cada muñeca, empezando por la neutra, va con todas las pinzas, empezando por la neutra.
    double munecas[MAX_VALORES], pinzas[MAX_VALORES], rango[2];
    rango_modelo(3, rango);
    munecas[0] = MUNECA_NEUTRA;
    int n_munecas = 1 + alrededor(MUNECA_NEUTRA, rango, munecas + 1, MAX_VALORES - 1);
    rango_modelo(4, rango);
    pinzas[0] = PINZA_NEUTRA;
    int n_pinzas = 1 + alrededor(PINZA_NEUTRA, rango, pinzas + 1, MAX_VALORES - 1);
    bool llega = false;
    for (int m = 0; m < n_munecas && !llega; m++) {
        for (int p = 0; p < n_pinzas && !llega; p++) {
            llega = probar_posicion(&b, munecas[m], pinzas[p]);
        }
    }

    Resultado *r = &b.resultado;
    if (!llega) {
        r->hay_silueta = true;
        if (b.hay_silueta) {
            r->estado = "colision";
            r->silueta = b.primera_silueta;
        } else if (!b.paso0) {
            CadenaVirtual neutra = cadena_virtual(MUNECA_NEUTRA, PINZA_NEUTRA);
            double estirada[3];
            cadena_estirada(objetivo, neutra, estirada);
            Silueta silueta = {.angulos = {estirada[0], estirada[1], estirada[2], MUNECA_NEUTRA, PINZA_NEUTRA},
                               .error = NAN};
            r->estado = "fuera_de_alcance";
            r->silueta = silueta;
        } else {
            Silueta silueta = {.error = b.mejor_error};
            memcpy(silueta.angulos, b.mejor_pose, sizeof(silueta.angulos));
            r->estado = "bloqueado_por_limites";
            r->silueta = silueta;
        }
    }
    free(b.fallidas);
    free(b.chocadas);
    r->tiempo_ms = 1000.0 * (double)(clock() - comienzo) / CLOCKS_PER_SEC;
    return *r;
}
