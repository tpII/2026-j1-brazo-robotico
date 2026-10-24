"""La búsqueda de la muñeca y la pinza.

El usuario solo da el punto. FABRIK v3 calcula la base, el hombro y el codo, y la búsqueda elige
la muñeca y la pinza: primero la posición neutra, después las de la pinza con la muñeca neutra y
por último cada posición de la muñeca con todas las de la pinza, de a PASO_BUSQUEDA grados, de la
más cercana a la neutra a la más lejana. Se queda con la primera posición que llega.

Si una posición tiene la misma cadena virtual que otra que ya falló por alcance o por límites,
también falla y se saltea sin contarla. Pasa con la muñeca girada y la pinza alineada: p5, la punta
de la pinza, queda sobre el eje de la muñeca y girarla no lo mueve.

Con los choques, una pose válida se acepta solo si ni ella ni el camino desde
la pose actual chocan. Si choca, se descarta igual que una que no llega: se prueba el otro plano y
después la siguiente posición. Una posición con la misma cadena que otra que llegó pero chocaba
reusa sus ángulos con su propia muñeca y su propia pinza.
En C será a busqueda.c. Implementación J1,.
"""
import time
from dataclasses import dataclass, field

import numpy as np

import cinematica
import colisiones
import configuracion as cfg
import fabrik
import plano
import servos
import trayectoria

ESTADOS = ("ok", "fuera_de_alcance", "bloqueado_por_limites", "colision")


@dataclass
class Silueta:
    """El mejor intento cuando el brazo no se mueve.

    Attributes:
        angulos: la pose que se dibuja, en grados del modelo.
        error: su distancia al objetivo (cm), si se conoce.
        colision: la colisiones.Colision de la pose, si choca.
    """
    angulos: tuple
    error: float | None = None
    colision: object | None = None


@dataclass
class Resultado:
    """Lo que devuelve una búsqueda.

    Attributes:
        estado: uno de ESTADOS.
        angulos: la pose final, si el estado es "ok".
        error: su distancia al objetivo (cm), si el estado es "ok".
        silueta: la Silueta, si el brazo no se mueve.
        posiciones: posiciones de la muñeca y la pinza probadas.
        ejecuciones: ejecuciones de FABRIK, una por plano iterado.
        iteraciones: iteraciones de la ejecución aceptada.
        iteraciones_totales: iteraciones de todas las ejecuciones.
        tiempo_ms: lo que tardó la búsqueda.
        pasos: la trayectoria validada, cuando se revisan los choques.
        corridas: todas las plano.Corrida, para las mediciones.
    """
    estado: str
    angulos: tuple | None
    error: float | None
    silueta: Silueta | None
    posiciones: int
    ejecuciones: int
    iteraciones: int
    iteraciones_totales: int
    tiempo_ms: float
    pasos: list | None = None
    corridas: list = field(default_factory=list)


def _alrededor(neutra, rango):
    """Los valores alrededor de la posición neutra, de a PASO_BUSQUEDA grados.

    Args:
        neutra: la posición neutra, en grados del modelo.
        rango: la tupla (mínimo, máximo) del servo, en grados del modelo.

    Returns:
        neutra + paso, neutra − paso, neutra + 2·paso, ... sin salir del rango.
    """
    lo, hi = rango
    valores, k = [], 1
    while neutra + k * cfg.PASO_BUSQUEDA <= hi + 1e-9 or neutra - k * cfg.PASO_BUSQUEDA >= lo - 1e-9:
        for valor in (neutra + k * cfg.PASO_BUSQUEDA, neutra - k * cfg.PASO_BUSQUEDA):
            if lo - 1e-9 <= valor <= hi + 1e-9:
                valores.append(float(valor))
        k += 1
    return valores


def posiciones_busqueda():
    """Las posiciones de la muñeca y la pinza, en el orden en que se prueban.

    Cada muñeca, empezando por la neutra, va con todas las pinzas, empezando por la neutra. La muñeca
    sola no mueve p5: solo lo saca del plano del brazo cuando la pinza está inclinada.

    Returns:
        Una lista de tuplas (θ3, θ4); son 361 con los rangos del firmware.
    """
    munecas = [cfg.MUNECA_NEUTRA] + _alrededor(cfg.MUNECA_NEUTRA, servos.rango_modelo(3))
    pinzas = [cfg.PINZA_NEUTRA] + _alrededor(cfg.PINZA_NEUTRA, servos.rango_modelo(4))
    return [(t3, t4) for t3 in munecas for t4 in pinzas]


def _misma_cadena(a, b):
    """Si dos cadenas virtuales tienen el mismo l2, alpha y d."""
    return abs(a.l2 - b.l2) < 1e-9 and abs(a.alfa - b.alfa) < 1e-9 and abs(a.d - b.d) < 1e-9


