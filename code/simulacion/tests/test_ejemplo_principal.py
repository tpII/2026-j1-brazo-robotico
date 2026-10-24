"""El ejemplo (5, -5, 7) de la implementación: Paso 1, soluciones exactas y las tres versiones."""
import unittest

import numpy as np

import experimentos.ejemplo_principal


class TestEjemploPrincipal(unittest.TestCase):
    """El ejemplo (5, −5, 7), de la sección 5.2 a la 6.4."""
    @classmethod
    def setUpClass(cls):
        cls.r = experimentos.ejemplo_principal.calcular()

    def test_paso_1_y_soluciones_exactas(self):
        """Impl. 5.2 y 6.1."""
        p = self.r["paso1"]
        for clave, valor in (("rho", 7.07), ("d", 0.0), ("r_c", 7.07), ("theta0", -45.0), ("phi", -45.0),
                             ("desvio", 0.0)):
            self.assertAlmostEqual(p[clave], valor, delta=0.005, msg=clave)
        np.testing.assert_allclose(p["T"], [7.07, 3.00], atol=0.005)
        np.testing.assert_allclose(self.r["exactas"]["arriba"], [133.79, -145.67], atol=0.005)
        np.testing.assert_allclose(self.r["exactas"]["abajo"], [-87.81, 140.90], atol=0.005)

    def test_figura_4(self):
        """Impl. Figura 4: (6, 1, 6) llega con la muñeca en 20° y la pinza en 140°; la base queda en rango."""
        s = self.r["superior"]
        self.assertEqual((s["muneca"], s["pinza"]), (20.0, 140.0))
        for clave, valor in (("phi", 9.46), ("d", 1.13), ("r_c", 5.98), ("theta0", -1.21), ("error", 0.35)):
            self.assertAlmostEqual(s[clave], valor, delta=0.005, msg=clave)

    def test_versiones_en_el_ejemplo(self):
        """Impl. 5.6, 6.2, 6.3 y 6.4."""
        np.testing.assert_allclose(self.r["v3"]["angulos"][:3], [-45.0, 127.49, -142.26], atol=0.005)
        np.testing.assert_allclose(self.r["v3"]["p5"], [5.33, -5.33, 6.90], atol=0.005)
        np.testing.assert_allclose(self.r["v1"]["angulos"], [-81.51, 137.49], atol=0.005)
        np.testing.assert_allclose(self.r["v1_recortado"]["angulos"], [0.0, -180.0], atol=0.005)
        self.assertAlmostEqual(self.r["v1_recortado"]["error"], 12.66, delta=0.005)
        v2 = self.r["v2"]   # se traba: el recorte lleva el hombro a 0° y el brazo queda recto
        self.assertAlmostEqual(v2["hombro_sin_recortar"], -54.61, delta=0.005)
        self.assertAlmostEqual(v2["hombro_recortado"], 0.0, delta=0.005)
        np.testing.assert_allclose(v2["angulos"], [0.0, 0.0], atol=0.005)
        self.assertAlmostEqual(v2["error"], 12.09, delta=0.005)
        self.assertEqual((v2["iteraciones"], v2["motivo"]), (1, "eta"))
        traba = self.r["v2_traba"]   # (5, -5, 8): también se traba, con el hombro en 0° y el brazo recto
        self.assertAlmostEqual(traba["hombro_sin_recortar"], -50.66, delta=0.005)
        np.testing.assert_allclose(traba["angulos"], [0.0, 0.0], atol=0.005)
        self.assertAlmostEqual(traba["error"], 12.34, delta=0.005)
        self.assertEqual((traba["iteraciones"], traba["motivo"]), (1, "eta"))


if __name__ == "__main__":
    unittest.main()
