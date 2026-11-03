"""Datos del brazo y parámetros de la simulación.

Archivo de 2025 (grupo J5, code/python/modular/configuracion.py).
"""
import numpy as np

# --- 1. CONFIGURACIÓN GENERAL (configuracion.c) --- #
RADIO_ESFERA = 1.0                      # radio de todas las esferas (cm)
T_BASE_GLOBAL = np.array([0.0, 0.0, 0.0])  # posición de la base en el marco global

# MATRIZ_COLISIONES[i][j] = 1 indica que se revisa el choque entre los eslabones i y j.
MATRIZ_COLISIONES = np.array([
    [0, 0, 0, 0, 1],  # Eslabón 0
    [0, 0, 1, 0, 1],  # Eslabón 1
    [0, 1, 0, 0, 0],  # Eslabón 2
    [0, 0, 0, 0, 0],  # Eslabón 3
    [1, 1, 0, 0, 0],  # Eslabón 4
])

# --- 2. VECTORES DE DESPLAZAMIENTO (configuracion.c) --- #
# Posición del final de cada eslabón en su marco local (cm).
SALTO_ESLABONES = [
    np.array([0.0, 0.0, 3.0]),
    np.array([3.3, 0.0, 0.0]),
    np.array([4.0, 0.5, 0.0]),
    np.array([0.0, 0.0, -2.0]),
    np.array([0.0, 0.0, 2.0]),
]

# --- 3. ESFERAS LOCALES (configuracion.c) --- #
esferas_local = [
    np.array([[0.0, 0.0, 0.0]]),
    np.array([[0.0, 0.0, 0.0], [2.0, 0.0, 0.0]]),
    np.array([[0.0, 0.0, 0.0], [2.0, 0.5, 0.0], [4.0, 0.5, 0.0]]),
    np.array([[0.0, 0.0, 0.0]]),
    np.array([[0.0, 0.0, 0.0], [0.0, -2.0, 0.0]]),
]


# Para abajo es implementado por el grupo J1.

# --- 4. ROTACIONES FIJAS, PISO Y TOLERANCIA (cinematica.c) --- #
# Rotaciones fijas de montaje (eje, grados). En C están escritas dentro de
# calcular_configuracion_modular (cinematica.c); un giro de 0° es la identidad.
ROTACIONES_FIJAS = (("z", 0.0), ("x", 90.0), ("z", 0.0), ("y", -90.0), ("y", 90.0))

# Piso y tolerancia de choque (cinematica.c: plano_z = 0.0 y tol = 1e-9).
PLANO_PISO_Z = 0.0
TOL_COLISION = 1e-9

# --- 5. SERVOS (firmware.ino) --- #
SERVO_MIN_US = (400, 400, 400, 400, 400)
SERVO_MAX_US = (2600, 1800, 2600, 2000, 2600)
SERVO_HOME_US = (400, 1400, 1200, 960, 400)

# Tablas de calibración (us, grados del servo).
LUT_GENERICO = ((400, 0.0), (767, 30.0), (1134, 60.0), (1501, 90.0), (1868, 120.0), (2235, 150.0), (2600, 180.0))
LUT1 = ((400, 0.0), (800, 30.0), (1000, 60.0), (1100, 90.0), (1200, 120.0), (1500, 150.0), (1800, 180.0))
LUT2 = ((400, 180.0), (767, 150.0), (1134, 120.0), (1501, 90.0), (1868, 60.0), (2235, 30.0), (2600, 0.0))
LUT3 = ((400, 0.0), (500, 30.0), (600, 60.0), (900, 90.0), (1300, 120.0), (1700, 150.0), (2000, 180.0))
LUT_POR_SERVO = (LUT_GENERICO, LUT1, LUT2, LUT3, LUT_GENERICO)  # como getAngleForServo

# Grados del modelo: theta = signo * s + desvío (home() y loop() de firmware.ino).
SIGNO_MODELO = (-1, 1, -1, 1, 1)
DESVIO_MODELO = (0.0, 0.0, 0.0, -90.0, 0.0)

