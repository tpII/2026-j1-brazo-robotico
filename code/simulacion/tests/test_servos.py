"""servos.py: pulso <-> grados del servo <-> grados del modelo, igual que firmware.ino."""
import unittest

import numpy as np

import experimentos.pose_cero_y_home
import servos


class TestServos(unittest.TestCase):
    """Pulsos, grados del servo y grados del modelo."""
    def test_pulso_a_servo_como_get_angle_for_servo(self):
        """pulso_a_servo da lo mismo que getAngleForServo, con recorte en los extremos."""
        self.assertAlmostEqual(servos.pulso_a_servo(1, 1400), 140.0, places=9)
        self.assertAlmostEqual(servos.pulso_a_servo(2, 1200), 114.6049, places=4)
        self.assertAlmostEqual(servos.pulso_a_servo(3, 960), 94.5, places=9)
        self.assertEqual(servos.pulso_a_servo(1, 300), 0.0)      # recorte abajo
        self.assertEqual(servos.pulso_a_servo(1, 1900), 180.0)   # recorte arriba
        self.assertEqual(servos.pulso_a_servo(2, 2600), 0.0)     # LUT2 invertida

    def test_ida_y_vuelta(self):
        """servo_a_pulso es la inversa de pulso_a_servo."""
        for i in range(5):
            for s in range(0, 181, 15):
                self.assertAlmostEqual(servos.pulso_a_servo(i, servos.servo_a_pulso(i, s)), s, places=9)

    def test_conversion_al_modelo(self):
        """La conversión entre grados del servo y del modelo, y su inversa."""
        self.assertEqual(servos.servo_a_modelo(0, 30.0), -30.0)
        self.assertEqual(servos.servo_a_modelo(3, 94.5), 4.5)
        for i in range(5):
            self.assertAlmostEqual(servos.modelo_a_servo(i, servos.servo_a_modelo(i, 37.0)), 37.0)

    def test_rangos_del_modelo(self):
        """Los rangos en grados del modelo."""
        self.assertEqual([servos.rango_modelo(i) for i in range(5)],
                         [(-180.0, 0.0), (0.0, 180.0), (-180.0, 0.0), (-90.0, 90.0), (0.0, 180.0)])

    def test_home(self):
        """El home en grados del modelo."""
        esperado = (0.0, 140.0, -114.6049, 4.5, 0.0)
        for a, b in zip(servos.home_modelo(), esperado):
            self.assertAlmostEqual(a, b, places=4)

    def test_pulsos_de_home(self):
        """Los pulsos del home son los de firmware.ino."""
        self.assertEqual(servos.pulsos_de(servos.home_modelo()), [400, 1400, 1200, 960, 400])

    def test_redondeo_hacia_arriba_en_el_medio(self):
        """Un pulso con medio us se redondea hacia arriba, como lround."""
        s = servos.pulso_a_servo(4, 400.5)
        self.assertEqual(servos.pulsos_de((0.0, 90.0, -90.0, 0.0, s))[4], 401)

    def test_experimento_pose_cero_y_home(self):
        """El home y los rangos de cada servo."""
        r = experimentos.pose_cero_y_home.calcular()
        np.testing.assert_allclose(r["home"], (0.0, 140.0, -114.6, 4.5, 0.0), atol=0.005)
        self.assertEqual([s["pulsos"] for s in r["servos"]],
                         [(400, 2600), (400, 1800), (400, 2600), (400, 2000), (400, 2600)])
        self.assertEqual([s["rango_servo"] for s in r["servos"]], [(0.0, 180.0)] * 5)
        self.assertEqual([s["rango_modelo"] for s in r["servos"]],
                         [(-180.0, 0.0), (0.0, 180.0), (-180.0, 0.0), (-90.0, 90.0), (0.0, 180.0)])


if __name__ == "__main__":
    unittest.main()
