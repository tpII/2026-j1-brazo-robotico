"""Este archivo es común para todas las versiones del algoritmo de FABRIK.

Implementación de la cadena virtual, el Paso 0, el Paso 1 (planos
candidatos), los ángulos del Paso 3, el recorte angular, la cadena estirada, el control final
y la resolución de los planos en orden. Cada versión pone su propio paso de iteración.
En C va a plano.c.
Implementación J1.
"""
import math
from dataclasses import dataclass

import numpy as np

import cinematica
import configuracion as cfg
import servos

D2R = math.pi / 180


@dataclass
class CadenaVirtual:
    """La cadena hombro, codo, p5 dentro del plano.

    Attributes:
        l1: distancia del hombro al codo (cm).
        l2: distancia del codo a p5, dentro del plano (cm).
        alfa: ángulo fijo entre el segmento codo-p5 y el antebrazo (grados).
        d: distancia de p5 al plano (cm).
        z_hombro: altura del hombro (cm).
    """
    l1: float
    l2: float
    alfa: float
    d: float
    z_hombro: float


@dataclass
class Paso:
    """Una fila de la traza de la iteración.

    Los puntos son (r, h) del plano, medidos desde el hombro.

    Attributes:
        k: número de iteración.
        codo_forward: el codo después del forward.
        codo_espejado: el codo después de la regla del codo, o None si no se espejó.
        codo: el codo después del backward.
        punta: la punta después del backward.
        error: distancia de la punta al objetivo (cm).
    """
    k: int
    codo_forward: tuple | None
    codo_espejado: tuple | None
    codo: tuple
    punta: tuple
    error: float


@dataclass
class Ejecucion:
    """Resultado de una ejecución de la iteración en un plano.

    Attributes:
        theta1: ángulo del hombro al terminar (grados del modelo).
        theta2: ángulo del codo al terminar (grados del modelo).
        error: distancia final de la punta al objetivo (cm).
        iteraciones: iteraciones hechas.
        motivo: por qué cortó: "eps", "eta" o "kmax".
        recortes: cuántas veces se recortó un ángulo a su rango.
        mejor_error: el error de la iteración más cercana al objetivo.
        mejor_theta1: el hombro de esa iteración.
        mejor_theta2: el codo de esa iteración.
        traza: los Paso de cada iteración, si se pidieron.
    """
    theta1: float
    theta2: float
    error: float
    iteraciones: int
    motivo: str
    recortes: int
    mejor_error: float
    mejor_theta1: float
    mejor_theta2: float
    traza: list | None = None


@dataclass
class SolucionPlano:
    """Lo que pasó en un plano candidato.

    Attributes:
        theta0: ángulo de la base de ese plano (grados del modelo).
        r_c: distancia horizontal del objetivo dentro del plano (cm).
        alcanzable: si pasó el Paso 0 dentro del plano.
        ejecucion: la Ejecucion de la iteración, si se iteró.
        angulos: (theta0, theta1, theta2, theta3, theta4) al terminar la ejecución.
        error: distancia final de la punta al objetivo (cm).
        valida: si pasó el control final.
    """
    theta0: float
    r_c: float
    alcanzable: bool
    ejecucion: Ejecucion | None = None
    angulos: tuple | None = None
    error: float | None = None
    valida: bool = False


@dataclass
class Solucion:
    """Resultado de probar los planos en orden para una muñeca y una pinza.

    Attributes:
        valida: si algún plano dio una pose válida.
        angulos: la pose del primer plano válido, o None.
        error: el error de esa pose (cm), o None.
        planos: la SolucionPlano de cada plano probado.
    """
    valida: bool
    angulos: tuple | None
    error: float | None
    planos: list


def u2(g):
    """Vector unitario del plano a g grados.

    Args:
        g: ángulo en grados, medido desde el eje r.

    Returns:
        El vector (cos g, sin g).
    """
    return np.array([math.cos(g * D2R), math.sin(g * D2R)])


def ang(v):
    """Ángulo de un vector del plano, en grados.

    Args:
        v: vector (r, h).

    Returns:
        atan2(h, r) en grados, entre -180 y 180.
    """
    return math.atan2(v[1], v[0]) / D2R


def _norm180(a):
    """Lleva un ángulo en grados a [-180, 180)."""
    return (a + 180) % 360 - 180


