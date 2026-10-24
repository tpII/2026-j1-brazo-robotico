/**
 * @file El brazo 3D, armado con las piezas del brazo real en el marco de cada eslabón.
 *
 * Archivo nuevo de 2026 (grupo J1). La telemetría manda en `marcos` la rotación y el origen de
 * cada eslabón, los mismos que usa la cinemática directa (M_i). Cada pieza se arma en el marco
 * que la mueve, así que el dibujo sigue exactamente a la simulación:
 * - lo que gira con la articulación i (las placas, el disco de la muñeca o la mano) va en M_i;
 * - el cuerpo del servo i va en M_i · Rz(−θ_i), el marco de la articulación antes de girar. Ese
 *   marco queda fijo al eslabón anterior, como el servo atornillado en el brazo real.
 *
 * Los largos también salen de la telemetría, así que el dibujo se adapta si cambian las medidas
 * del modelo. Las formas copian las fotos del brazo armado: el soporte en X con la caja del servo
 * de la base, que es negro; el plato redondo con el servo del hombro acostado encima; el brazo, de
 * dos placas caladas que llegan hasta el final del servo del codo, y el antebrazo, con una ranura
 * y la caja del servo de la muñeca. La pinza es la de antes: la muñeca gira su base, que lleva el
 * servo 4, y ese servo inclina una mano en U con dos dedos cortos que terminan en p5, la punta de
 * la pinza. Las medidas de las piezas son aproximadas; para los cálculos valen los puntos y las
 * esferas.
 */
import * as THREE from "three";

/**
 * Escala de las piezas respecto del brazo real. Desde que el modelo tiene las medidas del brazo
 * armado (23-oct), las piezas van a tamaño real.
 */
const ESCALA = 1;

/** Servo SG90 (cm): largo del cuerpo, ancho, alto a lo largo del eje y distancia del eje a la punta. */
const SG90 = { largo: 2.3 * ESCALA, ancho: 1.2 * ESCALA, alto: 2.3 * ESCALA, eje: 0.6 * ESCALA };

/** Espesor de las placas impresas en 3D (cm). */
const PLACA = 0.25 * ESCALA;

/** Altura del eje de salida del servo sobre su cara (cm). */
const SALIDA = 0.3 * ESCALA;

/** Espesor de un horn (cm). */
const HORN = 0.1;

/** Espesor del plato que gira con la base (cm). */
const ALTO_PLATO = 0.5;

/** Colores del brazo real: PLA gris, servos azules, el de la base negro y horns blancos. */
const COLORES = { pla: 0xc9cdd2, servo: 0x2d5fd6, servoBase: 0x2b2e34, horn: 0xf2f2f2 };

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
export function crearBrazo(escena, { conBase = true, color = null, opacidad = 1 } = {}) {
  const materiales = color === null ? materialesReales() : materialesFantasma(color, opacidad);
  const grupo = new THREE.Group();
  grupo.visible = false;
  escena.add(grupo);
  const eslabones = Array.from({ length: 5 }, () => marcoMovil(grupo));
  const carcasas = Array.from({ length: 5 }, () => marcoMovil(grupo));
  let saltos = null;

  function actualizar(dibujo) {
    grupo.visible = Boolean(dibujo?.marcos);
    if (!grupo.visible) return;
    const nuevos = saltosDe(dibujo);
    if (!saltos || !parecidos(saltos, nuevos)) {
      saltos = nuevos;
      armar({ eslabones, carcasas }, materiales, saltos, conBase);
    }
    dibujo.marcos.forEach((filas, i) => {
      const marco = matrizDe(filas);
      colocar(eslabones[i], marco);
      colocar(carcasas[i], marco.clone().multiply(giroZ(-dibujo.angles[i])));
    });
  }

  return { grupo, actualizar };
}

/**
 * Arma todas las piezas a partir de los saltos (el vector de cada eslabón en su propio marco).
 *
 * @param {{eslabones: THREE.Group[], carcasas: THREE.Group[]}} marcos
 * @param {Record<string, THREE.Material>} m - Materiales.
 * @param {number[][]} saltos - Los cinco SALTO_ESLABONES de configuracion.py, en cm.
 * @param {boolean} conBase
 */