# --- 6. FABRIK Y BÚSQUEDA --- #
EPSILON = 0.5          # cm: el error que se acepta
ETA = 1e-4             # cm: p5 dejó de moverse
K_MAX = 20             # iteraciones como máximo
PASO_BUSQUEDA = 10.0   # grados entre posiciones de la muñeca y de la pinza
MUNECA_NEUTRA = 0.0
PINZA_NEUTRA = 90.0    # alineada con el antebrazo en el modelo

# --- 7. MOVIMIENTO --- #
DT = 0.02                    # s entre pasos
VEL_MAX_ARTICULACION = 60.0  # °/s
VEL_MAX_PUNTO = 10.0         # cm/s, para p1 a p5 y los centros de las esferas
T_RAMPA = 0.3                # s para arrancar y para frenar

# --- 8. PUNTA DE LA PINZA (de los STL) --- #
# La mano es un ArmFront: del eje del servo 4 al final de la bandeja.
SALTO_ESLABONES[4] = np.array([0.0, -4.33, 0.0])

# --- 9. MEDIDAS DEL BRAZO REAL (de los STL de las piezas y de la hoja de datos del MG90S) --- #
# El fondo de la base giratoria queda a 3.29 cm de la mesa (caja del soporte, tapa del eje del
# MG90S y horn) y el eje del hombro, 1.29 cm más arriba.
SALTO_ESLABONES[0] = np.array([0.0, 0.0, 4.58])
# Arm.stl mide 4.95 cm entre ejes; el brazo armado tiene el tramo del medio 1.3 cm más largo.
SALTO_ESLABONES[1] = np.array([6.25, 0.0, 0.0])
# Del codo al horn de la muñeca, con el servo contra la pared de atrás de la caja. El eje de la
# muñeca queda 0.3 cm al costado (sobre el eje del codo) porque está cerca de una punta del servo.
SALTO_ESLABONES[2] = np.array([5.89, 0.0, 0.3])
# La base de la mano es otro Swing, con el eje del servo 4 a 1.29 cm de su fondo.
SALTO_ESLABONES[3] = np.array([0.0, 0.0, -1.29])

# --- 10. ESFERAS DE COLISIÓN (de las medidas del brazo) --- #
# Una esfera en cada articulación y otra cada 1.5 cm como máximo, sobre el salto de cada eslabón.
# En la pala la última queda a un radio de la punta, así la esfera no sobresale.
RADIO_ESFERA = 1.5
esferas_local = [
    np.array([[0.0, 0.0, 0.0], [0.0, 0.0, 1.145], [0.0, 0.0, 2.29], [0.0, 0.0, 3.435]]),
    np.array([[0.0, 0.0, 0.0], [1.25, 0.0, 0.0], [2.5, 0.0, 0.0], [3.75, 0.0, 0.0], [5.0, 0.0, 0.0]]),
    np.array([[0.0, 0.0, 0.0], [1.4725, 0.0, 0.075], [2.945, 0.0, 0.15], [4.4175, 0.0, 0.225]]),
    np.array([[0.0, 0.0, 0.0]]),
    np.array([[0.0, 0.0, 0.0], [0.0, -1.415, 0.0], [0.0, -2.83, 0.0]]),
]

# Los pares contiguos y el (2, 4) no se revisan: sus esferas se superponen en todas las poses (la
# base de la mano mide 1.29 cm y deja la pala junto al antebrazo). El choque del brazo con el
# antebrazo lo cubre el tope del codo.
MATRIZ_COLISIONES = np.array([
    [0, 0, 1, 1, 1],  # Eslabón 0
    [0, 0, 0, 1, 1],  # Eslabón 1
    [1, 0, 0, 0, 0],  # Eslabón 2
    [1, 1, 0, 0, 0],  # Eslabón 3
    [1, 1, 0, 0, 0],  # Eslabón 4
])

# Con el codo más cerrado que esto (grados del modelo), el antebrazo toca el brazo.
TOPE_CODO = -139.0

# Caja de la pala en su marco (cm): x es el grosor, y el largo y z el ancho del ArmFront de la mano.
# Contra la mesa se revisan sus 8 esquinas.
CAJA_PALA = ((-0.85, 0.75), (-4.38, 0.82), (-2.14, 1.56))
ESQUINAS_PALA = np.array([[x, y, z] for x in CAJA_PALA[0] for y in CAJA_PALA[1] for z in CAJA_PALA[2]])
