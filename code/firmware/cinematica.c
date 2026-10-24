#include "cinematica.h"
#include "configuracion.h" 
#include "algebra_lineal.h"
#include "numpy.h"
#include <stdio.h>
#include <stdlib.h> // Para malloc, free
#include <string.h> // Para memcpy


// --- Lógica de Detección de Colisión (Implementación de Placeholders) ---

// Placeholder para np.linalg.norm(centro_a - centro_b)
double np_linalg_norm(const double *a, const double *b) {
    // Asume vectores 3D (índices 0, 1, 2)
    double dx = a[0] - b[0];
    double dy = a[1] - b[1];
    double dz = a[2] - b[2];
    return sqrt(dx * dx + dy * dy + dz * dz);
}

// Placeholder para funciones de ColisionSet (Set de Tuplas)
void ColisionSet_add(ColisionSet *set, const double *centro) {
    // realloc es seguro si set->coords es NULL inicialmente.
    set->coords = (Vector3D *)realloc(set->coords, (set->count + 1) * sizeof(Vector3D));
    if (set->coords == NULL) return; // Error de memoria
    memcpy(set->coords[set->count], centro, sizeof(Vector3D));
    set->count++;
}

// Simula set1.union(set2)
void ColisionSet_union(ColisionSet *result, const ColisionSet *set1, const ColisionSet *set2) {
    // Nota: 'result' DEBE tener result->coords = NULL y result->count = 0 antes de esta llamada.
    for (size_t k = 0; k < set1->count; k++) {
        ColisionSet_add(result, set1->coords[k]);
    }
    for (size_t k = 0; k < set2->count; k++) {
        ColisionSet_add(result, set2->coords[k]);
    }
}


// --- Lógica de Detección de Colisión ---

bool verificar_distancia(const double *centro_a, const double *centro_b, double radio, double tol) {
    double distancia = np_linalg_norm(centro_a, centro_b);
    return distancia <= 2.0 * radio + tol;
}

void detectar_colision_plano_base(bool *colision_detectada, ColisionSet *centros_colisionantes_plano, 
                                  const EsferasGlobales esferas_globales, double radio, double plano_z) {
    /* Detecta si alguna esfera colisiona con el plano base (Z = plano_z). */
    *colision_detectada = false;
    
    // CORRECCIÓN 1: Eliminar liberación insegura.
    // La inicialización/limpieza de centros_colisionantes_plano (free, count=0) 
    // debe hacerse en el llamador (calcular_configuracion_modular).

    for (int i = 1; i < 5; i++) { // Itera sobre los eslabones, comenzando desde el índice 1
        
        const Vector3D *esf_eslabon = (const Vector3D *)esferas_globales[i];
        
        int num_esferas_en_eslabon;
        if (i == 1) num_esferas_en_eslabon = 2; 
        else if (i == 2) num_esferas_en_eslabon = 3; 
        else if (i == 3) num_esferas_en_eslabon = 1; 
        else if (i == 4) num_esferas_en_eslabon = 2; 
        else num_esferas_en_eslabon = 0; 

        for (int j = 0; j < num_esferas_en_eslabon; j++) {
            const double *centro = esf_eslabon[j]; 
            double z_centro = centro[2];
            
            if (z_centro <= radio + plano_z) { 
                *colision_detectada = true;
                ColisionSet_add(centros_colisionantes_plano, centro);
            }
        }
    }
}

