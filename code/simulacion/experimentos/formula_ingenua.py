"""El ángulo entre cada eslabón y su propio eje no depende de la pose.

Para cada pose se calcula atan2(||d_i x z_i||, d_i . z_i), con d_i = p(i+1) - p(i) y z_i el eje de giro
del servo i que da la cinemática directa.
"""
import math

import numpy as np

import cinematica
from experimentos import guardar

POSES = ((0, 0, 0, 0, 0), (-30, 60, -90, 20, 0), (-120, 100, -40, -60, 90))


def calcular():
    """Mide el ángulo entre cada eslabón y su propio eje en varias poses.

    Returns:
        Un diccionario con "filas": la pose y los cinco ángulos de cada una.
    """
    filas = []
    for pose in POSES:
        g = cinematica.cinematica_directa(pose)
        resultado = []
        for i in range(5):
            d, z = g.puntos[i + 1] - g.puntos[i], g.ejes[i]
            resultado.append(math.degrees(math.atan2(np.linalg.norm(np.cross(d, z)), np.dot(d, z))))
        filas.append({"pose": pose, "resultado": resultado})
    return {"filas": filas}


def main():
    """Imprime la salida y la guarda en resultados/formula_ingenua.txt."""
    lineas = ["Tabla 4: ángulos reales del modelo -> resultado de la fórmula (grados)"]
    for fila in calcular()["filas"]:
        lineas.append(f"  {fila['pose']} -> {tuple(round(a, 2) + 0.0 for a in fila['resultado'])}")
    guardar("formula_ingenua", lineas)


if __name__ == "__main__":
    main()
