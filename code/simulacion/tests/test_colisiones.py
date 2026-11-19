"""colisiones.py: pares de la matriz, tope del codo y mesa, con las esferas de 1.5 cm."""
import math
import unittest

import cinematica
import colisiones
import configuracion as cfg
import experimentos.reglas_esferas
import servos


def _resumen(pose):
    col = colisiones.detectar_colisiones(pose)
    return col.hay_choque, col.pares, col.esferas, col.piso


class TestColisiones(unittest.TestCase):
    """La detección de choques."""
    def test_par_0_4(self):
        """La pala choca con la columna de la base; se marcan las dos esferas que se tocan."""
        self.assertEqual(_resumen((-180, 60, -130, 0, 0)), (True, [(0, 4)], [(0, 2), (4, 2)], []))

    def test_tope_del_codo(self):
        """Con el codo más cerrado que -139° chocan el brazo y el antebrazo; se marcan sus esferas junto al codo."""
        self.assertEqual(_resumen((0, 90, -140, 0, 90)), (True, [(1, 2)], [(1, 4), (2, 0)], []))
        self.assertFalse(colisiones.detectar_colisiones((0, 90, -139, 0, 90)).hay_choque)

    def test_mesa_con_las_esferas(self):
        """Con el antebrazo hacia abajo tocan la mesa su última esfera, la de la muñeca y la pala."""
        esperadas = [(2, 3), (3, 0), (4, 2)]
        self.assertEqual(_resumen((0, 0, -90, 0, 90)), (True, [], esperadas, esperadas))

    def test_mesa_con_la_caja_de_la_pala(self):
        """Una esquina de la caja de la pala toca la mesa; se marca la esfera de la pala más cercana."""
        self.assertEqual(_resumen((0, 0, -5, 0, 0)), (True, [], [(4, 2)], [(4, 2)]))

    def test_pala_acostada_cerca_de_la_mesa(self):
        """La pala acostada a 1.9 mm de la mesa no la toca, aunque sus esferas estén a menos de un radio."""
        pose = (0, 0, -30, 0, 120)
        self.assertTrue(all(c[2] < cfg.RADIO_ESFERA for c in cinematica.cinematica_directa(pose).esferas[4]))
        self.assertFalse(colisiones.detectar_colisiones(pose).hay_choque)

    def test_pose_cero_y_home_libres(self):
        """La pose cero, el home y la pinza neutra no chocan."""
        for pose in ((0, 0, 0, 0, 0), servos.home_modelo(), (0, 0, 0, 0, 90)):
            self.assertFalse(colisiones.detectar_colisiones(pose).hay_choque)

    def test_reglas_esferas_rapido(self):
        """Los pares que no se revisan chocarían en todas las poses; los que se revisan, solo en algunas."""
        r = experimentos.reglas_esferas.calcular(n=5000)
        distancias = {"(0, 1)": 4.58 / 4, "(1, 2)": 6.25 / 5, "(2, 3)": math.hypot(5.89, 0.5) / 4, "(3, 4)": 1.29,
                      "(2, 4)": math.hypot(5.89 / 4 + 1.29, 0.5 / 4)}
        for par, distancia in distancias.items():
            self.assertAlmostEqual(r["sin_revisar"][par][0], distancia, places=9)
            self.assertAlmostEqual(r["sin_revisar"][par][1], distancia, places=9)
            self.assertLess(r["sin_revisar"][par][1], 2 * cfg.RADIO_ESFERA)
        self.assertEqual(list(r["revisados"]), ["(0, 2)", "(0, 3)", "(0, 4)", "(1, 3)", "(1, 4)"])
        self.assertTrue(all(choques > 0 for choques in r["revisados"].values()))


if __name__ == "__main__":
    unittest.main()