ColisionSet detectar_colision_par(const void *esf_a_ptr, const void *esf_b_ptr, 
                                  int rows_a, int rows_b, int idx_ignore_1, 
                                  int idx_ignore_2, double radio) {
    /* Detecta colisiones entre dos eslabones, ignorando la esfera de la junta si es contiguo. */
    ColisionSet centros_colisionantes;
    // CORRECCIÓN 2: Inicialización local crucial para realloc.
    centros_colisionantes.count = 0;
    centros_colisionantes.coords = NULL; 

    const Vector3D *esf_a = (const Vector3D *)esf_a_ptr;
    const Vector3D *esf_b = (const Vector3D *)esf_b_ptr;
    
    double tol = 1e-9;
    
    // ... (Lógica de detección y ColisionSet_add permanece igual) ...
    for (int i = 0; i < rows_a; i++) {
        const double *centro_a = esf_a[i];
        for (int j = 0; j < rows_b; j++) {
            const double *centro_b = esf_b[j];
            
            if (i == idx_ignore_1 && j == idx_ignore_2) {
                continue;
            }
            
            if (verificar_distancia(centro_a, centro_b, radio, tol)) {
                // ... (impresiones) ...
                ColisionSet_add(&centros_colisionantes, centro_a);
                ColisionSet_add(&centros_colisionantes, centro_b);
            }
        }
    }
    
    return centros_colisionantes;
}

void detectar_colision_total_modular(bool *colision_detectada, ColisionSet *centros_colisionantes_global, 
                                     const EsferasGlobales esferas_globales, double radio) {
    /* Detecta colisiones en toda la configuración del robot. */
    int num_eslabones = 5; 
    
    // CORRECCIÓN 3: Eliminar liberación insegura.
    // La inicialización/limpieza es responsabilidad del llamador (calcular_configuracion_modular).
    centros_colisionantes_global->count = 0; 
    
    for (int i = 0; i < num_eslabones; i++) {
        for (int j = i + 1; j < num_eslabones; j++) {
            
            if (MATRIZ_COLISIONES[i][j] == 0) {
                //printf("Omitiendo chequeo entre Eslabón %d y Eslabón %d (MATRIZ_COLISIONES[i, j] = 0)\n", i, j);
                continue;
            }
            
            // ... (Cálculo de rows_i, rows_j, idx_ignore_1, idx_ignore_2) ...
            
            const void *esf_i_ptr = esferas_globales[i];
            const void *esf_j_ptr = esferas_globales[j];
            int rows_i, rows_j;
            if (i == 0) rows_i = 1; else if (i == 1) rows_i = 2; else if (i == 2) rows_i = 3; else if (i == 3) rows_i = 1; else rows_i = 2; 
            if (j == 0) rows_j = 1; else if (j == 1) rows_j = 2; else if (j == 2) rows_j = 3; else if (j == 3) rows_j = 1; else rows_j = 2; 
            int idx_ignore_1, idx_ignore_2;
            if (j == i + 1) {
                idx_ignore_1 = rows_i - 1;
                idx_ignore_2 = 0;
            } else {
                idx_ignore_1 = -1;
                idx_ignore_2 = -1;
            }
            
            //printf("Verificando colisión entre Eslabón A=%d y Eslabón B=%d...\n", i + 1, j + 1);
            ColisionSet centros_colisionantes_par = detectar_colision_par(esf_i_ptr, esf_j_ptr, rows_i, rows_j, idx_ignore_1, idx_ignore_2, radio);

            if (centros_colisionantes_par.count > 0) {
                for (size_t k = 0; k < centros_colisionantes_par.count; k++) {
                    ColisionSet_add(centros_colisionantes_global, centros_colisionantes_par.coords[k]);
                }
                // Liberar memoria temporal del set parcial (CORRECTO)
                free(centros_colisionantes_par.coords);
            }
        }
    }
    
    *colision_detectada = centros_colisionantes_global->count > 0;
}


// --- Función principal: cinemática directa ---

