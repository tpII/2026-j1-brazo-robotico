# algebra_lineal.py
"""Rotaciones y traslaciones 4x4 en coordenadas homogéneas.

Archivo de 2025 (grupo J5, code/python/modular/algebra_lineal.py). En C equivale a algebra_lineal.c.
"""

import numpy as np
import math

# --- Funciones de Álgebra Lineal ---
def matriz_rotacion_x(angulo_grados):
    """Rotación 4x4 alrededor del eje X.

    Args:
        angulo_grados: ángulo en grados, positivo según la regla de la mano derecha.

    Returns:
        La matriz homogénea 4x4 de la rotación.
    """
    angulo_rad = math.radians(angulo_grados)
    c, s = np.cos(angulo_rad), np.sin(angulo_rad)
    R = np.array([
        [1, 0,  0, 0],
        [0, c, -s, 0],
        [0, s,  c, 0],
        [0, 0,  0, 1]
    ])
    return R

def matriz_rotacion_y(angulo_grados):
    """Rotación 4x4 alrededor del eje Y.

    Args:
        angulo_grados: ángulo en grados, positivo según la regla de la mano derecha.

    Returns:
        La matriz homogénea 4x4 de la rotación.
    """
    angulo_rad = math.radians(angulo_grados)
    c, s = np.cos(angulo_rad), np.sin(angulo_rad)
    R = np.array([
        [ c, 0, s, 0],
        [ 0, 1, 0, 0],
        [-s, 0, c, 0],
        [ 0, 0, 0, 1]
    ])
    return R

def matriz_rotacion_z(angulo_grados):
    """Rotación 4x4 alrededor del eje Z.

    Args:
        angulo_grados: ángulo en grados, positivo según la regla de la mano derecha.

    Returns:
        La matriz homogénea 4x4 de la rotación.
    """
    angulo_rad = math.radians(angulo_grados)
    c, s = np.cos(angulo_rad), np.sin(angulo_rad)
    R = np.array([
        [c, -s, 0, 0],
        [s,  c, 0, 0],
        [0,  0, 1, 0],
        [0,  0, 0, 1]
    ])
    return R

# --> Matriz de traslación 4x4 que traslada un punto
def matriz_traslacion(tx, ty, tz):
    """Genera la matriz de traslación 4x4.

    Args:
        tx: traslación en x (cm).
        ty: traslación en y (cm).
        tz: traslación en z (cm).

    Returns:
        La matriz homogénea 4x4 que le suma (tx, ty, tz) a un punto.
    """
    T = np.identity(4)
    T[0, 3] = tx
    T[1, 3] = ty
    T[2, 3] = tz
    return T

def aplicar_transformada(matriz_transformada, coordenada_local):
    """Aplica la matriz de transformación a una coordenada 3D local.

    Args:
        matriz_transformada: matriz homogénea 4x4 que lleva del marco local al global.
        coordenada_local: el punto (x, y, z) en el marco local.

    Returns:
        El punto (x, y, z) en el marco global.
    """
    # Convertir a coordenadas homogéneas [x, y, z, 1]
    p_local_homogeneo = np.append(coordenada_local, 1) 
    p_global_homogeneo = matriz_transformada @ p_local_homogeneo
    # Devolver las coordenadas 3D globales [x, y, z]
    return p_global_homogeneo[:3]