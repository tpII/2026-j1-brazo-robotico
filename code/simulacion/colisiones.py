"""Choques entre eslabones y contra el piso, con las reglas de cinematica.c.

- Se revisan los pares i < j con MATRIZ_COLISIONES[i][j] = 1.
- En un par contiguo (j = i + 1) se excluye la última esfera de i con la primera de j,
  porque comparten la articulación.
- Hay choque si la distancia entre centros es <= 2r + TOL_COLISION.
- El piso se revisa en los eslabones 1 a 4: hay contacto si z <= r + PLANO_PISO_Z.
"""
from dataclasses import dataclass

import numpy as np

import configuracion as cfg


@dataclass
class Colision:
    """Resultado de revisar una pose.

    Attributes:
        hay_choque: si algún par de eslabones choca o alguna esfera toca el piso.
        pares: los pares (i, j) de eslabones que chocan.
        esferas: las esferas (eslabón, esfera) que chocan entre eslabones o con el piso, sin
            repetir.
        piso: las esferas (eslabón, esfera) que tocan el piso.
    """
    hay_choque: bool
    pares: list
    esferas: list
    piso: list


def detectar_colisiones(esferas):
    """Revisa los choques de una pose a partir de los centros globales de sus esferas.

    Args:
        esferas: los 5 arreglos (n_i, 3) de Geometria.esferas.

    Returns:
        La Colision de la pose.
    """
    pares, involucradas, piso = [], [], []
    limite = 2 * cfg.RADIO_ESFERA + cfg.TOL_COLISION

    for i in range(5):
        for j in range(i + 1, 5):
            if cfg.MATRIZ_COLISIONES[i][j] == 0:
                continue
            excluida = (len(esferas[i]) - 1, 0) if j == i + 1 else None
            choca_el_par = False
            for a, centro_a in enumerate(esferas[i]):
                for b, centro_b in enumerate(esferas[j]):
                    if (a, b) != excluida and np.linalg.norm(centro_a - centro_b) <= limite:
                        choca_el_par = True
                        for indice in ((i, a), (j, b)):
                            if indice not in involucradas:
                                involucradas.append(indice)
            if choca_el_par:
                pares.append((i, j))

    for i in range(1, 5):
        for a, centro in enumerate(esferas[i]):
            if centro[2] <= cfg.RADIO_ESFERA + cfg.PLANO_PISO_Z:
                piso.append((i, a))
                if (i, a) not in involucradas:
                    involucradas.append((i, a))

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
