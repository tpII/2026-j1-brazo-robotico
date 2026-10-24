"""Implementación, Tabla 1, sección 2.2 y Tabla 9: la pose cero, el home y los rangos de cada servo."""
import numpy as np

import cinematica
import configuracion as cfg
import servos
from experimentos import guardar

NOMBRES = ("Base", "Hombro", "Codo", "Muñeca", "Pinza")


def calcular():
    """Calcula la pose cero, el home y los datos de cada servo.

    Returns:
        Un diccionario con los puntos y los ejes de la pose cero, la pose del home y, por cada
        servo, sus pulsos mínimo y máximo, el pulso del home y sus rangos en grados del servo y
        del modelo.
    """
    g = cinematica.cinematica_directa((0, 0, 0, 0, 0))
    datos_servos = []
    for i in range(5):
        datos_servos.append({
            "pulsos": (cfg.SERVO_MIN_US[i], cfg.SERVO_MAX_US[i]),
            "home_us": cfg.SERVO_HOME_US[i],
            "rango_servo": servos.rango_servo(i),
            "rango_modelo": servos.rango_modelo(i),
        })
    return {"puntos": g.puntos, "ejes": g.ejes, "home": servos.home_modelo(), "servos": datos_servos}


def main():
    """Imprime la salida y la guarda en resultados/pose_cero_y_home.txt."""
    r = calcular()
    lineas = ["Tabla 1: posición y eje de giro de cada articulación en la pose cero (cm)"]
    for i, p in enumerate(r["puntos"]):
        eje = (np.round(r["ejes"][i], 3) + 0.0).tolist() if i < 5 else "---"   # + 0.0 saca los -0.0
        lineas.append(f"  p{i}: {(np.round(p, 3) + 0.0).tolist()}  eje: {eje}")
    lineas.append("")
    lineas.append("Sección 2.2: home del firmware")
    lineas.append(f"  pulsos: {list(cfg.SERVO_HOME_US)} µs")
    lineas.append(f"  ángulos del modelo: {[round(a, 4) for a in r['home']]}")
    lineas.append("")
    lineas.append("Tabla 9: pulsos de main.ino, rango del servo y rango del modelo (grados)")
    for nombre, s in zip(NOMBRES, r["servos"]):
        lineas.append(f"  {nombre:7s} {s['pulsos'][0]} a {s['pulsos'][1]} µs | servo {s['rango_servo']} | "
                      f"modelo {s['rango_modelo']}")
    guardar("pose_cero_y_home", lineas)


if __name__ == "__main__":
    main()