function armar({ eslabones, carcasas }, m, saltos, conBase) {
  [...eslabones, ...carcasas].forEach(vaciar);
  // El servo del hombro va acostado sobre el plato, así que su eje queda a medio ancho del plato.
  const techoPlato = Math.hypot(...saltos[0]) - SG90.ancho / 2;
  // De p3 a la cara de atrás del servo 4, que está a SG90.largo − SG90.eje detrás de su eje (p4).
  const hastaServoPinza = Math.hypot(...saltos[3]) - (SG90.largo - SG90.eje);
  if (conBase) armarBase(carcasas[0], m, techoPlato - ALTO_PLATO);
  armarPlato(eslabones[0], m, techoPlato);
  armarServoHombro(carcasas[1], m);
  armarBrazo(eslabones[1], m, saltos[1]);
  armarAntebrazo(eslabones[2], m, saltos[2], hastaServoPinza - PLACA - HORN - SALIDA);
  armarServoMuneca(carcasas[3], m, hastaServoPinza);
  armarBasePinza(eslabones[3], m, hastaServoPinza, Math.hypot(...saltos[3]));
  armarServoPinza(carcasas[4], m);
  armarMano(eslabones[4], m, saltos[4]);
}

/**
 * El soporte en X, con un agujero en cada punta, y la caja con el servo de la base: no giran. El
 * servo es negro, como en el brazo armado, y su cara de arriba llega a `techo`, debajo del plato.
 */
function armarBase(grupo, m, techo) {
  const forma = contorno([-5.3, 0], 0.7, [5.3, 0], 0.7);
  for (const x of [-5.3, 5.3]) {
    const agujero = new THREE.Path();
    agujero.absarc(x, 0, 0.3, 0, 2 * Math.PI, false);
    forma.holes.push(agujero);
  }
  [Math.PI / 4, -Math.PI / 4].forEach((angulo, i) => {
    const barra = placaPerfilada(m.pla, forma, 0.4, 0.2 + 0.01 * i); // la segunda, apenas más arriba
    barra.rotation.z = angulo;
    grupo.add(barra);
  });
  const servoBase = servo(m, m.servoBase);
  servoBase.position.z = techo - SG90.alto / 2;
  grupo.add(servoBase);
  // La caja va centrada en el eje, como el plato y el hombro, y encierra el servo, que queda corrido
  // hacia −y porque su eje no está en el medio del cuerpo.
  const altoCaja = techo - 0.75; // hasta las orejas: la parte de arriba del servo queda a la vista
  const largoCaja = 2 * (SG90.largo - SG90.eje) + 0.6;
  grupo.add(caja(m.pla, [SG90.ancho + 0.6, largoCaja, altoCaja], [0, 0, altoCaja / 2]));
}

/** El plato redondo que gira con la base, con la cara de arriba en `techo`. */
function armarPlato(grupo, m, techo) {
  grupo.add(disco(m.pla, 1.9, ALTO_PLATO, [0, 0, techo - ALTO_PLATO / 2]));
}

/** El servo del hombro, acostado sobre el plato y con el cuerpo hacia atrás del eje, como en las fotos. */
function armarServoHombro(grupo, m) {
  const hombro = servo(m);
  hombro.rotation.z = -Math.PI / 2; // el cuerpo hacia −x y el ancho, vertical
  grupo.add(hombro);
}

/**
 * El brazo: dos placas caladas a los costados de los servos del hombro y del codo, con los dos
 * horns. Llegan hasta 1 cm después del eje del codo, donde termina su servo (8.5 cm desde el plato
 * en el brazo armado), y se angostan hacia el hombro para pasar sobre el plato.
 */
