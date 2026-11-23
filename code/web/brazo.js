/**
 * @file El brazo 3D, armado con las piezas impresas en el marco de cada eslabón.
 *
 * Las piezas salen de los STL (web/piezas.js): el soporte en X (Foot), la base redonda con la cuna
 * de un servo (Swing), el brazo (Arm) y la horquilla con la caja de un servo (ArmFront). La base
 * redonda está dos veces, como base giratoria y como base de la mano, y ArmFront también, como
 * antebrazo y como mano en forma de pala. Los servos MG90S no están en los STL y se dibujan con
 * cajas y cilindros.
 *
 * La telemetría manda en `marcos` la rotación y el origen de cada eslabón, los mismos que usa la
 * cinemática directa (M_i). Cada pieza va en el marco que la mueve, así el dibujo sigue a la
 * simulación: la pieza que gira con la articulación i, con su horn, va en M_i, y el cuerpo de cada
 * servo va con la pieza que lo sostiene. Los largos también salen de la telemetría: el brazo y la
 * pala se estiran por el medio hasta los largos del modelo, y el servo de la muñeca se corre sobre
 * su eje hasta que el horn toca la base de la mano.
 * Implementación J1.
 */

/** Las piezas están en mm y la escena, en cm. */
const MM = 0.1;

/**
 * Servo MG90S (mm), con las cotas de su hoja de datos: el cuerpo (B y D), las orejas (E), la tapa
 * redonda del eje hasta C y el eje hasta A. El alto del cuerpo es aproximado. La altura de las
 * orejas y la distancia del eje a la punta del cuerpo son las que usan las cunas de los STL.
 */
const MG90S = {
  largo: 22.8, ancho: 12.4, alto: 22.7, ejeDesdePunta: 5.9,
  orejas: { largo: 32.1, desdeFondo: 16.2, espesor: 2.5 },
  tapa: 28.4, eje: 32.5,
};

/** Espesor de un horn (mm). */
const HORN = 1.5;

/** Medidas de las piezas, tomadas de los STL (mm, en el marco de cada STL). */
const FOOT = { tapaCaja: 19.2 };
const SWING = { ejeServo: 12.9, paredCuna: -11.5 };
const ARM = { codo: 49.5, altoEje: 8, estirarDesde: 27, placaHorn: 17.5 };
const ARMFRONT = { punta: 43.3, estirarDesde: 26.25 };

/** Colores del brazo real: PLA blanco y servos MG90S de carcasa oscura, etiqueta magenta y horns negros. */
const COLORES = { pla: 0xe4e6e9, servo: 0x3b3734, etiqueta: 0xd0328c, horn: 0x1f1f1f };

/**
 * Del marco de cada STL al marco de su eslabón: las tres primeras filas de la matriz, con los mm
 * pasados a cm. En las horquillas el eje de la articulación es y del STL y la pieza crece sobre x.
 */
const A_ESLABON = {
  // Brazo y antebrazo: la pieza sobre x del eslabón y la y del STL contra el eje de giro (z).
  brazo: [MM, 0, 0, 0, 0, 0, MM, -ARM.altoEje * MM, 0, -MM, 0, 0],
  // Base de la mano: el disco mira hacia adelante (-z del eslabón) y el eje del servo 4 queda sobre x.
  baseMano: [0, -MM, 0, 0, -MM, 0, 0, 0, 0, 0, -MM, 0],
  // La pala: crece hacia -y del eslabón, que es hacia p5, con la y del STL contra el eje del servo 4.
  pala: [0, 0, MM, -ARM.altoEje * MM, -MM, 0, 0, 0, 0, -MM, 0, 0],
};

/** Un horn en la cara de afuera de la placa de una horquilla, sobre el eje (mm del STL). */
const HORN_EN_PLACA = [1, 0, 0, 0, 0, 0, 1, ARM.placaHorn, 0, -1, 0, ARM.altoEje];

/** Un horn debajo del disco de una base redonda (mm del STL). */
const HORN_BAJO_DISCO = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, -HORN];

