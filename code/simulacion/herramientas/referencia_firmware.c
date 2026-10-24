/*
 * Datos de referencia del firmware para los tests de la simulación.
 *
 * Con cinematica.c y configuracion.c del firmware (los de 2025, sin cambios)
 * calcula las esferas globales y el resultado de colisión de un conjunto de
 * poses y escribe un JSON por stdout: la pose cero, el home y 400 poses al azar
 * dentro de los rangos del modelo. Cómo compilarlo y correrlo: README de
 * code/simulacion/.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cinematica.h"
#include "configuracion.h"

#define N_AZAR 400

static const int FILAS[5] = {1, 2, 3, 1, 2};
static const double RANGOS[5][2] = {
    {-180.0, 0.0}, {0.0, 180.0}, {-180.0, 0.0}, {-90.0, 90.0}, {0.0, 180.0}};

static uint32_t estado_azar = 12345;

/* Generador congruencial propio, para que las poses sean las mismas en
 * cualquier máquina. */
static double azar(void) {
  estado_azar =
      (uint32_t)((1103515245ULL * estado_azar + 12345ULL) % 2147483648ULL);
  return estado_azar / 2147483648.0;
}

static void escribir_pose(const double angulos[5], int primera) {
  EsferasGlobales esferas;
  ColisionSet centros;
  centros.coords = NULL;
  centros.count = 0;
  for (int i = 0; i < 5; i++)
    esferas[i] = NULL;

  bool choque = calcular_configuracion_modular(angulos, esferas_local,
                                               RADIO_ESFERA, esferas, &centros);

  printf("%s\n  {\"angulos\": [", primera ? "" : ",");
  for (int i = 0; i < 5; i++)
    printf("%s%.17g", i ? ", " : "", angulos[i]);
  printf("], \"colision\": %s, \"esferas\": [", choque ? "true" : "false");
  for (int i = 0; i < 5; i++) {
    const Vector3D *e = (const Vector3D *)esferas[i];
    printf("%s[", i ? ", " : "");
    for (int j = 0; j < FILAS[i]; j++) {
      printf("%s[%.17g, %.17g, %.17g]", j ? ", " : "", e[j][0], e[j][1],
             e[j][2]);
    }
    printf("]");
  }
  printf("], \"centros\": [");
  for (size_t k = 0; k < centros.count; k++) {
    printf("%s[%.17g, %.17g, %.17g]", k ? ", " : "", centros.coords[k][0],
           centros.coords[k][1], centros.coords[k][2]);
  }
  printf("]}");

  free(centros.coords);
  for (int i = 0; i < 5; i++)
    free((void *)esferas[i]);
}

int main(void) {
  const double fijas[2][5] = {
      {0.0, 0.0, 0.0, 0.0, 0.0},                   /* pose cero */
      {0.0, 140.0, -114.60490463215259, 4.5, 0.0}, /* home del firmware */
  };

  printf("{\"fuente\": \"cinematica.c y configuracion.c de code/firmware "
         "(2025)\", \"poses\": [");
  escribir_pose(fijas[0], 1);
  escribir_pose(fijas[1], 0);
  for (int p = 0; p < N_AZAR; p++) {
    double angulos[5];
    for (int i = 0; i < 5; i++) {
      angulos[i] = RANGOS[i][0] + azar() * (RANGOS[i][1] - RANGOS[i][0]);
    }
    escribir_pose(angulos, 0);
  }
  printf("\n]}\n");
  return 0;
}