function armarBrazo(grupo, m, salto) {
  const largo = Math.hypot(salto[0], salto[1]);
  const piezas = new THREE.Group();
  piezas.rotation.z = Math.atan2(salto[1], salto[0]);
  const z = SG90.alto / 2 + SALIDA + PLACA / 2 + 0.05; // por fuera del eje de salida
  const forma = contorno([0, 0], SG90.ancho / 2 - 0.05, [largo, 0], 1);
  forma.holes.push(contorno([0.3 * largo, 0], 0.35, [0.6 * largo, 0], 0.35));
  for (const lado of [z, -z]) piezas.add(placaPerfilada(m.pla, forma, PLACA, lado));
  for (const x of [0, largo]) piezas.add(disco(m.horn, 0.5 * ESCALA, 0.1, [x, 0, z - PLACA / 2 - 0.05]));
  grupo.add(piezas);
}

/**
 * El antebrazo: el servo del codo hacia atrás, dos placas con una ranura que se ensanchan hacia la
 * muñeca y la caja del servo de la muñeca, abierta a los costados. La cara del eje de ese servo
 * queda `frenteMuneca` cm delante de p3.
 */
function armarAntebrazo(grupo, m, salto, frenteMuneca) {
  const codo = servo(m);
  codo.rotation.z = -Math.PI / 2; // el cuerpo hacia atrás del codo
  grupo.add(codo);
  const z = SG90.alto / 2 + 0.05 + PLACA / 2;
  // El servo de la muñeca en este marco: de su cara de atrás a la del eje, y de abajo hacia arriba.
  const delante = salto[0] + frenteMuneca;
  const atras = delante - SG90.alto;
  const abajo = salto[1] - (SG90.largo - SG90.eje);
  const arriba = salto[1] + SG90.eje;
  const medio = (abajo + arriba) / 2;
  // Las placas van de una punta redondeada detrás del codo hasta la pared de atrás de la caja.
  const forma = new THREE.Shape();
  forma.absarc(-(SG90.largo - SG90.eje) + 0.6, 0, 0.75, Math.PI / 2, (3 * Math.PI) / 2, false);
  forma.lineTo(atras, abajo - PLACA);
  forma.lineTo(atras, arriba + PLACA);
  forma.closePath();
  forma.holes.push(contorno([0.45 * salto[0], -0.4], 0.15, [0.45 * salto[0], 0.45], 0.15));
  for (const lado of [z, -z]) grupo.add(placaPerfilada(m.pla, forma, PLACA, lado));
  // La caja de la muñeca: una pared atrás, de placa a placa, y dos travesaños hasta las orejas.
  grupo.add(caja(m.pla, [PLACA, arriba - abajo + 2 * PLACA, 2 * z + PLACA], [atras - PLACA / 2, medio, 0]));
  const largoCaja = delante - 0.6 * ESCALA - 0.125 * ESCALA - (atras - PLACA);
  for (const y of [abajo - PLACA / 2, arriba + PLACA / 2]) {
    grupo.add(caja(m.pla, [largoCaja, PLACA, SG90.ancho + 2 * PLACA], [atras - PLACA + largoCaja / 2, y, 0]));
  }
}

/**
 * El servo de la muñeca, dentro del antebrazo, con el eje hacia adelante (−z). Se corre sobre su eje
 * hasta que el horn toca la base de la pinza.
 */
function armarServoMuneca(grupo, m, hastaServoPinza) {
  const muneca = servo(m);
  muneca.rotation.y = Math.PI; // el eje mira hacia adelante; el cuerpo queda hacia el codo
  muneca.position.z = SG90.alto / 2 + SALIDA + HORN - (hastaServoPinza - PLACA);
  grupo.add(muneca);
}

/**
 * La base de la pinza, impresa en 3D, que gira con la muñeca: el horn del servo de la muñeca
 * atornillado a una placa de atrás, y dos placas, arriba y abajo, que sostienen el servo 4.
 * Adelante es −z, y el eje del servo 4 va sobre x y pasa por p4, en (0, 0, −largo).
 *
 * @param {THREE.Group} grupo
 * @param {Record<string, THREE.Material>} m
 * @param {number} hastaServoPinza - De p3 a la cara de atrás del servo 4 (cm).
 * @param {number} largo - De p3 a p4 (cm).
 */
