"""plano.py: lo común a las versiones de FABRIK en el plano (implementación, secciones 5.1 a 5.5 y 8.1)."""
import math
import unittest

import numpy as np

import cinematica
import experimentos.cadena_virtual
import experimentos.formula_ingenua
import plano


class TestPlano(unittest.TestCase):
    """Lo común a las versiones de FABRIK en el plano."""
    def test_cadena_virtual_tabla_5(self):
        """Impl. Tabla 5: l2, α y d según la muñeca y la pinza."""
        filas = {(0, 0): (8.59, -26.27, 0), (0, 90): (12.01, 2.39, 0), (0, 180): (9.07, 31.94, 0),
                 (90, 0): (7.72, 3.72, -4.3)}
        for (t3, t4), (l2, alfa, d) in filas.items():
            c = plano.cadena_virtual(t3, t4)
            self.assertAlmostEqual(c.l2, l2, delta=0.005)
            self.assertAlmostEqual(c.alfa, alfa, delta=0.005)
            self.assertAlmostEqual(c.d, d, delta=1e-9)
            self.assertAlmostEqual(c.l1, 6.9, places=12)
            self.assertAlmostEqual(c.z_hombro, 4.0, places=12)
        filas_experimento = experimentos.cadena_virtual.calcular()["filas"]
        self.assertEqual([(f["theta3"], f["theta4"]) for f in filas_experimento], list(filas))

    def test_cadena_virtual_igual_a_la_formula(self):
        """La fórmula cerrada de la sección 5.3."""
        for t3 in range(-90, 91, 15):
            for t4 in range(0, 181, 15):
                c = plano.cadena_virtual(t3, t4)
                s3, c3 = math.sin(math.radians(t3)), math.cos(math.radians(t3))
                s4, c4 = math.sin(math.radians(t4)), math.cos(math.radians(t4))
                v = (7.7 + 4.3 * s4, 0.5 - 4.3 * c3 * c4)
                self.assertAlmostEqual(c.l2, math.hypot(*v), places=9)
                self.assertAlmostEqual(c.alfa, math.degrees(math.atan2(v[1], v[0])), places=9)
                self.assertAlmostEqual(c.d, -4.3 * s3 * c4, places=9)

    def test_supuestos_del_metodo(self):
        """Los supuestos del método en el plano valen con las medidas del modelo."""
        # Valen con las medidas del brazo armado. Si una medida nueva los rompe, hay que agregar el desvío del
        # brazo a la cadena virtual.
        g = cinematica.cinematica_directa((0, 0, 0, 0, 0))
        np.testing.assert_allclose(g.puntos[1][:2], [0, 0], atol=1e-12)                    # hombro sobre el eje de la base
        np.testing.assert_allclose((g.puntos[2] - g.puntos[1])[1:], [0, 0], atol=1e-12)    # brazo horizontal en la pose cero
        np.testing.assert_allclose(g.ejes[1], [0, -1, 0], atol=1e-12)
        np.testing.assert_allclose(g.ejes[2], [0, -1, 0], atol=1e-12)

    def test_alcance(self):
        """Paso 0: 18.91 cm con la muñeca y la pinza neutras, de 15.49 a 18.92 según la pinza."""
        r = experimentos.cadena_virtual.calcular()
        self.assertAlmostEqual(r["alcance_neutra"], 18.91, delta=0.005)
        self.assertAlmostEqual(r["alcance_min"], 15.49, delta=0.005)
        self.assertAlmostEqual(r["alcance_max"], 18.92, delta=0.005)
        self.assertEqual((r["pinza_min"], r["pinza_max"]), (0.0, 93.5))

    def test_desplazamiento_por_grado(self):
        """Tabla 8 (ε): un grado en el hombro mueve la punta hasta 0.33 cm."""
        r = experimentos.cadena_virtual.calcular()
        self.assertAlmostEqual(r["cm_por_grado"], r["alcance_max"] * math.pi / 180)
        self.assertAlmostEqual(r["cm_por_grado"], 0.33, delta=0.005)

    def test_paso_1_ejemplo(self):
        """Sección 5.2: con p5 en el plano (d = 0), θ0 = -45°, r_c = ρ = 7.07, T = (7.07, 3.00)."""
        objetivo = np.array([5.0, -5.0, 7.0])
        c = plano.cadena_virtual(0, 90)
        (t0, rc), = plano.planos_candidatos(objetivo, c.d)
        self.assertAlmostEqual(t0, -45.0, delta=1e-9)
        self.assertAlmostEqual(rc, 7.07, delta=0.005)
        T = plano.objetivo_en_plano(objetivo, rc, c)
        np.testing.assert_allclose(T, [7.07, 3.00], atol=0.005)

    def test_planos_de_5_5_7(self):
        """El plano de 45° deja la base fuera de rango: queda el de -135°, inclinado hacia atrás."""
        (t0, rc), = plano.planos_candidatos(np.array([5.0, 5.0, 7.0]), 0.0)
        self.assertAlmostEqual(t0, -135.0, delta=1e-9)
        self.assertAlmostEqual(rc, -7.07, delta=0.005)

    def test_planos_degenerados(self):
        """Puntos a revisar, 3."""
        self.assertEqual(plano.planos_candidatos(np.array([0.0, 0.0, 9.0]), -2.0), [])   # ρ < |d|
        for o, d in (((0.0, 0.0, 9.0), 0.0), ((0.0, -2.0, 5.0), -2.0)):                 # r_c = 0
            planos = plano.planos_candidatos(np.array(o), d)
            self.assertTrue(planos)
            for t0, rc in planos:
                self.assertTrue(plano.en_rango(t0, -180, 0))
                self.assertAlmostEqual(rc, 0.0, places=9)

    def test_recorte_angular(self):
        """Sección 8.1: -170° se lleva a 180° con el rango del hombro."""
        self.assertEqual(plano.ajustar_angulo(-170, 0, 180), (180, True))
        self.assertEqual(plano.ajustar_angulo(350, 0, 180), (0, True))
        self.assertEqual(plano.ajustar_angulo(90, 0, 180), (90, False))

    def test_en_rango_con_tolerancia(self):
        """en_rango acepta los topes con una tolerancia y rechaza lo que se pasa."""
        self.assertTrue(plano.en_rango(180 + 1e-7, 0, 180))
        self.assertTrue(plano.en_rango(-1e-7, 0, 180))
        self.assertFalse(plano.en_rango(181, 0, 180))

    def test_angulos_desde_puntos(self):
        """Paso 3 con los puntos finales del ejemplo."""
        t1, t2 = plano.angulos_desde_puntos(np.array([1.025925, 3.136468]), np.array([6.980094, 4.029668]), 4.763642)
        self.assertAlmostEqual(t1, 71.89, delta=0.005)
        self.assertAlmostEqual(t2, -68.12, delta=0.005)

    def test_cadena_estirada(self):
        """Dentro del plano, hombro -> p5 apunta a T y mide l1 + l2."""
        c = plano.cadena_virtual(0, 90)
        t0, t1, t2 = plano.cadena_estirada(np.array([25.0, 0.0, c.z_hombro]), c)   # a la altura del hombro
        p5 = cinematica.cinematica_directa((t0, t1, t2, 0, 90)).puntos[5]
        r = p5[0] * math.cos(math.radians(t0)) + p5[1] * math.sin(math.radians(t0))
        self.assertAlmostEqual(p5[2], c.z_hombro, places=9)
        self.assertAlmostEqual(r, c.l1 + c.l2, places=9)
        t0, t1, t2 = plano.cadena_estirada(np.array([0.0, 0.0, 25.0]), c)   # sobre el eje: apunta hacia arriba
        self.assertAlmostEqual(cinematica.cinematica_directa((t0, t1, t2, 0, 90)).puntos[5][2],
                               c.z_hombro + c.l1 + c.l2, places=9)

    def test_formula_ingenua_tabla_4(self):
        """Impl. Tabla 4: la fórmula con el propio eje da siempre (0, 90, 90, 180, 90)."""
        for fila in experimentos.formula_ingenua.calcular()["filas"]:
            np.testing.assert_allclose(fila["resultado"], [0, 90, 90, 180, 90], atol=0.01)


if __name__ == "__main__":
    unittest.main()
