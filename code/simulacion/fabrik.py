"""FABRIK v3: la versión final.

En el plano del brazo, cada iteración hace:
1. forward: p5 sobre el objetivo y el codo a l2 de p5;
2. regla del codo (pole): si el codo quedó del lado prohibido de la recta hombro -> objetivo,
   se lo espeja respecto de esa recta;
3. backward: el codo a l1 del hombro, con el hombro recortado a su rango, y la punta a l2
   del codo, con el giro relativo theta2 + alpha recortado a [-180° + alpha, alpha].
Corta por epsilon (llegó), por eta (p5 dejó de moverse) o por k_max, en ese orden.
En C va a fabrik.c. Implementación J1.
"""
import numpy as np

import configuracion as cfg
import plano
from plano import Ejecucion, Paso, ajustar_angulo, ang, u2

_, (LO_HOMBRO, HI_HOMBRO), (LO_CODO, HI_CODO) = plano.rangos_fabrik()


def iterar(T, cadena, theta1_ini, theta2_ini, traza=False):
    """Paso 2: itera en el plano desde la pose (theta1, theta2) hasta el objetivo T.

    Args:
        T: el objetivo en el plano, medido desde el hombro (cm).
        cadena: la CadenaVirtual de la muñeca y la pinza elegidas.
        theta1_ini: hombro de partida (grados del modelo).
        theta2_ini: codo de partida (grados del modelo).
        traza: si guarda cada iteración en Ejecucion.traza.

    Returns:
        La plano.Ejecucion.
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
        codo_forward, codo_espejado = E.copy(), None
        # regla del codo
        if T[0] * E[1] - T[1] * E[0] < 0:
            u = T / np.linalg.norm(T)
            E = 2 * np.dot(E, u) * u - E
            codo_espejado = E.copy()
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
            pasos.append(Paso(k, tuple(codo_forward), None if codo_espejado is None else tuple(codo_espejado),
                              tuple(E), tuple(P), error))
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
    return Ejecucion(theta1, rel - alfa, error, k, motivo, recortes, *mejor, pasos)


def resolver(objetivo, theta3, theta4, theta1_ini, theta2_ini):
    """Pasos 0 a 3 y control final con la muñeca y la pinza dadas, probando los planos en orden.

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
