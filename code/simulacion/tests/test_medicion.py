"""Los números de los informes que salen de miles de poses u objetivos, y los objetivos mismos.

Los lentos se saltean con: RAPIDO=1 python -m unittest discover -s tests
"""
import os
import unittest

import numpy as np

import cinematica
import experimentos
import experimentos.medicion_busqueda
import experimentos.medicion_versiones
import experimentos.reglas_esferas

LENTO = unittest.skipIf(os.environ.get("RAPIDO") == "1", "test lento (RAPIDO=1)")


class TestObjetivos(unittest.TestCase):
    """Los 2000 objetivos de las mediciones (semilla 12345)."""
    def test_mismos_casos_con_la_pinza_al_azar(self):
        """Los ángulos y las poses de inicio de siempre; la pinza se sortea al final y p5 sale de los cinco ángulos."""
        objetivos = experimentos.objetivos_medicion()
        self.assertEqual(len(objetivos), 2000)
        primero = objetivos[0]
        np.testing.assert_allclose(primero["angulos"][:4], [-139.07951595590947, 57.01650114775552,
                                                            -36.474217680107856, 31.72584073517542], atol=1e-12)
        np.testing.assert_allclose(primero["inicio_azar"], [70.39971910834362, -120.09349298405078], atol=1e-12)
        pinzas = np.array([o["angulos"][4] for o in objetivos])
        self.assertTrue(np.all((pinzas >= 0) & (pinzas <= 180)))
        self.assertGreater(pinzas.std(), 45)   # uniforme en [0°, 180°]: unos 52°
        for o in objetivos[:50]:
            np.testing.assert_allclose(cinematica.cinematica_directa(o["angulos"]).puntos[5], o["objetivo"], atol=1e-12)


@LENTO
class TestMedicion(unittest.TestCase):
    """Los números que salen de miles de poses u objetivos; tardan alrededor de un minuto."""
    def test_reglas_esferas(self):
        """Las reglas de las esferas con 100000 poses al azar."""
        r = experimentos.reglas_esferas.calcular()
        self.assertEqual(r["n"], 100000)
        self.assertEqual(r["revisados"], {"(0, 2)": 7221, "(0, 3)": 14239, "(0, 4)": 16142, "(1, 3)": 16764,
                                          "(1, 4)": 13917})

    def test_versiones(self):
        """Cuántos objetivos resuelve cada versión y cuántas iteraciones usa la v3."""
        r = experimentos.medicion_versiones.calcular()
        esperado = {"v1": (757, 1410), "v1+espejo": (1951, 1902), "v2": (848, 1487), "v3": (1980, 1975)}
        for version, (cero, azar) in esperado.items():
            self.assertEqual((r[version]["cero"]["resueltos"], r[version]["azar"]["resueltos"]), (cero, azar), version)
        self.assertAlmostEqual(r["v3"]["cero"]["iteraciones"], 3.08, delta=0.005)   # «3.1 iteraciones»

    def test_busqueda_y_k_max(self):
        """La búsqueda sobre los 2000 objetivos y los cortes por k_max."""
        r = experimentos.medicion_busqueda.calcular()
        self.assertEqual((r["neutra"], r["resueltos"], r["con_muneca"]), (1651, 1995, 139))   # 82.55 % y 99.75 %
        self.assertAlmostEqual(r["posiciones_promedio"], 7.571, delta=0.0005)  # «7.6 posiciones»
        self.assertEqual((r["posiciones_mediana"], r["posiciones_maximo"]), (1.0, 325))
        self.assertAlmostEqual(r["iteraciones_promedio"], 23.5295, delta=0.0005)  # «23.5 iteraciones»
        self.assertEqual(r["iteraciones_mediana"], 2.0)
        self.assertEqual((r["no_llegan"], r["por_kmax"], r["por_eta"]), (11068, 898, 10170))
        self.assertEqual((r["kmax_con_recortes"], r["kmax_tope_codo"]), (583, 394))
        # Cinco del lado +y, cerca de la base, que no llegan inclinándose hacia atrás.
        self.assertEqual([(f["indice"], f["estado"]) for f in r["fallas"]],
                         [(145, "bloqueado_por_limites"), (696, "bloqueado_por_limites"), (846, "bloqueado_por_limites"),
                          (847, "bloqueado_por_limites"), (1356, "bloqueado_por_limites")])
        e = r["ejemplo"]
        self.assertFalse(e["valida_neutra"])
        self.assertAlmostEqual(e["error_neutra"], 0.52, delta=0.005)
        self.assertEqual((e["estado"], e["muneca"], e["pinza"], e["posiciones"]), ("ok", 0.0, 110.0, 4))
        self.assertAlmostEqual(e["error"], 0.47, delta=0.005)


if __name__ == "__main__":
    unittest.main()