def cadena_virtual(theta3, theta4):
    """Cadena virtual para una muñeca y una pinza dadas.

    Se calcula con la cinemática directa en la pose cero.

    Args:
        theta3: ángulo de la muñeca.
        theta4: ángulo de la pinza.

    Returns:
        La CadenaVirtual.
    """
    p = cinematica.cinematica_directa((0.0, 0.0, 0.0, theta3, theta4), con_esferas=False).puntos
    v = (p[5][0] - p[2][0], p[5][2] - p[2][2])
    return CadenaVirtual(l1=float(np.linalg.norm(p[2] - p[1])), l2=math.hypot(*v), alfa=ang(v),
                         d=float(p[5][1]), z_hombro=float(p[1][2]))


def planos_candidatos(objetivo, d):
    """Paso 1: los planos que dejan p5 sobre el objetivo, primero el que apunta hacia él.

    Descarta los que dejan la base fuera de su rango. Si rho < |d| no hay ninguno.

    Args:
        objetivo: el punto (x, y, z) en cm.
        d: distancia de p5 al plano, de la cadena virtual (cm).

    Returns:
        Una lista de 0 a 2 tuplas (theta0, r_c).
    """
    x, y = objetivo[0], objetivo[1]
    rho = math.hypot(x, y)
    if rho < abs(d):
        return []
    lo, hi = servos.rango_modelo(0)
    planos = []
    for signo in (1, -1):
        r_c = signo * math.sqrt(rho * rho - d * d)
        theta0 = _norm180(math.atan2(y, x) / D2R - math.atan2(d, r_c) / D2R)
        if en_rango(theta0, lo, hi):
            planos.append((theta0, r_c))
    return planos


def objetivo_en_plano(objetivo, r_c, cadena):
    """El objetivo dentro del plano, medido desde el hombro: T = (r_c, z - z_hombro).

    Args:
        objetivo: el punto (x, y, z) en cm.
        r_c: distancia horizontal del objetivo dentro del plano (cm).
        cadena: la CadenaVirtual.

    Returns:
        El vector T.
    """
    return np.array([r_c, objetivo[2] - cadena.z_hombro])


def alcanzable(T, cadena):
    """Paso 0 dentro del plano: ||T|| <= l1 + l2.

    Args:
        T: el objetivo en el plano, desde el hombro.
        cadena: la CadenaVirtual.

    Returns:
        True si la cadena estirada llega.
    """
    return np.linalg.norm(T) <= cadena.l1 + cadena.l2


def angulos_desde_puntos(E, P, alfa):
    """Paso 3: theta1 y theta2 a partir del codo y la punta del plano.

    Args:
        E: el codo (r, h), desde el hombro.
        P: la punta (r, h), desde el hombro.
        alfa: el ángulo alfa de la cadena virtual (grados).

    Returns:
        La tupla (theta1, theta2) en grados del modelo.
    """
    theta1 = ang(E)
    return theta1, ang(P - E) - alfa - theta1


def cadena_estirada(objetivo, cadena):
    """La cadena estirada hacia el objetivo, sin límites: la silueta de «fuera de alcance».

    Args:
        objetivo: el punto (x, y, z) en cm.
        cadena: la CadenaVirtual.

    Returns:
        La tupla (theta0, theta1, theta2) en grados del modelo.
    """
    x, y, z = objetivo
    rho = math.hypot(x, y)
    r_c = math.sqrt(max(rho * rho - cadena.d * cadena.d, 0.0))
    theta0 = _norm180(math.atan2(y, x) / D2R - math.atan2(cadena.d, r_c) / D2R)
    theta1 = ang((r_c, z - cadena.z_hombro))
    return theta0, theta1, -cadena.alfa


def ajustar_angulo(a, lo, hi):
    """Recorte angular.

    El ángulo se lleva a [lo, lo + 360) y, si cae afuera, va al tope más cercano medido en ángulo.

    Args:
        a: ángulo en grados.
        lo: tope mínimo en grados.
        hi: tope máximo en grados.

    Returns:
        La tupla (ángulo en [lo, hi], si hubo que recortar).
    """
    a = lo + (a - lo) % 360.0
    if a <= hi + 1e-9:
        return a, False
    return (hi, True) if (a - hi) <= (lo + 360.0 - a) else (lo, True)


