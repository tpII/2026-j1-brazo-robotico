#import "@preview/barcala:0.3.0": informe
#import "@preview/zero:0.3.2": *
#let norm(x) = $lr(‖ #x ‖)$

#show: informe.with(
  institucion: "unlp",
  unidad-academica: "informática",
  asignatura: "Taller de Proyecto II",
  equipo: "Grupo J1",
  autores: (
    (
      nombre: "Tarifa, Armando Ezequiel",
      email: "ezequiel.tarifa.2002@gmail.com",
      legajo: "02893/4",
    ),
    (
      nombre: "Guitera Woida, Emma",
      email: "emmagw04@gmail.com",
      legajo: "03352/2",
    ),
    (
      nombre: "Terruzzi, Ana Justina",
      email: "justinaterruzzi@gmail.com",
      legajo: "02557/9",
    ),
  ),
  titulo: "Investigación de Modelos: Cinemática Inversa y Detección de Autocolisiones",
  fecha: "2026-10-02",
)

#outline(
  title: "Índice",
  depth: 3,
  indent: auto,
)

#pagebreak()

= Introducción

En este documento de investigación se recopila la fundamentación teórica, comparación de algoritmos y decisiones de diseño correspondientes a los hitos *H1.2* (Cinemática inversa iterativa) y *H1.3* (Modelo de esferas para detección de autocolisiones) del proyecto J1.

A lo largo del informe se referencia lo que el grupo J5 del año 2025 implementó en su proyecto @grupo_j5_2025, para establecer qué se hereda directamente, qué se modifica y qué es nuevo en nuestro proyecto.

= Modelos implementados heredados

Antes de avanzar con modelos nuevos, se mencionan los modelos implementados por el grupo anterior @grupo_j5_2025.

== Cinemática Directa (FK) - Implementada en 2025

El grupo J5 modeló la cinemática directa con matrices homogéneas de rotación y traslación que se aplican en cadena. La implementación en C resultante construye cada matriz $T_(i-1)^i$ combinando una rotación de corrección de montaje ($R_"fix"$), la rotación del servo y la traslación del eslabón.

La cadena cinemática del brazo de 5 grados de libertad se modela como:

$ T_"total" = T_0^1 dot.op T_1^2 dot.op T_2^3 dot.op T_3^4 dot.op T_4^5 $

La secuencia de ejes de rotación es *Z-Y-Y-X-Y* para los servos 0 a 4. Cada $R_"fix"$ alinea el eje físico del servo con el eje $Z$ matemático para que todas las rotaciones se expresen uniformemente.

Esta implementación fue codificada en C (`cinematica.c`, `algebra_lineal.c`, `numpy.c`) para el ESP32 y replicada en Python (`cinematica.py`, `algebra_lineal.py`) para simulación.

Se hereda para el proyecto 2026 sin modificaciones en la lógica de cinemática directa.

== Cinemática Inversa (IK) - No implementada en 2025

El proyecto del 2025 *no implementó cinemática inversa*. El brazo se controlaba exclusivamente mediante un potenciómetro analógico y botones pulsadores: el operador seleccionaba un servo y lo rotaba manualmente. El sistema verificaba colisiones con cada pose resultante, pero no existía la posibilidad de enviar un destino $(X, Y, Z)$ y que el sistema calculara automáticamente los ángulos necesarios.

La cinemática inversa se implementa en nuestro proyecto 2026.

== Detección de Autocolisiones - Implementada en 2025

El grupo J5 implementó un sistema de detección de autocolisiones basado en *esferas delimitadoras* @grupo_j5_2025. El modelo define 9 esferas distribuidas en los 5 eslabones con radio uniforme $r = 1.0 "cm"$, verificando pares de esferas filtrados por una matriz booleana 5×5 para evitar falsos positivos entre eslabones adyacentes.

Se hereda para el proyecto 2026, con otra cantidad, tamaño y ubicación de las esferas, otros pares verificados (@sec-esferas).