/**
 * Crea un brazo 3D vacío; las piezas se arman con la primera pose que llega.
 *
 * @param {THREE.Scene} escena
 * @param {object} [opciones]
 * @param {boolean} [opciones.conBase=true] - Si dibuja el soporte y el servo de la base, que no se
 *   mueven. Las siluetas no los necesitan.
 * @param {number | null} [opciones.color=null] - Con null, los colores del brazo real; con un
 *   color, todo el brazo de ese color y transparente.
 * @param {number} [opciones.opacidad=1] - Opacidad cuando se usa `color`.
 * @returns {{grupo: THREE.Group, actualizar: (dibujo: object | null) => void}} El grupo de la
 *   escena y la función que lo lleva a una pose de la telemetría (con null, lo oculta).
 */
function crearBrazo(escena, { conBase = true, color = null, opacidad = 1 } = {}) {
  const materiales = color === null ? materialesReales() : materialesTransparentes(color, opacidad);
  const grupo = new THREE.Group();
  grupo.visible = false;
  escena.add(grupo);
  const fijo = new THREE.Group();
  grupo.add(fijo);
  const eslabones = Array.from({ length: 5 }, () => marcoMovil(grupo));
  let saltos = null;

  function actualizar(dibujo) {
    grupo.visible = Boolean(dibujo?.marcos);
    if (!grupo.visible) return;
    const nuevos = saltosDe(dibujo);
    if (!saltos || !parecidos(saltos, nuevos)) {
      saltos = nuevos;
      armar(fijo, eslabones, materiales, saltos, conBase);
    }
    dibujo.marcos.forEach((filas, i) => colocar(eslabones[i], matrizDe(filas)));
  }

  return { grupo, actualizar };
}

/**
 * Arma todas las piezas a partir de los saltos (el vector de cada eslabón en su propio marco).
 *
 * @param {THREE.Group} fijo - Lo que no se mueve: el soporte y el servo de la base.
 * @param {THREE.Group[]} eslabones - El marco de cada eslabón.
 * @param {Record<string, THREE.Material>} m - Materiales.
 * @param {number[][]} saltos - Los cinco SALTO_ESLABONES de configuracion.py, en cm.
 * @param {boolean} conBase
 */
function armar(fijo, eslabones, m, saltos, conBase) {
  [fijo, ...eslabones].forEach(vaciar);
  const largoMuneca = Math.hypot(...saltos[3]);
  if (conBase) armarSoporte(fijo, m);
  armarBaseGiratoria(eslabones[0], m, saltos[0][2]);
  armarBrazo(eslabones[1], m, Math.hypot(...saltos[1]));
  armarAntebrazo(eslabones[2], m, saltos[2], largoMuneca);
  armarBaseMano(eslabones[3], m, largoMuneca);
  armarPala(eslabones[4], m, Math.hypot(...saltos[4]));
}

/**
 * El soporte en X y el servo de la base, parado en la caja con las orejas sobre la tapa.
 *
 * @param {THREE.Group} grupo
 * @param {Record<string, THREE.Material>} m
 */
function armarSoporte(grupo, m) {
  grupo.add(pieza("Foot", m.pla, [MM, 0, 0, 0, 0, MM, 0, 0, 0, 0, MM, 0]));
  const fondo = FOOT.tapaCaja - MG90S.orejas.desdeFondo;
  grupo.add(servo(m, [0, MM, 0, 0, -MM, 0, 0, 0, 0, 0, MM, fondo * MM])); // el cuerpo hacia -x desde el eje
}

/**
 * La base giratoria (Swing), con su horn debajo y el servo del hombro acostado en la cuna. El eje
 * de ese servo queda a la altura del hombro.
 *
 * @param {THREE.Group} grupo
 * @param {Record<string, THREE.Material>} m
 * @param {number} alturaHombro - De la mesa al eje del hombro (cm).
 */
function armarBaseGiratoria(grupo, m, alturaHombro) {
  const aBase = [MM, 0, 0, 0, 0, MM, 0, 0, 0, 0, MM, alturaHombro - SWING.ejeServo * MM];
  grupo.add(pieza("Swing", m.pla, aBase));
  grupo.add(horn(m, componer(aBase, HORN_BAJO_DISCO)));
  grupo.add(servo(m, componer(aBase, servoEnCuna())));
}

