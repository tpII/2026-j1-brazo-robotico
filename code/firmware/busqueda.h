/*
 * La búsqueda de la muñeca y la pinza.
 * Implementación J1.
 */
#ifndef BUSQUEDA_H
#define BUSQUEDA_H

#include "colisiones.h"  // Para Colision
#include "numpy.h"       // Para Vector3D
#include "trayectoria.h" // Para Pose
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// El mejor intento cuando el brazo no se mueve.
typedef struct {
    double angulos[5];  // la pose que se dibuja, en grados del modelo
    double error;       // su distancia al objetivo (cm), o NAN si no se conoce
    Colision colision;  // la Colision de la pose si choca; si no, sin esferas
} Silueta;

// Lo que devuelve una búsqueda.
typedef struct {
    const char *estado;       // "ok", "fuera_de_alcance", "bloqueado_por_limites" o "colision"
    double angulos[5];        // la pose final si el estado es "ok"; si no, NAN
    double error;             // su distancia al objetivo (cm) si el estado es "ok"; si no, NAN
    bool hay_silueta;         // si hay silueta: cuando el brazo no se mueve
    Silueta silueta;          // la Silueta, si la hay
    int posiciones;           // posiciones de la muñeca y la pinza probadas
    int ejecuciones;          // ejecuciones de FABRIK, una por plano iterado
    int iteraciones;          // iteraciones de la ejecución aceptada
    int iteraciones_totales;  // iteraciones de todas las ejecuciones
    double tiempo_ms;         // lo que tardó la búsqueda
    Pose *pasos;              // la trayectoria validada, cuando se revisan los choques; se libera con free
    int n_pasos;              // cuántos pasos tiene
} Resultado;

/**
 * Busca una pose que llegue al objetivo desde la pose actual, en una sola llamada.
 * @param objetivo El punto (x, y, z) en cm.
 * @param actual La pose donde está el brazo (cinco ángulos del modelo, en grados).
 * @param revisar_colisiones Con false no se revisan choques ni trayectoria; es el modo con el que
 *        se midieron los números del informe.
 * @return El Resultado.
 */
Resultado buscar(const Vector3D objetivo, const double actual[5], bool revisar_colisiones);

#ifdef __cplusplus
}
#endif

#endif // BUSQUEDA_H
