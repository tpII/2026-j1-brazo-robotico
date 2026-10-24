# Simulación del brazo (Hito 2, etapa 1)

Simulación en Python del firmware del ESP32, para programar y probar el Hito 2 sin el brazo. Cada módulo corresponde a un módulo de C del firmware, con las mismas funciones y los mismos datos, así el pasaje a C es una traducción.

La simulación también respalda los informes. Cada número de `docs/investigaciones/implementacion-fabrik.pdf` y de la parte de esferas de `investigacion-modelos.pdf` sale de un experimento de esta carpeta, y un test verifica que dé lo mismo que el informe (tabla de trazabilidad, al final).

## Cómo se usa

Todos los comandos se corren desde esta carpeta (`code/simulacion/`). El entorno `.venv` ya tiene numpy, que es la única dependencia además de la biblioteca estándar.

- Todos los tests:

  ```bash
  .venv/bin/python -m unittest discover -s tests
  ```

- Sin los tests lentos de `tests/test_medicion.py`, que recorren miles de poses u objetivos:

  ```bash
  RAPIDO=1 .venv/bin/python -m unittest discover -s tests
  ```

- Un experimento, que imprime su salida y la guarda en `resultados/<nombre>.txt`:

  ```bash
  .venv/bin/python -m experimentos.<nombre>
  ```

- El emulador del ESP32, con la página del gemelo digital en `http://localhost:8000`:

  ```bash
  .venv/bin/python servidor.py
  ```

  Responde a los mismos pedidos que va a atender el firmware: `GET /` (la página `code/index.html`), `GET /web/...` (su estilo y sus módulos de JavaScript, de `code/web/`), `GET /data` (la telemetría en JSON), `POST /set` con los campos `x`, `y` y `z`, y `POST /detener`. Por ejemplo, `curl -X POST -d 'x=5&y=-5&z=7' localhost:8000/set`.

## El gemelo digital

La página es `code/index.html`, con el estilo en `code/web/estilos.css` y el código en módulos de `code/web/`. Hay que abrirla desde el emulador (`http://localhost:8000`) o desde el ESP32, no como archivo, porque el navegador no carga módulos desde `file://`. Dibuja con Three.js, en las coordenadas del robot y con z hacia arriba:
- el brazo en 3D, armado con las piezas del brazo real (soporte en X, servos SG90, placas impresas y la mano en U) en el marco de cada eslabón que manda la telemetría (`marcos`), así que sigue exactamente a la cinemática directa. Las piezas van a tamaño real, con las formas de las fotos del brazo armado (23-oct);
- las esferas de colisión de cada eslabón, transparentes, que se ocultan con «Mostrar las esferas de colisión»; las que chocan se ven siempre, en rojo;
- mientras se mueve, la pose a la que va, en azul transparente;
- cuando no llega, la silueta gris del mejor intento con su mensaje («fuera de alcance», «bloqueado por límites» o «colisión»);
- el objetivo, con una línea punteada hasta el piso.

El objetivo se escribe como «x, y, z» o se elige con dos clics: el primero sobre el piso fija x e y y levanta una línea vertical, y el segundo, sobre esa línea, fija la altura z y lo manda. Escape cancela. «Detener» frena el brazo en el paso en que está.

Los datos del panel:
- **Error:** distancia entre el objetivo y p5, la punta de la pinza, en la pose elegida. Se acepta hasta ε = 0.5 cm. Si no llega, es la del mejor intento.
- **Iteraciones:** las de FABRIK en la pose elegida (20 como máximo) y, entre paréntesis, las de todos los planos y posiciones que probó la búsqueda.
- **Posiciones:** cuántas posiciones de la muñeca y la pinza probó la búsqueda, de 361, hasta encontrar una pose que llega sin chocar. No cuenta las que saltea por tener la misma cadena que una que ya falló.
- **Cálculo:** lo que tardó la búsqueda.

La página necesita internet para bajar Three.js. Con `?servidor=http://IP` lee de otra dirección: por ejemplo, `http://localhost:8000/?servidor=http://192.168.0.193` muestra la pose que manda el ESP32 con el firmware de 2025, que todavía no atiende `/set` ni manda los marcos, así que se ven solo las esferas.

## Archivos

La columna «Origen» separa el código del grupo J5 (2025) del nuestro (J1, 2026). En los archivos que vienen de 2025, lo de J5 va arriba y lo nuevo al final, con un comentario `# 2026` arriba de cada definición. Las funciones siguen el formato de docstring de Google (Args, Returns, Raises), con las descripciones en castellano.

