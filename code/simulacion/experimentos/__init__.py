"""Experimentos realizados para el informe.

Cada experimento tiene calcular(), que devuelve los valores (lo usan los tests),
y main(), que los imprime y los guarda en resultados/<nombre>.txt.
Se corren desde code/simulacion/ con: python -m experimentos.<nombre>
"""
from pathlib import Path

import numpy as np

import cinematica
import servos

CARPETA_SIM = Path(__file__).resolve().parent.parent


def guardar(nombre, lineas):
    """Imprime las líneas y las guarda en resultados/<nombre>.txt.

    Args:
        nombre: el nombre del experimento.
        lineas: las líneas de la salida, sin el salto de línea final.

    Returns:
        El Path del archivo guardado.
    """
    texto = "\n".join(lineas) + "\n"
    print(texto, end="")
    ruta = CARPETA_SIM / "resultados" / f"{nombre}.txt"
    ruta.parent.mkdir(exist_ok=True)
    ruta.write_text(texto, encoding="utf-8")
    return ruta


def objetivos_medicion(semilla=12345, n=2000):
    """Los objetivos alcanzables de las mediciones de la implementación (Tabla 7 y sección 9).

    Para cada uno se eligen al azar la base, el hombro, el codo y la muñeca dentro del rango del
    modelo y después una pose al azar para empezar, en ese orden, el mismo que usó el prototipo que
    midió el informe. Las pinzas se sortean al final con el mismo generador, para que esos valores
    no cambien: el prototipo dejaba la pinza en 0° porque entonces no movía p5. p5 sale de la
    cinemática directa de los cinco ángulos.

    Args:
        semilla: la semilla del generador de números al azar.
        n: cuántos objetivos se generan.

    Returns:
        Una lista de diccionarios con el objetivo (x, y, z), los cinco ángulos que lo generaron y la
        pose de inicio al azar (θ1, θ2).
    """
    rng = np.random.default_rng(semilla)
    rangos = [servos.rango_modelo(i) for i in range(5)]
    sorteos = []
    for _ in range(n):
        angulos = tuple(rng.uniform(*r) for r in rangos[:4])
        sorteos.append((angulos, (rng.uniform(*rangos[1]), rng.uniform(*rangos[2]))))
    pinzas = rng.uniform(*rangos[4], size=n)
    objetivos = []
    for (angulos, inicio_azar), pinza in zip(sorteos, pinzas):
        angulos = (*angulos, float(pinza))
        objetivos.append({"objetivo": cinematica.cinematica_directa(angulos).puntos[5], "angulos": angulos,
                          "inicio_azar": inicio_azar})
    return objetivos
