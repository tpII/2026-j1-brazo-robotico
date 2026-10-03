#import "@preview/barcala:0.3.0": informe
#import "@preview/zero:0.3.2": *
#let norm(x) = $lr(‖ #x ‖)$

#show: informe.with(
  institucion: "unlp",
  unidad-academica: "informática",
  asignatura: "Taller de Proyecto II. 2026",
  equipo: "Grupo J1",
  autores: (
    (
      nombre: "Tarifa, Armando Ezequiel",
      email: "ezequiel.tarifa.2002@gmail.com",
      legajo: "02893/4",
    ),
    (
      nombre: "Guitera Woida, Emma",
      email: "emma.guitera.woida@gmail.com",
      legajo: "03352/2",
    ),
    (
      nombre: "Terruzzi, Ana Justina",
      email: "justina.terruzzi@gmail.com",
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

Antes de avanzar con modelos nuevos, mencionaremos los modelos implementados por el grupo anterior @grupo_j5_2025.

== Cinemática Directa (FK) - Implementada en 2025

El grupo J5 adoptó la convención *Denavit-Hartenberg (DH)* como marco teórico para modelar la cinemática directa. La implementación en C resultante construye cada matriz $T_(i-1)^i$ combinando una rotación de corrección de montaje ($R_"fix"$), la rotación del servo y la traslación del eslabón, lo que es equivalente al producto DH estándar.

La cadena cinemática del brazo de 5 grados de libertad se modela como:

$ T_"total" = T_0^1 dot.op T_1^2 dot.op T_2^3 dot.op T_3^4 dot.op T_4^5 $

La secuencia de ejes de rotación es *Z-Y-Y-X-Y* para los servos 0 a 4. Cada $R_"fix"$ alinea el eje físico del servo con el eje $Z$ matemático para que todas las rotaciones se expresen uniformemente.

Esta implementación fue codificada en C (`cinematica.c`, `algebra_lineal.c`, `numpy.c`) para el ESP32 y replicada en Python (`cinematica.py`, `algebra_lineal.py`) para simulación.

Se hereda para el proyecto 2026 sin modificaciones en la lógica de cinemática directa.

== Cinemática Inversa (IK) - No implementada en 2025

El proyecto del 2025 *no implementó cinemática inversa*. El control del brazo se realizaba exclusivamente mediante un potenciómetro analógico y botones pulsadores: el operador seleccionaba un servo y lo rotoba manualmente. El sistema verificaba colisiones con cada pose resultante, pero no existía la posibilidad de enviar un destino $(X, Y, Z)$ y que el sistema calculara automáticamente los ángulos necesarios.

La implementación de cinemática inversa se implementara en nuestro proyecto 2026.

== Detección de Autocolisiones - Implementada en 2025

El grupo J5 implementó un sistema de detección de autocolisiones basado en *esferas delimitadoras* @grupo_j5_2025. El modelo define 9 esferas distribuidas en los 5 eslabones con radio uniforme $r = 1.0 "cm"$, verificando pares de esferas filtrados por una matriz booleana 5x5 para evitar falsos positivos entre eslabones adyacentes.

Se hereda para el proyecto 2026.

#pagebreak()
= Cinemática Inversa Iterativa - Hito 1.2

== Definición del problema

Dado un punto objetivo $bold(p)^* in RR^3$ en el espacio de trabajo del brazo, el problema de la cinemática inversa consiste en encontrar el vector de ángulos articulares:

$ bold(theta)^* = (theta_0^*, theta_1^*, theta_2^*, theta_3^*, theta_4^*) $

tal que la evaluación de la cinemática directa resulte en dicho punto:

$ "FK"(bold(theta)^*) = bold(p)^* $

Para un brazo de 5 grados de libertad con geometría no esférica, la resolución analítica cerrada es altamente compleja y propensa a errores de implementación debido a la cantidad de casos y condiciones. Por esta razón, se opta por un enfoque numérico iterativo capaces de ejecutarse eficientemente en el ESP32.

== Algoritmos evaluados

Se estudiaron y compararon cuatro métodos de cinemática inversa iterativa:

=== Jacobiano transpuesto

Calcula el Jacobiano $J(bold(theta)) in RR^(3 times n)$ y actualiza los ángulos con:

$ Delta bold(theta) = alpha dot.op J^T dot.op (bold(p)^* - "FK"(bold(theta))) $

- *Ventajas:* No requiere inversión de matrices.
- *Desventajas:* Convergencia lenta (20-100 iteraciones), oscilaciones cerca de singularidades cinemáticas, requiere elegir $alpha$ adecuadamente.

=== Damped Least Squares (DLS)

Resuelve el sistema regularizado:

$ Delta bold(theta) = (J^T J + lambda^2 I)^(-1) J^T dot.op (bold(p)^* - "FK"(bold(theta))) $

- *Ventajas:* Robusto frente a singularidades gracias al parámetro de amortiguación $lambda$.
- *Desventajas:* Requiere una matriz $n times n$ en cada iteración, costoso para el ESP32, el parámetro $lambda$ debe ser ajustado dinámicamente.

=== CCD (Cyclic Coordinate Descent)

Ajusta un ángulo por vez, recorriendo las articulaciones cíclicamente. Cada articulación $i$ se rota para minimizar la distancia entre el efector y el objetivo en su plano de acción.

- *Ventajas:* Sin matrices, simple de implementar.
- *Desventajas:* Convergencia errática, no garantiza el camino más corto.

== FABRIK (Forward And Backward Reaching Inverse Kinematics)

FABRIK es un algoritmo que resuelve la cinemática inversa de forma iterativa, ajustando sucesivamente los ángulos de las articulaciones para acercar el efector al objetivo. Opera con posiciones cartesianas de las articulaciones @fabrik_aristidou_web @wiki_ik_heuristic.

- *Ventajas:* Sin matrices ni derivadas, convergencia en 5-20 iteraciones, movimientos naturales.
- *Desventajas:* No maneja restricciones angulares de forma nativa (se aplican como post-proceso).

== Tabla comparativa

#align(center)[
  #table(
    columns: (1.5fr, 1fr, 1fr, 1fr, 1fr),
    stroke: 0.5pt + gray,
    inset: 6pt,
    fill: (_, row) => if row == 0 { luma(220) } else if calc.even(row) { luma(245) } else { white },
    [*Criterio*], [*Jac. Transpuesto*], [*DLS*], [*FABRIK*], [*CCD*],
    [Necesita matriz], [No], [Sí ($n times n$)], [No], [No],
    [Singularidades], [Problemático], [Robusto ($lambda$)], [Inmune], [Robusto],
    [Costo por iteración], [Bajo], [Medio-Alto], [Muy bajo], [Muy bajo],
    [Convergencia típica], [20–100 iter.], [10–50 iter.], [5–20 iter.], [10–50 iter.],
    [Naturalidad del movimiento], [Media], [Media], [Alta], [Baja],
    [Implementación en C para ESP32], [Media], [Alta (inversión)], [Baja-Media], [Baja],
    [Adecuado para ESP32], [Aceptable], [Aceptable], [*Óptimo*], [Aceptable],
  )
]

