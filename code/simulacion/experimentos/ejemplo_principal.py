"""El ejemplo que recorre toda la implementación: el objetivo (5, -5, 7) con la muñeca y la pinza neutras.
"""
import math

import numpy as np

import busqueda
import cinematica
import configuracion as cfg
import fabrik
import fabrik_v1
import fabrik_v2
import plano
import servos
from experimentos import guardar, v0_latigo

OBJETIVO = np.array([5.0, -5.0, 7.0])
OBJETIVO_COSTADO = np.array([6.0, 1.0, 6.0])   # del lado +y, la base no puede apuntarle
OBJETIVO_TRABA = np.array([5.0, -5.0, 8.0])     # un centímetro más arriba, v2 se traba


def _redondear(v, n=3):
    """Redondea a n decimales y devuelve una lista, sin -0.0."""
    return (np.round(np.asarray(v, dtype=float), n) + 0.0).tolist()


def _iteraciones(n):
    """«1 iteración» o «n iteraciones»."""
    return f"{n} iteración" if n == 1 else f"{n} iteraciones"


def calcular():
    """Recorre el ejemplo (5, -5, 7) con todas las versiones y arma las poses de las figuras.

    Returns:
        Un diccionario con el Paso 1 (rho, d, r_c, theta y T), el Paso 1 de (6, 1, 6), la
        traza de v3, las poses de las figuras, las soluciones exactas con el codo arriba y
        abajo, y lo que dan v3, v1, v1 recortada y v2, también con (5, -5, 8).
    """
    c = plano.cadena_virtual(cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA)
    (theta0, r_c), = plano.planos_candidatos(OBJETIVO, c.d)
    T = plano.objetivo_en_plano(OBJETIVO, r_c, c)
    paso1 = {"rho": math.hypot(OBJETIVO[0], OBJETIVO[1]), "d": c.d, "r_c": r_c, "theta0": theta0,
             "phi": math.degrees(math.atan2(OBJETIVO[1], OBJETIVO[0])), "desvio": math.degrees(math.atan2(c.d, r_c)),
             "T": T}

    def pose(theta1, theta2):
        """La pose completa en el plano del ejemplo, con la muñeca y la pinza neutras."""
        return (theta0, theta1, theta2, cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA)

    def p5(theta1, theta2):
        """La punta de esa pose, con la cinemática directa."""
        return cinematica.cinematica_directa(pose(theta1, theta2)).puntos[5]

    v3 = fabrik.iterar(T, c, 0.0, 0.0, traza=True)

    norma_T = np.linalg.norm(T)
    beta = math.degrees(math.acos((c.l1 ** 2 + norma_T ** 2 - c.l2 ** 2) / (2 * c.l1 * norma_T)))
    exactas = {nombre: plano.angulos_desde_puntos(c.l1 * plano.u2(theta1), T, c.alfa)
               for nombre, theta1 in (("arriba", plano.ang(T) + beta), ("abajo", plano.ang(T) - beta))}

    v1 = fabrik_v1.iterar(T, c, 0.0, 0.0)
    t1_recortado, _ = plano.ajustar_angulo(v1.theta1, *servos.rango_modelo(1))
    t2_recortado, _ = plano.ajustar_angulo(v1.theta2, *servos.rango_modelo(2))
    v2 = fabrik_v2.iterar(T, c, 0.0, 0.0, traza=True)
    (_, r_c_traba), = plano.planos_candidatos(OBJETIVO_TRABA, c.d)
    traba = fabrik_v2.iterar(plano.objetivo_en_plano(OBJETIVO_TRABA, r_c_traba, c), c, 0.0, 0.0, traza=True)

    costado = busqueda.buscar(OBJETIVO_COSTADO, servos.home_modelo(), revisar_colisiones=False)
    c_costado = plano.cadena_virtual(*costado.angulos[3:])
    r_c_costado = next(rc for t0, rc in plano.planos_candidatos(OBJETIVO_COSTADO, c_costado.d)
                       if abs(t0 - costado.angulos[0]) < 1e-9)
    superior = {"phi": math.degrees(math.atan2(OBJETIVO_COSTADO[1], OBJETIVO_COSTADO[0])), "d": c_costado.d,
                "r_c": r_c_costado, "theta0": costado.angulos[0], "muneca": costado.angulos[3],
                "pinza": costado.angulos[4], "error": costado.error}

    u = plano.u2(theta0)

    def al_espacio(q):
        """Un punto (r, h) del plano, en coordenadas del brazo."""
        return [q[0] * u[0], q[0] * u[1], c.z_hombro + q[1]]

    figuras = {
        "home": cinematica.cinematica_directa((0.0, 0.0, 0.0, 0.0, 0.0)).puntos,
        "inicial": cinematica.cinematica_directa(pose(0.0, 0.0)).puntos,
        "arriba": cinematica.cinematica_directa(pose(*exactas["arriba"])).puntos,
        "abajo": cinematica.cinematica_directa(pose(*exactas["abajo"])).puntos,
        "sin-regla": cinematica.cinematica_directa(pose(v1.theta1, v1.theta2)).puntos,
        "con-regla": cinematica.cinematica_directa(pose(v3.theta1, v3.theta2)).puntos,
        "codo-forward": al_espacio(v3.traza[1].codo_forward),
        "codo-reflejado": al_espacio(v3.traza[1].codo_espejado),
        "latigo": v0_latigo.calcular()["puntos"],
        "objetivo-plano": [r_c * u[0], r_c * u[1], OBJETIVO[2]],
        "u-plano": u,
        "superior": [*OBJETIVO_COSTADO[:2], superior["phi"], superior["theta0"], superior["r_c"], superior["d"]],
    }
    return {
        "paso1": paso1, "superior": superior, "traza_v3": v3.traza, "figuras": figuras, "exactas": exactas,
        "v3": {"angulos": pose(v3.theta1, v3.theta2), "p5": p5(v3.theta1, v3.theta2), "error": v3.error,
               "iteraciones": v3.iteraciones},
        "v1": {"angulos": (v1.theta1, v1.theta2), "error": v1.error, "iteraciones": v1.iteraciones},
        "v1_recortado": {"angulos": (t1_recortado, t2_recortado),
                         "error": float(np.linalg.norm(p5(t1_recortado, t2_recortado) - OBJETIVO))},
        "v2": {"angulos": (v2.theta1, v2.theta2), "error": v2.error, "iteraciones": v2.iteraciones,
               "motivo": v2.motivo, "hombro_sin_recortar": plano.ang(v2.traza[1].codo_forward),
               "hombro_recortado": plano.ang(v2.traza[1].codo)},
        "v2_traba": {"angulos": (traba.theta1, traba.theta2), "error": traba.error, "iteraciones": traba.iteraciones,
                     "motivo": traba.motivo, "hombro_sin_recortar": plano.ang(traba.traza[1].codo_forward),
                     "hombro_recortado": plano.ang(traba.traza[1].codo)},
    }


