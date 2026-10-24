/**
 * @file Las esferas de colisión de cada eslabón.
 *
 * De la página del grupo J5 (2025) vienen los colores de los eslabones, el rojo de las esferas que
 * chocan y la forma de reconocerlas por sus coordenadas (`colliding_centers`). Las esferas se
 * dibujan con poca opacidad encima del brazo 3D y se pueden ocultar, salvo las que chocan.
 */
import * as THREE from "three";

/** Color de las esferas de cada eslabón, de la base a la pinza. */
export const COLORES_ESLABON = [0x228b22, 0xffff00, 0xffa500, 0x00ffff, 0xff00ff];

/** Color de las esferas que chocan. */
export const COLOR_CHOQUE = 0xff0000;

/**
 * Reconoce las esferas que chocan comparando sus coordenadas con `colliding_centers`.
 *
 * @param {object} dibujo - Una pose de la telemetría, con `spheres` y `colliding_centers`.
 * @returns {Set<string>} Las esferas que chocan, como claves `"eslabón,esfera"`.
 */
function chocanPorCoordenadas(dibujo) {
  const clave = (centro) => centro.map((v) => v.toFixed(6)).join(",");
  const centros = new Set((dibujo.colliding_centers ?? []).map(clave));
  const chocan = new Set();
  dibujo.spheres.forEach((eslabon, i) =>
    eslabon.forEach((centro, j) => {
      if (centros.has(clave(centro))) chocan.add(`${i},${j}`);
    }),
  );
  return chocan;
}

// Para abajo es implementado por el grupo J1.

/** Opacidad de las esferas que chocan: casi opacas, para que se vean a través del brazo. */
const OPACIDAD_CHOQUE = 0.9;

/** Geometría de radio 1 que comparten todas las esferas; cada malla la escala a su radio. */
const GEOMETRIA_ESFERA = new THREE.SphereGeometry(1, 24, 16);

/**
 * Las esferas que chocan, con los índices que manda la simulación (`esferas_en_choque`).
 * Si no vienen, como con el firmware de 2025, se reconocen por sus coordenadas.
 *
 * @param {object} dibujo - Una pose de la telemetría.
 * @returns {Set<string>} Claves `"eslabón,esfera"`.
 */
function esferasQueChocan(dibujo) {
  if (dibujo.esferas_en_choque) {
    return new Set(dibujo.esferas_en_choque.map(([i, j]) => `${i},${j}`));
  }
  return chocanPorCoordenadas(dibujo);
}

/**
 * Crea la capa de esferas de un brazo. Las mallas se agregan a medida que hacen falta, así que no
 * depende de cuántas esferas tenga cada eslabón.
 *
 * @param {THREE.Scene} escena
 * @param {object} [opciones]
 * @param {number | null} [opciones.color=null] - Un color para todas; con null, el de cada eslabón.
 * @param {number} [opciones.opacidad=0.22] - Opacidad de las esferas que no chocan.
 * @returns {{grupo: THREE.Group, mallas: THREE.Mesh[][], color: number | null, opacidad: number}}
 */
export function crearEsferas(escena, { color = null, opacidad = 0.22 } = {}) {
  const grupo = new THREE.Group();
  grupo.renderOrder = 2; // después del brazo, porque son transparentes
  escena.add(grupo);
  return { grupo, mallas: [], color, opacidad };
}

/**
 * Ubica las esferas de una pose. Las que chocan se ven siempre, en rojo; las demás, solo si
 * `visibles` es true.
 *
 * @param {ReturnType<typeof crearEsferas>} capa
 * @param {object | null} dibujo - Una pose de la telemetría, o null para ocultar la capa.
 * @param {number} radio - Radio de las esferas en cm (`sphere_radius`).
 * @param {boolean} visibles - Si se muestran las esferas que no chocan.
 */
export function actualizarEsferas(capa, dibujo, radio, visibles) {
  capa.grupo.visible = Boolean(dibujo);
  if (!dibujo) return;
  const chocan = esferasQueChocan(dibujo);
  dibujo.spheres.forEach((eslabon, i) => {
    capa.mallas[i] ??= [];
    eslabon.forEach((centro, j) => {
      const malla = (capa.mallas[i][j] ??= nuevaEsfera(capa));
      const choca = chocan.has(`${i},${j}`);
      malla.visible = choca || visibles;
      malla.position.set(...centro);
      malla.scale.setScalar(radio);
      malla.material.color.setHex(choca ? COLOR_CHOQUE : (capa.color ?? COLORES_ESLABON[i]));
      malla.material.opacity = choca ? OPACIDAD_CHOQUE : capa.opacidad;
    });
  });
}

/**
 * Una esfera de radio 1, transparente y sin escribir en el buffer de profundidad.
 *
 * @param {ReturnType<typeof crearEsferas>} capa
 * @returns {THREE.Mesh}
 */
function nuevaEsfera(capa) {
  const malla = new THREE.Mesh(
    GEOMETRIA_ESFERA,
    new THREE.MeshStandardMaterial({ roughness: 0.5, transparent: true, depthWrite: false }),
  );
  malla.renderOrder = 2;
  capa.grupo.add(malla);
  return malla;
}
