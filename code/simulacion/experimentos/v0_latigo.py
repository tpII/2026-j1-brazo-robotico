"""Implementación, Tabla 3 y Figura 3: FABRIK 3D con rótulas libres (v0) hacia (5, -5, 7) desde la pose cero.

El ángulo en p3 es entre las direcciones consecutivas p2->p3 y p3->p4. La inclinación de la
columna es el ángulo entre p0->p1 y el eje z.
"""
import math

import numpy as np

import cinematica
import fabrik_v0
from experimentos import guardar

OBJETIVO = (5.0, -5.0, 7.0)
POSE_INICIAL = (0.0, 0.0, 0.0, 0.0, 0.0)


def _angulo(a, b):
    """Ángulo entre dos vectores, en grados."""
    coseno = np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b))
    return math.degrees(math.acos(min(1.0, max(-1.0, coseno))))


def calcular():
    """Corre FABRIK 3D desde la pose cero y mide cuánto se deforma la cadena.

    Returns:
        Un diccionario con las iteraciones, el error, el motivo de corte, los puntos finales, el
        hombro movido y los ángulos de la Tabla 3 (columna y p3), también en el modelo.
    """
    r = fabrik_v0.resolver(np.array(OBJETIVO), POSE_INICIAL)
    p, m = r.puntos, cinematica.cinematica_directa(POSE_INICIAL).puntos
    return {"iteraciones": r.iteraciones, "error": r.error, "motivo": r.motivo, "puntos": p, "hombro": p[1],
            "inclinacion_columna": _angulo(p[1] - p[0], np.array([0.0, 0.0, 1.0])),
            "angulo_p3": _angulo(p[3] - p[2], p[4] - p[3]), "angulo_p3_modelo": _angulo(m[3] - m[2], m[4] - m[3])}


def main():
    """Imprime la salida y la guarda en resultados/v0_latigo.txt."""
    r = calcular()
    lineas = [f"Tabla 3: FABRIK 3D libre hacia {OBJETIVO} desde la pose cero: {r['iteraciones']} iteración, "
              f"error {r['error']:.4f} cm (corta por {r['motivo']})",
              f"  columna de la base inclinada {r['inclinacion_columna']:.1f}°",
              f"  hombro p1 movido a {np.round(r['hombro'], 2).tolist()}",
              f"  ángulo del antebrazo en p3: {r['angulo_p3']:.1f}° (en el modelo, {r['angulo_p3_modelo']:.1f}°)",
              "",
              "Figura 3 (a): puntos p0 a p5 (cm)"]
    lineas += [f"  p{i}: {np.round(punto, 3).tolist()}" for i, punto in enumerate(r["puntos"])]
    guardar("v0_latigo", lineas)


if __name__ == "__main__":
    main()
