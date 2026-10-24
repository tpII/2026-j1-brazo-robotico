"""trayectoria.py: movimiento sincronizado con límites de velocidad y rampas."""
import unittest

import numpy as np

import cinematica
import colisiones
import servos
import trayectoria

HOME = tuple(servos.home_modelo())
POSE_B = (-30.0, 90.0, -90.0, 0.0, 90.0)
# Extremos libres y camino que choca: la segunda esfera de la pinza toca el piso desde theta4 = 31°.
PAR_QUE_CHOCA = ((-90.0, 40.0, -100.0, -90.0, 0.0), (-90.0, 40.0, -100.0, -90.0, 180.0))


def _choca(pose):
    return colisiones.detectar_colisiones(cinematica.cinematica_directa(pose).esferas).hay_choque


def _pares_al_azar(n, semilla):
    rng = np.random.default_rng(semilla)
    rangos = [servos.rango_modelo(i) for i in range(5)]
    for _ in range(n):
        yield tuple(rng.uniform(*r) for r in rangos), tuple(rng.uniform(*r) for r in rangos)


def _fraccion(a, b, pose):
    """Qué fracción del camino recto de a a b recorrió una pose."""
    a, b, pose = np.array(a), np.array(b), np.array(pose)
    return float(np.dot(pose - a, b - a) / np.dot(b - a, b - a))


class TestTrayectoria(unittest.TestCase):
    """El movimiento seguro entre dos poses."""
    def test_extremos(self):
        """La trayectoria termina justo en la pose final y no repite la inicial."""
        t = trayectoria.planificar(HOME, POSE_B)
        self.assertEqual(t.pasos[-1], POSE_B)
        self.assertNotEqual(t.pasos[0], HOME)

    def test_solo_la_pinza(self):
        """90° a 60 °/s, más una rampa: 1.8 s en 90 pasos de 20 ms."""
        b = HOME[:4] + (90.0,)
        t = trayectoria.planificar(HOME, b)
        self.assertAlmostEqual(t.duracion, 1.8, places=9)
        self.assertEqual(len(t.pasos), 90)

    def test_limites_en_cada_paso_y_duracion_minima(self):
        """Ningún paso pasa los límites de velocidad y el movimiento no es más lento de lo necesario."""
        for a, b in _pares_al_azar(20, 2026):
            t = trayectoria.planificar(a, b)
            poses = [a] + t.pasos
            articulacion = max(np.abs(np.subtract(q, p)).max() for p, q in zip(poses, poses[1:]))
            punto = max(np.linalg.norm(trayectoria.puntos_vigilados(q) - trayectoria.puntos_vigilados(p), axis=1).max()
                        for p, q in zip(poses, poses[1:]))
            self.assertLessEqual(articulacion, 60 * 0.02 + 1e-9)
            self.assertLessEqual(punto, 10 * 0.02 + 1e-9)
            if t.duracion >= 1.0:   # los cortos duran lo mínimo de las rampas (0.6 s)
                self.assertGreaterEqual(max(articulacion / 1.2, punto / 0.2), 0.9)   # no es más lenta de lo necesario

    def test_rampas(self):
        """El movimiento arranca y frena de a poco durante 0.3 s."""
        b = (-170.0, 20.0, -20.0, 60.0, 170.0)
        t = trayectoria.planificar(HOME, b)
        avance = np.diff([0.0] + [_fraccion(HOME, b, p) for p in t.pasos])
        self.assertTrue(np.all(np.diff(avance[:15]) > 0))    # arranca de a poco durante 0.3 s
        self.assertTrue(np.all(np.diff(avance[-15:]) < 0))   # y frena de a poco
        np.testing.assert_allclose(avance[15:-15], avance[15], rtol=1e-9)

    def test_mismo_punto(self):
        """De una pose a la misma, un solo paso y 0 s."""
        t = trayectoria.planificar(HOME, HOME)
        self.assertEqual((t.pasos, t.duracion), ([HOME], 0.0))

    def test_primer_choque(self):
        """primer_choque encuentra el paso que choca en un camino con los extremos libres."""
        self.assertIsNone(trayectoria.primer_choque(trayectoria.planificar(HOME, POSE_B).pasos))
        a, b = PAR_QUE_CHOCA
        self.assertFalse(_choca(a) or _choca(b))
        pasos = trayectoria.planificar(a, b).pasos
        i = trayectoria.primer_choque(pasos)
        self.assertIsNotNone(i)
        self.assertTrue(_choca(pasos[i]))
        self.assertFalse(any(_choca(p) for p in pasos[:i]))


if __name__ == "__main__":
    unittest.main()
