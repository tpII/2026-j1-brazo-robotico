"""Módulo central de lógica.

Guarda el estado del brazo, atiende los pedidos de objetivo, avanza un paso del movimiento cada
vez que se lo llama (cada 20 ms) y arma la telemetría que lee la página del gemelo digital.
- Un pedido mientras el brazo se mueve se rechaza con «ocupado»; «Detener» frena el brazo
  en el paso en que está.
- Un objetivo con valores que no son números finitos, o debajo del piso, se rechaza sin buscar:
  p5 no tiene esfera, así que las esferas no lo detectarían.
En C será centralizador.c y la parte del loop() de main.ino que mueve los servos.
Implementación J1.
"""
import dataclasses
import math

import numpy as np

import busqueda
import cinematica
import colisiones
import configuracion as cfg
import plano
import servos


def _dibujo(pose, choque=None):
    """Lo que la página necesita para dibujar una pose.

    Args:
        pose: los cinco ángulos del modelo, en grados.
        choque: la colisiones.Colision de la pose, para marcar las esferas que chocan.

    Returns:
        Un diccionario con los ángulos, los puntos, los marcos, las esferas y las que chocan.
    """
    g = cinematica.cinematica_directa(pose)
    indices = choque.esferas if choque is not None else []
    return {"angles": [float(a) for a in pose], "puntos": g.puntos.tolist(), "marcos": g.marcos.tolist(),
            "spheres": [e.tolist() for e in g.esferas], "colliding_centers": colisiones.centros(g.esferas, indices),
            "esferas_en_choque": [list(i) for i in indices]}


def _servos_fuera_de_rango(estado, pose):
    """Para cada servo, si el ángulo que hacía falta para llegar queda fuera de su rango.

    La cadena estirada de «fuera de alcance» no tiene en cuenta los límites, así que se marcan los
    ángulos que salen del rango. En «bloqueado por límites», FABRIK ya recortó el hombro y el codo
    en cada paso: el que quedó en un tope es el que no deja llegar.
    Una pose que choca respeta los rangos y no marca ninguno.

    Args:
        estado: el resultado de la búsqueda.
        pose: los cinco ángulos de la silueta, en grados del modelo.

    Returns:
        Una lista de cinco booleanos, para que la página pinte en rojo esos ángulos.
    """
    marcas = []
    for i, angulo in enumerate(pose):
        lo, hi = servos.rango_modelo(i)
        if estado == "fuera_de_alcance":
            marcas.append(not plano.en_rango(angulo, lo, hi))
        elif estado == "bloqueado_por_limites":
            marcas.append(i in (1, 2) and min(abs(angulo - lo), abs(angulo - hi)) < 1e-6)
        else:
            marcas.append(False)
    return marcas


