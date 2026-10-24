"""configuracion.py tiene que ser una copia fiel del firmware (configuracion.c y main.ino).

Los archivos del firmware se leen sin mostrarlos: solo se extraen los arreglos que
hacen falta, y los mensajes de error muestran valores, no líneas.
"""
import re
import unittest
from pathlib import Path

import configuracion as cfg

FIRMWARE = Path(__file__).resolve().parent.parent.parent / "firmware"


def _sin_comentarios(texto):
    texto = re.sub(r"/\*.*?\*/", "", texto, flags=re.S)
    return re.sub(r"//[^\n]*", "", texto)


def _leer(nombre):
    return _sin_comentarios((FIRMWARE / nombre).read_text(encoding="utf-8"))


def _numeros(texto, nombre):
    """Números de `nombre = {...};` o `nombre = valor;`, en orden."""
    m = re.search(re.escape(nombre) + r"\s*=\s*(\{.*?\};|[^;{]*;)", texto, flags=re.S)
    if m is None:
        raise AssertionError(f"no se encontró {nombre} en el firmware")
    return [float(x) for x in re.findall(r"-?\d+(?:\.\d+)?", m.group(1))]


def _compacto(texto):
    return re.sub(r"\s+", "", texto)


def saltos_del_firmware():
    """Los cinco SALTO_i de configuracion.c, que todavía son los de J5, como listas [x, y, z]."""
    c = _leer("configuracion.c")
    return [_numeros(c, f"SALTO_{i}") for i in range(5)]


class TestConfiguracion(unittest.TestCase):
    """configuracion.py frente a los archivos del firmware."""
    @classmethod
    def setUpClass(cls):
        cls.c = _leer("configuracion.c")
        cls.ino = _leer("main.ino")

    def test_geometria_igual_a_configuracion_c(self):
        """Las esferas y la base son las de configuracion.c; los saltos se reemplazan por las medidas."""
        c = self.c
        self.assertEqual(cfg.RADIO_ESFERA, _numeros(c, "RADIO_ESFERA")[0])
        self.assertEqual(cfg.T_BASE_GLOBAL.tolist(), _numeros(c, "T_BASE_GLOBAL"))
        self.assertEqual(cfg.MATRIZ_COLISIONES.flatten().tolist(), _numeros(c, "MATRIZ_COLISIONES[5][5]"))
        for i in range(5):
            self.assertEqual(cfg.esferas_local[i].flatten().tolist(), _numeros(c, f"esf_{i}"))

    def test_saltos_son_las_medidas_del_brazo(self):
        """Los saltos son las medidas del brazo armado (secciones 8 y 9); el firmware todavía tiene los de J5."""
        medidas = [[0.0, 0.0, 4.0], [6.9, 0.0, 0.0], [4.5, 0.5, 0.0], [0.0, 0.0, -3.2], [0.0, -4.3, 0.0]]
        self.assertEqual([salto.tolist() for salto in cfg.SALTO_ESLABONES], medidas)
        j5 = [[0.0, 0.0, 3.0], [3.3, 0.0, 0.0], [4.0, 0.5, 0.0], [0.0, 0.0, -2.0], [0.0, 0.0, 2.0]]
        self.assertEqual(saltos_del_firmware(), j5)

    def test_pulsos_y_tablas_iguales_a_main_ino(self):
        """Los pulsos y las tablas de calibración son los de main.ino."""
        ino = self.ino
        self.assertEqual(list(cfg.SERVO_MIN_US), _numeros(ino, "servoMinUs[SERVO_COUNT]"))
        self.assertEqual(list(cfg.SERVO_MAX_US), _numeros(ino, "servoMaxUs[SERVO_COUNT]"))
        self.assertEqual(list(cfg.SERVO_HOME_US), _numeros(ino, "servoHomePulse[SERVO_COUNT]"))
        for nombre in ("LUT_GENERICO", "LUT1", "LUT2", "LUT3"):
            tabla = [v for par in getattr(cfg, nombre) for v in par]
            self.assertEqual(tabla, _numeros(ino, nombre + "[]"), nombre)

    def test_tabla_y_conversion_por_servo(self):
        """Cada servo usa la tabla y la conversión de signos de main.ino."""
        # getAngleForServo: servo 2 -> LUT2, 1 -> LUT1, 3 -> LUT3, el resto -> LUT_GENERICO
        t = _compacto(self.ino)
        for fragmento in ("if(servo_idx==2){local_lut=LUT2;", "elseif(servo_idx==1){local_lut=LUT1;",
                          "elseif(servo_idx==3){local_lut=LUT3;", "else{local_lut=LUT_GENERICO;",
                          "if(i==3){ang-=90.0f;}", "if(i==0||i==2){ang=-1*ang;}"):
            self.assertIn(fragmento, t)
        self.assertEqual(cfg.LUT_POR_SERVO, (cfg.LUT_GENERICO, cfg.LUT1, cfg.LUT2, cfg.LUT3, cfg.LUT_GENERICO))
        self.assertEqual(cfg.SIGNO_MODELO, (-1, 1, -1, 1, 1))
        self.assertEqual(cfg.DESVIO_MODELO, (0.0, 0.0, 0.0, -90.0, 0.0))

    def test_parametros_de_los_informes(self):
        """Las tolerancias, la búsqueda y los límites de velocidad son los de los informes y el diseño."""
        self.assertEqual((cfg.EPSILON, cfg.ETA, cfg.K_MAX), (0.5, 1e-4, 20))
        self.assertEqual((cfg.PASO_BUSQUEDA, cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA), (10.0, 0.0, 90.0))
        self.assertEqual((cfg.DT, cfg.VEL_MAX_ARTICULACION, cfg.VEL_MAX_PUNTO, cfg.T_RAMPA), (0.02, 60.0, 10.0, 0.3))


if __name__ == "__main__":
    unittest.main()
