"""colisiones.py: las mismas reglas que cinematica.c (pares de la matriz, junta compartida y piso)."""
import json
import unittest
from pathlib import Path

import numpy as np

import cinematica
import colisiones
import experimentos.reglas_esferas
import servos
from test_cinematica import con_saltos_del_firmware

REFERENCIA = json.loads((Path(__file__).resolve().parent.parent / "datos" / "referencia_firmware_2025.json")
                        .read_text(encoding="utf-8"))


def _mismos_puntos(a, b, tol):
    """Cada punto de a tiene uno de b a menos de tol, y al revés (sin importar orden ni repetidos)."""
    a, b = np.array(a, float).reshape(-1, 3), np.array(b, float).reshape(-1, 3)
    if len(a) == 0 or len(b) == 0:
        return len(a) == len(b)
    d = np.linalg.norm(a[:, None, :] - b[None, :, :], axis=2)
    return bool(np.all(d.min(axis=1) < tol) and np.all(d.min(axis=0) < tol))


class TestColisiones(unittest.TestCase):
    """La detección de choques, comparada con el firmware."""
    def test_igual_al_firmware(self):
        """Con los saltos de configuracion.c, los choques coinciden con los de cinematica.c en 402 poses."""
        with con_saltos_del_firmware():
            for pose in REFERENCIA["poses"]:
                g = cinematica.cinematica_directa(pose["angulos"])
                col = colisiones.detectar_colisiones(g.esferas)
                self.assertEqual(col.hay_choque, pose["colision"], pose["angulos"])
                nuestros = colisiones.centros(g.esferas, col.esferas)
                self.assertTrue(_mismos_puntos(nuestros, pose["centros"], tol=1e-9), pose["angulos"])

    def test_hay_casos_de_cada_tipo(self):
        """Los datos del firmware tienen choques de los pares (0, 4) y (1, 2) y contra el piso."""
        # Con las medidas de J5 el par (1, 4) no puede chocar (la segunda esfera de la pinza nunca apunta
        # hacia el hombro con θ4 entre 0 y 180°); se prueba aparte con esferas armadas.
        vistos = set()
        with con_saltos_del_firmware():
            for pose in REFERENCIA["poses"]:
                col = colisiones.detectar_colisiones(cinematica.cinematica_directa(pose["angulos"]).esferas)
                vistos |= set(col.pares) | ({"piso"} if col.piso else set())
        self.assertTrue({(0, 4), (1, 2), "piso"} <= vistos, vistos)

    def test_par_1_4_con_esferas_armadas(self):
        """El par (1, 4) se detecta con esferas ubicadas a mano."""
        esferas = [np.array([[0.0, 0.0, 10.0]]),
                   np.array([[0.0, 0.0, 13.0], [2.0, 0.0, 13.0]]),
                   np.array([[5.0, 0.0, 13.0], [7.0, 0.0, 13.0], [9.0, 0.0, 13.0]]),
                   np.array([[9.0, 0.0, 13.0]]),
                   np.array([[11.0, 0.0, 13.0], [2.0, 1.0, 13.0]])]
        col = colisiones.detectar_colisiones(esferas)
        self.assertEqual((col.hay_choque, col.pares, col.esferas, col.piso), (True, [(1, 4)], [(1, 1), (4, 1)], []))

    def test_pose_cero_y_home_libres(self):
        """La pose cero, el home y la pinza neutra no chocan."""
        for pose in ((0, 0, 0, 0, 0), servos.home_modelo(), (0, 0, 0, 0, 90)):
            self.assertFalse(colisiones.detectar_colisiones(cinematica.cinematica_directa(pose).esferas).hay_choque)

    def test_reglas_esferas_rapido(self):
        """Inv. 4.5, con menos poses que el experimento: las distancias con las medidas del brazo."""
        r = experimentos.reglas_esferas.calcular(n=5000)
        self.assertEqual(r["pares"]["(0, 2)"]["sin_detectar"], 0)
        for clave, valor in (("dist_2_4", 3.7), ("dist_junta_1_2", 4.9)):
            self.assertAlmostEqual(r[clave][0], valor, places=9)
            self.assertAlmostEqual(r[clave][1], valor, places=9)
        self.assertAlmostEqual(r["dist_muneca"], 0.5, places=9)

    @unittest.expectedFailure
    def test_par_1_3_siempre_detectado(self):
        """Inv. 4.5: todo choque del par (1, 3) lo detecta el piso u otro par.

        Valía con las medidas de J5. Con las del brazo armado y las esferas de configuracion.c, el brazo y
        la muñeca chocan sin que nada lo detecte; falla hasta que se rehagan las esferas.
        """
        r = experimentos.reglas_esferas.calcular(n=5000)
        self.assertEqual(r["pares"]["(1, 3)"]["sin_detectar"], 0)


if __name__ == "__main__":
    unittest.main()