class Centralizador:
    """El estado del brazo y lo que se puede pedirle.

    Attributes:
        angulos: la pose actual, en grados del modelo.
        pulsos: los pulsos que reciben los servos, en us.
        estado: "quieto", "moviendo" o "detenido".
        objetivo: el último objetivo (x, y, z) pedido, o None.
        resultado: el busqueda.Resultado de la última búsqueda, o None.
    """

    def __init__(self):
        """Arranca con el brazo en el home."""
        self.iniciar()

    def iniciar(self):
        """El brazo en el home del firmware, como setup()."""
        self.angulos = servos.home_modelo()
        self.pulsos = list(cfg.SERVO_HOME_US)
        self.estado = "quieto"
        self.objetivo = None
        self.resultado = None
        self._pasos, self._indice = [], 0

    def pedir_objetivo(self, x, y, z):
        """Busca una pose para el objetivo y, si llega, empieza a moverse.

        Args:
            x: coordenada x del objetivo (cm).
            y: coordenada y del objetivo (cm).
            z: coordenada z del objetivo (cm).

        Returns:
            La respuesta de /set: resultado, error_cm y metricas. Si el brazo se está moviendo,
            el resultado es "ocupado" y no se busca nada.

        Raises:
            ValueError: si algún valor no es un número finito o si z está debajo del piso.
        """
        objetivo = (float(x), float(y), float(z))
        if not all(math.isfinite(v) for v in objetivo):
            raise ValueError("Los valores de x, y y z tienen que ser números.")
        if objetivo[2] < cfg.PLANO_PISO_Z:
            raise ValueError("El objetivo está debajo del piso (z < 0).")
        if self.estado == "moviendo":
            return {"resultado": "ocupado", "error_cm": None, "metricas": None}
        self.objetivo = objetivo
        self.resultado = busqueda.buscar(np.array(objetivo), self.angulos)
        if self.resultado.estado == "ok":
            self._pasos, self._indice = self.resultado.pasos, 0
            self.estado = "moviendo"
        else:
            self._pasos, self._indice = [], 0
            self.estado = "quieto"
        return {"resultado": self.resultado.estado, "error_cm": self._error(), "metricas": self._metricas()}

    def avanzar(self):
        """Un paso del movimiento. Antes de aplicarlo revisa que no choque."""
        if self.estado != "moviendo":
            return
        paso = self._pasos[self._indice]
        choque = colisiones.detectar_colisiones(cinematica.cinematica_directa(paso).esferas)
        if choque.hay_choque:   # no debería pasar: la búsqueda ya validó el camino
            self.estado = "detenido"
            self.resultado = dataclasses.replace(self.resultado, estado="colision",
                                                 silueta=busqueda.Silueta(paso, None, choque))
            return
        self.angulos = paso
        self.pulsos = servos.pulsos_de(paso)
        self._indice += 1
        if self._indice == len(self._pasos):
            self.estado = "quieto"

    def detener(self):
        """Frena el brazo en el paso en que está."""
        if self.estado == "moviendo":
            self.estado = "detenido"

    def _error(self):
        """El error de la última búsqueda en cm, o None."""
        r = self.resultado
        return float(r.error) if r is not None and r.error is not None else None

    def _metricas(self):
        """Los contadores de la última búsqueda para la página, o None."""
        r = self.resultado
        if r is None:
            return None
        return {"posiciones": r.posiciones, "ejecuciones": r.ejecuciones, "iteraciones": r.iteraciones,
                "iteraciones_totales": r.iteraciones_totales, "tiempo_ms": round(r.tiempo_ms, 3)}

    def telemetria(self):
        """El JSON de /data, con las claves de J5 y las nuevas.

        Returns:
            Un diccionario listo para json.dumps.
        """
        g = cinematica.cinematica_directa(self.angulos)
        choque = colisiones.detectar_colisiones(g.esferas)
        r = self.resultado
        silueta = None
        if r is not None and r.silueta is not None:
            silueta = _dibujo(r.silueta.angulos, r.silueta.colision)
            silueta["error_cm"] = float(r.silueta.error) if r.silueta.error is not None else None
            silueta["servos_fuera_de_rango"] = _servos_fuera_de_rango(r.estado, r.silueta.angulos)
        return {
            "angles": [float(a) for a in self.angulos],
            "collision_detected": choque.hay_choque,
            "colliding_centers": colisiones.centros(g.esferas, choque.esferas),
            "sphere_radius": cfg.RADIO_ESFERA,
            "spheres": [e.tolist() for e in g.esferas],
            "puntos": g.puntos.tolist(),
            "marcos": g.marcos.tolist(),
            "esferas_en_choque": [list(i) for i in choque.esferas],
            "estado": self.estado,
            "pulsos_us": list(self.pulsos),
            "objetivo": list(self.objetivo) if self.objetivo is not None else None,
            "resultado": r.estado if r is not None else None,
            "error_cm": self._error(),
            "metricas": self._metricas(),
            "silueta": silueta,
            "pose_final": _dibujo(self._pasos[-1]) if self.estado == "moviendo" else None,
        }
