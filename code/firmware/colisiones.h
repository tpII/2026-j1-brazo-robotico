/*
 * Choques entre eslabones y contra el piso, con las reglas de cinematica.c.
 * Implementación J1.
 */
#ifndef COLISIONES_H
#define COLISIONES_H

#include "cinematica.h" // Para Geometria
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Una esfera: el eslabón y su número dentro del eslabón.
typedef struct {
    int eslabon;
    int esfera;
} IndiceEsfera;

// Resultado de revisar una pose. Hay 5 eslabones (10 pares como máximo) y 9 esferas.
typedef struct {
    bool hay_choque;          // si algún par de eslabones choca o alguna esfera toca el piso
    int n_pares;              // cuántos pares chocan
    int pares[10][2];         // los pares (i, j) de eslabones que chocan
    int n_esferas;            // cuántas esferas chocan
    IndiceEsfera esferas[9];  // las esferas que chocan entre eslabones o con el piso, sin repetir
    int n_piso;               // cuántas esferas tocan el piso
    IndiceEsfera piso[9];     // las esferas que tocan el piso
} Colision;

/**
 * Revisa los choques de una pose a partir de los centros globales de sus esferas.
 * @param g La Geometria de la pose, con las esferas calculadas.
 * @return La Colision de la pose.
 */
Colision detectar_colisiones(const Geometria *g);

#ifdef __cplusplus
}
#endif

#endif // COLISIONES_H
