"""Cuántos de los 2000 objetivos resuelve cada versión.

Cada versión se corre con la muñeca y la pinza con las que se generó el objetivo, desde la pose
cero (theta1 = theta2 = 0) y desde una pose al azar. Un objetivo está resuelto si el control final lo
acepta: error <= 0.5 cm, ángulos en rango y p5 a 0.5 cm o menos. El tiempo depende de la máquina.
"""
import time

import numpy as np

import fabrik
import fabrik_v1
import fabrik_v2
from experimentos import CARPETA_SIM, guardar, objetivos_medicion

VARIANTES = {
    "v1": (fabrik_v1.resolver, {}),
    "v1+espejo": (fabrik_v1.resolver, {"espejo_final": True}),
    "v2": (fabrik_v2.resolver, {}),
    "v3": (fabrik.resolver, {}),
}
DESCRIPCION = {
    "v1": "plano sin límites y chequeo al final",
    "v1+espejo": "v1 + espejo del codo una sola vez, al final",
    "v2": "límites en cada paso, sin regla del codo",
    "v3": "límites en cada paso y regla del codo",
}


def calcular():
    """Resuelve los 2000 objetivos con cada versión, desde la pose cero y desde una pose al azar.

    Returns:
        Un diccionario por versión y por inicio ("cero" o "azar") con los objetivos resueltos,
        las iteraciones promedio de los resueltos y los ms por objetivo.
    """
    objetivos = objetivos_medicion()
    resultado = {}
    for nombre, (resolver, opciones) in VARIANTES.items():
        resultado[nombre] = {}
        for inicio in ("cero", "azar"):
            resueltos, iteraciones = 0, []
            comienzo = time.perf_counter()
            for item in objetivos:
                theta1, theta2 = (0.0, 0.0) if inicio == "cero" else item["inicio_azar"]
                s = resolver(item["objetivo"], item["angulos"][3], item["angulos"][4], theta1, theta2, **opciones)
                if s.valida:
                    resueltos += 1
                    iteraciones.append(s.planos[-1].ejecucion.iteraciones)
            ms = 1000 * (time.perf_counter() - comienzo) / len(objetivos)
            resultado[nombre][inicio] = {"resueltos": resueltos, "iteraciones": float(np.mean(iteraciones)), "ms": ms}
    return resultado


def main():
    """Imprime la salida y la guarda en resultados/medicion_versiones.txt."""
    r = calcular()
    lineas = ["Tabla 7: objetivos resueltos sobre 2000 alcanzables (semilla 12345)",
              "  variante: desde la pose cero | desde una pose al azar (iteraciones promedio de las resueltas, ms por objetivo)"]
    for nombre, filas in r.items():
        cero, azar = filas["cero"], filas["azar"]
        lineas.append(f"  {nombre:9s} ({DESCRIPCION[nombre]}): {cero['resueltos']} ({cero['resueltos'] / 20:.2f} %), "
                      f"{cero['iteraciones']:.2f} it., {cero['ms']:.3f} ms | {azar['resueltos']} "
                      f"({azar['resueltos'] / 20:.2f} %), {azar['iteraciones']:.2f} it., {azar['ms']:.3f} ms")
    v3 = r["v3"]["cero"]
    lineas += ["", f"Tabla 10: v3 con {v3['iteraciones']:.1f} iteraciones en promedio y {v3['ms']:.2f} ms por objetivo "
               f"en esta máquina"]
    guardar("medicion_versiones", lineas)


if __name__ == "__main__":
    main()
