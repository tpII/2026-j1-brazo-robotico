"""Los valores guardados en configuracion.py.

Los pulsos y las tablas de calibración son los que se midieron en los servos MG90S del brazo, los
mismos de configuracion.c, aunque estos tests no leen los archivos del firmware. Las medidas del brazo
salen de los STL de las piezas y de la hoja de datos del MG90S, y las esferas van a lo largo del eje.
Los valores que aparecen en docs/investigaciones/implementacion-fabrik.typ se comparan además con el .typ.
"""
import re
import unittest

import numpy as np

import configuracion as cfg
import servos
from experimentos import CARPETA_SIM

TYP = CARPETA_SIM.parent.parent / "docs" / "investigaciones" / "implementacion-fabrik.typ"


class TestConfiguracion(unittest.TestCase):
    """Los valores guardados en configuracion.py."""
    def test_geometria(self):
        """Las 17 esferas de 1.5 cm, la base, la matriz de colisiones, la caja de la pala y el tope del codo."""
        self.assertEqual(cfg.RADIO_ESFERA, 1.5)
        self.assertEqual(cfg.T_BASE_GLOBAL.tolist(), [0.0, 0.0, 0.0])
        self.assertEqual(cfg.MATRIZ_COLISIONES.tolist(), [[0, 0, 1, 1, 1], [0, 0, 0, 1, 1], [1, 0, 0, 0, 0],
                                                          [1, 1, 0, 0, 0], [1, 1, 0, 0, 0]])
        esferas = [[[0.0, 0.0, 0.0], [0.0, 0.0, 1.145], [0.0, 0.0, 2.29], [0.0, 0.0, 3.435]],
                   [[0.0, 0.0, 0.0], [1.25, 0.0, 0.0], [2.5, 0.0, 0.0], [3.75, 0.0, 0.0], [5.0, 0.0, 0.0]],
                   [[0.0, 0.0, 0.0], [1.4725, 0.0, 0.125], [2.945, 0.0, 0.25], [4.4175, 0.0, 0.375]],
                   [[0.0, 0.0, 0.0]],
                   [[0.0, 0.0, 0.0], [0.0, -1.415, 0.0], [0.0, -2.83, 0.0]]]
        self.assertEqual([e.tolist() for e in cfg.esferas_local], esferas)
        caja = ((-0.80, 0.80), (-4.33, 0.80), (-2.19, 1.56))
        self.assertEqual(cfg.ESQUINAS_PALA.tolist(), [[x, y, z] for x in caja[0] for y in caja[1] for z in caja[2]])
        self.assertEqual(cfg.TOPE_CODO, -139.0)

    def test_esferas_a_lo_largo_del_eje(self):
        """Una esfera en cada articulación y otra cada 1.5 cm como máximo, sobre el salto de cada eslabón.

        En la pala la última queda a un radio de la punta.
        """
        for i, (centros, salto) in enumerate(zip(cfg.esferas_local, cfg.SALTO_ESLABONES)):
            largo = np.linalg.norm(salto)
            t = centros @ salto / largo   # distancia de cada centro a la articulación, a lo largo del salto
            np.testing.assert_allclose(np.outer(t / largo, salto), centros, atol=1e-12)
            self.assertEqual(t[0], 0.0)
            if i < 4:
                separaciones = np.diff(np.append(t, largo))
            else:
                self.assertAlmostEqual(t[-1], largo - cfg.RADIO_ESFERA, places=12)
                separaciones = np.diff(t)
            self.assertTrue(np.all(separaciones > 0), i)
            self.assertLessEqual(separaciones.max(), 1.5 + 1e-12, i)

    def test_saltos_son_las_medidas_del_brazo(self):
        """Los saltos son las medidas del brazo, sacadas de los STL y de la hoja de datos del MG90S."""
        medidas = [[0.0, 0.0, 4.58], [6.25, 0.0, 0.0], [5.89, 0.0, 0.5], [0.0, 0.0, -1.29], [0.0, -4.33, 0.0]]
        self.assertEqual([salto.tolist() for salto in cfg.SALTO_ESLABONES], medidas)

    def test_pulsos_y_tablas_de_los_mg90s(self):
        """Los pulsos y las tablas de calibración medidos en los MG90S, con el brazo en L a 1500 us."""
        self.assertEqual(cfg.SERVO_MIN_US, (550, 550, 550, 550, 450))
        self.assertEqual(cfg.SERVO_MAX_US, (2450, 2450, 2500, 2450, 2400))
        self.assertEqual(cfg.SERVO_HOME_US, (550, 2006, 1199, 1550, 450))
        pulsos = [[550, 1000, 1500, 2050, 2450], [550, 1000, 1500, 1950, 2450], [550, 950, 1500, 2050, 2500],
                  [550, 1050, 1500, 2000, 2450], [450, 900, 1500, 2000, 2400]]
        self.assertEqual([[us for us, _ in lut] for lut in cfg.LUT_POR_SERVO], pulsos)
        grados = [0.0, 45.0, 90.0, 135.0, 180.0]
        self.assertEqual([[s for _, s in lut] for lut in cfg.LUT_POR_SERVO],
                         [grados, grados, grados[::-1], grados, grados])

    def test_limites_en_los_extremos_de_cada_tabla(self):
        """Los límites de cada servo son los extremos de su tabla de calibración."""
        for i, lut in enumerate(cfg.LUT_POR_SERVO):
            self.assertEqual((cfg.SERVO_MIN_US[i], cfg.SERVO_MAX_US[i]), (lut[0][0], lut[-1][0]), i)

    def test_conversion_por_servo(self):
        """Cada servo usa la conversión de signos que tenía el firmware del grupo J5."""
        self.assertEqual(cfg.SIGNO_MODELO, (-1, 1, -1, 1, 1))
        self.assertEqual(cfg.DESVIO_MODELO, (0.0, 0.0, 0.0, -90.0, 0.0))

    def test_parametros_de_los_informes(self):
        """Las tolerancias, la búsqueda y los límites de velocidad son los de los informes y el diseño."""
        self.assertEqual((cfg.EPSILON, cfg.ETA, cfg.K_MAX), (0.5, 1e-4, 20))
        self.assertEqual((cfg.PASO_BUSQUEDA, cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA), (10.0, 0.0, 90.0))
        self.assertEqual((cfg.DT, cfg.VEL_MAX_ARTICULACION, cfg.VEL_MAX_PUNTO, cfg.T_RAMPA), (0.02, 60.0, 10.0, 0.3))


