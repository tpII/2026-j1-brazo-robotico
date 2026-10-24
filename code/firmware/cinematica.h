#ifndef CINEMATICA_H
#define CINEMATICA_H

#include "numpy.h"         // Para Matrix4x4, Vector3D, Vector4D y funciones np_*
#include "algebra_lineal.h" // Para las declaraciones de matriz_rotacion_* y aplicar_transformada
#include <stdbool.h>       // Para usar bool
#include <stddef.h>        // Para size_t

#ifdef __cplusplus
extern "C" {
#endif

// Declaración de placeholders para estructuras de datos complejos (sets y listas)
// En C, usaremos un enfoque basado en listas enlazadas o estructuras de array dinámicas,
// pero aquí solo declaramos un tipo genérico para mantener la estructura lógica.

// Placeholder para un "Set" de coordenadas 3D (para centros colisionantes)
typedef struct {
    size_t count;
    Vector3D *coords; // Array dinámico de tuplas (coordenadas)
} ColisionSet;

// Placeholder para una "Lista" de arrays de esferas (posiciones globales)
// La implementación C sería un array de punteros a arrays 2D.
typedef const void *EsferasGlobales[5]; // Array de punteros a arrays 2D (esf_global_i)


// --- Lógica de Detección de Colisión ---

/**
 * Verifica si la distancia entre dos centros es menor o igual a 2*radio.
 * @param centro_a Vector 3D/4D (coordenada).
 * @param centro_b Vector 3D/4D (coordenada).
 * @param radio Radio de las esferas.
 * @param tol Tolerancia pequeña para la comparación de flotantes.
 * @return bool: True si colisionan.
 */
bool verificar_distancia(const double *centro_a, const double *centro_b, double radio, double tol);

/**
 * Detecta si alguna esfera colisiona con el plano base (Z = plano_z).
 * @param esferas_globales Array de arrays 2D (posiciones globales).
 * @param radio Radio de la esfera.
 * @param plano_z Posición del plano base (por defecto 0.0).
 * @return tuple: (colision_detectada, centros_colisionantes_plano)
 */
void detectar_colision_plano_base(bool *colision_detectada, ColisionSet *centros_colisionantes_plano, 
                                  const EsferasGlobales esferas_globales, double radio, double plano_z);

/**
 * Detecta colisiones entre dos eslabones, ignorando la esfera de la junta si es contiguo.
 * @param esf_a Esferas del eslabón A.
 * @param esf_b Esferas del eslabón B.
 * @param idx_ignore_1 Índice de la esfera a ignorar en esf_a (última, si es contiguo).
 * @param idx_ignore_2 Índice de la esfera a ignorar en esf_b (primera, si es contiguo).
 * @param radio Radio de la esfera.
 * @return ColisionSet: Lista de centros colisionantes (tuplas de coordenadas).
 */
ColisionSet detectar_colision_par(const void *esf_a, const void *esf_b, 
                                  int rows_a, int rows_b, int idx_ignore_1, 
                                  int idx_ignore_2, double radio);

/**
 * Detecta colisiones en toda la configuración del robot.
 * @param esferas_globales Array de arrays 2D (posiciones globales).
 * @param radio Radio de la esfera.
 * @return tuple: (booleano_colision_total, centros_colisionantes_global)
 */
void detectar_colision_total_modular(bool *colision_detectada, ColisionSet *centros_colisionantes_global, 
                                     const EsferasGlobales esferas_globales, double radio);


// --- Función principal: cinemática directa ---

/**
 * Calcula la posición global de las esferas para cada eslabón y chequea colisiones.
 * @param angulos_grados Array de ángulos de rotación (junta).
 * @param esferas_locales Lista de coordenadas locales de las esferas de cada eslabón.
 * @param radio Radio de las esferas.
 * @param esferas_globales (Salida) Array de arrays 2D (posiciones globales).
 * @param centros_colisionantes (Salida) Set de coordenadas 3D de esferas en colisión.
 * @return bool: Colisión total detectada.
 */
bool calcular_configuracion_modular(const double *angulos_grados, 
                                    const void *esferas_locales[5], double radio,
                                    EsferasGlobales esferas_globales,
                                    ColisionSet *centros_colisionantes);


// --- Lógica de Límites Físicos ---

/**
 * Valida que los ángulos estén dentro de los límites físicos.
 * @param angulo Angulo a validar.
 * @param min_angulo Mínimo permitido.
 * @param max_angulo Máximo permitido.
 * @return bool: True si el angulo está fuera de los límites.
 */
bool validador_limites_fisico(double angulo, double min_angulo, double max_angulo);

#ifdef __cplusplus
}
#endif

#endif // CINEMATICA_H