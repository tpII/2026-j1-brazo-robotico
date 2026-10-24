"""el caso de la cadena recta.

La cadena virtual hombro, codo, p5 está recta con la muñeca y la pinza neutras y el objetivo cae
sobre esa recta. Se ve qué hace FABRIK v3 solo (corta por n si el codo no se mueve) y qué hace la
búsqueda, que pasa a otra posición de la pinza, con su propio alpha, y llega.
"""
import math

import numpy as np

import configuracion as cfg
import busqueda
import fabrik
import plano
from experimentos import guardar

# (inclinación de la cadena recta θ1, distancia del objetivo al hombro, giro de la base θ0)
CASOS = ((30.0, 7.0, -45.0), (60.0, 5.0, -100.0), (10.0, 8.5, -20.0))


def calcular():
    """Parte de la cadena virtual recta hacia objetivos sobre esa recta.

    Returns:
        Un diccionario con "casos": por cada objetivo, cómo corta la iteración con la muñeca y
        la pinza neutras y qué encuentra la búsqueda (estado, posiciones, muñeca, pinza, ángulos y error).
    """
    c = plano.cadena_virtual(cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA)
    casos = []
    for theta1, distancia, theta0 in CASOS:
        theta2 = -c.alfa                               # cadena virtual recta
        T = distancia * plano.u2(theta1)               # objetivo sobre esa recta, dentro del plano
        rho = math.hypot(T[0], c.d)
        fi = math.radians(theta0) + math.atan2(c.d, T[0])
        objetivo = np.array([rho * math.cos(fi), rho * math.sin(fi), c.z_hombro + T[1]])
        neutra = fabrik.iterar(T, c, theta1, theta2)
        r = busqueda.buscar(objetivo, (theta0, theta1, theta2, cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA),
                            revisar_colisiones=False)
        casos.append({"objetivo": objetivo, "theta1": theta1, "distancia": distancia,
                      "error_neutra": neutra.error, "iteraciones_neutra": neutra.iteraciones,
                      "motivo_neutra": neutra.motivo, "estado": r.estado, "posiciones": r.posiciones,
                      "muneca": r.angulos[3] if r.angulos else None, "pinza": r.angulos[4] if r.angulos else None,
                      "angulos": r.angulos, "error": r.error,
                      "iteraciones": r.iteraciones})
    return {"casos": casos}


def main():
    """Imprime la salida y la guarda en resultados/cadena_recta.txt."""
    lineas = ["Implementación 7.3: cadena virtual recta con la muñeca y la pinza neutras y el objetivo sobre "
              "la recta"]
    for caso in calcular()["casos"]:
        lineas.append(f"  objetivo {np.round(caso['objetivo'], 3).tolist()} (cadena recta a {caso['theta1']:.0f}°, "
                      f"a {caso['distancia']} cm del hombro)")
        lineas.append(f"    v3 con la muñeca y la pinza neutras: error {caso['error_neutra']:.2f} cm, "
                      f"k = {caso['iteraciones_neutra']}, corta por {caso['motivo_neutra']}")
        angulos = tuple(round(a, 2) for a in caso["angulos"][:3]) if caso["angulos"] else None
        lineas.append(f"    búsqueda: {caso['estado']} con la muñeca en {caso['muneca']}° y la pinza en "
                      f"{caso['pinza']}°, {caso['posiciones']} posiciones, ángulos {angulos}, "
                      f"error {caso['error']:.2f} cm, k = {caso['iteraciones']}")
    guardar("cadena_recta", lineas)


if __name__ == "__main__":
    main()
