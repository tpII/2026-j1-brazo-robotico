"""fabrik.py (v3): límites en cada paso y regla del codo (implementación, secciones 5 a 8)."""
import unittest

import numpy as np

import cinematica
import experimentos.regla_del_codo
import fabrik
import plano
import servos

OBJ = np.array([5.0, -5.0, 7.0])
HOME = servos.home_modelo()


def _objetivos(n, semilla):
    """Objetivos alcanzables: ángulos al azar y p5 con la cinemática directa, con la muñeca y la pinza."""
    rng = np.random.default_rng(semilla)
    for _ in range(n):
        th = [rng.uniform(-180, 0), rng.uniform(0, 180), rng.uniform(-180, 0), rng.uniform(-90, 90),
              rng.uniform(0, 180)]
        yield cinematica.cinematica_directa(th).puntos[5], th[3], th[4]


class TestFabrik(unittest.TestCase):
    """FABRIK v3, la versión adoptada."""
    def _corrida_ejemplo(self):
        c = plano.cadena_virtual(0, 90)
        (t0, rc), = plano.planos_candidatos(OBJ, c.d)
        return fabrik.iterar(plano.objetivo_en_plano(OBJ, rc, c), c, 0.0, 0.0, traza=True)

    def test_traza_tabla_6(self):
        """Impl. Tabla 6: la traza de las iteraciones del ejemplo."""
        r = self._corrida_ejemplo()
        filas = [   # codo después del forward, codo espejado, codo y punta después del backward, error
            (None, None, [6.90, 0.00], [18.90, 0.50], 12.09),
            ([6.39, -8.99], [-2.03, 10.84], [-1.27, 6.78], [9.67, 1.82], 2.85),
            ([-3.87, 7.96], None, [-3.01, 6.21], [8.43, 2.57], 1.43),
            ([-4.37, 6.64], None, [-3.80, 5.76], [7.84, 2.80], 0.80),
            ([-4.57, 5.96], None, [-4.20, 5.48], [7.53, 2.90], 0.47),
        ]
        self.assertEqual(len(r.traza), len(filas))
        for paso, (forward, espejado, codo, punta, error) in zip(r.traza, filas):
            for valor, esperado in ((paso.codo_forward, forward), (paso.codo_espejado, espejado)):
                if esperado is None:
                    self.assertIsNone(valor)
                else:
                    np.testing.assert_allclose(valor, esperado, atol=0.005)
            np.testing.assert_allclose(paso.codo, codo, atol=0.005)
            np.testing.assert_allclose(paso.punta, punta, atol=0.005)
            self.assertAlmostEqual(paso.error, error, delta=0.005)
        self.assertEqual((r.iteraciones, r.motivo), (4, "eps"))

    def test_ejemplo_seccion_5_6(self):
        """Impl. 5.6: los ángulos y la punta del ejemplo."""
        s = fabrik.resolver(OBJ, 0.0, 90.0, 0.0, 0.0)
        self.assertTrue(s.valida)
        np.testing.assert_allclose(s.angulos[:3], [-45.0, 127.49, -142.26], atol=0.005)
        np.testing.assert_allclose(cinematica.cinematica_directa(s.angulos).puntos[5], [5.33, -5.33, 6.90], atol=0.005)
        self.assertAlmostEqual(s.error, 0.47, delta=0.005)

    def test_fk_de_ik_y_rangos(self):
        """Con objetivos al azar, la cinemática directa de la solución llega al objetivo y respeta los rangos."""
        validas = 0
        for o, t3, t4 in _objetivos(50, 2026):
            s = fabrik.resolver(o, t3, t4, 0.0, 0.0)
            for p in s.planos:
                if p.corrida:   # v3 recorta en cada paso: el hombro y el codo nunca salen de rango
                    self.assertTrue(plano.en_rango(p.corrida.theta1, 0, 180))
                    self.assertTrue(plano.en_rango(p.corrida.theta2, -180, 0))
            if s.valida:
                validas += 1
                self.assertLessEqual(np.linalg.norm(cinematica.cinematica_directa(s.angulos).puntos[5] - o), 0.5 + 1e-9)
        self.assertGreaterEqual(validas, 48)

    def test_fuera_de_alcance(self):
        """El plano existe, pero ‖T‖ = 19.80 > l1 + l2."""
        s = fabrik.resolver(np.array([14.0, -14.0, 4.0]), 0.0, 90.0, 0.0, 0.0)
        self.assertFalse(s.valida)
        self.assertTrue(s.planos)
        self.assertTrue(all(not p.alcanzable for p in s.planos))

    def test_mejor_intento_5_5_7(self):
        """El plano de -135° no llega por los límites y guarda la iteración de menor error."""
        s = fabrik.resolver(np.array([5.0, 5.0, 7.0]), 0.0, 90.0, HOME[1], HOME[2])
        c = s.planos[0].corrida
        self.assertFalse(s.valida)
        self.assertAlmostEqual(s.planos[0].theta0, -135.0, delta=1e-9)
        self.assertAlmostEqual(c.mejor_error, 9.0055, delta=0.0005)
        self.assertAlmostEqual(c.mejor_theta1, 180.0, delta=0.005)
        self.assertAlmostEqual(c.mejor_theta2, -89.12, delta=0.005)
        self.assertEqual((c.motivo, c.iteraciones), ("eta", 2))

    def test_regla_del_codo_seccion_7_3(self):
        """Impl. 7.3: las fronteras de la regla del codo y el espejo módulo 360°."""
        r = experimentos.regla_del_codo.calcular()
        self.assertAlmostEqual(r["frontera_pinza"], 83.32, delta=0.005)   # cos θ4 = 0.5 / 4.3
        self.assertLess(r["max_dif_s"], 1e-9)
        self.assertEqual([caso["theta4"] for caso in r["espejos"]], [90.0, 60.0])
        for caso in r["espejos"]:   # θ4 = 90 con θ2 = -2 y θ4 = 60 con θ2 = -176
            self.assertLess(caso["s_antes"], 0)
            self.assertAlmostEqual(caso["theta2_espejo"], caso["formula"], places=6)
            self.assertTrue(caso["valido"])
            self.assertLess(caso["dif_punta"], 1e-9)


if __name__ == "__main__":
    unittest.main()
