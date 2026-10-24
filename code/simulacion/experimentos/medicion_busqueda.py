"""La búsqueda de la muñeca y la pinza sobre los 2000 objetivos.

Desde el home del firmware y sin usar la muñeca ni la pinza con las que se generó cada objetivo. Como en el
informe, sin revisar choques (los choques se prueban en tests/test_busqueda.py).
"""
import math

import numpy as np

import busqueda
import configuracion as cfg
import fabrik
import plano
import servos
from experimentos import guardar, objetivos_medicion

EJEMPLO = np.array([3.0, -3.0, 0.0])   # sobre el piso, cerca de la base: con la pinza alineada no llega


def _en_tope(angulo, lo, hi):
    """Si un ángulo quedó en uno de sus topes."""
    return abs(angulo - lo) < 1e-6 or abs(angulo - hi) < 1e-6


def calcular():
    """Busca los 2000 objetivos desde el home, sin choques, y cuenta cómo terminan las ejecuciones.

    Returns:
        Un diccionario con los resueltos con la muñeca y la pinza neutras y con la búsqueda (y cuántos
        de estos con la muñeca girada), las posiciones y las iteraciones (promedio y mediana), cuántas
        ejecuciones no llegan y por qué cortan, las fallas y el ejemplo (3, -3, 0).
    """
    objetivos = objetivos_medicion()
    home = servos.home_modelo()
    neutra = sum(fabrik.resolver(item["objetivo"], cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA, home[1], home[2]).valida
                 for item in objetivos)
    resultados = [busqueda.buscar(item["objetivo"], home, revisar_colisiones=False) for item in objetivos]

    ejecuciones = [c for r in resultados for c in r.detalle_ejecuciones]
    no_llegan = [c for c in ejecuciones if c.error > cfg.EPSILON]
    por_kmax = [c for c in no_llegan if c.motivo == "kmax"]
    con_recortes = [c for c in por_kmax if c.recortes > 0]
    (lo1, hi1), (lo2, hi2) = servos.rango_modelo(1), servos.rango_modelo(2)
    tope_codo = [c for c in con_recortes if _en_tope(plano.ajustar_angulo(c.theta2, lo2, hi2)[0], lo2, hi2)
                 and not _en_tope(c.theta1, lo1, hi1)]

    fallas = []
    for i, (item, r) in enumerate(zip(objetivos, resultados)):
        if r.estado != "ok":
            o = item["objetivo"]
            fallas.append({"indice": i, "objetivo": o, "rho": math.hypot(o[0], o[1]), "estado": r.estado,
                           "muneca_generadora": item["angulos"][3], "pinza_generadora": item["angulos"][4]})

    n = fabrik.resolver(EJEMPLO, cfg.MUNECA_NEUTRA, cfg.PINZA_NEUTRA, home[1], home[2])
    e = busqueda.buscar(EJEMPLO, home, revisar_colisiones=False)
    ejemplo = {"valida_neutra": n.valida, "error_neutra": min(p.ejecucion.mejor_error for p in n.planos if p.ejecucion),
               "estado": e.estado, "muneca": e.angulos[3], "pinza": e.angulos[4], "angulos": e.angulos,
               "error": e.error, "posiciones": e.posiciones}

    resueltos = [r for r in resultados if r.estado == "ok"]
    return {"n": len(objetivos), "neutra": neutra, "resueltos": len(resueltos),
            "con_muneca": sum(r.angulos[3] != cfg.MUNECA_NEUTRA for r in resueltos),
            "posiciones_promedio": float(np.mean([r.posiciones for r in resultados])),
            "posiciones_mediana": float(np.median([r.posiciones for r in resultados])),
            "posiciones_maximo": max(r.posiciones for r in resultados),
            "iteraciones_promedio": float(np.mean([r.iteraciones_totales for r in resultados])),
            "iteraciones_mediana": float(np.median([r.iteraciones_totales for r in resultados])),
            "no_llegan": len(no_llegan), "por_kmax": len(por_kmax), "por_eta": sum(c.motivo == "eta" for c in no_llegan),
            "kmax_con_recortes": len(con_recortes), "kmax_tope_codo": len(tope_codo),
            "fallas": fallas, "ejemplo": ejemplo}


def main():
    """Imprime la salida y la guarda en resultados/medicion_busqueda.txt."""
    r = calcular()
    n = r["n"]
    lineas = [f"Sección 9: búsqueda de la muñeca y la pinza sobre {n} objetivos, desde el home, sin choques",
              f"  muñeca y pinza neutras: {r['neutra']} ({100 * r['neutra'] / n:.2f} %); búsqueda: {r['resueltos']} "
              f"({100 * r['resueltos'] / n:.2f} %)",
              f"  de los resueltos, {r['resueltos'] - r['con_muneca']} con la muñeca neutra (la neutra y la pinza "
              f"sola) y {r['con_muneca']} con la muñeca girada y la pinza",
              f"  posiciones probadas: {r['posiciones_promedio']:.3f} en promedio, mediana "
              f"{r['posiciones_mediana']:.0f} (máximo {r['posiciones_maximo']}); iteraciones de FABRIK por objetivo: "
              f"{r['iteraciones_promedio']:.2f} en promedio, mediana {r['iteraciones_mediana']:.0f}",
              f"  objetivos sin resolver ({len(r['fallas'])}):"]
    for f in r["fallas"]:
        lineas.append(f"    #{f['indice']}: {np.round(f['objetivo'], 2).tolist()}, a {f['rho']:.2f} cm del eje de la base "
                      f"({f['estado']}; generado con la muñeca en {f['muneca_generadora']:.1f}° y la pinza en "
                      f"{f['pinza_generadora']:.1f}°)")
    e = r["ejemplo"]
    lineas += [f"  ejemplo {tuple(float(v) for v in EJEMPLO)}: con la muñeca y la pinza neutras queda a "
               f"{e['error_neutra']:.2f} cm; la búsqueda llega con la muñeca en {e['muneca']:+.0f}° y la pinza en "
               f"{e['pinza']:.0f}°, a {e['error']:.2f} cm, después de {e['posiciones']} posiciones "
               f"(ángulos {np.round(e['angulos'][:3], 2).tolist()})",
               "",
               f"Tabla 8 (k_max): de las {r['no_llegan']} ejecuciones que no llegan, {r['por_kmax']} cortan por k_max "
               f"({100 * r['por_kmax'] / r['no_llegan']:.0f} %) y {r['por_eta']} por η",
               f"  de las {r['por_kmax']} por k_max, {r['kmax_con_recortes']} recortaron algún ángulo y "
               f"{r['kmax_tope_codo']} terminan contra un tope del codo"]
    guardar("medicion_busqueda", lineas)


if __name__ == "__main__":
    main()