| Archivo | Qué hace | Equivale en C a | Origen |
|---|---|---|---|
| `configuracion.py` | Geometría, esferas, matriz de colisiones, pulsos, tablas de calibración y parámetros de FABRIK, de la búsqueda y del movimiento | `configuracion.c` y los arreglos de `main.ino` | J5 (nombres de 2025, secciones 1 a 3) y J1 (secciones 4 a 9; la 8 y la 9 ponen las medidas del brazo armado) |
| `algebra_lineal.py` | Rotaciones y traslaciones 4×4 | `algebra_lineal.c` y `numpy.c` | J5, igual que en 2025 salvo la documentación |
| `servos.py` | Pulso ↔ grados del servo ↔ grados del modelo, rangos, home y pulsos de una pose | `getAngleForServo` y `home()` de `main.ino` (en C, `servos.c`) | J1 |
| `cinematica.py` | Cinemática directa: puntos p0 a p5, ejes de giro, marcos de cada eslabón y centros de las esferas | `calcular_configuracion_modular` de `cinematica.c` | Archivo de J5, rehecho por J1: no queda ninguna función de 2025 |
| `colisiones.py` | Choques entre eslabones (matriz de colisiones y junta compartida) y contra el piso | `detectar_colision_*` de `cinematica.c` (en C, `colisiones.c`) | J1, con las reglas de J5 |
| `plano.py` | Lo común a v1, v2 y v3: cadena virtual, Paso 0, Paso 1, Paso 3, recorte angular, cadena estirada, control final y resolución de los planos en orden | `plano.c` (nuevo) | J1 |
| `fabrik.py` | FABRIK v3, la versión adoptada: límites en cada paso y regla del codo | `fabrik.c` (nuevo) | J1 |
| `fabrik_v1.py`, `fabrik_v2.py` | Versiones descartadas, solo para respaldar el informe: v1 sin límites (y con el espejo al final de la Tabla 7), v2 con límites y sin regla | No se portan | J1 |
| `fabrik_v0.py` | Versión descartada: FABRIK 3D con rótulas libres (Algoritmo 1 del paper), solo para respaldar el informe | No se porta | J1 |
| `trayectoria.py` | Movimiento seguro: camino recto en ángulos, 60 °/s por articulación, 10 cm/s para cualquier punto del brazo, rampas de 0.3 s y un paso cada 20 ms; primer paso que choca | `trayectoria.c` (nuevo) | J1 |
| `busqueda.py` | La búsqueda de la muñeca y la pinza (361 posiciones: cada muñeca con cada pinza), los planos, la silueta y el resultado (`ok`, `fuera_de_alcance`, `bloqueado_por_limites`, `colision`) | `busqueda.c` (nuevo) | J1 |
| `centralizador.py` | El «centralizador de lógica» del Plan: estado del brazo, pedir un objetivo, avanzar un paso, «Detener» y la telemetría JSON | `centralizador.c` y el `loop()` de `main.ino` | J1 |
| `servidor.py` | Emulador del ESP32: el mismo bucle que `loop()` y los pedidos `/`, `/web/...`, `/data`, `/set` y `/detener` | El `WebServer` de `main.ino` | J1 |
| `herramientas/referencia_firmware.c` | Calcula con los `.c` del firmware las esferas y los choques de 402 poses | — | J1 |
| `datos/referencia_firmware_2025.json` | La salida de ese programa, con la que se comparan `cinematica.py` y `colisiones.py` | — | J1, con el C de J5 |
| `datos/objetivos_semilla_12345.csv` | Los 2000 objetivos de las mediciones (con los cinco ángulos que los generaron y la pose al azar de inicio), para revisarlos sin correr nada | — | J1 |
| `tests/` | Un `test_<módulo>.py` por módulo, más `test_figuras.py` y `test_medicion.py` | — | J1 |
| `experimentos/` | Un script por resultado de los informes | — | J1 |
| `resultados/` | La salida de cada experimento | — | J1 |

`tests/test_configuracion.py` lee `code/firmware/configuracion.c` y `main.ino` y verifica que `configuracion.py` tenga los mismos valores. Si cambia el firmware, el test avisa. La excepción son los saltos: `configuracion.py` los reemplaza por las medidas del brazo armado (la punta de la pinza el 22-oct y el resto el 23-oct), y el test deja registrada la diferencia con el firmware hasta el pasaje a C. Por eso los tests que comparan con los datos del firmware de 2025 usan los saltos de `configuracion.c`.

## Regenerar los datos del firmware

`datos/referencia_firmware_2025.json` se generó con `cinematica.c` y `configuracion.c` de `code/firmware/`, que son los del grupo J5 (2025) sin cambios. No se usa `main.ino`. Para regenerarlo hace falta un compilador de C (`cc`):

```bash
mkdir -p build && cc -O2 -I ../firmware -o build/referencia_firmware herramientas/referencia_firmware.c ../firmware/cinematica.c ../firmware/configuracion.c ../firmware/algebra_lineal.c ../firmware/numpy.c -lm
```

```bash
build/referencia_firmware > datos/referencia_firmware_2025.json
```

La carpeta `build/` está en `.gitignore`.

## Trazabilidad con los informes

«Impl.» es `implementacion-fabrik.pdf` e «Inv.» es `investigacion-modelos.pdf`, con la numeración del PDF. La salida de cada experimento está en `resultados/<experimento>.txt`. Para regenerarlas todas se corre `.venv/bin/python -m experimentos.todos`, que tarda unos 15 segundos.

