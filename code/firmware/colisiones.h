/*
 * Choques entre eslabones y contra la mesa, con las esferas de configuracion.c.
 *
 * En la simulación es colisiones.py. Implementación J1.
 */
#ifndef COLISIONES_H
#define COLISIONES_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Una esfera: el eslabón y su número dentro del eslabón.
typedef struct {
    int eslabon;
    int esfera;
} IndiceEsfera;

// Resultado de revisar una pose. Hay 5 eslabones (10 pares como máximo) y 17 esferas.
typedef struct {
    bool hay_choque;           // si algún par de eslabones choca o algo toca la mesa
    int n_pares;               // cuántos pares chocan
    int pares[10][2];          // los pares (i, j) de eslabones que chocan
    int n_esferas;             // cuántas esferas chocan
    IndiceEsfera esferas[17];  // las esferas que chocan entre eslabones o con la mesa, sin repetir
    int n_piso;                // cuántas esferas tocan la mesa
    IndiceEsfera piso[17];     // las esferas que tocan la mesa; en la pala, la más cercana a la esquina que la toca
} Colision;

/**
 * Revisa los choques de una pose.
 *
 * Args:
 *     pose: los cinco ángulos del modelo, en grados.
 *
 * Returns:
 *     La Colision de la pose.
 */
Colision detectar_colisiones(const double pose[5]);

#ifdef __cplusplus
}
#endif

#endif // COLISIONES_H