#pagebreak()
= Cinemática Inversa Iterativa - Hito 1.2

== Definición del problema

Dado un punto objetivo $bold(p)^* in RR^3$ en el espacio de trabajo del brazo, el problema de la cinemática inversa consiste en encontrar el vector de ángulos articulares:

$ bold(theta)^* = (theta_0^*, theta_1^*, theta_2^*, theta_3^*, theta_4^*) $

tal que la evaluación de la cinemática directa resulte en dicho punto:

$ "FK"(bold(theta)^*) = bold(p)^* $

Para un brazo de 5 grados de libertad con geometría no esférica, la resolución analítica cerrada es altamente compleja y propensa a errores de implementación debido a la cantidad de casos y condiciones. Por esta razón, se opta por un enfoque numérico iterativo capaz de ejecutarse eficientemente en el ESP32.

== Algoritmos evaluados

Se estudiaron y compararon cuatro métodos de cinemática inversa iterativa:

=== Jacobiano transpuesto

Calcula el Jacobiano $J(bold(theta)) in RR^(3 times n)$ y actualiza los ángulos con:

$ Delta bold(theta) = alpha dot.op J^T dot.op (bold(p)^* - "FK"(bold(theta))) $

- *Ventajas:* No requiere inversión de matrices.
- *Desventajas:* Convergencia lenta (1311 iteraciones en promedio en la Tabla 1 del paper @aristidou2011fabrik), movimientos que pueden oscilar, requiere elegir $alpha$ adecuadamente.

=== Damped Least Squares (DLS)

Resuelve el sistema regularizado:

$ Delta bold(theta) = (J^T J + lambda^2 I)^(-1) J^T dot.op (bold(p)^* - "FK"(bold(theta))) $

- *Ventajas:* Robusto frente a singularidades gracias al parámetro de amortiguación $lambda$.
- *Desventajas:* Requiere invertir una matriz $n times n$ en cada iteración, costoso para el ESP32, y hay que elegir el parámetro $lambda$ (el paper usa un valor fijo, 1.1).

=== CCD (Cyclic Coordinate Descent)

Ajusta un ángulo por vez, recorriendo las articulaciones cíclicamente. Cada articulación $i$ se rota para minimizar la distancia entre el efector y el objetivo en su plano de acción.

- *Ventajas:* Sin matrices, simple de implementar.
- *Desventajas:* Convergencia errática, no garantiza el camino más corto.

== FABRIK (Forward And Backward Reaching Inverse Kinematics)

FABRIK es un algoritmo que resuelve la cinemática inversa de forma iterativa, ajustando sucesivamente las posiciones de las articulaciones para acercar el efector al objetivo. Opera con posiciones cartesianas de las articulaciones. Cada articulación se reubica sobre la recta que pasa por su posición anterior y por la nueva posición de la articulación vecina, a la distancia que fija el largo del eslabón @aristidou2011fabrik @fabrik_aristidou_web @wiki_ik_heuristic.

- *Ventajas:* Sin matrices ni derivadas, convergencia en 5-20 iteraciones, movimientos naturales. Las restricciones articulares se pueden incorporar dentro del algoritmo, en cada paso de la iteración @aristidou2011fabrik.
- *Desventajas:* Como el resultado son posiciones, hace falta un paso final para obtener el ángulo de cada servo. Además, cada tipo de articulación (bisagra, rótula) necesita su propia forma de restringirse.

#pagebreak()

== Tabla comparativa

