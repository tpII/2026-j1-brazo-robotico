/*
 * Choques entre eslabones y contra el piso, con las reglas de cinematica.c.
 *
 * - Se revisan los pares i < j con MATRIZ_COLISIONES[i][j] = 1.
 * - En un par contiguo (j = i + 1) se excluye la última esfera de i con la primera de j,
 *   porque comparten la articulación.
 * - Hay choque si la distancia entre centros es <= 2r + TOL_COLISION.
 * - El piso se revisa en los eslabones 1 a 4: hay contacto si z <= r + PLANO_PISO_Z.
 * Implementación J1.
 */
#include "colisiones.h"
#include "configuracion.h"

static bool contiene(const IndiceEsfera *lista, int n, IndiceEsfera indice) {
    /* Si la esfera ya está en la lista. */
    for (int k = 0; k < n; k++) {
        if (lista[k].eslabon == indice.eslabon && lista[k].esfera == indice.esfera) {
            return true;
        }
    }
    return false;
}

Colision detectar_colisiones(const Geometria *g) {
    /* Revisa los choques de una pose a partir de los centros globales de sus esferas. */
    Colision choque = {.hay_choque = false, .n_pares = 0, .n_esferas = 0, .n_piso = 0};
    double limite = 2 * RADIO_ESFERA + TOL_COLISION;

    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            if (MATRIZ_COLISIONES[i][j] == 0) {
                continue;
            }
            bool choca_el_par = false;
            for (int a = 0; a < CANTIDAD_ESFERAS[i]; a++) {
                for (int b = 0; b < CANTIDAD_ESFERAS[j]; b++) {
                    bool excluida = j == i + 1 && a == CANTIDAD_ESFERAS[i] - 1 && b == 0;
                    if (!excluida && np_linalg_norm(g->esferas[i][a], g->esferas[j][b]) <= limite) {
                        choca_el_par = true;
                        IndiceEsfera indices[2] = {{i, a}, {j, b}};
                        for (int k = 0; k < 2; k++) {
                            if (!contiene(choque.esferas, choque.n_esferas, indices[k])) {
                                choque.esferas[choque.n_esferas++] = indices[k];
                            }
                        }
                    }
                }
            }
            if (choca_el_par) {
                choque.pares[choque.n_pares][0] = i;
                choque.pares[choque.n_pares][1] = j;
                choque.n_pares++;
            }
        }
    }

    for (int i = 1; i < 5; i++) {
        for (int a = 0; a < CANTIDAD_ESFERAS[i]; a++) {
            if (g->esferas[i][a][2] <= RADIO_ESFERA + PLANO_PISO_Z) {
                IndiceEsfera indice = {i, a};
                choque.piso[choque.n_piso++] = indice;
                if (!contiene(choque.esferas, choque.n_esferas, indice)) {
                    choque.esferas[choque.n_esferas++] = indice;
                }
            }
        }
    }

    choque.hay_choque = choque.n_pares > 0 || choque.n_piso > 0;
    return choque;
}
