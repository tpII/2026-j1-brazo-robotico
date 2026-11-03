"""fabrik_v0.py: FABRIK 3D con rótulas libres, el modelo general aplicado directo."""
import unittest

import numpy as np

import experimentos.v0_latigo
import fabrik_v0


class TestFabrikV0(unittest.TestCase):
    """FABRIK v0, en 3D con rótulas libres."""
    def test_latigo(self):
        """FABRIK 3D llega, pero deforma la cadena como un látigo."""
        r = experimentos.v0_latigo.calcular()
        self.assertEqual(r["iteraciones"], 1)
        self.assertLess(r["error"], 0.02)
        self.assertAlmostEqual(r["inclinacion_columna"], 15.5, delta=0.05)
        np.testing.assert_allclose(r["hombro"], [-1.20, -0.23, 4.41], atol=0.005)
        self.assertAlmostEqual(r["angulo_p3"], 106.0, delta=0.05)
        self.assertAlmostEqual(r["angulo_p3_modelo"], 2.9, delta=0.05)

    def test_inalcanzable_cadena_estirada(self):
        """Algoritmo 1 del paper: la cadena en línea recta."""
        r = fabrik_v0.resolver(np.array([30.0, 0.0, 3.0]), (0, 0, 0, 0, 0))
        self.assertEqual(r.iteraciones, 0)
        direcciones = np.diff(r.puntos, axis=0)
        direcciones /= np.linalg.norm(direcciones, axis=1)[:, None]
        np.testing.assert_allclose(direcciones, np.tile(direcciones[0], (5, 1)), atol=1e-9)


if __name__ == "__main__":
    unittest.main()