def main():
    """Imprime la salida y la guarda en resultados/ejemplo_principal.txt."""
    r = calcular()
    p, s = r["paso1"], r["superior"]
    lineas = [f"Objetivo {tuple(_redondear(OBJETIVO, 1))} con la muñeca en {cfg.MUNECA_NEUTRA}° y la pinza en "
              f"{cfg.PINZA_NEUTRA}°",
              "",
              f"Sección 5.2 (Paso 1): ρ = {p['rho']:.2f} cm, d = {p['d']:.2f} cm, r_c = {p['r_c']:.2f} cm, "
              f"θ0 = {p['phi']:.2f}° - ({p['desvio']:.2f}°) = {p['theta0']:.2f}°, T = {_redondear(p['T'], 2)} cm",
              f"Figura 4: {tuple(_redondear(OBJETIVO_COSTADO, 1))} con la muñeca en {s['muneca']:.0f}° y la pinza en "
              f"{s['pinza']:.0f}°: φ = {s['phi']:.2f}°, d = {s['d']:.2f} cm, r_c = {s['r_c']:.2f} cm, "
              f"θ0 = {s['theta0']:.2f}°, error {s['error']:.2f} cm",
              "",
              "Tabla 6: iteraciones de la v3 desde la pose cero (cm, dentro del plano)"]
    for paso in r["traza_v3"]:
        if paso.k == 0:
            lineas.append(f"  k = 0: codo {_redondear(paso.codo, 2)}, punta {_redondear(paso.punta, 2)}, "
                          f"error {paso.error:.2f}")
        else:
            espejo = f", espejado a {_redondear(paso.codo_espejado, 2)}" if paso.codo_espejado else ""
            lineas.append(f"  k = {paso.k}: forward deja el codo en {_redondear(paso.codo_forward, 2)}{espejo}; "
                          f"backward: codo {_redondear(paso.codo, 2)}, punta {_redondear(paso.punta, 2)}, "
                          f"error {paso.error:.2f}")
    v3 = r["v3"]
    lineas += [f"Sección 5.6: ángulos (θ0, θ1, θ2) = {_redondear(v3['angulos'][:3], 2)}, p5 = {_redondear(v3['p5'], 2)}, "
               f"a {np.linalg.norm(v3['p5'] - OBJETIVO):.2f} cm",
               "",
               f"Sección 6.1: codo arriba (θ1, θ2) = {_redondear(r['exactas']['arriba'], 2)}, "
               f"codo abajo = {_redondear(r['exactas']['abajo'], 2)}",
               f"Sección 6.2 (v1): (θ1, θ2) = {_redondear(r['v1']['angulos'], 2)}, error {r['v1']['error']:.2f} cm "
               f"en {_iteraciones(r['v1']['iteraciones'])}",
               f"Sección 6.3 (v1 con recorte al final): {_redondear(r['v1_recortado']['angulos'], 2)}, p5 a "
               f"{r['v1_recortado']['error']:.2f} cm",
               f"Sección 6.4 (v2): el primer backward deja el hombro en {r['v2']['hombro_sin_recortar']:.2f}° y el "
               f"recorte lo lleva a {r['v2']['hombro_recortado']:.2f}°; termina en {_redondear(r['v2']['angulos'], 2)}, "
               f"error {r['v2']['error']:.2f} cm, {_iteraciones(r['v2']['iteraciones'])}, corta por {r['v2']['motivo']}",
               f"Sección 6.4 (v2 con {tuple(_redondear(OBJETIVO_TRABA, 1))}): el primer backward deja el hombro en "
               f"{r['v2_traba']['hombro_sin_recortar']:.2f}° y el recorte lo lleva a "
               f"{r['v2_traba']['hombro_recortado']:.2f}°; queda en {_redondear(r['v2_traba']['angulos'], 2)}, error "
               f"{r['v2_traba']['error']:.2f} cm, {_iteraciones(r['v2_traba']['iteraciones'])}, corta por "
               f"{r['v2_traba']['motivo']}",
               "",
               "Coordenadas de las figuras (cm)"]
    for nombre, puntos in r["figuras"].items():
        lineas.append(f"  {nombre}: {_redondear(puntos)}")
    guardar("ejemplo_principal", lineas)


if __name__ == "__main__":
    main()