function armarBasePinza(grupo, m, hastaServoPinza, largo) {
  const atras = hastaServoPinza - PLACA; // donde apoya el horn de la muñeca
  const adelante = largo + SG90.eje; // la cara de adelante del servo 4
  grupo.add(disco(m.horn, 0.5 * ESCALA, HORN, [0, 0, -(atras - HORN / 2)]));
  grupo.add(caja(m.pla, [SG90.alto, SG90.ancho + 2 * PLACA, PLACA], [0, 0, -(atras + PLACA / 2)]));
  for (const y of [SG90.ancho / 2 + PLACA / 2, -(SG90.ancho / 2 + PLACA / 2)]) {
    grupo.add(caja(m.pla, [SG90.alto, PLACA, adelante - atras], [0, y, -(atras + adelante) / 2]));
  }
}

/** El servo 4, dentro de la base de la pinza: el eje sobre p4 y el cuerpo hacia la muñeca (−x). */
function armarServoPinza(grupo, m) {
  const pinza = servo(m);
  pinza.rotation.z = -Math.PI / 2;
  grupo.add(pinza);
}

/**
 * La mano: una U que gira sobre el eje del servo 4, con una placa a cada lado del servo (una
 * atornillada al horn), un travesaño y dos dedos cortos que terminan en p5.
 *
 * @param {THREE.Group} grupo
 * @param {Record<string, THREE.Material>} m
 * @param {number[]} salto - De p4 a p5 en el marco de la pinza: la punta de los dedos.
 */
function armarMano(grupo, m, salto) {
  const largo = Math.hypot(salto[0], salto[1]);
  const piezas = new THREE.Group();
  piezas.rotation.z = Math.atan2(salto[1], salto[0]); // los dedos sobre +x
  const lado = SG90.alto / 2 + SALIDA + HORN + PLACA / 2; // por fuera del horn
  const travesano = largo - 0.85; // los dedos miden 0.8 cm y terminan en p5
  for (const z of [lado, -lado]) piezas.add(placa(m, travesano, 0.9 * ESCALA, z));
  piezas.add(disco(m.horn, 0.5 * ESCALA, HORN, [0, 0, lado - PLACA / 2 - HORN / 2]));
  piezas.add(caja(m.pla, [PLACA * 1.5, 0.9 * ESCALA, 2 * lado], [travesano, 0, 0]));
  for (const z of [lado / 2, -lado / 2]) {
    piezas.add(caja(m.pla, [0.8, 0.8 * ESCALA, 0.6 * ESCALA], [travesano + 0.45, 0, z]));
  }
  grupo.add(piezas);
}

/**
 * Un servo SG90 con el eje de salida en el origen, a lo largo de +z. El cuerpo queda centrado en
 * z y se extiende hacia −y desde el eje, con las orejas de montaje y el eje sobre la cara +z.
 *
 * @param {Record<string, THREE.Material>} m
 * @param {THREE.Material} [material=m.servo] - El del cuerpo; el servo de la base es negro.
 * @returns {THREE.Group}
 */
function servo(m, material = m.servo) {
  const grupo = new THREE.Group();
  const centroY = -(SG90.largo / 2 - SG90.eje);
  grupo.add(caja(material, [SG90.ancho, SG90.largo, SG90.alto], [0, centroY, 0]));
  grupo.add(
    caja(material, [SG90.ancho, SG90.largo + 0.9 * ESCALA, 0.25 * ESCALA], [0, centroY, SG90.alto / 2 - 0.6 * ESCALA]),
  );
  grupo.add(disco(m.horn, 0.25 * ESCALA, SALIDA, [0, 0, SG90.alto / 2 + SALIDA / 2]));
  return grupo;
}

/** Una placa sobre el eje x, de 0 a `largo`, con las puntas redondeadas, centrada en `z`. */
function placa(m, largo, ancho, z) {
  const grupo = new THREE.Group();
  grupo.add(caja(m.pla, [largo, ancho, PLACA], [largo / 2, 0, z]));
  grupo.add(disco(m.pla, ancho / 2, PLACA, [0, 0, z]), disco(m.pla, ancho / 2, PLACA, [largo, 0, z]));
  return grupo;
}

