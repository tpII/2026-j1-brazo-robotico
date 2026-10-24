"""Cinemática directa: la misma cadena que calcular_configuracion_modular (cinematica.c).

Para cada eslabón i: M = M_acum * (Rfix_i * Rz(theta_i)) y después M_acum = M * T(SALTO_i).
A diferencia del C, también devuelve los puntos p0 a p5 y los ejes de giro de los servos.
"""
from dataclasses import dataclass

import numpy as np

import configuracion as cfg
from algebra_lineal import (aplicar_transformada, matriz_rotacion_x, matriz_rotacion_y,
                            matriz_rotacion_z, matriz_traslacion)

_ROTACION = {"x": matriz_rotacion_x, "y": matriz_rotacion_y, "z": matriz_rotacion_z}
RFIX = [_ROTACION[eje](grados) for eje, grados in cfg.ROTACIONES_FIJAS]
T_BASE = matriz_traslacion(*cfg.T_BASE_GLOBAL)
T_SALTO = [matriz_traslacion(*salto) for salto in cfg.SALTO_ESLABONES]


@dataclass
class Geometria:
    """Posición del brazo para unos ángulos dados.

    Attributes:
        puntos: arreglo (6, 3) con p0 a p5.
        ejes: arreglo (5, 3) con el eje de giro de cada servo, en coordenadas globales.
        esferas: 5 arreglos (n_i, 3) con los centros globales de las esferas, o None si no se
            calcularon.
        marcos: arreglo (5, 3, 4) con la rotación y el origen de cada eslabón, las tres primeras
            filas de M.
    """
    puntos: np.ndarray
    ejes: np.ndarray
    esferas: list | None
    marcos: np.ndarray


def cinematica_directa(angulos, con_esferas=True):
    """Puntos, ejes, marcos y esferas del brazo para cinco ángulos del modelo.

    Args:
        angulos: los cinco ángulos del modelo, en grados.
        con_esferas: con False no calcula las esferas, para los usos que solo necesitan los puntos
            (la cadena virtual y el control final de FABRIK).

    Returns:
        La Geometria de la pose.
    """
    M_acum = T_BASE
    puntos, ejes, esferas, marcos = [], [], [], []
    for i, angulo in enumerate(angulos):
        M = M_acum @ (RFIX[i] @ matriz_rotacion_z(angulo))
        marcos.append(M[:3])
        puntos.append(M[:3, 3])
        ejes.append(M[:3, 2])   # el giro en z no cambia la tercera columna de M_acum * Rfix
        if con_esferas:
            esferas.append(np.array([aplicar_transformada(M, c) for c in cfg.esferas_local[i]]))
        M_acum = M @ T_SALTO[i]
    puntos.append(M_acum[:3, 3])
    return Geometria(np.array(puntos), np.array(ejes), esferas if con_esferas else None, np.array(marcos))
