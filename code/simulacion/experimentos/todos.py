"""Corre todos los experimentos, en el orden de los informes, y regenera resultados/.

Se corre desde code/simulacion/ con: python -m experimentos.todos
"""
import importlib
import time

ORDEN = (
    "pose_cero_y_home",      # Implementación, Tabla 1, sección 2.2 y Tabla 9
    "v0_latigo",             # Tabla 3 y Figura 3
    "formula_ingenua",       # Tabla 4
    "cadena_virtual",        # Tabla 5 y sección 5.1
    "ejemplo_principal",     # secciones 5.2 a 6.4, Tabla 6 y figuras 4 a 7
    "medicion_versiones",    # Tabla 7, Tabla 10 y Figura 9
    "regla_del_codo",        # sección 7.3
    "cadena_recta",          # sección 7.3 y Tabla 8 (η)
    "medicion_busqueda",     # sección 9 y Tabla 8 (k_max)
    "reglas_esferas",        # Investigación de modelos, sección 4.5
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
