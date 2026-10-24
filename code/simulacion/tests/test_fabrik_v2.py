"""fabrik_v2.py: límites en cada paso, sin regla del codo."""
import unittest

import numpy as np

import fabrik_v2
import plano

OBJ = np.array([5.0, -5.0, 7.0])
CADENA_NEUTRA = plano.cadena_virtual(0.0, 90.0)
(_, RC_EJEMPLO), = plano.planos_candidatos(OBJ, CADENA_NEUTRA.d)
T_EJEMPLO = plano.objetivo_en_plano(OBJ, RC_EJEMPLO, CADENA_NEUTRA)


class TestFabrikV2(unittest.TestCase):
    """FABRIK v2, con límites y sin regla del codo."""
    def test_v2_en_el_ejemplo(self):
        """El primer recorte lleva el hombro de -54.61° a 0°; queda recto, a 12.09 cm, y corta por eta."""
        r = fabrik_v2.iterar(T_EJEMPLO, CADENA_NEUTRA, 0.0, 0.0)
        self.assertAlmostEqual(r.theta1, 0.0, delta=0.005)
        self.assertAlmostEqual(r.theta2, 0.0, delta=0.005)
        self.assertAlmostEqual(r.error, 12.09, delta=0.005)
        self.assertEqual((r.iteraciones, r.motivo), (1, "eta"))

    def test_v2_trabado(self):
        """Con (5, -5, 8) pasa lo mismo: el hombro va a 0°, queda recto, a 12.34 cm, y corta por eta."""
        objetivo = np.array([5.0, -5.0, 8.0])
        (_, rc), = plano.planos_candidatos(objetivo, CADENA_NEUTRA.d)
        r = fabrik_v2.iterar(plano.objetivo_en_plano(objetivo, rc, CADENA_NEUTRA), CADENA_NEUTRA, 0.0, 0.0)
        self.assertAlmostEqual(r.theta1, 0.0, delta=0.005)
        self.assertAlmostEqual(r.theta2, 0.0, delta=0.005)
        self.assertAlmostEqual(r.error, 12.34, delta=0.005)
        self.assertEqual((r.iteraciones, r.motivo), (1, "eta"))


if __name__ == "__main__":
    unittest.main()
