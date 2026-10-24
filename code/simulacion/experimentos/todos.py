"""Corre todos los experimentos, en el orden de los informes, y regenera resultados/.

Se corre desde code/simulacion/ con: python -m experimentos.todos
"""
import importlib
import time

ORDEN = (
    "pose_cero_y_home",
    "v0_latigo",
    "formula_ingenua",
    "cadena_virtual",
    "ejemplo_principal",
    "medicion_versiones",
    "regla_del_codo",
    "medicion_busqueda",
    "reglas_esferas",
)


def main():
    """Corre los experimentos en orden y muestra cuánto tardó cada uno."""
    for nombre in ORDEN:
        comienzo = time.perf_counter()
        print(f"=== {nombre}")
        importlib.import_module(f"experimentos.{nombre}").main()
        print(f"({time.perf_counter() - comienzo:.1f} s)\n")


if __name__ == "__main__":
    main()
