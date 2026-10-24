"""Las fronteras de la regla del codo y el espejo módulo 360°.

- La frontera de la pinza: con theta4 por debajo de unos 83.3° (la pinza inclinada hacia abajo),
  alpha < 0 y la segunda frontera de la regla (theta2 = -180° - alpha) entra en el rango del codo.
  Se busca el theta4 donde alpha = 0, con la muñeca neutra.
- s = T_r*E_h - T_h*E_r vale l1*l2*sin(-(theta2 + alpha)) cuando el objetivo está en la punta.
- Dos casos del lado prohibido que el espejo lleva a otra configuración válida que llega al
  mismo punto: theta2' = -theta2 - 2*alpha (codo casi recto) y theta2' = -theta2 - 2*alpha - 360°
  (codo casi plegado).
"""
import math

import numpy as np

import configuracion as cfg
import plano
from experimentos import guardar

THETA1 = 40.0


def _frontera_pinza():
    """theta4 donde alpha = 0 con la muñeca neutra, por bisección entre 0° (alpha < 0) y 90° (alpha > 0)."""
    a, b = 0.0, 90.0
    for _ in range(60):
        m = (a + b) / 2
        if plano.cadena_virtual(cfg.MUNECA_NEUTRA, m).alfa < 0:
            a = m
        else:
            b = m
    return (a + b) / 2


def _codo_y_punta(cadena, theta1, theta2):
    """El codo y la punta del plano para theta1 y theta2."""
    E = cadena.l1 * plano.u2(theta1)
    return E, E + cadena.l2 * plano.u2(theta1 + theta2 + cadena.alfa)


def _espejo(theta4, theta2, vueltas):
    """Espeja el codo de una pose y lo compara con la fórmula theta2' = -theta2 - 2alpha - 360°*vueltas.

    Args:
        theta4: ángulo de la pinza (grados del modelo); la muñeca queda neutra.
        theta2: ángulo del codo antes de espejar (grados del modelo).
        vueltas: 0 o 1, las vueltas que resta la fórmula.

    Returns:
        Un diccionario con alpha, el signo de s antes de espejar, el codo espejado, la fórmula, si la
        pose espejada es válida y cuánto se movió la punta.
    """
    c = plano.cadena_virtual(cfg.MUNECA_NEUTRA, theta4)
    E, P = _codo_y_punta(c, THETA1, theta2)
    s_antes = P[0] * E[1] - P[1] * E[0]
    u = P / np.linalg.norm(P)
    E2 = 2 * np.dot(E, u) * u - E
    theta1_e, theta2_e = plano.angulos_desde_puntos(E2, P, c.alfa)
    theta1_e, theta2_e = theta1_e % 360.0, (theta2_e + 180) % 360 - 180
    _, P2 = _codo_y_punta(c, theta1_e, theta2_e)
    valido = plano.en_rango(theta1_e, 0, 180) and plano.en_rango(theta2_e, -180, 0)
    return {"theta4": theta4, "alfa": c.alfa, "theta2": theta2, "s_antes": s_antes, "theta2_espejo": theta2_e,
            "formula": -theta2 - 2 * c.alfa - 360.0 * vueltas, "theta1_espejo": theta1_e, "valido": valido,
            "dif_punta": float(np.linalg.norm(P2 - P))}


def calcular():
    """Verifica las fronteras de la regla del codo y los dos espejos.

    Returns:
        Un diccionario con la pinza donde alpha cambia de signo, la mayor diferencia entre s y su
        fórmula cerrada, y los dos casos de espejo.
    """
    c = plano.cadena_virtual(cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA)
    max_dif_s = 0.0
    for theta2 in range(-180, 1):
        E, P = _codo_y_punta(c, THETA1, theta2)
        s = P[0] * E[1] - P[1] * E[0]
        max_dif_s = max(max_dif_s, abs(s - c.l1 * c.l2 * math.sin(math.radians(-(theta2 + c.alfa)))))
    return {"frontera_pinza": _frontera_pinza(), "max_dif_s": max_dif_s,
            "espejos": [_espejo(90.0, -2.0, 0), _espejo(60.0, -176.0, 1)]}


def main():
    """Imprime la salida y la guarda en resultados/regla_del_codo.txt."""
    r = calcular()
    lineas = ["Implementación 7.3: fronteras de la regla del codo",
              f"  α = 0 con la pinza en θ4 = {r['frontera_pinza']:.2f}°; por debajo, α < 0 y la frontera "
              f"θ2 = -180° - α queda dentro del rango del codo",
              f"  s = T_r·E_h - T_h·E_r contra l1·l2·sin(-(θ2 + α)), θ2 de -180° a 0°: diferencia máxima "
              f"{r['max_dif_s']:.1e}"]
    for e in r["espejos"]:
        lineas.append(f"  θ4 = {e['theta4']:.0f}° (α = {e['alfa']:.2f}°), θ1 = {THETA1:.0f}°, θ2 = {e['theta2']:.0f}°: "
                      f"s = {e['s_antes']:.3f} < 0 -> espejo θ1' = {e['theta1_espejo']:.2f}°, "
                      f"θ2' = {e['theta2_espejo']:.2f}° (fórmula {e['formula']:.2f}°), en rango: {e['valido']}, "
                      f"misma punta a {e['dif_punta']:.1e} cm")
    guardar("regla_del_codo", lineas)


if __name__ == "__main__":
    main()
