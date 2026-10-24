"""FABRIK v0: el modelo general de la investigación, en 3D sobre p0 a p5 como rótulas libres.

Descartada: la cadena se acomoda como un látigo y las posiciones que da no tienen ángulos de servo que las reproduzcan,
así que no recupera ángulos. 
Implementación J1.
"""
from dataclasses import dataclass

import numpy as np

import cinematica
import configuracion as cfg


@dataclass
class ResultadoV0:
    """Resultado de FABRIK 3D.

    Attributes:
        puntos: arreglo (6, 3) con p0 a p5 al terminar.
        error: distancia de p5 al objetivo (cm).
        iteraciones: iteraciones hechas.
        motivo: por qué cortó: "eps", "eta", "kmax" o "inalcanzable".
    """
    puntos: np.ndarray
    error: float
    iteraciones: int
    motivo: str


def iterar(objetivo, puntos_iniciales):
    """FABRIK 3D con la raíz p0 fija y las distancias de la pose inicial.

    Args:
        objetivo: el punto (x, y, z) en cm.
        puntos_iniciales: p0 a p5 de la pose de partida, arreglo (6, 3).

    Returns:
        El ResultadoV0.
    """
    p = np.array(puntos_iniciales, dtype=float)
    t = np.asarray(objetivo, dtype=float)
    d = [np.linalg.norm(p[i + 1] - p[i]) for i in range(5)]
    raiz = p[0].copy()

    if np.linalg.norm(t - p[0]) > sum(d):   # inalcanzable: cadena estirada hacia el objetivo
        for i in range(5):
            lam = d[i] / np.linalg.norm(t - p[i])
            p[i + 1] = (1 - lam) * p[i] + lam * t
        return ResultadoV0(p, float(np.linalg.norm(p[5] - t)), 0, "inalcanzable")

    error, k, motivo = float(np.linalg.norm(p[5] - t)), 0, "eps"
    while error > cfg.EPSILON and k < cfg.K_MAX:
        k += 1
        anterior = p[5].copy()
        p[5] = t                                    # forward
        for i in range(4, -1, -1):
            lam = d[i] / np.linalg.norm(p[i + 1] - p[i])
            p[i] = (1 - lam) * p[i + 1] + lam * p[i]
        p[0] = raiz                                 # backward
        for i in range(5):
            lam = d[i] / np.linalg.norm(p[i + 1] - p[i])
            p[i + 1] = (1 - lam) * p[i] + lam * p[i + 1]
        error = float(np.linalg.norm(p[5] - t))
        if error <= cfg.EPSILON:
            motivo = "eps"
            break
        if np.linalg.norm(p[5] - anterior) < cfg.ETA:
            motivo = "eta"
            break
    else:
        if error > cfg.EPSILON:
            motivo = "kmax"
    return ResultadoV0(p, error, k, motivo)


def resolver(objetivo, angulos_iniciales):
    """Arma los puntos con la cinemática directa y corre FABRIK 3D.

    Args:
        objetivo: el punto (x, y, z) en cm.
        angulos_iniciales: la pose de partida, en grados del modelo.

    Returns:
        El ResultadoV0.
    """
    return iterar(objetivo, cinematica.cinematica_directa(angulos_iniciales).puntos)