#align(center)[
  #table(
    columns: (1.5fr, 1fr, 1fr, 1fr, 1fr),
    stroke: 0.5pt + gray,
    inset: 6pt,
    fill: (_, row) => if row == 0 { luma(220) } else if calc.even(row) { luma(245) } else { white },
    [*Criterio*], [*Jac. Transpuesto*], [*DLS*], [*FABRIK*], [*CCD*],
    [Invierte una matriz], [No], [Sí ($n times n$)], [No], [No],
    [Singularidades], [Robusto (sin inversa)], [Robusto ($lambda$)], [Inmune], [Robusto],
    [Tiempo por iteración (Tabla 1 del paper)], [9.90 ms], [10.49 ms], [0.86 ms], [4.69 ms],
    [Convergencia típica], [1311 iter.], [999 iter.], [5–20 iter.], [26 iter.],
    [Naturalidad del movimiento], [Media], [Media], [Alta], [Baja],
    [Implementación en C para ESP32], [Media], [Alta (inversión)], [Baja-Media], [Baja],
    [Adecuado para ESP32], [Aceptable], [Aceptable], [*Óptimo*], [Aceptable],
  )
]

== Decisión: FABRIK

Se selecciona *FABRIK* como algoritmo de cinemática inversa para el proyecto 2026. La decisión se tomó por los siguientes motivos:

- *Sin inversión matricial:* Opera con sumas, restas y normalizaciones de vectores 3D. No necesita calcular jacobianos ni inversiones de matrices. Por lo tanto, su implementación en C es más simple y rápida que la de los métodos basados en el Jacobiano.
- *Inmunidad a singularidades:* Al no depender de derivadas, no existe configuración del brazo que genere una matriz singular.
- *Convergencia rápida:* Aristidou & Lasenby reportan que, en sus pruebas, FABRIK resultó unas 10 veces más rápido que CCD y unas mil veces más rápido que los métodos basados en el Jacobiano @aristidou2011fabrik.
- *Implementación real disponible:* Existe una librería de código abierto (NocKinematics) que implementa FABRIK en 3D para Arduino y ESP32, aunque sin límites articulares @nockinematics.
- *Movimiento natural:* Los trazados son suaves y predecibles.

== Formalización matemática del algoritmo

Sean $bold(p)_0, bold(p)_1, ..., bold(p)_n$ las posiciones globales de las $n+1$ articulaciones, con $bold(p)_0$ siendo la base fija del brazo y $bold(p)_n$ el extremo efector. Sean $l_i = norm(bold(p)_i - bold(p)_(i-1))$ las longitudes mecánicas constantes de cada eslabón.

=== Paso 0 — Verificación de alcanzabilidad

Antes de iterar, se comprueba si el objetivo está dentro del rango máximo del brazo:

$ norm(bold(p)^* - bold(p)_0) <= sum_(i=1)^n l_i $

Si no se cumple, el objetivo es inalcanzable y no se ejecuta ninguna iteración.


=== Paso 1 — Fase Forward (llevar la punta al objetivo)

Se coloca el efector directamente en el objetivo y se propagan las restricciones de longitud hacia la base:

$
  bold(p)_n &arrow.l bold(p)^* \
  bold(p)_(i-1) &arrow.l bold(p)_i + frac(bold(p)_(i-1) - bold(p)_i, norm(bold(p)_(i-1) - bold(p)_i)) dot.op l_i quad text("para ") i = n, n-1, ..., 1
$

#pagebreak()

=== Paso 2 — Fase Backward (anclar la base)

Como la base quedó desplazada, se la devuelve a su posición fija $bold(p)_"base"$ y se propagan las restricciones hacia adelante:

$
  bold(p)_0 &arrow.l bold(p)_"base" \
  bold(p)_i &arrow.l bold(p)_(i-1) + frac(bold(p)_i - bold(p)_(i-1), norm(bold(p)_i - bold(p)_(i-1))) dot.op l_i quad text("para ") i = 1, 2, ..., n
$

Los pasos 1 y 2 se repiten alternadamente hasta que se cumple alguno de los tres criterios de corte que plantea el paper @aristidou2011fabrik:

$
  norm(bold(p)_n - bold(p)^*) <= epsilon quad "o" quad norm(bold(p)_n^((k)) - bold(p)_n^((k-1))) < eta quad "o" quad k > k_"max"
$

