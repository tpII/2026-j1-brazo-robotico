"""Por qué la matriz de colisiones revisa unos pares y otros no.

Con poses al azar dentro de los rangos del firmware se mide:
- en los pares que no se revisan (los contiguos y el (2, 4)), la distancia entre la última esfera
  de un eslabón y la primera del otro: es siempre la misma y menor que 2r, así que esos pares
  chocarían en todas las poses;
- en los pares que se revisan, en cuántas poses chocan.
El choque del brazo con el antebrazo, que sus esferas no pueden ver, lo cubre el tope del codo.
"""
import numpy as np

import cinematica
import configuracion as cfg
import servos
from experimentos import guardar

SIN_REVISAR = ((0, 1), (1, 2), (2, 3), (3, 4), (2, 4))


def _choca(esferas_a, esferas_b):
    """Choque entre dos eslabones revisando todas sus esferas."""
    limite = 2 * cfg.RADIO_ESFERA + cfg.TOL_COLISION
    return any(np.linalg.norm(a - b) <= limite for a in esferas_a for b in esferas_b)


def calcular(n=100000, semilla=7):
    """Recorre poses al azar y mide los pares que la matriz de colisiones revisa y los que no.

    Args:
        n: cuántas poses al azar se prueban.
        semilla: la semilla del generador de números al azar.

    Returns:
        Un diccionario con la menor y la mayor distancia entre la última esfera de un eslabón y la
        primera del otro en cada par que no se revisa, y las poses con choque de cada par que se revisa.
    """
    rng = np.random.default_rng(semilla)
    rangos = [servos.rango_modelo(i) for i in range(5)]
    revisados = [(i, j) for i in range(5) for j in range(i + 1, 5) if cfg.MATRIZ_COLISIONES[i][j]]
    distancias = {par: [] for par in SIN_REVISAR}
    choques = {par: 0 for par in revisados}
    for _ in range(n):
        th = [rng.uniform(*r) for r in rangos]
        e = cinematica.cinematica_directa(th).esferas
        for i, j in SIN_REVISAR:
            distancias[(i, j)].append(np.linalg.norm(e[i][-1] - e[j][0]))
        for i, j in revisados:
            if _choca(e[i], e[j]):
                choques[(i, j)] += 1
    return {"n": n, "semilla": semilla,
            "sin_revisar": {str(par): (min(d), max(d)) for par, d in distancias.items()},
            "revisados": {str(par): c for par, c in choques.items()}}


def main():
    """Imprime la salida y la guarda en resultados/reglas_esferas.txt."""
    r = calcular()
    lineas = [f"Investigación 4.5: {r['n']} poses al azar dentro de los rangos del firmware (semilla {r['semilla']})",
              f"  pares que no se revisan, distancia entre la última esfera de un eslabón y la primera del otro "
              f"(2r = {2 * cfg.RADIO_ESFERA} cm):"]
    for par, (minimo, maximo) in r["sin_revisar"].items():
        lineas.append(f"    {par}: entre {minimo:.6f} y {maximo:.6f} cm")
    lineas.append("  pares que se revisan, poses con choque:")
    for par, choques in r["revisados"].items():
        lineas.append(f"    {par}: {choques}")
    guardar("reglas_esferas", lineas)


if __name__ == "__main__":
    main()
