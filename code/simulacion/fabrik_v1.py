"""FABRIK v1: método de bisagras sin restricciones, con chequeo de límites al final.

Descartada: según la pose inicial converge con el codo abajo. Con espejo_final=True es la variante que espeja el codo una
sola vez, al final.
Implementación J1.
"""
import numpy as np

import configuracion as cfg
import plano
from plano import Ejecucion, Paso, ang, u2


def iterar(T, cadena, theta1_ini, theta2_ini, traza=False):
    """Paso 2 sin límites ni regla del codo: forward y backward dentro del plano.

    Args:
        T: el objetivo en el plano, medido desde el hombro (cm).
        cadena: la CadenaVirtual de la muñeca y la pinza elegidas.
        theta1_ini: hombro de partida (grados del modelo).
        theta2_ini: codo de partida (grados del modelo).
        traza: si guarda cada iteración en Ejecucion.traza.

    Returns:
        La plano.Ejecucion, sin recortes.
    """
    l1, l2, alfa = cadena.l1, cadena.l2, cadena.alfa
    E = l1 * u2(theta1_ini)
    P = E + l2 * u2(theta1_ini + theta2_ini + alfa)
    error, k, motivo = float(np.linalg.norm(P - T)), 0, "eps"
    mejor = None
    theta1, rel = theta1_ini, theta2_ini + alfa
    pasos = [Paso(0, None, None, tuple(E), tuple(P), error)] if traza else None

    while error > cfg.EPSILON and k < cfg.K_MAX:
        k += 1
        P_anterior = P.copy()
        # forward
        P = T.copy()
        E = P + l2 * (E - P) / np.linalg.norm(E - P)
        codo_forward = E.copy()
        # backward
        E = l1 * E / np.linalg.norm(E)
        theta1 = ang(E)
        direccion = (P - E) / np.linalg.norm(P - E)
        P = E + l2 * direccion
        rel = ang(direccion) - theta1

        error = float(np.linalg.norm(P - T))
        if mejor is None or error < mejor[0]:
            mejor = (error, theta1, rel - alfa)
        if traza:
            pasos.append(Paso(k, tuple(codo_forward), None, tuple(E), tuple(P), error))
        if error <= cfg.EPSILON:
            motivo = "eps"
            break
        if np.linalg.norm(P - P_anterior) < cfg.ETA:
            motivo = "eta"
            break
    else:
        if error > cfg.EPSILON:
            motivo = "kmax"

    if k == 0:
        mejor = (error, theta1, rel - alfa)
    return Ejecucion(theta1, rel - alfa, error, k, motivo, 0, *mejor, pasos)


def resolver(objetivo, theta3, theta4, theta1_ini, theta2_ini, espejo_final=False):
    """Prueba los planos en orden; los límites se revisan recién en el control final.

    Args:
        objetivo: el punto (x, y, z) en cm.
        theta3: ángulo de la muñeca (grados del modelo).
        theta4: ángulo de la pinza (grados del modelo).
        theta1_ini: hombro de partida (grados del modelo).
        theta2_ini: codo de partida (grados del modelo).
        espejo_final: la variante que espeja el codo una vez, al final.

    Returns:
        La plano.Solucion.
    """
    return plano.resolver(iterar, objetivo, theta3, theta4, theta1_ini, theta2_ini, espejo_final)