El primero indica que el efector llegó al objetivo. El segundo compara la posición del efector en dos iteraciones seguidas. Si casi no se movió, seguir iterando no mejora el resultado. Por esta razón, es el criterio que corta la iteración cuando el objetivo no se puede alcanzar. El tercero es un límite de seguridad.

=== Restricciones dentro de cada paso

Los pasos anteriores corresponden a la forma sin restricciones, en la que cada articulación se comporta como una rótula libre. Las restricciones articulares se aplican dentro de la iteración y no después de converger. Cada vez que el forward o el backward reubica una articulación, se verifica que la orientación resultante esté dentro del rango permitido. Si no lo está, se la lleva a ese rango antes de calcular la siguiente. Según el paper, los algoritmos de cinemática inversa suelen ser más efectivos si las restricciones se aplican en cada paso y no al final. Además, FABRIK con restricciones llega a la solución siempre que el objetivo esté al alcance y exista una configuración de las articulaciones que permita llegar @aristidou2011fabrik.

Para articulaciones de un solo grado de libertad, como una bisagra, el paper propone una simplificación. La raíz y el objetivo, junto con la orientación del objetivo, definen un plano. Las articulaciones se proyectan sobre ese plano y FABRIK se aplica en 2D dentro de él.

Con restricciones, la verificación del Paso 0 es necesaria pero no suficiente. Un objetivo que está dentro del rango máximo puede quedar fuera de alcance por los límites de los servos. En ese caso el efector deja de acercarse y la iteración termina por el segundo criterio o, si la cadena sigue deslizándose contra un tope, por el tercero.

=== Parámetros elegidos

#align(center)[
  #table(
    columns: (auto, auto, auto),
    stroke: 0.5pt + gray,
    inset: 6pt,
    fill: (_, row) => if row == 0 { luma(220) } else { white },
    [*Parámetro*], [*Valor*], [*Justificación*],
    [$epsilon$ (tolerancia)], [0.5 cm], [Resolución mecánica del MG90S (~1° ≈ 0.31 cm en el extremo)],
    [$eta$ (sin movimiento)], [$10^(-4)$ cm], [Detecta que el efector dejó de moverse entre dos iteraciones],
    [$k_"max"$ (iteraciones)], [20], [Se reportan convergencias en 5–15 iteraciones para cadenas similares],
  )
]

== Recuperación de ángulos articulares

FABRIK determina posiciones cartesianas $bold(p)_i$, pero los servos necesitan ángulos. El ángulo $theta_i$ es el giro alrededor del eje $bold(z)_i$ que lleva al eslabón desde su posición de cero ($theta_i = 0$) hasta la posición encontrada. Por lo tanto, se mide entre dos direcciones del plano perpendicular a $bold(z)_i$:

$ theta_i = op("atan2")(bold(z)_i dot.op (bold(r)_i times bold(d)_i), quad bold(r)_i dot.op bold(d)_i) $

donde $bold(d)_i = bold(p)_(i+1) - bold(p)_i$ es el vector director del eslabón, $bold(z)_i$ es el eje de rotación del servo $i$ expresado en coordenadas globales y $bold(r)_i$ es la dirección que tendría el eslabón con $theta_i = 0$. Los vectores $bold(d)_i$ y $bold(r)_i$ se toman proyectados sobre el plano perpendicular a $bold(z)_i$. Tanto $bold(z)_i$ como $bold(r)_i$ se obtienen de la cadena FK con los ángulos ya recuperados de los servos anteriores, así que el cálculo avanza en orden, desde la base hacia la pinza. El signo surge del propio $op("atan2")$, que da un valor positivo cuando el giro de $bold(r)_i$ a $bold(d)_i$ es antihorario visto desde la punta de $bold(z)_i$.

Medir en cambio el ángulo entre el eslabón y su propio eje, $op("atan2")(norm(bold(d)_i times bold(z)_i), bold(d)_i dot.op bold(z)_i)$, no sirve, porque una rotación alrededor de $bold(z)_i$ no cambia el ángulo que forma un vector con $bold(z)_i$. Con las medidas de nuestro brazo, ese cálculo da siempre $(0°, 90°, 85.15°, 180°, 90°)$, sin importar la pose.

