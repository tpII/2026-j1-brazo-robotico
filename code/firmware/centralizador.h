/*
 * Módulo central de lógica.
 * Implementación J1.
 */
#ifndef CENTRALIZADOR_H
#define CENTRALIZADOR_H

#include "busqueda.h" // Para Resultado
#include "numpy.h"    // Para Vector3D
#include <stdbool.h>
#include <stddef.h>   // Para size_t

#ifdef __cplusplus
extern "C" {
#endif

// El estado del brazo y lo que se puede pedirle.
typedef struct {
    double angulos[5];    // la pose actual, en grados del modelo
    int pulsos[5];        // los pulsos que reciben los servos, en us
    const char *estado;   // "quieto", "moviendo" o "detenido"
    bool hay_objetivo;    // si ya se pidió un objetivo
    Vector3D objetivo;    // el último objetivo (x, y, z) pedido
    bool hay_resultado;   // si ya hubo una búsqueda
    Resultado resultado;  // el Resultado de la última búsqueda; sus pasos son el movimiento
    int indice;           // el paso del movimiento que sigue
} Centralizador;

/**
 * El brazo en el home del firmware, como setup().
 * @param c El Centralizador.
 */
void iniciar(Centralizador *c);

/**
 * Busca una pose para el objetivo y, si llega, empieza a moverse.
 * @param c El Centralizador.
 * @param x Coordenada x del objetivo (cm).
 * @param y Coordenada y del objetivo (cm).
 * @param z Coordenada z del objetivo (cm).
 * @param respuesta (Salida) El JSON de /set: resultado, error_cm y metricas. Si el brazo se está
 *        moviendo, el resultado es "ocupado" y no se busca nada.
 * @param largo El tamaño de respuesta.
 * @return NULL, o el motivo del rechazo si algún valor no es un número finito o si z está debajo
 *         del piso. Entonces no se busca nada ni se escribe la respuesta.
 */
const char *pedir_objetivo(Centralizador *c, double x, double y, double z, char *respuesta, size_t largo);

/**
 * Un paso del movimiento. Antes de aplicarlo revisa que no choque.
 * @param c El Centralizador.
 */
void avanzar(Centralizador *c);

/**
 * Frena el brazo en el paso en que está.
 * @param c El Centralizador.
 */
void detener(Centralizador *c);

/**
 * El JSON de /data, con las claves de J5 y las nuevas.
 * @param c El Centralizador.
 * @param json (Salida) El texto del JSON.
 * @param largo El tamaño de json.
 * @return El largo del JSON. Si no es menor que largo, el JSON quedó cortado.
 */
size_t telemetria(const Centralizador *c, char *json, size_t largo);

#ifdef __cplusplus
}
#endif

#endif // CENTRALIZADOR_H