def _revisar(pose, actual):
    """Revisa los choques de una pose y del camino hasta ella.

    Args:
        pose: la pose válida que se quiere aceptar.
        actual: la pose donde está el brazo.

    Returns:
        La tupla (silueta, pasos). Si ni la pose ni el camino chocan, la silueta es None y los
        pasos son la trayectoria; si no, la silueta es la pose o el paso que choca y los pasos
        son None.
    """
    choque = colisiones.detectar_colisiones(cinematica.cinematica_directa(pose).esferas)
    if choque.hay_choque:
        return Silueta(tuple(pose), None, choque), None
    pasos = trayectoria.planificar(actual, pose).pasos
    i = trayectoria.primer_choque(pasos)
    if i is not None:
        return Silueta(pasos[i], None, colisiones.detectar_colisiones(cinematica.cinematica_directa(pasos[i]).esferas)), None
    return None, pasos


def revisar_pose(pose, actual):
    """Revisa si una pose o el camino hasta ella chocan.

    Args:
        pose: la pose que se quiere revisar.
        actual: la pose donde está el brazo.

    Returns:
        La Silueta de «colision», o None si todo está libre.
    """
    return _revisar(pose, actual)[0]


def buscar(objetivo, actual, revisar_colisiones=True):
    """Busca una pose que llegue al objetivo desde la pose actual, en una sola llamada.

    Args:
        objetivo: el punto (x, y, z) en cm.
        actual: la pose donde está el brazo (cinco ángulos del modelo, en grados).
        revisar_colisiones: con False no se revisan choques ni trayectoria; es el modo con el que
            se midieron los números de la sección 9 de la implementación.

    Returns:
        El Resultado.
    """
    comienzo = time.perf_counter()
    objetivo = np.asarray(objetivo, dtype=float)
    theta1_ini, theta2_ini = actual[1], actual[2]
    posiciones = ejecuciones = iteraciones_totales = 0
    corridas, fallidas, chocadas = [], [], []   # chocadas: (cadena, [(ángulos, error, iteraciones)])
    mejor = None              # (error, pose) de la iteración de menor error de todas
    paso0 = False             # si alguna posición pasó el Paso 0 en algún plano
    primera_silueta = None    # la primera pose válida que chocaba (o el primer paso que chocaba)

    def resultado(estado, angulos=None, error=None, silueta=None, iteraciones=0, pasos=None):
        """El Resultado con los contadores de la búsqueda hasta ahora."""
        return Resultado(estado, angulos, error, silueta, posiciones, ejecuciones, iteraciones, iteraciones_totales,
                         1000 * (time.perf_counter() - comienzo), pasos, corridas)

    def aceptar(pose):
        """Revisa los choques de una pose válida; devuelve los pasos si se acepta."""
        nonlocal primera_silueta
        silueta, pasos = _revisar(pose, actual)
        if silueta is None:
            return pasos
        if primera_silueta is None:
            primera_silueta = silueta
        return None

    for theta3, theta4 in posiciones_busqueda():
        cadena = plano.cadena_virtual(theta3, theta4)
        if any(_misma_cadena(cadena, otra) for otra in fallidas):
            continue
        posiciones += 1
        anterior = next((validas for otra, validas in chocadas if _misma_cadena(cadena, otra)), None)
        if anterior is not None:   # misma cadena que una que llegó pero chocaba: mismos ángulos, otro θ4
            for angulos, error, iteraciones in anterior:
                pose = tuple(angulos[:3]) + (theta3, theta4)
                pasos = aceptar(pose)
                if pasos is not None:
                    return resultado("ok", pose, error, iteraciones=iteraciones, pasos=pasos)
            continue
        validas = []
        for theta0, r_c in plano.planos_candidatos(objetivo, cadena.d):
            solucion = plano.resolver_plano(fabrik.iterar, objetivo, theta0, r_c, cadena, theta3, theta4,
                                            theta1_ini, theta2_ini)
            if not solucion.alcanzable:
                continue
            paso0 = True
            corrida = solucion.corrida
            ejecuciones += 1
            iteraciones_totales += corrida.iteraciones
            corridas.append(corrida)
            if mejor is None or corrida.mejor_error < mejor[0]:
                mejor = (corrida.mejor_error, (theta0, corrida.mejor_theta1, corrida.mejor_theta2, theta3, theta4))
            if not solucion.valida:
                continue
            if not revisar_colisiones:
                return resultado("ok", solucion.angulos, solucion.error, iteraciones=corrida.iteraciones)
            pasos = aceptar(solucion.angulos)
            if pasos is not None:
                return resultado("ok", solucion.angulos, solucion.error, iteraciones=corrida.iteraciones, pasos=pasos)
            validas.append((solucion.angulos, solucion.error, corrida.iteraciones))
        if validas:
            chocadas.append((cadena, validas))
        else:
            fallidas.append(cadena)

    if primera_silueta is not None:
        return resultado("colision", silueta=primera_silueta)
    if not paso0:
        neutra = plano.cadena_virtual(cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA)
        estirada = plano.cadena_estirada(objetivo, neutra) + (cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA)
        return resultado("fuera_de_alcance", silueta=Silueta(estirada))
    return resultado("bloqueado_por_limites", silueta=Silueta(mejor[1], mejor[0]))