/**
 * El brazo (Arm), estirado hasta su largo, con el horn del hombro y el servo del codo en su caja.
 *
 * @param {THREE.Group} grupo
 * @param {Record<string, THREE.Material>} m
 * @param {number} largo - Del hombro al codo (cm).
 */
function armarBrazo(grupo, m, largo) {
  const estirar = largo / MM - ARM.codo;
  grupo.add(pieza("Arm", m.pla, A_ESLABON.brazo, ARM.estirarDesde, estirar));
  grupo.add(horn(m, componer(A_ESLABON.brazo, HORN_EN_PLACA)));
  grupo.add(servo(m, componer(A_ESLABON.brazo, servoEnCuna(ARM.codo + estirar, ARM.altoEje))));
}

/**
 * El antebrazo (ArmFront), con el horn del codo y el servo de la muñeca en su caja. El eje de ese
 * servo va sobre el eje de giro del modelo, que pasa por p3.
 *
 * @param {THREE.Group} grupo
 * @param {Record<string, THREE.Material>} m
 * @param {number[]} salto - De p2 a p3 en el marco del antebrazo (cm).
 * @param {number} largoMuneca - De p3 a p4 (cm).
 */
function armarAntebrazo(grupo, m, salto, largoMuneca) {
  grupo.add(pieza("ArmFront", m.pla, A_ESLABON.brazo));
  grupo.add(horn(m, componer(A_ESLABON.brazo, HORN_EN_PLACA)));
  // Donde apoya la base de la mano, que tiene el eje del servo 4 (p4) a SWING.ejeServo de su fondo.
  const cara = (salto[0] + largoMuneca) / MM - SWING.ejeServo;
  const fondo = cara - HORN - MG90S.tapa;
  const lateral = -salto[2] / MM;
  const altura = salto[1] / MM + ARM.altoEje;
  // El cuerpo va hacia el medio de la caja, porque el eje está cerca de una punta.
  const ubicacion = lateral < 0.5
    ? [0, 0, 1, fondo, 0, -1, 0, lateral, 1, 0, 0, altura]
    : [0, 0, 1, fondo, 0, 1, 0, lateral, -1, 0, 0, altura];
  grupo.add(servo(m, componer(A_ESLABON.brazo, ubicacion)));
}

/**
 * La base de la mano (otro Swing), con el horn de la muñeca debajo y el servo 4, cuyo eje pasa por p4.
 *
 * @param {THREE.Group} grupo
 * @param {Record<string, THREE.Material>} m
 * @param {number} largoMuneca - De p3 a p4 (cm).
 */
function armarBaseMano(grupo, m, largoMuneca) {
  const aBase = A_ESLABON.baseMano.slice();
  aBase[11] = SWING.ejeServo * MM - largoMuneca;
  grupo.add(pieza("Swing", m.pla, aBase));
  grupo.add(horn(m, componer(aBase, HORN_BAJO_DISCO)));
  grupo.add(servo(m, componer(aBase, servoEnCuna())));
}

/**
 * La pala (otro ArmFront), estirada hasta su largo, con el horn del servo 4.
 *
 * @param {THREE.Group} grupo
 * @param {Record<string, THREE.Material>} m
 * @param {number} largo - De p4 a p5 (cm).
 */
function armarPala(grupo, m, largo) {
  grupo.add(pieza("ArmFront", m.pla, A_ESLABON.pala, ARMFRONT.estirarDesde, largo / MM - ARMFRONT.punta));
  grupo.add(horn(m, componer(A_ESLABON.pala, HORN_EN_PLACA)));
}

/**
 * Dónde va un servo acostado en la cuna de un Swing o en la caja del Arm (mm del STL): el eje
 * sobre +y en (x, z) y el cuerpo hacia -x, con el fondo contra la pared de atrás.
 *
 * @param {number} [x=0]
 * @param {number} [z=SWING.ejeServo]
 * @returns {number[]} Las tres primeras filas de la transformación, en 12 números.
 */
function servoEnCuna(x = 0, z = SWING.ejeServo) {
  return [0, 1, 0, x, 0, 0, 1, SWING.paredCuna, 1, 0, 0, z];
}

