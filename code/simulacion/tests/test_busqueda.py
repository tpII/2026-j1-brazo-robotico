"""busqueda.py: muñeca y pinza, planos, silueta y resultado (implementación, secciones 8.3 y 9; diseño §6.7)."""
import unittest

import numpy as np

import busqueda
import cinematica
import colisiones
import experimentos
import experimentos.cadena_recta
import servos
import trayectoria

HOME = servos.home_modelo()
ESTADOS = {"ok", "fuera_de_alcance", "bloqueado_por_limites", "colision"}
OBJETIVOS = experimentos.objetivos_medicion()
# Índices de OBJETIVOS hallados recorriéndolos en orden con la búsqueda (tarea 13 del plan).
INDICE_RESCATADO = 9   # el primero cuya primera solución choca, pero otra pose válida no
INDICE_COLISION = 2    # el primero en que todas sus poses válidas chocan
# Extremos libres y camino que choca (tests/test_trayectoria.py).
PAR_QUE_CHOCA = ((-90.0, 40.0, -100.0, -90.0, 0.0), (-90.0, 40.0, -100.0, -90.0, 180.0))


def _choca(pose):
    return colisiones.detectar_colisiones(cinematica.cinematica_directa(pose).esferas).hay_choque


class TestBusquedaSinChoques(unittest.TestCase):
    """La búsqueda sin revisar choques, el modo con el que se midieron los informes."""
    def test_posiciones(self):
        """Las 361 posiciones en orden: la neutra, la pinza de a 10° y después cada muñeca con cada pinza."""
        p = busqueda.posiciones_busqueda()
        self.assertEqual(len(p), 361)
        self.assertEqual(p[:3], [(0.0, 90.0), (0.0, 100.0), (0.0, 80.0)])
        self.assertEqual(p[17:22], [(0.0, 180.0), (0.0, 0.0), (10.0, 90.0), (10.0, 100.0), (10.0, 80.0)])
        self.assertEqual(p[37:39], [(10.0, 0.0), (-10.0, 90.0)])
        self.assertEqual(p[-1], (-90.0, 0.0))

    def test_muneca_con_la_pinza(self):
        """Del lado +y la base no apunta al objetivo: llega con la muñeca girada y la pinza inclinada."""
        r = busqueda.buscar(np.array([6.0, 1.0, 6.0]), HOME, revisar_colisiones=False)
        self.assertEqual(r.estado, "ok")
        self.assertEqual(r.angulos[3:], (20.0, 140.0))
        self.assertLessEqual(r.error, 0.5)

    def test_ejemplo_3_3_0(self):
        """Impl. 9: sobre el piso, cerca de la base; se resuelve con la pinza en 30°, a 0.41 cm."""
        r = busqueda.buscar(np.array([3.0, -3.0, 0.0]), HOME, revisar_colisiones=False)
        self.assertEqual((r.estado, r.posiciones, r.iteraciones), ("ok", 13, 19))
        np.testing.assert_allclose(r.angulos, [-45.0, 72.0, -132.58, 0.0, 30.0], atol=0.005)
        self.assertAlmostEqual(r.error, 0.41, delta=0.005)

    def test_bloqueado_5_5_7(self):
        """La silueta es la iteración de menor error de todas."""
        r = busqueda.buscar(np.array([5.0, 5.0, 7.0]), HOME, revisar_colisiones=False)
        self.assertEqual(r.estado, "bloqueado_por_limites")
        self.assertAlmostEqual(r.silueta.error, 4.452, delta=0.0005)
        np.testing.assert_allclose(r.silueta.angulos, [-97.55, 180.0, -116.93, 90.0, 180.0], atol=0.005)

    def test_fuera_de_alcance(self):
        """La silueta es la cadena estirada con la muñeca y la pinza neutras."""
        r = busqueda.buscar(np.array([20.0, 0.0, 3.0]), HOME, revisar_colisiones=False)
        self.assertEqual(r.estado, "fuera_de_alcance")
        self.assertEqual(r.silueta.angulos[3:], (0.0, 90.0))

    def test_cadena_recta(self):
        """Impl. 7.3 y Tabla 8 (η); resultados/cadena_recta.txt."""
        casos = experimentos.cadena_recta.calcular()["casos"]
        esperado = [(0.0, 90.0, 1, (-45.0, 157.48, -155.92), 0.455),
                    (10.0, 150.0, 30, (-114.99, 178.93, -177.58), 0.478),
                    (0.0, 90.0, 1, (-20.0, 116.57, -141.24), 0.372)]
        for caso, (t3, t4, posiciones, angulos, error) in zip(casos, esperado):
            self.assertEqual((caso["muneca"], caso["pinza"], caso["posiciones"]), (t3, t4, posiciones))
            np.testing.assert_allclose(caso["angulos"][:3], angulos, atol=0.005)
            self.assertAlmostEqual(caso["error"], error, delta=0.005)
        self.assertEqual([c["motivo_neutra"] for c in casos], ["eps", "eta", "eps"])

    def test_objetivos_sobre_el_eje(self):
        """Puntos a revisar, 3."""
        for o in ((0, 0, 9), (0, 0, 12), (0, 0, 20), (0, -2, 5)):
            self.assertIn(busqueda.buscar(np.array(o, float), HOME, revisar_colisiones=False).estado, ESTADOS)


