"""Movimiento seguro de una pose a otra.

El camino es la recta en ángulos del modelo, theta(s) = a + s*(b - a) con s de 0 a 1, así todas las
articulaciones arrancan y terminan juntas. La duración es la menor que cumple:
- ninguna articulación gira más de VEL_MAX_ARTICULACION (°/s);
- ningún punto del brazo (p1 a p5 y los centros de las esferas) va más rápido que VEL_MAX_PUNTO (cm/s);
- el movimiento arranca y frena con rampas de T_RAMPA segundos.
Se toma un paso cada DT, y después se verifica paso por paso que ninguno pase los límites.
En C va a trayectoria.c. Implementación J1.
"""
import math
from dataclasses import dataclass

import numpy as np

import cinematica
import colisiones
import configuracion as cfg


@dataclass
class Trayectoria:
    """Un movimiento planificado.

    Attributes:
        pasos: una pose cada DT, siendo el último paso la pose final.
        duracion: duración del movimiento en segundos.
    """
    pasos: list
    duracion: float


def puntos_vigilados(pose):
    """Los puntos del brazo cuya velocidad se limita: p1 a p5 y los 9 centros de las esferas.

    Args:
        pose: los cinco ángulos del modelo, en grados.

    Returns:
        Un arreglo (14, 3).
    """
    g = cinematica.cinematica_directa(pose)
    return np.vstack([g.puntos[1:], *g.esferas])


def _fraccion(t, duracion):
    """Fracción del camino recorrida en el instante t.

    Args:
        t: instante en segundos, de 0 a duracion.
        duracion: duración total en segundos.

    Returns:
        La fracción s, de 0 a 1.
    """
    velocidad = 1.0 / (duracion - cfg.T_RAMPA)
    aceleracion = velocidad / cfg.T_RAMPA
    if t < cfg.T_RAMPA:
        return 0.5 * aceleracion * t * t
    if t <= duracion - cfg.T_RAMPA:
        return velocidad * (t - cfg.T_RAMPA / 2)
    return 1.0 - 0.5 * aceleracion * (duracion - t) ** 2


def _pasos(a, b, cantidad):
    """Los pasos de a a b, uno cada DT."""
    duracion = cantidad * cfg.DT
    pasos = [tuple(float(v) for v in a + _fraccion(k * cfg.DT, duracion) * (b - a)) for k in range(1, cantidad)]
    return pasos + [tuple(float(v) for v in b)]


def _respeta_limites(a, pasos):
    """Si ningún paso gira una articulación ni mueve un punto más de lo permitido en DT."""
    max_angulo = cfg.VEL_MAX_ARTICULACION * cfg.DT + 1e-9
    max_punto = cfg.VEL_MAX_PUNTO * cfg.DT + 1e-9
    anterior, puntos_anteriores = np.asarray(a), puntos_vigilados(a)
    for paso in pasos:
        puntos = puntos_vigilados(paso)
        if np.abs(np.subtract(paso, anterior)).max() > max_angulo:
            return False
        if np.linalg.norm(puntos - puntos_anteriores, axis=1).max() > max_punto:
            return False
        anterior, puntos_anteriores = np.asarray(paso), puntos
    return True


def planificar(a, b):
    """Pasos de 20 ms de la pose a a la pose b, dentro de los límites de velocidad y con rampas.

    Args:
        a: la pose de partida (cinco ángulos, en grados).
        b: la pose de llegada.

    Returns:
        La Trayectoria. Si a y b son la misma pose, tiene un solo paso y dura 0 seg.
    """
    a, b = np.asarray(a, dtype=float), np.asarray(b, dtype=float)
    giro_maximo = float(np.abs(b - a).max())
    if giro_maximo < 1e-9:
        return Trayectoria([tuple(float(v) for v in b)], 0.0)

    # Mayor desplazamiento de un punto por unidad de tiempo, con una muestra por grado como mínimo.
    muestras = max(10, math.ceil(giro_maximo))
    recorrido, anteriores = 0.0, puntos_vigilados(a)
    for j in range(1, muestras + 1):
        puntos = puntos_vigilados(a + (j / muestras) * (b - a))
        recorrido = max(recorrido, float(np.linalg.norm(puntos - anteriores, axis=1).max()) * muestras)
        anteriores = puntos

    duracion = max(2 * cfg.T_RAMPA, cfg.T_RAMPA + max(giro_maximo / cfg.VEL_MAX_ARTICULACION,
                                                      recorrido / cfg.VEL_MAX_PUNTO))
    cantidad = math.ceil(duracion / cfg.DT - 1e-9)
    while True:
        pasos = _pasos(a, b, cantidad)
        if _respeta_limites(a, pasos):
            return Trayectoria(pasos, cantidad * cfg.DT)
        cantidad = max(cantidad + 1, math.ceil(cantidad * 1.05))   # más lento, sin perder el múltiplo de DT


def primer_choque(pasos):
    """El primer paso que choca.

    Args:
        pasos: las poses del camino.

    Returns:
        El índice del primer paso que choca, o None si el camino está libre.
    """
    for i, paso in enumerate(pasos):
        if colisiones.detectar_colisiones(cinematica.cinematica_directa(paso).esferas).hay_choque:
            return i
    return None