Por último, los ángulos recuperados solo reproducen las posiciones de FABRIK si esas posiciones respetan la estructura del brazo. Un eslabón alineado con su eje no cambia al girar, así que en la base y la muñeca el ángulo no se puede obtener de su propio eslabón. En nuestro brazo, el de la base surge de la orientación del plano del brazo y los de la muñeca y la pinza los elige el programa antes de iterar (@sec-aplicacion).

Los rangos de los servos son los que permite el firmware (`configuracion.c`). Se expresan en grados del servo, que el firmware obtiene a partir del ancho de pulso con sus tablas de calibración, medidas en el brazo, y se convierten a los ángulos del modelo con la misma regla que usa el firmware:

#align(center)[
  #table(
    columns: (auto, auto, auto, auto, auto),
    stroke: 0.5pt + gray,
    inset: 6pt,
    fill: (_, row) => if row == 0 { luma(220) } else { white },
    [*Servo*], [*Articulación*], [*Rango del servo (°)*], [*Conversión*], [*Rango del modelo (°)*],
    [0], [Base], [0 a 180], [$theta_0 = -s_0$], [-180 a 0],
    [1], [Hombro], [0 a 180], [$theta_1 = s_1$], [0 a 180],
    [2], [Codo], [0 a 180], [$theta_2 = -s_2$], [-180 a 0],
    [3], [Muñeca], [0 a 180], [$theta_3 = s_3 - 90°$], [-90 a 90],
    [4], [Pinza], [0 a 180], [$theta_4 = s_4$], [0 a 180],
  )
]

Recortar los ángulos recién después de converger no alcanza. El recorte cambia la pose y el efector deja de estar sobre el objetivo. Además, FABRIK sin restricciones puede converger a una solución que el brazo no puede hacer (por ejemplo, con el codo doblado hacia el lado contrario), aunque exista otra que sí respete los límites. Por eso los límites se aplican en cada paso, junto con una regla para el lado del codo (@sec-aplicacion), y, como control final, la pose se declara inválida si el efector quedó a más de $epsilon$ del objetivo o si algún ángulo quedó fuera de rango.

== Aplicación de FABRIK en nuestro brazo <sec-aplicacion>

Hasta aquí se describió FABRIK en su forma general. En nuestro brazo las articulaciones no son rótulas libres. La base gira alrededor del eje vertical, el hombro y el codo son bisagras de ejes paralelos, la muñeca rota sobre el eje del antebrazo y el servo de la pinza inclina hacia adelante y hacia atrás. Por esta razón, se va a aplicar una adaptación de la técnica que propone el paper para bisagras @aristidou2011fabrik. La base gira el plano vertical del brazo para que $bold(p)_5$, que queda al costado de ese plano y caiga sobre el objetivo. Dentro del plano, FABRIK resuelve el hombro y el codo en 2D, con los límites aplicados en cada paso. Si hay dos orientaciones posibles para la base, se prueba primero la que apunta hacia el objetivo. Para elegir entre las dos posiciones posibles del codo, y evitar que termine doblado hacia abajo, se agrega una regla similar al _pole_ que usan programas de animación como Blender y Maya @blender_ik_constraint @maya_rp_solver. El usuario solo indica el punto al que tiene que llegar la pinza. La muñeca y la pinza empiezan en su posición neutra y, si con ellas el punto queda fuera de alcance o bloqueado por los límites, o si la pose o el camino hasta ella chocan, el programa prueba otras posiciones de a 10°, primero en la pinza y después en la muñeca, hasta encontrar una que llegue sin chocar.

Esta aplicación se encuentra documentada en detalle en el informe *Implementación FABRIK* (`implementacion-fabrik.pdf`).

#pagebreak()
= Modelo de Esferas para Detección de Autocolisiones - Hito 1.3

== Contexto: el modelo del año anterior

