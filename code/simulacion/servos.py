"""Pulsos, grados del servo y grados del modelo, con las tablas de main.ino.

Equivale a getAngleForServo, home() y la conversión de signos de main.ino. En C va a
servos.c que sería un archivo nuevo. Implementación J1.
"""
import math

import configuracion as cfg


def pulso_a_servo(i, us):
    """Grados del servo i para un pulso, como getAngleForServo de main.ino.

    Interpola en la tabla de calibración del servo y recorta en los extremos de la tabla.

    Args:
        i: índice del servo (0 a 4).
        us: ancho de pulso en us.

    Returns:
        Los grados del servo, de 0 a 180.
    """
    lut = cfg.LUT_POR_SERVO[i]
    if us <= lut[0][0]:
        return lut[0][1]
    if us >= lut[-1][0]:
        return lut[-1][1]
    for (x0, y0), (x1, y1) in zip(lut, lut[1:]):
        if us < x1:
            t = (us - x0) / (x1 - x0)
            return y0 + t * (y1 - y0)
    return math.nan  # inalcanzable con una tabla ordenada, como en C


def servo_a_pulso(i, s):
    """Pulso que da s grados en el servo i.

    Usa la misma tabla que pulso_a_servo, al revés.

    Args:
        i: índice del servo (0 a 4).
        s: grados del servo; se recorta al rango de la tabla.

    Returns:
        El pulso en us, sin redondear.
    """
    lut = cfg.LUT_POR_SERVO[i]
    grados = [y for _, y in lut]
    s = min(max(s, min(grados)), max(grados))
    for (x0, y0), (x1, y1) in zip(lut, lut[1:]):
        if min(y0, y1) <= s <= max(y0, y1):
            return x0 + (s - y0) * (x1 - x0) / (y1 - y0)
    return math.nan


def servo_a_modelo(i, s):
    """Grados del modelo a partir de grados del servo: theta = signo * s + desvío.

    Args:
        i: índice del servo (0 a 4).
        s: grados del servo.

    Returns:
        Los grados del modelo.
    """
    return cfg.SIGNO_MODELO[i] * s + cfg.DESVIO_MODELO[i]


def modelo_a_servo(i, theta):
    """Grados del servo a partir de grados del modelo, la inversa de servo_a_modelo.

    Args:
        i: índice del servo (0 a 4).
        theta: grados del modelo.

    Returns:
        Los grados del servo.
    """
    return (theta - cfg.DESVIO_MODELO[i]) * cfg.SIGNO_MODELO[i]


def rango_servo(i):
    """Rango del servo i en grados del servo, con los pulsos que permite main.ino.

    Args:
        i: índice del servo (0 a 4).

    Returns:
        La tupla (mínimo, máximo).
    """
    a = pulso_a_servo(i, cfg.SERVO_MIN_US[i])
    b = pulso_a_servo(i, cfg.SERVO_MAX_US[i])
    return (min(a, b), max(a, b))


def rango_modelo(i):
    """Rango del servo i en grados del modelo.

    Args:
        i: índice del servo (0 a 4).

    Returns:
        La tupla (mínimo, máximo).
    """
    a, b = (servo_a_modelo(i, s) for s in rango_servo(i))
    return (min(a, b), max(a, b))


def home_modelo():
    """La pose del home del firmware (home() de main.ino) en grados del modelo.

    Returns:
        Los cinco ángulos, como tupla.
    """
    pose = []
    for i, us in enumerate(cfg.SERVO_HOME_US):
        us = min(max(us, cfg.SERVO_MIN_US[i]), cfg.SERVO_MAX_US[i])
        pose.append(servo_a_modelo(i, pulso_a_servo(i, us)))
    return tuple(pose)


def pulsos_de(pose):
    """Los cinco pulsos enteros para una pose del modelo.

    Redondea con los medios hacia arriba y recorta a los pulsos de main.ino.

    Args:
        pose: los cinco ángulos del modelo, en grados.

    Returns:
        La lista de los cinco pulsos en us.
    """
    pulsos = []
    for i, theta in enumerate(pose):
        us = math.floor(servo_a_pulso(i, modelo_a_servo(i, theta)) + 0.5)
        pulsos.append(min(max(us, cfg.SERVO_MIN_US[i]), cfg.SERVO_MAX_US[i]))
    return pulsos