def en_rango(a, lo, hi, tol=1e-6):
    """Si un ángulo está en [lo, hi], tomado módulo 360.

    Args:
        a: ángulo en grados.
        lo: tope mínimo en grados.
        hi: tope máximo en grados.
        tol: tolerancia en grados.

    Returns:
        True si está en el rango.
    """
    a = lo + (a - lo) % 360.0
    return a <= hi + tol or a >= lo + 360.0 - tol


def rangos_fabrik():
    """Los rangos que usa FABRIK: base, hombro y codo, en grados del modelo.

    Returns:
        Tres tuplas (mínimo, máximo).
    """
    return servos.rango_modelo(0), servos.rango_modelo(1), servos.rango_modelo(2)


def es_valida(objetivo, angulos, error):
    """Control final: error <= epsilon, ángulos en rango y p5 a epsilon o menos del objetivo.

    Args:
        objetivo: el punto (x, y, z) en cm.
        angulos: la pose (theta0, theta1, theta2, theta3, theta4) en grados del modelo.
        error: el error que dio la iteración (cm).

    Returns:
        True si la pose pasa el control.
    """
    if error > cfg.EPSILON:
        return False
    if not all(en_rango(a, lo, hi) for a, (lo, hi) in zip(angulos[:3], rangos_fabrik())):
        return False
    p5 = cinematica.cinematica_directa(angulos, con_esferas=False).puntos[5]
    return np.linalg.norm(p5 - objetivo) <= cfg.EPSILON + 1e-9


def resolver_plano(iterar, objetivo, theta0, r_c, cadena, theta3, theta4, theta1_ini, theta2_ini, espejo_final=False):
    """Paso 0, iteración y control final en un plano.

    Args:
        iterar: la función de iteración de una versión de FABRIK (fabrik.iterar, por ejemplo).
        objetivo: el punto (x, y, z) en cm.
        theta0: ángulo de la base del plano (grados del modelo).
        r_c: distancia horizontal del objetivo dentro del plano (cm).
        cadena: la CadenaVirtual.
        theta3: ángulo de la muñeca (grados del modelo).
        theta4: ángulo de la pinza (grados del modelo).
        theta1_ini: hombro de partida (grados del modelo).
        theta2_ini: codo de partida (grados del modelo).
        espejo_final: la variante de v1, que espeja el codo una vez al final.

    Returns:
        La SolucionPlano.
    """
    T = objetivo_en_plano(objetivo, r_c, cadena)
    if not alcanzable(T, cadena):
        return SolucionPlano(theta0, r_c, alcanzable=False)
    ejecucion = iterar(T, cadena, theta1_ini, theta2_ini)
    theta1, theta2, error = ejecucion.theta1, ejecucion.theta2, ejecucion.error
    if espejo_final:
        E = cadena.l1 * u2(theta1)
        if T[0] * E[1] - T[1] * E[0] < 0:   # el codo quedó del lado prohibido: se espeja una vez
            u = T / np.linalg.norm(T)
            E = 2 * np.dot(E, u) * u - E
            theta1 = ang(E)
            P = E + cadena.l2 * (T - E) / np.linalg.norm(T - E)
            theta2 = ang(P - E) - cadena.alfa - theta1
            error = float(np.linalg.norm(P - T))
    angulos = (theta0, theta1, theta2, theta3, theta4)
    return SolucionPlano(theta0, r_c, True, ejecucion, angulos, error, es_valida(objetivo, angulos, error))


def resolver(iterar, objetivo, theta3, theta4, theta1_ini, theta2_ini, espejo_final=False):
    """Prueba los planos candidatos en orden y se queda con el primero válido.

    Args:
        iterar: la función de iteración de una versión de FABRIK.
        objetivo: el punto (x, y, z) en cm.
        theta3: ángulo de la muñeca (grados del modelo).
        theta4: ángulo de la pinza (grados del modelo).
        theta1_ini: hombro de partida (grados del modelo).
        theta2_ini: codo de partida (grados del modelo).
        espejo_final: la variante de v1.

    Returns:
        La Solucion.
    """
    cadena = cadena_virtual(theta3, theta4)
    planos = []
    for theta0, r_c in planos_candidatos(objetivo, cadena.d):
        solucion = resolver_plano(iterar, objetivo, theta0, r_c, cadena, theta3, theta4, theta1_ini, theta2_ini,
                                  espejo_final)
        planos.append(solucion)
        if solucion.valida:
            return Solucion(True, solucion.angulos, solucion.error, planos)
    return Solucion(False, None, None, planos)