== Decisión: FABRIK

Se selecciona *FABRIK* como algoritmo de cinemática inversa para el proyecto 2026. La decisión fue tomada en base a los siguientes items:

- *Sin inversión matricial:* Opera con sumas, restas y normalizaciones de vectores 3D. No necesita calcular jacobianos ni inversiones de matrices. Por lo tanto, su implementación en C es mas simple y rapida que las otras alternativas.
- *Inmunidad a singularidades:* Al no depender de derivadas, no existe configuración del brazo que genere una matriz singular.
- *Convergencia rápida:* Aristidou & Lasenby reportan convergencia 3-5x más rápida que métodos de Jacobiano para precisión equivalente @fabrik_aristidou_web.
- *Implementación real disponible:* Existe una librería de referencia (NocKinematics) optimizada específicamente para Arduino/ESP32 @nockinematics.
- *Movimiento natural:* Los trazados son suaves y predecibles.

== Formalización matemática del algoritmo

Sean $bold(p)_0, bold(p)_1, ..., bold(p)_n$ las posiciones globales de las $n+1$ articulaciones, con $bold(p)_0$ siendo la base fija del brazo y $bold(p)_n$ el extremo efector. Sean $l_i = norm(bold(p)_i - bold(p)_(i-1))$ las longitudes mecánicas constantes de cada eslabón.

=== Paso 0 — Verificación de alcanzabilidad

Antes de iterar, se comprueba si el objetivo está dentro del rango máximo del brazo:

$ norm(bold(p)^* - bold(p)_0) <= sum_(i=1)^n l_i $

Si no se cumple, el objetivo es inalcanzable. El sistema responde con error HTTP al Gemelo Digital sin ejecutar ninguna iteración.

=== Paso 1 — Fase Forward (llevar la punta al objetivo)

Se coloca el efector directamente en el objetivo y se propagan las restricciones de longitud hacia la base:

$
  bold(p)_n &arrow.l bold(p)^* \
  bold(p)_(i-1) &arrow.l bold(p)_i + frac(bold(p)_(i-1) - bold(p)_i, norm(bold(p)_(i-1) - bold(p)_i)) dot.op l_i quad text("para ") i = n, n-1, ..., 1
$

=== Paso 2 — Fase Backward (anclar la base)

La base se ha "desplazado", por lo que se reancla en el origen físico y se propagan las restricciones hacia adelante:

$
  bold(p)_0 &arrow.l bold(p)_"base" \
  bold(p)_i &arrow.l bold(p)_(i-1) + frac(bold(p)_i - bold(p)_(i-1), norm(bold(p)_i - bold(p)_(i-1))) dot.op l_i quad text("para ") i = 1, 2, ..., n
$

Los pasos 1 y 2 se repiten alternadamente hasta que:

$ norm(bold(p)_n - bold(p)^*) < epsilon quad "o" quad k > k_"max" $

=== Parámetros elegidos

#align(center)[
  #table(
    columns: (auto, auto, auto),
    stroke: 0.5pt + gray,
    inset: 6pt,
    fill: (_, row) => if row == 0 { luma(220) } else { white },
    [*Parámetro*], [*Valor*], [*Justificación*],
    [$epsilon$ (tolerancia)], [0.5 cm], [Resolución mecánica del SG90 (~1° ≈ 0.3 cm en el extremo)],
    [$k_"max"$ (iteraciones)], [20], [Se reportan convergencia en 5–15 para cadenas similares],
  )
]

== Recuperación de ángulos articulares

FABRIK determina posiciones cartesianas $bold(p)_i$, pero los servos necesitan ángulos. La recuperación se realiza mediante:

$ theta_i = op("atan2")(norm(bold(d)_i times bold(z)_i), quad bold(d)_i dot.op bold(z)_i) $

donde $bold(d)_i = bold(p)_(i+1) - bold(p)_i$ es el vector director del eslabón y $bold(z)_i$ es el eje de rotación del servo $i$ expresado en coordenadas globales (obtenido de la cadena FK). El signo correcto se determina con la dirección del producto cruzado $bold(z)_i times bold(d)_i$.

Tras el cálculo, cada ángulo se limita a los rangos mecánicos de los servos SG90:

#align(center)[
  #table(
    columns: (auto, auto, auto, auto),
    stroke: 0.5pt + gray,
    inset: 6pt,
    fill: (_, row) => if row == 0 { luma(220) } else { white },
    [*Servo*], [*Articulación*], [*Mín (°)*], [*Máx (°)*],
    [0], [Base], [0], [170],
    [1], [Hombro], [0], [135],
    [2], [Codo], [-170], [0],
    [3], [Muñeca], [—], [—],
    [4], [Pinza], [0], [180],
  )
]

Si algún ángulo calculado queda fuera del rango permitido tras el clampeo, la pose se declara inválida.

#pagebreak()
= Modelo de Esferas para Detección de Autocolisiones — Hito 1.3

== Contexto: el modelo del año anterior

El grupo j5 (2025) diseñó e implementó un sistema de detección de autocolisiones basado en *esferas delimitadoras* @grupo_j5_2025. El modelo demostró ser funcional durante las pruebas del prototipo 2025: detectaba correctamente colisiones entre eslabones no abyacentes y contra el plano base, bloqueando movimientos peligrosos.

== Adopción del mismo modelo

Se evaluaron otras alternativas, como *Cápsulas* o *AABB / OBB*. Sin embargo, se concluyó que las esferas delimitadoras son la mejor opción para nuestro prototipo por las siguientes razones:

- *Ya está validado:* El grupo J5 lo probó extensivamente con el hardware real @grupo_j5_2025.
- *Costo computacional bajo:* La evaluación de colisión entre dos esferas se reduce a una sola operación de distancia euclidiana.
- *Compatibilidad directa:* El código C heredado ya implementa este modelo. No hay costo de re-implementación.
- *Suficiente para el caso de uso:* Los eslabones del brazo SG90 son cilindros cortos; las esferas proporcionan un margen de seguridad adecuado sin precisar geometrías mas complejas.

Las cápsulas (Swept Sphere Volumes) serían una mejora futura si se detectan falsos negativos con el hardware real, por el momento se mantendra el modelo de esferas.

== Fundamentación matemática

La intersección entre dos esferas $S_A$ y $S_B$ en $RR^3$ se evalúa con una única desigualdad euclidiana @mdn_3d_collision:

$ d(bold(c)_A, bold(c)_B) = sqrt((c_A^x - c_B^x)^2 + (c_A^y - c_B^y)^2 + (c_A^z - c_B^z)^2) <= r_A + r_B + delta $