class TestValoresDelInforme(unittest.TestCase):
    """configuracion.py frente a los valores escritos en implementacion-fabrik.typ."""
    @classmethod
    def setUpClass(cls):
        cls.typ = TYP.read_text(encoding="utf-8")

    def _buscar(self, patron):
        m = re.search(patron, self.typ, flags=re.S)
        self.assertIsNotNone(m, f"no se encontró en el .typ: {patron}")
        return m

    def test_home(self):
        """Los pulsos del home y sus ángulos del modelo, como en la sección de la cadena."""
        m = self._buscar(r"Lleva los servos a (\d+), (\d+), (\d+), (\d+) y (\d+) µs, que en ángulos del modelo "
                         r"son \$\((.*?)\)\$")
        self.assertEqual(tuple(int(v) for v in m.groups()[:5]), cfg.SERVO_HOME_US)
        angulos = [float(v) for v in re.findall(r"-?\d+(?:\.\d+)?", m.group(6))]
        for a, b in zip(servos.home_modelo(), angulos, strict=True):
            self.assertAlmostEqual(a, b, delta=0.05)   # el .typ tiene un decimal

    def test_limites_y_conversion(self):
        """Cada fila de la tabla de límites: pulsos, rango del servo, conversión y rango del modelo."""
        filas = re.findall(r"\[\w+ \((\d)\)\], \[(\d+) a (\d+) µs\], \[\$(\d+)\$ a \$(\d+)°\$\], "
                           r"\[\$theta_\d = (-?)s_\d(?: - (\d+)°)?\$\], \[\$(-?\d+)°\$ a \$(-?\d+)°\$\]", self.typ)
        self.assertEqual([int(f[0]) for f in filas], [0, 1, 2, 3, 4])
        for i, minimo, maximo, s_min, s_max, signo, desvio, m_min, m_max in filas:
            i = int(i)
            self.assertEqual((cfg.SERVO_MIN_US[i], cfg.SERVO_MAX_US[i]), (int(minimo), int(maximo)), i)
            self.assertEqual(servos.rango_servo(i), (float(s_min), float(s_max)), i)
            self.assertEqual(cfg.SIGNO_MODELO[i], -1 if signo else 1, i)
            self.assertEqual(cfg.DESVIO_MODELO[i], -float(desvio) if desvio else 0.0, i)
            self.assertEqual(servos.rango_modelo(i), (float(m_min), float(m_max)), i)

    def test_medidas_del_brazo(self):
        """La altura del hombro, el brazo, el antebrazo, su desvío hacia un costado y la punta de la pinza."""
        m = self._buscar(r"El eje del hombro está a ([\d.]+) cm de la mesa, el brazo mide ([\d.]+) cm.*?: "
                         r"([\d.]+) cm hasta el horn de la muñeca y ([\d.]+) cm más.*?"
                         r"La punta de la pinza está a ([\d.]+) cm")
        hombro, brazo, antebrazo, muneca, punta = (float(v) for v in m.groups())
        desvio = float(self._buscar(r"hay [\d.]+ cm a lo largo del antebrazo y ([\d.]+) cm hacia un costado").group(1))
        s = [salto.tolist() for salto in cfg.SALTO_ESLABONES]
        self.assertEqual((s[0][2], s[1][0], s[2][0], s[2][2], -s[3][2], -s[4][1]),
                         (hombro, brazo, antebrazo, desvio, muneca, punta))

    def test_criterios_de_corte(self):
        """epsilon, eta y k_max de la tabla de criterios de corte."""
        epsilon = float(self._buscar(r"\[\$epsilon\$ \(tolerancia\)\],\s*\[([\d.]+) cm\]").group(1))
        eta = 10.0 ** int(self._buscar(r"\[\$eta\$ \(sin cambio\)\],\s*\[\$10\^\((-?\d+)\)\$ cm\]").group(1))
        k_max = int(self._buscar(r"\[\$k_max\$ \(iteraciones\)\],\s*\[(\d+)\]").group(1))
        self.assertEqual((cfg.EPSILON, cfg.ETA, cfg.K_MAX), (epsilon, eta, k_max))

    def test_busqueda(self):
        """La muñeca y la pinza neutras y el paso de la búsqueda."""
        m = self._buscar(r"con la muñeca y la pinza neutras \(\$theta_3 = (-?\d+)°\$, \$theta_4 = (\d+)°\$\)")
        self.assertEqual((cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA), (float(m.group(1)), float(m.group(2))))
        paso = float(self._buscar(r"prueba otras posiciones de la pinza de a (\d+)°").group(1))
        self.assertEqual(cfg.PASO_BUSQUEDA, paso)


if __name__ == "__main__":
    unittest.main()