bool calcular_configuracion_modular(const double *angulos_grados, 
                                    const void *esferas_locales[5], double radio,
                                    EsferasGlobales esferas_globales,
                                    ColisionSet *centros_colisionantes) {
    /* Calcula la posición global de las esferas para cada eslabón. */
    
    // ... (Cálculos de M_acum y Rfix_storage permanecen iguales) ...

    Matrix4x4 Rfix_storage[5];
    np_eye(Rfix_storage[0], 4); 
    matriz_rotacion_x(Rfix_storage[1], 90.0); 
    np_eye(Rfix_storage[2], 4); 
    matriz_rotacion_y(Rfix_storage[3], -90.0); 
    matriz_rotacion_y(Rfix_storage[4], 90.0); 
    Matrix4x4 M_acum;
    matriz_traslacion(M_acum, T_BASE_GLOBAL[0], T_BASE_GLOBAL[1], T_BASE_GLOBAL[2]);
    Matrix4x4 M_eslabon, R_junta, M_temp; 

    // Iteramos sobre todos los eslabones
    for (int i = 0; i < 5; i++) { 
        // ... (cálculo de M_eslabon) ...
        // ... (asignación de esf_global_i usando malloc y esferas_globales[i] = esf_global_i) ...
        
        double angulo = angulos_grados[i];
        matriz_rotacion_z(R_junta, angulo);
        np_matmul_4x4_4x4(M_temp, Rfix_storage[i], R_junta);
        np_matmul_4x4_4x4(M_eslabon, M_acum, M_temp);

        const Vector3D *esferas_locales_i = (const Vector3D *)esferas_locales[i];
        int rows_i;
        if (i == 0) rows_i = 1; else if (i == 1) rows_i = 2; else if (i == 2) rows_i = 3; else if (i == 3) rows_i = 1; else rows_i = 2;

        Vector3D *esf_global_i = (Vector3D *)malloc(rows_i * sizeof(Vector3D));
        if (esf_global_i == NULL) return false; 
        
        for (int j = 0; j < rows_i; j++) {
            aplicar_transformada(esf_global_i[j], M_eslabon, esferas_locales_i[j]);
        }
        esferas_globales[i] = esf_global_i;

        // ... (actualización de M_acum) ...
        const double *vector_desplazamiento = *SALTO_ESLABONES[i];
        Matrix4x4 T_salto;
        matriz_traslacion(T_salto, vector_desplazamiento[0], vector_desplazamiento[1], vector_desplazamiento[2]);
        np_matmul_4x4_4x4(M_acum, M_eslabon, T_salto);
    }

    // 1. Detección de Colisión entre Eslabones
    bool colision_interna;
    ColisionSet centros_colisionantes_internos;
    centros_colisionantes_internos.coords = NULL; // CORRECCIÓN 4: Inicializar a NULL
    centros_colisionantes_internos.count = 0;

    detectar_colision_total_modular(&colision_interna, &centros_colisionantes_internos, 
        esferas_globales, radio);
    
    // 2. Detección de Colisión contra el Plano Base
    bool colision_plano;
    ColisionSet centros_colisionantes_plano;
    centros_colisionantes_plano.coords = NULL; // CORRECCIÓN 5: Inicializar a NULL
    centros_colisionantes_plano.count = 0;

    detectar_colision_plano_base(&colision_plano, &centros_colisionantes_plano, 
        esferas_globales, radio, 0.0); 

    // 3. Combinación de Resultados
    
    // NOTA: 'centros_colisionantes' de salida YA FUE INICIALIZADO A NULL en main.ino
    ColisionSet_union(centros_colisionantes, &centros_colisionantes_internos, &centros_colisionantes_plano);
    
    // Liberar memoria temporal de los sets parciales (Ahora son seguros)
    if (centros_colisionantes_internos.coords != NULL) free(centros_colisionantes_internos.coords);
    if (centros_colisionantes_plano.coords != NULL) free(centros_colisionantes_plano.coords);

    bool colision_total = colision_interna || colision_plano;

    return colision_total;
}


// --- Lógica de Límites Físicos (Permanece igual) ---

bool validador_limites_fisico(double angulo, double min_angulo, double max_angulo) {
    /* Valida que los ángulos estén dentro de los límites físicos. */
    if (min_angulo == -1.0 && max_angulo == -1.0) {
        return false; 
    } else if (angulo < min_angulo || angulo > max_angulo) {
        printf("Ángulo fuera de límites físicos: %f° (Límites: %f° - %f°)\n", angulo, min_angulo, max_angulo);
        return true;
    }
    return false;
}