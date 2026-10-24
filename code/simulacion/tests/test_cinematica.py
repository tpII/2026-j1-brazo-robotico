"""cinematica.py: misma cadena que calcular_configuracion_modular (cinematica.c)."""
import json
import unittest
from contextlib import contextmanager
from pathlib import Path

import numpy as np

import cinematica
import configuracion as cfg
from algebra_lineal import matriz_traslacion
from test_configuracion import saltos_del_firmware

REFERENCIA = json.loads((Path(__file__).resolve().parent.parent / "datos" / "referencia_firmware_2025.json")
                        .read_text(encoding="utf-8"))


@contextmanager
def con_saltos_del_firmware():
    """La cinemática con los saltos de configuracion.c, los que usó el firmware de 2025.

    Los datos de referencia salieron de ese firmware, que todavía tiene las medidas de J5. Las
    esferas locales son las mismas, así que con esos saltos se prueba la cadena de cinematica.c.
    """
    original = cinematica.T_SALTO
    cinematica.T_SALTO = [matriz_traslacion(*salto) for salto in saltos_del_firmware()]
    try:
        yield
    finally:
        cinematica.T_SALTO = original


class TestCinematica(unittest.TestCase):
    """La cinemática directa, comparada con el firmware y con los informes."""
    def test_pose_cero_tabla_1(self):
        """Impl. Tabla 1: puntos y ejes de la pose cero."""
        g = cinematica.cinematica_directa((0, 0, 0, 0, 0))
        np.testing.assert_allclose(g.puntos, [[0, 0, 0], [0, 0, 4], [6.9, 0, 4], [11.4, 0, 4.5], [14.6, 0, 4.5],
                                              [14.6, 0, 0.2]], atol=1e-12)
        np.testing.assert_allclose(g.ejes, [[0, 0, 1], [0, -1, 0], [0, -1, 0], [-1, 0, 0], [0, -1, 0]], atol=1e-12)

    def test_esferas_iguales_al_firmware(self):
        """Con los saltos de configuracion.c, las esferas coinciden con las de cinematica.c en 402 poses."""
        with con_saltos_del_firmware():
            for pose in REFERENCIA["poses"]:
                g = cinematica.cinematica_directa(pose["angulos"])
                for i in range(5):
                    np.testing.assert_allclose(g.esferas[i], pose["esferas"][i], atol=1e-9)

    def test_p5_es_la_punta_de_la_pinza(self):
        """p5 está a 4.3 cm del eje del servo 4, hacia la pala y en el plano que gira con la pinza."""
        g = cinematica.cinematica_directa((0, 0, 0, 0, 90))
        np.testing.assert_allclose(g.puntos[5], [18.9, 0, 4.5], atol=1e-12)
        for pose in ((-30, 60, -90, 20, 0), (-120, 100, -40, -60, 150)):
            g = cinematica.cinematica_directa(pose)
            mano = g.puntos[5] - g.puntos[4]
            self.assertAlmostEqual(np.linalg.norm(mano), 4.3, places=12)
            self.assertAlmostEqual(np.dot(mano, g.ejes[4]), 0.0, places=12)

    def test_la_pinza_mueve_p5(self):
        """Inclinar la pinza mueve su punta."""
        a = cinematica.cinematica_directa((-30, 60, -90, 20, 0)).puntos[5]
        b = cinematica.cinematica_directa((-30, 60, -90, 20, 120)).puntos[5]
        self.assertGreater(np.linalg.norm(a - b), 1.0)

    def test_con_la_pinza_alineada_la_muneca_no_mueve_p5(self):
        """Con la pinza en 90°, p5 queda sobre el eje de la muñeca y girarla no lo mueve."""
        a = cinematica.cinematica_directa((-30, 60, -90, 20, 90)).puntos[5]
        b = cinematica.cinematica_directa((-30, 60, -90, -70, 90)).puntos[5]
        np.testing.assert_allclose(a, b, atol=1e-12)

    def test_pinza_neutra_a_lo_largo_del_antebrazo(self):
        """Diseño, D12."""
        g = cinematica.cinematica_directa((0, 0, 0, 0, 90))
        np.testing.assert_allclose(g.esferas[4][1], [16.6, 0, 4.5], atol=1e-12)

    def test_marcos(self):
        """Rotación y origen de cada eslabón, con los que la página dibuja el brazo."""
        g = cinematica.cinematica_directa((-30, 60, -90, 20, 120))
        self.assertEqual(g.marcos.shape, (5, 3, 4))
        for i in range(5):
            R, origen = g.marcos[i][:, :3], g.marcos[i][:, 3]
            np.testing.assert_allclose(R @ R.T, np.eye(3), atol=1e-12)
            np.testing.assert_allclose(origen, g.puntos[i], atol=1e-12)
            np.testing.assert_allclose(R[:, 2], g.ejes[i], atol=1e-12)
            np.testing.assert_allclose(origen + R @ cfg.SALTO_ESLABONES[i], g.puntos[i + 1], atol=1e-12)
            for local, centro in zip(cfg.esferas_local[i], g.esferas[i]):
                np.testing.assert_allclose(origen + R @ local, centro, atol=1e-12)


if __name__ == "__main__":
    unittest.main()
