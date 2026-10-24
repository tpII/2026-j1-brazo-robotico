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
        """Inv. 4.5."""
        r = experimentos.reglas_esferas.calcular()
        self.assertEqual(r["n"], 100000)
        self.assertEqual((r["pares"]["(0, 2)"]["choques"], r["pares"]["(1, 3)"]["choques"]), (0, 9803))
        self.assertEqual((r["pares"]["(0, 2)"]["sin_detectar"], r["pares"]["(1, 3)"]["sin_detectar"]), (0, 504))
        self.assertAlmostEqual(r["dist_1_4"], 0.944, delta=0.0005)   # el par (1, 4) ya puede chocar; la matriz lo revisa

    def test_tabla_7_y_tabla_10(self):
        """Impl. Tabla 7, Tabla 10 y Fig. 9."""
        r = experimentos.medicion_versiones.calcular()
        esperado = {"v1": (723, 1383), "v1+espejo": (1947, 1902), "v2": (823, 1451), "v3": (1980, 1980)}
        for version, (cero, azar) in esperado.items():
            self.assertEqual((r[version]["cero"]["resueltos"], r[version]["azar"]["resueltos"]), (cero, azar), version)
        self.assertAlmostEqual(r["v3"]["cero"]["iteraciones"], 3.06, delta=0.005)   # «3.1 iteraciones»

    def test_seccion_9_y_k_max(self):
        """Impl. 9 y Tabla 8 (k_max)."""
        r = experimentos.medicion_busqueda.calcular()
        self.assertEqual((r["neutra"], r["resueltos"], r["con_muneca"]), (1699, 1995, 83))   # 84.95 % y 99.75 %
        self.assertAlmostEqual(r["posiciones_promedio"], 6.6715, delta=0.0005)  # «6.7 posiciones»
        self.assertEqual((r["posiciones_mediana"], r["posiciones_maximo"]), (1.0, 325))
        self.assertAlmostEqual(r["iteraciones_promedio"], 24.0605, delta=0.0005)  # «24.1 iteraciones»
        self.assertEqual(r["iteraciones_mediana"], 2.0)
        self.assertEqual((r["no_llegan"], r["por_kmax"], r["por_eta"]), (10822, 912, 9910))
        self.assertEqual((r["kmax_con_recortes"], r["kmax_tope_codo"]), (643, 415))
        # Cuatro del lado +y, cerca de la base, que no llegan inclinándose hacia atrás, y uno casi sobre el
        # eje de la base, en el borde del alcance.
        self.assertEqual([(f["indice"], f["estado"]) for f in r["fallas"]],
                         [(145, "bloqueado_por_limites"), (696, "bloqueado_por_limites"), (846, "bloqueado_por_limites"),
                          (955, "fuera_de_alcance"), (1816, "bloqueado_por_limites")])
        self.assertAlmostEqual(r["fallas"][3]["rho"], 0.73, delta=0.005)
        e = r["ejemplo"]
        self.assertFalse(e["valida_neutra"])
        self.assertAlmostEqual(e["error_neutra"], 0.68, delta=0.005)
        self.assertEqual((e["estado"], e["muneca"], e["pinza"], e["posiciones"]), ("ok", 0.0, 30.0, 13))
        self.assertAlmostEqual(e["error"], 0.41, delta=0.005)


if __name__ == "__main__":
    unittest.main()