class TestBusquedaConChoques(unittest.TestCase):
    """La búsqueda con choques (D1 y D8 del diseño)."""

    def test_rescate(self):
        """Si la primera solución choca, la búsqueda sigue hasta otra pose que no choca (D1)."""
        o = OBJETIVOS[INDICE_RESCATADO]["objetivo"]
        sin = busqueda.buscar(o, HOME, revisar_colisiones=False)
        con = busqueda.buscar(o, HOME)
        self.assertTrue(_choca(sin.angulos))
        self.assertEqual(con.estado, "ok")
        self.assertFalse(_choca(con.angulos))

    def test_estado_colision(self):
        """Si todas las poses que llegan chocan, el estado es «colision» y la silueta marca las esferas."""
        r = busqueda.buscar(OBJETIVOS[INDICE_COLISION]["objetivo"], HOME)
        self.assertEqual(r.estado, "colision")
        self.assertTrue(r.silueta.colision.esferas)
        self.assertTrue(_choca(r.silueta.angulos))

    def test_revisar_pose(self):
        """Una pose libre se descarta si el camino desde la pose actual choca (D8)."""
        a, b = PAR_QUE_CHOCA   # b está libre, pero el camino desde a choca
        pasos = trayectoria.planificar(a, b).pasos
        s = busqueda.revisar_pose(b, a)
        self.assertEqual(s.angulos, pasos[trayectoria.primer_choque(pasos)])
        self.assertTrue(s.colision.esferas)
        self.assertIsNone(busqueda.revisar_pose(HOME[:4] + (90.0,), HOME))
        pose_que_choca = busqueda.buscar(OBJETIVOS[INDICE_RESCATADO]["objetivo"], HOME, revisar_colisiones=False).angulos
        self.assertEqual(busqueda.revisar_pose(pose_que_choca, HOME).angulos, pose_que_choca)

    def test_ok_siempre_libre(self):
        """Cuando el estado es «ok», ningún paso de la trayectoria choca."""
        for item in OBJETIVOS[:50]:
            r = busqueda.buscar(item["objetivo"], HOME)
            if r.estado == "ok":
                self.assertIsNone(trayectoria.primer_choque(r.pasos))
                self.assertEqual(r.pasos[-1], r.angulos)

    def test_objetivos_sobre_el_eje_con_choques(self):
        """Puntos a revisar, 3."""
        for o in ((0, 0, 9), (0, 0, 12), (0, 0, 20), (0, -2, 5)):
            self.assertIn(busqueda.buscar(np.array(o, float), HOME).estado, ESTADOS)


if __name__ == "__main__":
    unittest.main()
