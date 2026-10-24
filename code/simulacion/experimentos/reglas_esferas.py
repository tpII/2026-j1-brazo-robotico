"""Investigación de modelos, sección 4.5: por qué la matriz de colisiones excluye ciertos pares.

Con poses al azar dentro de los rangos del firmware se mide si valen las reglas que explican la
matriz de J5:
- en los pares (0, 2) y (1, 3), todo choque lo detecta también el piso u otro par verificado;
- la última esfera del codo y la primera de la pinza están siempre a 2 cm (el par (2, 4) chocaría siempre);
- en la junta (1, 2), la última esfera del brazo y la primera del codo están siempre a 1.3 cm;
- la esfera de la muñeca coincide con la última del codo, así que el par (0, 3) repite al (0, 2).
También informa la menor distancia entre las esferas del brazo y las de la pinza (par (1, 4)). Con las
medidas de J5 valían todas; con las del brazo armado (23-oct) varias dejan de valer, porque las
esferas siguen siendo las de configuracion.c.
"""
import numpy as np

import cinematica
import colisiones
import configuracion as cfg
import servos
from experimentos import guardar


def _choca(esferas_a, esferas_b):
    """Choque entre dos eslabones revisando todas sus esferas, sin exclusiones."""
    limite = 2 * cfg.RADIO_ESFERA + cfg.TOL_COLISION
    return any(np.linalg.norm(a - b) <= limite for a in esferas_a for b in esferas_b)


def calcular(n=100000, semilla=7):
    """Recorre poses al azar y mide los pares que la matriz de colisiones excluye.

    Args:
        n: cuántas poses al azar se prueban.
        semilla: la semilla del generador de números al azar.

    Returns:
        Un diccionario con los choques de los pares (0, 2) y (1, 3) y cuántos no detecta la
        matriz, y las distancias de los pares (2, 4), (1, 2) en la junta, la muñeca y (1, 4).
    """
    rng = np.random.default_rng(semilla)
    rangos = [servos.rango_modelo(i) for i in range(5)]
    pares = {"(0, 2)": {"choques": 0, "sin_detectar": 0}, "(1, 3)": {"choques": 0, "sin_detectar": 0}}
    d_2_4, d_junta, d_muneca, d_1_4 = [], [], [], []
    for _ in range(n):
        th = [rng.uniform(*r) for r in rangos]
        e = cinematica.cinematica_directa(th).esferas
        detectado = colisiones.detectar_colisiones(e).hay_choque
        for i, j in ((0, 2), (1, 3)):
            if _choca(e[i], e[j]):
                pares[f"({i}, {j})"]["choques"] += 1
                if not detectado:
                    pares[f"({i}, {j})"]["sin_detectar"] += 1
        d_2_4.append(np.linalg.norm(e[2][-1] - e[4][0]))
        d_junta.append(np.linalg.norm(e[1][-1] - e[2][0]))
        d_muneca.append(np.linalg.norm(e[3][0] - e[2][-1]))
        d_1_4.append(min(np.linalg.norm(a - b) for a in e[1] for b in e[4]))
    return {"n": n, "semilla": semilla, "pares": pares,
            "dist_2_4": (min(d_2_4), max(d_2_4)), "dist_junta_1_2": (min(d_junta), max(d_junta)),
            "dist_muneca": max(d_muneca), "dist_1_4": min(d_1_4)}


def main():
    """Imprime la salida y la guarda en resultados/reglas_esferas.txt."""
    r = calcular()
    lineas = [f"Investigación 4.5: {r['n']} poses al azar dentro de los rangos del firmware (semilla {r['semilla']})"]
    for par, c in r["pares"].items():
        lineas.append(f"  par {par}: {c['choques']} poses con choque; sin detectar por el piso ni por otro par: "
                      f"{c['sin_detectar']}")
    lineas.append(f"  última esfera del codo y primera de la pinza: entre {r['dist_2_4'][0]:.6f} y "
                  f"{r['dist_2_4'][1]:.6f} cm (2r = {2 * cfg.RADIO_ESFERA} cm)")
    lineas.append(f"  junta (1, 2), última esfera del brazo y primera del codo: entre {r['dist_junta_1_2'][0]:.6f} y "
                  f"{r['dist_junta_1_2'][1]:.6f} cm")
    coinciden = "coinciden" if r["dist_muneca"] < 1e-9 else "no coinciden"
    lineas.append(f"  esfera de la muñeca y última del codo: a lo sumo {r['dist_muneca']:.2e} cm ({coinciden})")
    if r["dist_1_4"] > 2 * cfg.RADIO_ESFERA:
        conclusion = "así que con las medidas y los rangos actuales no llega a chocar"
    else:
        conclusion = "así que puede chocar; la matriz revisa ese par"
    lineas.append(f"  par (1, 4): menor distancia entre esferas {r['dist_1_4']:.3f} cm, {conclusion}")
    guardar("reglas_esferas", lineas)


if __name__ == "__main__":
    main()
