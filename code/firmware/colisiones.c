/*
 * Choques entre eslabones y contra la mesa, con las esferas de configuracion.c.
 *
 * - Se revisan los pares i < j con MATRIZ_COLISIONES_MEDIDA[i][j] = 1: hay choque si la distancia
 *   entre dos centros es <= 2r + TOL_COLISION.
 * - El codo más cerrado que TOPE_CODO cuenta como choque del par (1, 2).
 * - La mesa se revisa con las esferas de los eslabones 1 a 3, que la tocan si z <= r + PLANO_PISO_Z,
 *   y con las 8 esquinas de la caja de la pala, que la tocan si z <= PLANO_PISO_Z. La pala es una
 *   bandeja plana: con sus esferas no podría acostarse cerca de la mesa.
 * Implementación J1.
 */
#include "colisiones.h"
#include "cinematica.h"
#include "configuracion.h"
#include <math.h>
#include <string.h> // Para memcpy

static bool contiene(const IndiceEsfera *lista, int n, IndiceEsfera indice) {
    /* Si la esfera ya está en la lista. */
    for (int k = 0; k < n; k++) {
        if (lista[k].eslabon == indice.eslabon && lista[k].esfera == indice.esfera) {
            return true;
        }
    }
    return false;
}

static void marcar(Colision *choque, int eslabon, int esfera) {
    /* Agrega una esfera a las que chocan, si no estaba. */
    IndiceEsfera indice = {eslabon, esfera};
    if (!contiene(choque->esferas, choque->n_esferas, indice)) {
        choque->esferas[choque->n_esferas++] = indice;
    }
}

static void agregar_par(Colision *choque, int i, int j) {
    /* Agrega un par de eslabones a los que chocan. */
    choque->pares[choque->n_pares][0] = i;
    choque->pares[choque->n_pares][1] = j;
    choque->n_pares++;
}

static void tocar_la_mesa(Colision *choque, int eslabon, int esfera) {
    /* Agrega una esfera a las que tocan la mesa y a las que chocan. */
    IndiceEsfera indice = {eslabon, esfera};
    choque->piso[choque->n_piso++] = indice;
    marcar(choque, eslabon, esfera);
}

Colision detectar_colisiones(const double pose[5]) {
    /* Revisa los choques de una pose. */
    Geometria g;
    cinematica_directa(pose, true, &g);
    Colision choque = {.hay_choque = false, .n_pares = 0, .n_esferas = 0, .n_piso = 0};
    double limite = 2 * RADIO_ESFERA_MEDIDO + TOL_COLISION;

    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            if (MATRIZ_COLISIONES_MEDIDA[i][j] == 0) {
                continue;
            }
            bool choca_el_par = false;
            for (int a = 0; a < CANTIDAD_ESFERAS[i]; a++) {
                for (int b = 0; b < CANTIDAD_ESFERAS[j]; b++) {
                    if (np_linalg_norm(g.esferas[i][a], g.esferas[j][b]) <= limite) {
                        choca_el_par = true;
                        marcar(&choque, i, a);
                        marcar(&choque, j, b);
                    }
                }
            }
            if (choca_el_par) {
                agregar_par(&choque, i, j);
            }
        }
    }

    if (pose[2] < TOPE_CODO) {  // se marcan las esferas del brazo y del antebrazo que están junto al codo
        agregar_par(&choque, 1, 2);
        marcar(&choque, 1, CANTIDAD_ESFERAS[1] - 1);
        marcar(&choque, 2, 0);
    }

    for (int i = 1; i < 4; i++) {
        for (int a = 0; a < CANTIDAD_ESFERAS[i]; a++) {
            if (g.esferas[i][a][2] <= RADIO_ESFERA_MEDIDO + PLANO_PISO_Z) {
                tocar_la_mesa(&choque, i, a);
            }
        }
    }

    // La esquina más baja de la pala, llevada del marco de la pala al global.
    Vector3D baja = {0.0, 0.0, INFINITY};
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            for (int z = 0; z < 2; z++) {
                Vector3D local = {CAJA_PALA[0][x], CAJA_PALA[1][y], CAJA_PALA[2][z]};
                Vector3D esquina;
                for (int f = 0; f < 3; f++) {
                    esquina[f] = g.marcos[4][f][3];
                    for (int c = 0; c < 3; c++) {
                        esquina[f] += g.marcos[4][f][c] * local[c];
                    }
                }
                if (esquina[2] < baja[2]) {
                    memcpy(baja, esquina, sizeof(Vector3D));
                }
            }
        }
    }
    if (baja[2] <= PLANO_PISO_Z) {  // se marca la esfera de la pala más cercana a esa esquina
        int cercana = 0;
        for (int a = 1; a < CANTIDAD_ESFERAS[4]; a++) {
            if (np_linalg_norm(g.esferas[4][a], baja) < np_linalg_norm(g.esferas[4][cercana], baja)) {
                cercana = a;
            }
        }
        tocar_la_mesa(&choque, 4, cercana);
    }

    choque.hay_choque = choque.n_pares > 0 || choque.n_piso > 0;
    return choque;
}