/**
 * Una pieza de piezas.js con su transformación. Si `estirar` no es cero, lo que está después de
 * x = `desde` (mm) se corre `estirar` mm sobre x, así la pieza llega al largo del modelo.
 *
 * @param {string} nombre - La pieza en PIEZAS.
 * @param {THREE.Material} material
 * @param {number[]} filas - Las tres primeras filas de la matriz del STL al marco del eslabón.
 * @param {number} [desde=0]
 * @param {number} [estirar=0]
 * @returns {THREE.Mesh}
 */
function pieza(nombre, material, filas, desde = 0, estirar = 0) {
  const { posiciones, indices } = PIEZAS[nombre];
  const vertices = new Float32Array(indices.length * 3);
  indices.forEach((indice, k) => {
    const x = posiciones[3 * indice];
    vertices[3 * k] = x > desde ? x + estirar : x;
    vertices[3 * k + 1] = posiciones[3 * indice + 1];
    vertices[3 * k + 2] = posiciones[3 * indice + 2];
  });
  const geometria = new THREE.BufferGeometry();
  geometria.setAttribute("position", new THREE.BufferAttribute(vertices, 3));
  geometria.applyMatrix4(matrizDe(filas));
  geometria.computeVertexNormals(); // los vértices no se comparten: cada cara queda plana
  return new THREE.Mesh(geometria, material);
}

/**
 * Un servo MG90S con su transformación. En su marco (mm) el eje de salida va sobre +z y pasa por el
 * origen, el fondo del cuerpo está en z = 0 y el cuerpo se extiende hacia -y desde el eje.
 *
 * @param {Record<string, THREE.Material>} m
 * @param {number[]} filas - Las tres primeras filas de su transformación, en 12 números.
 * @returns {THREE.Group}
 */
function servo(m, filas) {
  const grupo = new THREE.Group();
  const centroY = MG90S.ejeDesdePunta - MG90S.largo / 2;
  const { orejas } = MG90S;
  grupo.add(caja(m.servo, [MG90S.ancho, MG90S.largo, MG90S.alto], [0, centroY, MG90S.alto / 2]));
  grupo.add(caja(m.servo, [MG90S.ancho, orejas.largo, orejas.espesor], [0, centroY, orejas.desdeFondo + orejas.espesor / 2]));
  grupo.add(caja(m.etiqueta, [0.3, MG90S.largo - 4, 9], [MG90S.ancho / 2 + 0.15, centroY, 8]));
  const tapa = MG90S.tapa - MG90S.alto;
  grupo.add(disco(m.servo, MG90S.ancho / 2, tapa, [0, 0, MG90S.alto + tapa / 2]));
  grupo.add(disco(m.horn, 2.4, MG90S.eje - MG90S.tapa, [0, 0, (MG90S.tapa + MG90S.eje) / 2]));
  grupo.matrixAutoUpdate = false;
  grupo.matrix.copy(matrizDe(filas));
  return grupo;
}

/**
 * Un horn redondo con su transformación: un disco sobre +z con la cara de abajo en el origen (mm).
 *
 * @param {Record<string, THREE.Material>} m
 * @param {number[]} filas - Las tres primeras filas de su transformación, en 12 números.
 * @returns {THREE.Mesh}
 */
function horn(m, filas) {
  const malla = disco(m.horn, 3.5, HORN, [0, 0, HORN / 2]);
  malla.applyMatrix4(matrizDe(filas));
  return malla;
}

/**
 * Una caja centrada en un punto.
 *
 * @param {THREE.Material} material
 * @param {number[]} medidas - Los lados [x, y, z].
 * @param {number[]} centro
 * @returns {THREE.Mesh}
 */
function caja(material, medidas, centro) {
  const malla = new THREE.Mesh(new THREE.BoxGeometry(...medidas), material);
  malla.position.set(...centro);
  return malla;
}

/**
 * Un cilindro con el eje en z, centrado en un punto.
 *
 * @param {THREE.Material} material
 * @param {number} radio
 * @param {number} alto
 * @param {number[]} centro
 * @returns {THREE.Mesh}
 */