/**
 * El contorno de dos círculos unidos por sus tangentes de afuera: una placa de puntas redondeadas
 * que puede ser más ancha en una punta que en la otra. También sirve de agujero.
 *
 * @param {number[]} centro1 - [x, y] de una punta.
 * @param {number} radio1
 * @param {number[]} centro2 - [x, y] de la otra punta.
 * @param {number} radio2
 * @returns {THREE.Shape}
 */
function contorno([x1, y1], radio1, [x2, y2], radio2) {
  const rumbo = Math.atan2(y2 - y1, x2 - x1);
  // Ángulo entre el rumbo y la normal de cada tangente, sobre los dos círculos.
  const abertura = Math.acos((radio1 - radio2) / Math.hypot(x2 - x1, y2 - y1));
  const forma = new THREE.Shape();
  forma.absarc(x1, y1, radio1, rumbo + abertura, rumbo + 2 * Math.PI - abertura, false);
  forma.absarc(x2, y2, radio2, rumbo - abertura, rumbo + abertura, false);
  forma.closePath();
  return forma;
}

/** Una placa con el contorno de `forma` en el plano xy y de espesor `espesor`, centrada en `z`. */
function placaPerfilada(material, forma, espesor, z) {
  const geometria = new THREE.ExtrudeGeometry(forma, { depth: espesor, bevelEnabled: false, curveSegments: 24 });
  geometria.translate(0, 0, -espesor / 2);
  const malla = new THREE.Mesh(geometria, material);
  malla.position.z = z;
  return malla;
}

/** Una caja de medidas [x, y, z] centrada en un punto. */
function caja(material, medidas, centro) {
  const malla = new THREE.Mesh(new THREE.BoxGeometry(...medidas), material);
  malla.position.set(...centro);
  return malla;
}

/** Un cilindro con el eje en z, centrado en un punto. */
function disco(material, radio, alto, centro) {
  const geometria = new THREE.CylinderGeometry(radio, radio, alto, 32);
  geometria.rotateX(Math.PI / 2); // CylinderGeometry viene a lo largo de y
  const malla = new THREE.Mesh(geometria, material);
  malla.position.set(...centro);
  return malla;
}

function materialesReales() {
  const material = (color, rugosidad) => new THREE.MeshStandardMaterial({ color, roughness: rugosidad });
  return {
    pla: material(COLORES.pla, 0.8),
    servo: material(COLORES.servo, 0.35),
    servoBase: material(COLORES.servoBase, 0.3),
    horn: material(COLORES.horn, 0.6),
  };
}

function materialesFantasma(color, opacidad) {
  const material = new THREE.MeshStandardMaterial({ color, roughness: 0.6, transparent: true, opacity: opacidad, depthWrite: false });
  return { pla: material, servo: material, servoBase: material, horn: material };
}

/** Un grupo cuya matriz se escribe a mano con `colocar`. */
function marcoMovil(padre) {
  const grupo = new THREE.Group();
  grupo.matrixAutoUpdate = false;
  padre.add(grupo);
  return grupo;
}

function colocar(grupo, matriz) {
  grupo.matrix.copy(matriz);
  grupo.matrixWorldNeedsUpdate = true;
}

function vaciar(grupo) {
  grupo.traverse((objeto) => objeto.geometry?.dispose());
  grupo.clear();
}

/** La matriz 4×4 de un marco de la telemetría (tres filas de cuatro números). */
function matrizDe([a, b, c]) {
  return new THREE.Matrix4().set(...a, ...b, ...c, 0, 0, 0, 1);
}

/** Giro de un ángulo en grados alrededor de z, como matriz_rotacion_z de algebra_lineal. */
function giroZ(grados) {
  return new THREE.Matrix4().makeRotationZ((grados * Math.PI) / 180);
}

/**
 * El vector de cada eslabón expresado en su propio marco: Rᵢᵀ (pᵢ₊₁ − pᵢ). Es SALTO_ESLABONES[i].
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

function parecidos(a, b) {
  return a.every((salto, i) => salto.every((v, k) => Math.abs(v - b[i][k]) < 1e-6));
}
