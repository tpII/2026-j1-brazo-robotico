"""Las coordenadas que dibujan las figuras 3 a 7 de la implementación salen de la simulación.

Lee los bloques de coordenadas de docs/investigaciones/implementacion-fabrik.typ y los compara
con los que calcula experimentos.ejemplo_principal (el .typ tiene tres decimales).
"""
import re
import unittest

import numpy as np

import experimentos.ejemplo_principal
from experimentos import CARPETA_SIM

TYP = CARPETA_SIM.parent.parent / "docs" / "investigaciones" / "implementacion-fabrik.typ"
NUM = r"(-?\d+\.\d+)"
TERNA = re.compile(r"\(\s*" + NUM + r",\s*" + NUM + r",\s*" + NUM + r",?\s*\)")
PAR = re.compile(r"\(\s*" + NUM + r",\s*" + NUM + r"\s*\)")
NOMBRES = {"home", "inicial", "arriba", "abajo", "sin-regla", "con-regla", "codo-forward", "codo-reflejado",
           "latigo", "objetivo-plano", "u-plano", "superior"}


def _coordenadas_del_typ(typ):
    figuras = {}
    poses = re.search(r"#let poses = \((.*?)\n\)", typ, re.S).group(1)
    for nombre, cuerpo in re.findall(r"\n  ([\w-]+): \((.*?)\n  \),", poses, re.S):
        figuras[nombre] = [[float(v) for v in t] for t in TERNA.findall(cuerpo)]
    for nombre in ("codo-forward", "codo-reflejado", "objetivo-plano"):
        figuras[nombre] = [float(v) for v in re.search(r"#let " + nombre + r" = " + TERNA.pattern, typ).groups()]
    latigo = re.search(r"#let latigo = \((.*?)\n\)", typ, re.S).group(1)
    figuras["latigo"] = [[float(v) for v in t] for t in TERNA.findall(latigo)]
    figuras["u-plano"] = [float(v) for v in re.search(r"#let u-plano = " + PAR.pattern, typ).groups()]
    superior = re.search(r"#let superior = \((.*?)\)\n", typ).group(1)
    figuras["superior"] = [float(v) for v in re.findall(NUM, superior)]
    return figuras


class TestFiguras(unittest.TestCase):
    """Las figuras del informe frente a la simulación."""
    def test_figuras_iguales_al_typ(self):
        """Las coordenadas de las figuras 3 a 7 del .typ salen de la simulación."""
        del_typ = _coordenadas_del_typ(TYP.read_text(encoding="utf-8"))
        self.assertEqual(set(del_typ), NOMBRES)
        calculadas = experimentos.ejemplo_principal.calcular()["figuras"]
        for nombre, puntos in del_typ.items():
            np.testing.assert_allclose(calculadas[nombre], puntos, atol=1e-3, err_msg=nombre)


if __name__ == "__main__":
    unittest.main()
