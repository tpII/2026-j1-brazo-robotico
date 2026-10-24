"""FABRIK v2: v1 con los límites del hombro y del codo en cada paso, sin regla del codo.

Descartada: si el codo choca contra su límite por el lado equivocado, la cadena queda trabada y recta. 
Implementación J1.
"""
import numpy as np

import configuracion as cfg
import plano
from plano import Corrida, Paso, ajustar_angulo, ang, u2

_, (LO_HOMBRO, HI_HOMBRO), (LO_CODO, HI_CODO) = plano.rangos_fabrik()


def iterar(T, cadena, theta1_ini, theta2_ini, traza=False):
    """Paso 2 con los límites en cada backward y sin regla del codo.

    Args:
        T: el objetivo en el plano, medido desde el hombro (cm).
        cadena: la CadenaVirtual de la muñeca y la pinza elegidas.
        theta1_ini: hombro de partida (grados del modelo).
        theta2_ini: codo de partida (grados del modelo).
        traza: si guarda cada iteración en Corrida.traza.

    Returns:
        La plano.Corrida.
    """
    l1, l2, alfa = cadena.l1, cadena.l2, cadena.alfa
    E = l1 * u2(theta1_ini)
    P = E + l2 * u2(theta1_ini + theta2_ini + alfa)
    error, k, motivo, recortes = float(np.linalg.norm(P - T)), 0, "eps", 0
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
        # backward, con los límites del hombro y del codo
        E = l1 * E / np.linalg.norm(E)
        theta1, recorte = ajustar_angulo(ang(E), LO_HOMBRO, HI_HOMBRO)
        recortes += recorte
        E = l1 * u2(theta1)
        direccion = (P - E) / np.linalg.norm(P - E)
        rel, recorte = ajustar_angulo(ang(direccion) - theta1, LO_CODO + alfa, HI_CODO + alfa)
        recortes += recorte
        P = E + l2 * u2(theta1 + rel)

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
    return Corrida(theta1, rel - alfa, error, k, motivo, recortes, *mejor, pasos)


def resolver(objetivo, theta3, theta4, theta1_ini, theta2_ini):
    """Prueba los planos en orden con la muñeca y la pinza dadas.

    Args:
        objetivo: el punto (x, y, z) en cm.
        theta3: ángulo de la muñeca (grados del modelo).
        theta4: ángulo de la pinza (grados del modelo).
        theta1_ini: hombro de partida (grados del modelo).
        theta2_ini: codo de partida (grados del modelo).

    Returns:
        La plano.Solucion.
    """
    return plano.resolver(iterar, objetivo, theta3, theta4, theta1_ini, theta2_ini)