| Informe | Dato | Experimento | Test |
|---|---|---|---|
| Impl. Tabla 1 y Fig. 2 | Puntos y ejes en la pose cero | `pose_cero_y_home` | `test_cinematica` |
| Impl. 2.2 y Tabla 9; Inv. tabla de rangos | Home (0, 140, −114.6, 4.5, 0) desde los pulsos; pulsos y rango 0–180° de cada servo | `pose_cero_y_home` | `test_servos`, `test_configuracion` |
| Impl. Tabla 3 y Fig. 3 | v0: columna inclinada 2.3°, hombro movido a (0.12, 0.00, 3.00), 73.9° en p3 | `v0_latigo` | `test_fabrik_v0` |
| Impl. Tabla 4 | La fórmula del ángulo con el propio eje da siempre (0, 90, 90, 180, 90) | `formula_ingenua` | `test_plano` |
| Impl. Tabla 5 y 5.1 | Cadena virtual según la muñeca y la pinza; l1 + l2 = 13.61 cm (de 10.40 a 13.62 según la pinza) | `cadena_virtual` | `test_plano` |
| Impl. 5.2 y Fig. 4 | ρ = r_c = 7.07, d = 0, θ0 = −45°, T = (7.07, 4.00); Fig. 4: (6, 1, 6) con la muñeca en 20° y la pinza en 140°, d = 1.13, θ0 = −1.21° | `ejemplo_principal` | `test_plano`, `test_ejemplo_principal` |
| Impl. Tabla 6 y 5.6 | Iteraciones del ejemplo (5, −5, 7); ángulos (−45, 149.04, −138.73); p5 = (5.10, −5.10, 7.03) | `ejemplo_principal` | `test_fabrik`, `test_ejemplo_principal` |
| Impl. 6.1 y Fig. 5 | Codo arriba (152.81, −141.61) y codo abajo (−93.82, 136.05) | `ejemplo_principal` | `test_ejemplo_principal` |
| Impl. 6.2, 6.3 y Fig. 6 | v1: (−90.05, 133.17); recorte al final: (180, −180), 3.50 cm | `ejemplo_principal` | `test_fabrik_v1`, `test_ejemplo_principal` |
| Impl. 6.4 | v2: en el ejemplo llega a 0.49 cm porque el recorte lleva el hombro a 180°; con (5, −5, 8) se traba en (0, 0), 7.93 cm, y corta por η | `ejemplo_principal` | `test_fabrik_v2`, `test_ejemplo_principal` |
| Impl. Fig. 3 a 7 | Coordenadas de las poses dibujadas y de la vista superior, comparadas con las del `.typ` | `ejemplo_principal`, `v0_latigo` | `test_figuras` |
| Impl. Tabla 7, Tabla 10 y Fig. 9 | 864/1408, 1969/1876, 1073/1545, 1997/1992; 1.7 iteraciones; el tiempo por objetivo | `medicion_versiones` | `test_medicion` (lento) |
| Impl. 7.3 | Fronteras de la regla en θ2 = −α y θ2 = −180° − α; la segunda entra en el rango del codo con la pinza por debajo de unos 83.3°, y su espejo se toma módulo 360° | `regla_del_codo` | `test_fabrik` |
| Impl. 7.3 y Tabla 8 (η) | Cadena recta: corta por η y la búsqueda llega con la pinza en 10° | `cadena_recta` | `test_busqueda` |
| Impl. 8.1 | Recorte angular: −170° va a 180° | — | `test_plano` |
| Impl. Tabla 8 (ε) | Con el alcance máximo de 13.62 cm, un grado en el hombro mueve la punta hasta 0.24 cm | `cadena_virtual` | `test_plano` |
| Impl. Tabla 8 (k_max) | Una de cada cuatro ejecuciones que no llegan corta por k_max (1573 de 6161), la mayoría contra un tope del codo (962) | `medicion_busqueda` | `test_medicion` (lento) |
| Impl. 9 | 1566 (78.3 %), 1999 (99.95 %), 38 con la muñeca; 4.3 posiciones y 23.7 iteraciones (mediana 2); 1 falla en el borde del alcance; (3, −3, 0) a 1.82 cm con la pinza neutra, con la pinza en 30° y 0.27 cm | `medicion_busqueda` | `test_busqueda`, `test_medicion` (lento) |
| Inv. 4.5 | Reglas de exclusión de esferas: pares (0, 2) y (1, 3), los 2 cm del par (2, 4), los 1.3 cm de la junta (1, 2), la esfera de la muñeca | `reglas_esferas` | `test_colisiones`, `test_medicion` (lento) |

Notas:
- La búsqueda con choques, el resultado «bloqueado por límites» y la trayectoria no tienen números en los informes. Se prueban en `test_busqueda`, `test_trayectoria` y `test_centralizador`.
- Algunos valores no salen de la simulación. ε = 0.5 cm, η y k_max los eligió el grupo, y la Tabla 2 viene del paper.
- El tiempo por objetivo de la Tabla 10 depende de la máquina. En una Mac del grupo, la v3 tarda entre 0.10 y 0.11 ms por objetivo; por eso el informe dice «unos 0.1 ms».
- Todos los tests tardan unos 20 segundos; con `RAPIDO=1`, unos 6.
