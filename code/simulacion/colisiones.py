"""Choques entre eslabones y contra la mesa, con las esferas de configuracion.py.

- Se revisan los pares i < j con MATRIZ_COLISIONES[i][j] = 1: hay choque si la distancia entre
  dos centros es <= 2r + TOL_COLISION.
- El codo más cerrado que TOPE_CODO cuenta como choque del par (1, 2).
- La mesa se revisa con las esferas de los eslabones 1 a 3, que la tocan si z <= r + PLANO_PISO_Z,
  y con las 8 esquinas de la caja de la pala, que la tocan si z <= PLANO_PISO_Z. La pala es una
  bandeja plana: con sus esferas no podría acostarse cerca de la mesa.
En C va a colisiones.c. Implementación J1.
"""
from dataclasses import dataclass

import numpy as np

import cinematica
import configuracion as cfg


@dataclass
class Colision:
    """Resultado de revisar una pose.

    Attributes:
        hay_choque: si algún par de eslabones choca o algo toca la mesa.
        pares: los pares (i, j) de eslabones que chocan.
        esferas: las esferas (eslabón, esfera) que chocan entre eslabones o con la mesa, sin
            repetir.
        piso: las esferas (eslabón, esfera) que tocan la mesa; en la pala, la más cercana a la
            esquina que la toca.
    """
    hay_choque: bool
    pares: list
    esferas: list
    piso: list


def detectar_colisiones(pose):
    """Revisa los choques de una pose.

    Args:
        pose: los cinco ángulos del modelo, en grados.

    Returns:
        La Colision de la pose.
    """
    g = cinematica.cinematica_directa(pose)
    esferas = g.esferas
    pares, involucradas, piso = [], [], []
    limite = 2 * cfg.RADIO_ESFERA + cfg.TOL_COLISION

    def marcar(indice):
        """Agrega una esfera a las que chocan, si no estaba."""
        if indice not in involucradas:
            involucradas.append(indice)

    for i in range(5):
        for j in range(i + 1, 5):
            if cfg.MATRIZ_COLISIONES[i][j] == 0:
                continue
            choca_el_par = False
            for a, centro_a in enumerate(esferas[i]):
                for b, centro_b in enumerate(esferas[j]):
                    if np.linalg.norm(centro_a - centro_b) <= limite:
                        choca_el_par = True
                        marcar((i, a))
                        marcar((j, b))
            if choca_el_par:
                pares.append((i, j))

    if pose[2] < cfg.TOPE_CODO:   # se marcan las esferas del brazo y del antebrazo que están junto al codo
        pares.append((1, 2))
        marcar((1, len(esferas[1]) - 1))
        marcar((2, 0))

    for i in (1, 2, 3):
        for a, centro in enumerate(esferas[i]):
            if centro[2] <= cfg.RADIO_ESFERA + cfg.PLANO_PISO_Z:
                piso.append((i, a))
                marcar((i, a))

    M = g.marcos[4]
    esquinas = cfg.ESQUINAS_PALA @ M[:, :3].T + M[:, 3]
    baja = esquinas[np.argmin(esquinas[:, 2])]
    if baja[2] <= cfg.PLANO_PISO_Z:   # se marca la esfera de la pala más cercana a esa esquina
        a = int(np.argmin(np.linalg.norm(esferas[4] - baja, axis=1)))
        piso.append((4, a))
        marcar((4, a))

    return Colision(bool(pares or piso), pares, involucradas, piso)


def centros(esferas, indices):
    """Coordenadas de algunas esferas, como listas, para la telemetría.

    Args:
        esferas: los 5 arreglos (n_i, 3) de Geometria.esferas.
        indices: pares (eslabón, esfera).

    Returns:
        Una lista [x, y, z] por cada índice.
    """
    return [[float(v) for v in esferas[i][a]] for i, a in indices]