function disco(material, radio, alto, centro) {
  const geometria = new THREE.CylinderGeometry(radio, radio, alto, 32);
  geometria.rotateX(Math.PI / 2); // CylinderGeometry viene a lo largo de y
  const malla = new THREE.Mesh(geometria, material);
  malla.position.set(...centro);
  return malla;
}

/**
 * Los materiales con los colores del brazo real.
 *
 * @returns {Record<string, THREE.Material>}
 */
function materialesReales() {
  const material = (color, rugosidad) => new THREE.MeshStandardMaterial({ color, roughness: rugosidad });
  return {
    pla: material(COLORES.pla, 0.8),
    servo: material(COLORES.servo, 0.35),
    etiqueta: material(COLORES.etiqueta, 0.5),
    horn: material(COLORES.horn, 0.6),
  };
}

/**
 * Un mismo material transparente para todas las piezas, como en la pose final y la silueta.
 *
 * @param {number} color
 * @param {number} opacidad
 * @returns {Record<string, THREE.Material>}
 */
function materialesTransparentes(color, opacidad) {
  const material = new THREE.MeshStandardMaterial({ color, roughness: 0.6, transparent: true, opacity: opacidad, depthWrite: false });
  return { pla: material, servo: material, etiqueta: material, horn: material };
}

/**
 * Un grupo cuya matriz se escribe a mano con `colocar`.
 *
 * @param {THREE.Object3D} padre
 * @returns {THREE.Group}
 */
function marcoMovil(padre) {
  const grupo = new THREE.Group();
  grupo.matrixAutoUpdate = false;
  padre.add(grupo);
  return grupo;
}

/**
 * Ubica un marco móvil con una matriz.
 *
 * @param {THREE.Group} grupo
 * @param {THREE.Matrix4} matriz
 */
function colocar(grupo, matriz) {
  grupo.matrix.copy(matriz);
  grupo.matrixWorldNeedsUpdate = true;
}

/**
 * Saca las piezas de un grupo y libera sus geometrías.
 *
 * @param {THREE.Group} grupo
 */
function vaciar(grupo) {
  grupo.traverse((objeto) => objeto.geometry?.dispose());
  grupo.clear();
}

/**
 * La matriz 4x4 de tres filas de cuatro números: un marco de la telemetría o una transformación de 12 números.
 *
 * @param {number[][] | number[]} filas
 * @returns {THREE.Matrix4}
 */
function matrizDe(filas) {
  const [a, b, c] = filas.length === 12 ? [filas.slice(0, 4), filas.slice(4, 8), filas.slice(8)] : filas;
  return new THREE.Matrix4().set(...a, ...b, ...c, 0, 0, 0, 1);
}

/**
 * La transformación que aplica primero `b` y después `a`, las dos con sus tres primeras filas.
 *
 * @param {number[]} a
 * @param {number[]} b
 * @returns {number[]} Las tres primeras filas del resultado, en 12 números.
 */
function componer(a, b) {
  const e = matrizDe(a).multiply(matrizDe(b)).elements; // por columnas
  return [e[0], e[4], e[8], e[12], e[1], e[5], e[9], e[13], e[2], e[6], e[10], e[14]];
}

/**
 * El vector de cada eslabón expresado en su propio marco: R_i^T (p_(i+1) - p_i). Es SALTO_ESLABONES[i].
 *
 * @param {{marcos: number[][][], puntos: number[][]}} dibujo
 * @returns {number[][]}
 */
function saltosDe({ marcos, puntos }) {
  return marcos.map((filas, i) => {
    const d = [0, 1, 2].map((k) => puntos[i + 1][k] - puntos[i][k]);
    return [0, 1, 2].map((j) => filas[0][j] * d[0] + filas[1][j] * d[1] + filas[2][j] * d[2]);
  });
}

/**
 * Si dos juegos de saltos coinciden, para no volver a armar las piezas con cada telemetría.
 *
 * @param {number[][]} a
 * @param {number[][]} b
 * @returns {boolean}
 */
function parecidos(a, b) {
  return a.every((salto, i) => salto.every((v, k) => Math.abs(v - b[i][k]) < 1e-6));
}