Para el prototipo, se usa radio uniforme $r_"esf" = 1.0 "cm"$ (heredado del año anterior) y tolerancia numérica $delta = 10^(-9)$. La condición de colisión se simplifica a:

$ d(bold(c)_A, bold(c)_B) <= 2 dot.op r_"esf" $

== Distribución de esferas por eslabón

Las coordenadas se definen en el espacio local de cada eslabón como vectores homogéneos $tilde(c)_(i,j)^"loc" = (x, y, z, 1)^T$. La transformación a coordenadas globales reutiliza la misma cadena de matrices $M_"eslabon"^((i))$ calculada por la FK:

$ c_(i,j)^"glob" = M_"eslabon"^((i)) dot.op tilde(c)_(i,j)^"loc" $

#align(center)[
  #table(
    columns: (auto, auto, 1fr, auto),
    stroke: 0.5pt + gray,
    inset: 6pt,
    fill: (_, row) => if row == 0 { luma(220) } else if calc.even(row) { luma(245) } else { white },
    [*Eslabón*], [*Articulación*], [*Centros locales $(x, y, z)$*], [*Cant.*],
    [0], [Base], [$(0, 0, 0)$], [1],
    [1], [Hombro], [$(0, 0, 0)$; $(2, 0, 0)$], [2],
    [2], [Codo], [$(0, 0, 0)$; $(2, 0.5, 0)$; $(4, 0.5, 0)$], [3],
    [3], [Muñeca], [$(0, 0, 0)$], [1],
    [4], [Pinza], [$(0, 0, 0)$; $(0, -2, 0)$], [2],
    table.hline(stroke: 1pt),
    [], [*Total*], [], [*9 esferas*],
  )
]

_Estos valores son los documentados en_ `configuracion.c` _del prototipo 2025_ @grupo_j5_2025. _Serán validados contra el hardware físico real una vez ensamblado, ajustando posiciones y radio si corresponde._

== Matriz booleana de exclusión 5×5

Sin optimización, verificar colisiones entre las 9 esferas requeriría $binom(9, 2) = 36$ evaluaciones de distancia en cada ciclo. La *Matriz de Colisiones* booleana (heredada de @grupo_j5_2025) filtra los pares físicamente imposibles:

$
  M_"colisiones" = mat(
    0, 0, 0, 0, 1;
    0, 0, 1, 0, 1;
    0, 1, 0, 0, 0;
    0, 0, 0, 0, 0;
    1, 1, 0, 0, 0;
  )
$

$M[i, j] = 1 arrow.r$ verifica colisión entre eslabón $i$ y eslabón $j$.

=== Reglas de exclusión

- *Diagonal* ($i = j$): Un eslabón no colisiona consigo mismo.
- *Adyacentes* ($|i - j| = 1$): Los eslabones consecutivos comparten una articulación física. Sus esferas extremas se superponen por diseño mecánico. Es fisicamente imposible que estos eslabones colisionen entre sí.
- *Casi-adyacentes* ($|i - j| = 2$): La geometría del brazo y los límites angulares de los SG90 hacen que estos pares rara vez puedan alcanzar una configuración de colisión real.

=== Exclusión de articulación compartida

Para pares adyacentes que sí se verifican según la matriz, se excluye adicionalmente el cruce entre la última esfera del eslabón $i$ y la primera del eslabón $j$, ya que comparten el punto de articulación:

$ text("Excluir: ") (c_(i, text("última")), quad c_(j, text("primera"))) $

== Colisión contra el plano base

Adicionalmente, cada esfera (excepto las del eslabón 0, que es la base) se verifica contra el plano del suelo ($Z = 0$):

$ c_(i,j)^"glob" [z] <= r_"esf" quad forall i in {1, 2, 3, 4}, quad forall j $

Si cualquier esfera penetra el plano, la pose se rechaza inmediatamente.

#pagebreak()
= Conclusión

La investigación establece que:

+ El modelo de *cinematica directa* (FK) del año anterior es matemáticamente correcto y se adopta sin modificaciones.
+ El modelo de *esferas delimitadoras* para colisiones fue probado en hardware real por el grupo J5 y es el más adecuado para las restricciones del ESP32. Se adopta directamente.
+ *FABRIK* es el algoritmo óptimo para IK en nuestro contexto: sin matrices, convergencia rápida, implementación simple en C. Es la contribución técnica nueva del grupo J1.

#pagebreak()
#bibliography("referencias.bib", title: "Referencias", style: "ieee")
