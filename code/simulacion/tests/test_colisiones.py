"""colisiones.py: las mismas reglas que cinematica.c (pares de la matriz, junta compartida y piso)."""
import unittest

import numpy as np

import cinematica
import colisiones
import experimentos.reglas_esferas
import servos


class TestColisiones(unittest.TestCase):
    """La detección de choques."""
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
        """Las reglas de las esferas con menos poses que el experimento, con las medidas del brazo."""
        r = experimentos.reglas_esferas.calcular(n=5000)
        self.assertEqual(r["pares"]["(0, 2)"]["sin_detectar"], 0)
        for clave, valor in (("dist_2_4", 3.7), ("dist_junta_1_2", 4.9)):
            self.assertAlmostEqual(r[clave][0], valor, places=9)
            self.assertAlmostEqual(r[clave][1], valor, places=9)
        self.assertAlmostEqual(r["dist_muneca"], 0.5, places=9)

    @unittest.expectedFailure
    def test_par_1_3_siempre_detectado(self):
        """Todo choque del par (1, 3) lo detecta el piso u otro par.

        Valía con las medidas de J5. Con las del brazo armado y las esferas de configuracion.c, el brazo y
        la muñeca chocan sin que nada lo detecte; falla hasta que se rehagan las esferas.
        """
        r = experimentos.reglas_esferas.calcular(n=5000)
        self.assertEqual(r["pares"]["(1, 3)"]["sin_detectar"], 0)


if __name__ == "__main__":
    unittest.main()
