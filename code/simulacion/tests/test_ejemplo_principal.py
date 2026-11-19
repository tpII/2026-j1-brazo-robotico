"""El ejemplo (5, -5, 7): Paso 1, soluciones exactas y las tres versiones."""
import unittest

import numpy as np

import experimentos.ejemplo_principal


class TestEjemploPrincipal(unittest.TestCase):
    """El ejemplo (5, -5, 7)."""
    @classmethod
    def setUpClass(cls):
        cls.r = experimentos.ejemplo_principal.calcular()

    def test_paso_1_y_soluciones_exactas(self):
        """El Paso 1 y las dos soluciones exactas, codo arriba y codo abajo."""
        p = self.r["paso1"]
        for clave, valor in (("rho", 7.07), ("d", -0.5), ("r_c", 7.05), ("theta0", -40.95), ("phi", -45.0),
                             ("desvio", -4.05)):
            self.assertAlmostEqual(p[clave], valor, delta=0.005, msg=clave)
        np.testing.assert_allclose(p["T"], [7.05, 2.42], atol=0.005)
        np.testing.assert_allclose(self.r["exactas"]["arriba"], [132.87, -143.69], atol=0.005)
        np.testing.assert_allclose(self.r["exactas"]["abajo"], [-94.99, 143.69], atol=0.005)

    def test_objetivo_al_costado(self):
        """(6, 1, 6) llega con la muñeca en 30° y la pinza en 140°; la base queda en rango."""
        s = self.r["superior"]
        self.assertEqual((s["muneca"], s["pinza"]), (30.0, 140.0))
        for clave, valor in (("phi", 9.46), ("d", 1.16), ("r_c", 5.97), ("theta0", -1.52), ("error", 0.41)):
            self.assertAlmostEqual(s[clave], valor, delta=0.005, msg=clave)

    def test_versiones_en_el_ejemplo(self):
        """Lo que dan v3, v1, v1 con recorte al final y v2."""
        np.testing.assert_allclose(self.r["v3"]["angulos"][:3], [-40.95, 127.43, -140.62], atol=0.005)
        np.testing.assert_allclose(self.r["v3"]["p5"], [5.27, -5.23, 6.92], atol=0.005)
        np.testing.assert_allclose(self.r["v1"]["angulos"], [-89.56, 140.62], atol=0.005)
        np.testing.assert_allclose(self.r["v1_recortado"]["angulos"], [0.0, -180.0], atol=0.005)
        self.assertAlmostEqual(self.r["v1_recortado"]["error"], 12.55, delta=0.005)
        v2 = self.r["v2"]   # se traba: el recorte lleva el hombro a 0° y el brazo queda recto
        self.assertAlmostEqual(v2["hombro_sin_recortar"], -68.05, delta=0.005)
        self.assertAlmostEqual(v2["hombro_recortado"], 0.0, delta=0.005)
        np.testing.assert_allclose(v2["angulos"], [0.0, 0.0], atol=0.005)
        self.assertAlmostEqual(v2["error"], 10.98, delta=0.005)
        self.assertEqual((v2["iteraciones"], v2["motivo"]), (1, "eta"))
        traba = self.r["v2_traba"]   # (5, -5, 8): también se traba, con el hombro en 0° y el brazo recto
        self.assertAlmostEqual(traba["hombro_sin_recortar"], -60.41, delta=0.005)
        np.testing.assert_allclose(traba["angulos"], [0.0, 0.0], atol=0.005)
        self.assertAlmostEqual(traba["error"], 11.24, delta=0.005)
        self.assertEqual((traba["iteraciones"], traba["motivo"]), (1, "eta"))


if __name__ == "__main__":
    unittest.main()
