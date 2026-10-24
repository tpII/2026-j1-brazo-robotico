"""fabrik_v1.py: plano sin límites y chequeo al final (implementación, secciones 6.2 y 6.3)."""
import unittest

import numpy as np

import cinematica
import fabrik_v1
import plano

OBJ = np.array([5.0, -5.0, 7.0])
CADENA_NEUTRA = plano.cadena_virtual(0.0, 90.0)
(T0_EJEMPLO, RC_EJEMPLO), = plano.planos_candidatos(OBJ, CADENA_NEUTRA.d)
T_EJEMPLO = plano.objetivo_en_plano(OBJ, RC_EJEMPLO, CADENA_NEUTRA)


class TestFabrikV1(unittest.TestCase):
    """FABRIK v1, sin límites durante la iteración."""
    def test_v1_converge_con_el_codo_abajo(self):
        """6.2: θ1 = -81.51°, θ2 = +137.49°, 0.47 cm en cuatro iteraciones."""
        r = fabrik_v1.iterar(T_EJEMPLO, CADENA_NEUTRA, 0.0, 0.0)
        self.assertAlmostEqual(r.theta1, -81.51, delta=0.005)
        self.assertAlmostEqual(r.theta2, 137.49, delta=0.005)
        self.assertAlmostEqual(r.error, 0.47, delta=0.005)
        self.assertEqual(r.iteraciones, 4)

    def test_v1_recorte_al_final(self):
        """6.3: recortar al final deja el brazo plegado sobre sí mismo, a 12.66 cm."""
        r = fabrik_v1.iterar(T_EJEMPLO, CADENA_NEUTRA, 0.0, 0.0)
        t1, _ = plano.ajustar_angulo(r.theta1, 0, 180)
        t2, _ = plano.ajustar_angulo(r.theta2, -180, 0)
        self.assertEqual((t1, t2), (0, -180))
        p5 = cinematica.cinematica_directa((T0_EJEMPLO, t1, t2, 0, 90)).puntos[5]
        self.assertAlmostEqual(np.linalg.norm(p5 - OBJ), 12.66, delta=0.005)

    def test_v1_rechaza_el_ejemplo(self):
        """v1 deja el codo abajo en el ejemplo y el control final lo rechaza."""
        self.assertFalse(fabrik_v1.resolver(OBJ, 0.0, 90.0, 0.0, 0.0).valida)


if __name__ == "__main__":
    unittest.main()
