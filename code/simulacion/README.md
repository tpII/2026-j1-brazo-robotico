# Simulación del brazo en Python

Esta carpeta tiene el firmware del ESP32 escrito en Python, para programar y probar el brazo simulado. Cada archivo corresponde a un archivo de C del firmware, con las mismas funciones y los mismos datos.

## Primeros pasos

Se ejecuta desde (`code/simulacion/`). Se necesita la libreria numpy.

1. Abrir el gemelo digital. Se levanta el emulador del ESP32:

   ```bash
   .venv/bin/python servidor.py
   ```

   y se entra a `http://localhost:8000`.

2. Correr los tests:

   ```bash
   .venv/bin/python -m unittest discover -s tests
   ```

3. Regenerar los resultados del informe. Cada experimento guarda su salida en `resultados/<nombre>.txt`:

   ```bash
   .venv/bin/python -m experimentos.todos
   ```

   Para correr uno solo: `.venv/bin/python -m experimentos.<nombre>`.

## La página del gemelo digital

**Cómo se elige un objetivo.** Se escribe «x, y, z» en el cuadro y se aprieta «Mover», o se hacen dos clics sobre la vista: el primero, sobre el piso, fija x e y y levanta una línea vertical; el segundo, sobre esa línea, fija la altura. Escape cancela y «Detener» frena el brazo donde está.

**Qué se ve.**
- El brazo en 3D, a tamaño real, con las piezas del brazo armado. Sigue a la cinemática directa, porque cada pieza se ubica con los marcos que manda la telemetría.
- Las esferas de colisión, transparentes. Se ocultan con «Mostrar las esferas de colisión», salvo las que chocan, que siempre se ven en rojo.
- Mientras se mueve, en azul transparente, la pose a la que va.
- Si no llega, en gris, el mejor intento (la silueta) con el motivo: «fuera de alcance», «bloqueado por límites» o «colisión».
- El objetivo, con una línea punteada hasta el piso.

**Qué dice el panel.**
- *Error:* cuánto le falta a la punta de la pinza para llegar al objetivo. Se acepta hasta 0.5 cm.
- *Iteraciones:* las de FABRIK en la pose elegida (20 como máximo) y, entre paréntesis, las de todo lo que probó la búsqueda.
- *Posiciones:* cuántas combinaciones de muñeca y pinza probó la búsqueda, de 361 posibles.
- *Cálculo:* cuánto tardó la búsqueda.

El emulador atiende los pedidos del firmware (`/data`, `/set` y `/detener`) y además sirve la página:

| Pedido | Qué hace |
|---|---|
| `GET /` | La página |
| `GET /web/...` | El estilo y los scripts de la página |
| `GET /data` | La telemetría en JSON |
| `POST /set` | Manda un objetivo con los campos `x`, `y` y `z` |
| `POST /detener` | Frena el brazo |

Con el brazo no se usa nada de esta carpeta: el README del repositorio explica cómo se usa.

## Qué hay en cada archivo

Lo que se pasó a C, en `code/firmware/`:

| Archivo | Para qué sirve | En C |
|---|---|---|
| `configuracion.py` | Medidas del brazo, esferas, pulsos de los servos y parámetros de FABRIK y del movimiento | `configuracion.c` y los arreglos de `firmware.ino` |
| `algebra_lineal.py` | Rotaciones y traslaciones con matrices 4x4 | `algebra_lineal.c` |
| `servos.py` | Pasar de pulso a grados del servo y a grados del modelo, rangos y home | `servos.c` |
| `cinematica.py` | Cinemática directa: dónde queda cada articulación y cada esfera | `cinematica.c` |
| `colisiones.py` | Choques entre eslabones y contra el piso | `colisiones.c` |
| `plano.py` | Lo que comparten las versiones de FABRIK: la cadena virtual, los planos de la base y el control final | `plano.c` |
| `fabrik.py` | FABRIK v3, la versión elegida | `fabrik.c` |
| `busqueda.py` | Elige la muñeca y la pinza y arma el resultado | `busqueda.c` |
| `trayectoria.py` | Mueve el brazo sin pasarse de velocidad y revisa que el camino no choque | `trayectoria.c` |
| `centralizador.py` | Estado del brazo, objetivo, Detener y telemetría | `centralizador.c` y el `loop()` de `firmware.ino` |
| `servidor.py` | El emulador del ESP32 | El servidor web de `firmware.ino` |