El grupo J5 (2025) diseñó e implementó un sistema de detección de autocolisiones basado en *esferas delimitadoras* @grupo_j5_2025. El modelo se integró al firmware del prototipo 2025 para detectar colisiones entre eslabones y contra el plano base, y bloquear los movimientos peligrosos.

== Adopción del mismo modelo

Se evaluaron otras alternativas, como *cápsulas* o *AABB / OBB*. Sin embargo, se concluyó que las esferas delimitadoras son la mejor opción para nuestro prototipo por las siguientes razones:

- *Ya está implementado:* El grupo J5 lo programó en C y lo integró al firmware del ESP32 @grupo_j5_2025.
- *Costo computacional bajo:* La evaluación de colisión entre dos esferas se reduce a una sola operación de distancia euclidiana.
- *Suficiente para el caso de uso:* Los eslabones del brazo son piezas cortas. En las pruebas sobre las piezas alcanzó con esferas a lo largo de cada eslabón, un tope para el codo y las esquinas de la mano contra el suelo, sin precisar geometrías más complejas.

Las cápsulas (Swept Sphere Volumes) serían una mejora futura si se detectan falsos negativos con el hardware real. Por el momento, se mantendrá el modelo de esferas.

== Fundamentación matemática

La intersección entre dos esferas $S_A$ y $S_B$ en $RR^3$ se evalúa con una única desigualdad euclidiana @mdn_3d_collision:

$ d(bold(c)_A, bold(c)_B) = sqrt((c_A^x - c_B^x)^2 + (c_A^y - c_B^y)^2 + (c_A^z - c_B^z)^2) <= r_A + r_B + delta $

Se usa radio uniforme $r_"esf" = 1.5 "cm"$, elegido con pruebas sobre las piezas del brazo (@sec-esferas), y tolerancia numérica $delta = 10^(-9)$ cm. La condición de colisión se simplifica a:

$ d(bold(c)_A, bold(c)_B) <= 2 dot.op r_"esf" $

== Distribución de esferas por eslabón <sec-esferas>

Las coordenadas se definen en el espacio local de cada eslabón como vectores homogéneos $tilde(c)_(i,j)^"loc" = (x, y, z, 1)^T$. La transformación a coordenadas globales reutiliza la misma cadena de matrices $M_"eslabón"^((i))$ calculada por la FK:

$ c_(i,j)^"glob" = M_"eslabón"^((i)) dot.op tilde(c)_(i,j)^"loc" $

#align(center)[
  #table(
    columns: (auto, auto, 1fr, auto),
    stroke: 0.5pt + gray,
    inset: 6pt,
    fill: (_, row) => if row == 0 { luma(220) } else if calc.even(row) { luma(245) } else { white },
    [*Eslabón*], [*Articulación*], [*Centros locales $(x, y, z)$*], [*Cant.*],
    [0], [Base], [$(0, 0, 0)$; $(0, 0, 1.145)$; $(0, 0, 2.29)$; $(0, 0, 3.435)$], [4],
    [1], [Hombro], [$(0, 0, 0)$; $(1.25, 0, 0)$; $(2.5, 0, 0)$; $(3.75, 0, 0)$; $(5, 0, 0)$], [5],
    [2], [Codo], [$(0, 0, 0)$; $(1.4725, 0, 0.125)$; $(2.945, 0, 0.25)$; $(4.4175, 0, 0.375)$], [4],
    [3], [Muñeca], [$(0, 0, 0)$], [1],
    [4], [Pinza], [$(0, 0, 0)$; $(0, -1.415, 0)$; $(0, -2.83, 0)$], [3],
    table.hline(stroke: 1pt),
    [], [*Total*], [], [*17 esferas*],
  )
]

Las esferas van sobre la recta que une cada articulación con la siguiente o, en la pinza, con la punta. Hay una en cada articulación y otra cada 1.5 cm como máximo. En la pinza, la última queda a un radio de la punta, para que la esfera no sobresalga de la mano. La cantidad y el radio se eligieron con pruebas sobre las piezas del brazo y reemplazan a los de `configuracion.c` del prototipo 2025 @grupo_j5_2025.

