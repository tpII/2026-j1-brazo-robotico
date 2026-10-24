"""centralizador.py: el «centralizador de lógica» del Plan (diseño §6.9; D10 y D14)."""
import json
import unittest

import numpy as np

import centralizador

OBJ_OK = (5.0, -5.0, 7.0)   # llega con los choques activados (plan, «Estado»)


def _nuevo():
    c = centralizador.Centralizador()
    c.iniciar()
    return c


def _hasta_quieto(c):
    while c.telemetria()["estado"] == "moviendo":
        c.avanzar()


class TestCentralizador(unittest.TestCase):
    """El centralizador: pedidos, pasos, Detener y telemetría."""
    def test_iniciar(self):
        """Arranca quieto, con los pulsos del home y sin resultado."""
        t = _nuevo().telemetria()
        self.assertEqual((t["estado"], t["pulsos_us"], t["resultado"]), ("quieto", [400, 1400, 1200, 960, 400], None))

    def test_pedir_y_llegar(self):
        """Un objetivo alcanzable mueve el brazo paso a paso hasta que queda quieto."""
        c = _nuevo()
        r = c.pedir_objetivo(*OBJ_OK)
        self.assertEqual((r["resultado"], c.telemetria()["estado"]), ("ok", "moviendo"))
        n = 0
        while c.telemetria()["estado"] == "moviendo":
            c.avanzar()
            n += 1
        self.assertEqual(c.telemetria()["estado"], "quieto")
        self.assertEqual(n, len(c._pasos))

    def test_ocupado(self):
        """D10."""
        c = _nuevo()
        c.pedir_objetivo(*OBJ_OK)
        c.avanzar()
        self.assertEqual(c.pedir_objetivo(5.0, 5.0, 7.0)["resultado"], "ocupado")
        self.assertEqual(c.telemetria()["objetivo"], list(OBJ_OK))
        self.assertEqual(c.telemetria()["resultado"], "ok")   # la telemetría sigue mostrando el movimiento en curso

    def test_detener_y_seguir(self):
        """D14; Puntos a revisar, 5."""
        c = _nuevo()
        c.detener()
        self.assertEqual(c.telemetria()["estado"], "quieto")
        c.pedir_objetivo(*OBJ_OK)
        for _ in range(10):
            c.avanzar()
        c.detener()
        quieta = c.telemetria()["angles"]
        c.avanzar()
        self.assertEqual((c.telemetria()["estado"], c.telemetria()["angles"]), ("detenido", quieta))
        self.assertEqual(c.pedir_objetivo(*OBJ_OK)["resultado"], "ok")   # busca desde donde quedó

    def test_mismo_punto(self):
        """Puntos a revisar, 2."""
        c = _nuevo()
        c.pedir_objetivo(*OBJ_OK)
        _hasta_quieto(c)
        self.assertEqual(c.pedir_objetivo(*OBJ_OK)["resultado"], "ok")
        _hasta_quieto(c)
        self.assertEqual(c.telemetria()["estado"], "quieto")

    def test_entrada_rechazada(self):
        """Puntos a revisar, 4."""
        c = _nuevo()
        antes = c.telemetria()
        for x, y, z in ((6.0, -3.0, -1.0), (float("nan"), 0.0, 5.0), (0.0, float("inf"), 5.0)):
            with self.assertRaises(ValueError):
                c.pedir_objetivo(x, y, z)
        self.assertEqual(c.telemetria(), antes)

    def test_no_alcanza_no_se_mueve(self):
        """Si no llega, el brazo no se mueve y la telemetría trae la silueta del mejor intento."""
        c = _nuevo()
        antes = c.telemetria()["angles"]
        self.assertEqual(c.pedir_objetivo(5.0, 5.0, 7.0)["resultado"], "bloqueado_por_limites")
        t = c.telemetria()
        self.assertEqual((t["estado"], t["angles"]), ("quieto", antes))
        self.assertIsNotNone(t["silueta"])
        self.assertAlmostEqual(t["silueta"]["error_cm"], 4.452, delta=0.0005)   # «mejor intento a 4.452 cm»
        self.assertEqual(np.shape(t["silueta"]["marcos"]), (5, 3, 4))

    def test_bloqueado_marca_el_servo_en_el_tope(self):
        """En el mejor intento hacia (5, 5, 7) el hombro quedó en su tope de 180°: es el servo que no deja llegar."""
        c = _nuevo()
        c.pedir_objetivo(5.0, 5.0, 7.0)
        self.assertEqual(c.telemetria()["silueta"]["servos_fuera_de_rango"], [False, True, False, False, False])

    def test_fuera_de_alcance_marca_los_angulos_fuera_de_rango(self):
        """La cadena estirada hacia (19, 6, 1) no respeta los límites: la base apunta a 18° y el hombro baja de 0°."""
        c = _nuevo()
        self.assertEqual(c.pedir_objetivo(19.0, 6.0, 1.0)["resultado"], "fuera_de_alcance")
        self.assertEqual(c.telemetria()["silueta"]["servos_fuera_de_rango"], [True, True, False, False, False])

    def test_fuera_de_alcance_en_el_tope_no_se_marca(self):
        """Hacia (25, 0, 4), a la altura del hombro, la base y el hombro quedan en 0°, dentro de su rango."""
        c = _nuevo()
        self.assertEqual(c.pedir_objetivo(25.0, 0.0, 4.0)["resultado"], "fuera_de_alcance")
        self.assertEqual(c.telemetria()["silueta"]["servos_fuera_de_rango"], [False] * 5)

    def test_colision_no_marca_servos(self):
        """Una pose que choca respeta los rangos, aunque el hombro esté en un tope."""
        self.assertEqual(centralizador._servos_fuera_de_rango("colision", (-90.0, 180.0, -90.0, 0.0, 90.0)),
                         [False] * 5)

    def test_telemetria(self):
        """La telemetría tiene las claves de J5 y las nuevas, y se puede pasar a JSON."""
        c = _nuevo()
        c.pedir_objetivo(*OBJ_OK)
        t = c.telemetria()
        json.dumps(t)
        for clave in ("angles", "collision_detected", "colliding_centers", "sphere_radius", "spheres"):   # las de J5
            self.assertIn(clave, t)
        self.assertEqual([len(e) for e in t["spheres"]], [1, 2, 3, 1, 2])
        self.assertEqual(len(t["puntos"]), 6)
        self.assertEqual(np.shape(t["marcos"]), (5, 3, 4))   # para dibujar el brazo 3D
        self.assertIsNotNone(t["pose_final"])
        self.assertEqual(np.shape(t["pose_final"]["marcos"]), (5, 3, 4))
        self.assertIsNone(t["silueta"])
        self.assertEqual(set(t["metricas"]), {"posiciones", "ejecuciones", "iteraciones", "iteraciones_totales", "tiempo_ms"})

    def test_velocidad_en_el_movimiento(self):
        """En cada paso ninguna articulación gira más de 1.2° (60 °/s en 20 ms)."""
        c = _nuevo()
        c.pedir_objetivo(*OBJ_OK)
        antes = c.telemetria()["angles"]
        while c.telemetria()["estado"] == "moviendo":
            c.avanzar()
            ahora = c.telemetria()["angles"]
            self.assertLessEqual(max(abs(a - b) for a, b in zip(ahora, antes)), 1.2 + 1e-9)
            antes = ahora


if __name__ == "__main__":
    unittest.main()
