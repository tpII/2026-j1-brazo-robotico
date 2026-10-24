"""La cadena virtual según la muñeca y la pinza, y el alcance l1 + l2.

También lo que mueve la punta un grado en el hombro, con el alcance máximo.
"""
import math

import numpy as np

import cinematica
import configuracion as cfg
import plano
import servos
from experimentos import guardar

# (theta3, theta4): la pinza abajo, alineada y arriba con la muñeca neutra, y la muñeca girada.
POSICIONES = ((0.0, 0.0), (0.0, 90.0), (0.0, 180.0), (90.0, 0.0))


def calcular():
    """Calcula la cadena virtual y el alcance según la pinza.

    Returns:
        Un diccionario con las filas de la cadena virtual, l1, el alcance con la muñeca y la pinza neutras,
        el mínimo y el máximo según la pinza (con la muñeca neutra) con la pinza en que ocurren, y
        lo que mueve la punta un grado en el hombro.
    """
    filas = []
    for t3, t4 in POSICIONES:
        c = plano.cadena_virtual(t3, t4)
        p5 = cinematica.cinematica_directa((0.0, 0.0, 0.0, t3, t4)).puntos[5]
        filas.append({"theta3": t3, "theta4": t4, "l2": c.l2, "alfa": c.alfa, "d": c.d, "p5": p5})
    lo, hi = servos.rango_modelo(4)
    pinza = np.arange(lo, hi + 1e-9, 0.5)
    sumas = [c.l1 + c.l2 for c in (plano.cadena_virtual(cfg.MUNECA_NEUTRA, t4) for t4 in pinza)]
    neutra = plano.cadena_virtual(cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA)
    return {"filas": filas, "l1": neutra.l1, "alcance_neutra": neutra.l1 + neutra.l2,
            "alcance_min": min(sumas), "pinza_min": float(pinza[int(np.argmin(sumas))]),
            "alcance_max": max(sumas), "pinza_max": float(pinza[int(np.argmax(sumas))]),
            "cm_por_grado": max(sumas) * math.pi / 180}


def main():
    """Imprime la salida y la guarda en resultados/cadena_virtual.txt."""
    r = calcular()
    lineas = [f"Tabla 5: cadena virtual según la muñeca y la pinza (l1 = {r['l1']:.2f} cm)"]
    for f in r["filas"]:
        lineas.append(f"  θ3 = {f['theta3']:+.0f}°, θ4 = {f['theta4']:.0f}°: l2 = {f['l2']:.2f} cm, α = {f['alfa']:.2f}°, "
                      f"d = {round(f['d'], 2) + 0.0:.2f} cm, p5 = {(np.round(f['p5'], 2) + 0.0).tolist()}")
    lineas.append("")
    lineas.append(f"Sección 5.1: l1 + l2 = {r['alcance_neutra']:.2f} cm con la muñeca y la pinza neutras; según la "
                  f"pinza, va de {r['alcance_min']:.2f} cm (θ4 = {r['pinza_min']:.1f}°) a {r['alcance_max']:.2f} cm "
                  f"(θ4 = {r['pinza_max']:.1f}°)")
    lineas.append(f"Tabla 8 (ε): con ese alcance máximo, un grado en el hombro mueve la punta hasta "
                  f"{r['cm_por_grado']:.2f} cm")
    guardar("cadena_virtual", lineas)


if __name__ == "__main__":
    main()