#pagebreak()
== Matriz booleana de exclusión 5×5

Sin optimización, verificar colisiones entre las 17 esferas requeriría $binom(17, 2) = 136$ evaluaciones de distancia en cada ciclo. La *Matriz de Colisiones* booleana filtra los pares de eslabones que no hace falta verificar y deja 52 distancias:

$
  M_"colisiones" = mat(
    0, 0, 1, 1, 1;
    0, 0, 0, 1, 1;
    1, 0, 0, 0, 0;
    1, 1, 0, 0, 0;
    1, 1, 0, 0, 0;
  )
$

$M[i, j] = 1 arrow.r$ verifica colisión entre eslabón $i$ y eslabón $j$.

=== Reglas de exclusión

- *Diagonal* ($i = j$): Un eslabón no colisiona consigo mismo.
- *Adyacentes* ($|i - j| = 1$): Los eslabones consecutivos comparten una articulación física, y la última esfera de uno queda siempre a menos de $2 r_"esf"$ de la primera del siguiente, entre 1.1 y 1.5 cm. Se tocarían en cualquier pose, así que no se verifican. El choque del brazo con el antebrazo lo detecta el tope del codo (@sec-tope-codo).
- *Codo y pinza* $(2, 4)$: La base de la mano mide 1.29 cm y deja la pinza junto al antebrazo. La última esfera del codo y la primera de la pinza quedan siempre a 2.77 cm, menos que $2 r_"esf" = 3$ cm, y por eso este par tampoco se verifica.
- *Pares verificados* $(0, 2)$, $(0, 3)$, $(0, 4)$, $(1, 3)$ y $(1, 4)$: En 100~000 poses al azar dentro de los rangos del firmware, cada uno choca en miles de ellas, entre 7221 y 16~764.

=== Tope del codo <sec-tope-codo>

Con las piezas del brazo, el antebrazo toca el brazo cuando el codo está más cerrado que $-139°$ en grados del modelo. Como las esferas de esos dos eslabones se tocan en cualquier pose, ese choque se detecta con el ángulo del codo y cuenta como choque del par $(1, 2)$:

$ theta_2 < -139° $

== Colisión contra el plano base

Adicionalmente, las esferas de los eslabones 1 a 3 se verifican contra el plano del suelo ($Z = 0$):

$ c_(i,j)^"glob" [z] <= r_"esf" quad forall i in {1, 2, 3}, quad forall j $

La base (eslabón 0) está apoyada sobre el suelo y no se verifica. La mano (eslabón 4) es una bandeja plana y, con sus esferas de 1.5 cm, no podría acercarse al suelo ni con la bandeja paralela a él. Por eso, en lugar de sus esferas, se verifican las 8 esquinas de la caja que la contiene, que tocan el suelo si $z <= 0$. Si alguna esfera o esquina toca el plano, la pose se rechaza.

#pagebreak()
= Conclusión

La investigación establece que:

+ El modelo de *cinemática directa* (FK) del año anterior es matemáticamente correcto y se adopta sin cambios en su lógica. Sus medidas se reemplazaron por las de las piezas del brazo, en la simulación y en `configuracion.c`.
+ El modelo de *esferas delimitadoras* para colisiones fue implementado por el grupo J5 y es el más adecuado para las restricciones del ESP32. Se adopta con 17 esferas de 1.5 cm a lo largo de los eslabones, un tope para el codo y las esquinas de la mano contra el suelo, elegidos con pruebas sobre las piezas del brazo.
+ *FABRIK* es el algoritmo óptimo para IK en nuestro contexto: sin matrices, convergencia rápida, implementación simple en C. Es la contribución técnica nueva del grupo J1.

#pagebreak()
#bibliography("referencias.bib", title: "Referencias", style: "ieee")
