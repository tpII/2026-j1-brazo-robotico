/**
 * @file Elegir el objetivo con dos clics sobre la vista 3D.
 *
 * Archivo nuevo de 2026 (grupo J1). El primer clic sobre el piso fija x e y y levanta una línea
 * vertical en ese punto. El segundo clic fija la altura z: el rayo del clic se corta con un plano
 * vertical que pasa por la línea y mira hacia la cámara. Escape cancela. Arrastrar sigue girando
 * la vista: solo cuenta como clic si el puntero casi no se movió.
 */
import * as THREE from "three";

/** Altura de la línea vertical (cm): un poco más que el alcance del brazo. */
const ALTURA_MAXIMA = 25;

/** Cuánto puede moverse el puntero (px) entre apretar y soltar para que cuente como clic. */
const TOLERANCIA_CLIC = 5;

const COLOR_GUIA = 0xffc04d;

/**
 * Activa la selección del objetivo con dos clics.
 *
 * @param {object} opciones
 * @param {THREE.Scene} opciones.escena
 * @param {THREE.Camera} opciones.camara
 * @param {HTMLCanvasElement} opciones.lienzo - El lienzo de WebGL, que recibe los clics.
 * @param {(texto: string) => void} opciones.alCambiar - Recibe "x, y" o "x, y, z" mientras se elige.
 * @param {(x: number, y: number, z: number) => void} opciones.alElegir - Se llama con el objetivo
 *   elegido, redondeado a centésimas de cm.
 */
export function activarSeleccion({ escena, camara, lienzo, alCambiar, alElegir }) {
  const guias = crearGuias(escena);
  const rayo = new THREE.Raycaster();
  const piso = new THREE.Plane(new THREE.Vector3(0, 0, 1), 0);
  let base = null; // (x, y, 0) después del primer clic
  let apretado = null; // dónde se apretó el botón, para distinguir un clic de un arrastre

  /** El punto donde el rayo del puntero corta un plano, o null si no lo corta. */
  function corte(evento, plano) {
    const caja = lienzo.getBoundingClientRect();
    const puntero = new THREE.Vector2(
      ((evento.clientX - caja.left) / caja.width) * 2 - 1,
      -((evento.clientY - caja.top) / caja.height) * 2 + 1,
    );
    rayo.setFromCamera(puntero, camara);
    return rayo.ray.intersectPlane(plano, new THREE.Vector3());
  }

  /** La altura que marca el puntero sobre la línea vertical, entre 0 y ALTURA_MAXIMA. */
  function alturaEn(evento) {
    const punto = corte(evento, planoVertical(base, camara));
    return punto ? THREE.MathUtils.clamp(punto.z, 0, ALTURA_MAXIMA) : null;
  }

  function cancelar() {
    base = null;
    guias.ocultar();
  }

  lienzo.addEventListener("pointerdown", (evento) => {
    if (evento.button === 0) apretado = { x: evento.clientX, y: evento.clientY };
  });

  lienzo.addEventListener("pointermove", (evento) => {
    if (base === null) {
      const punto = corte(evento, piso);
      if (punto) guias.apuntar(punto);
      return;
    }
    const z = alturaEn(evento);
    if (z === null) return;
    guias.marcarAltura(z);
    alCambiar(texto(base.x, base.y, z));
  });

  lienzo.addEventListener("pointerleave", () => {
    if (base === null) guias.ocultar();
  });

  lienzo.addEventListener("pointerup", (evento) => {
    const fueClic =
      apretado && Math.hypot(evento.clientX - apretado.x, evento.clientY - apretado.y) <= TOLERANCIA_CLIC;
    apretado = null;
    if (!fueClic) return;
    if (base === null) {
      const punto = corte(evento, piso);
      if (!punto) return;
      base = new THREE.Vector3(redondear(punto.x), redondear(punto.y), 0);
      guias.fijarBase(base);
      alCambiar(texto(base.x, base.y));
      return;
    }
    const z = alturaEn(evento);
    if (z === null) return;
    const { x, y } = base;
    cancelar();
    alCambiar(texto(x, y, z));
    alElegir(x, y, redondear(z));
  });

  window.addEventListener("keydown", (evento) => {
    if (evento.key === "Escape") cancelar();
  });
}

/**
 * Un plano vertical que pasa por la base y mira hacia la cámara. Si la cámara mira justo desde
 * arriba, el plano mira hacia +x.
 *
 * @param {THREE.Vector3} base
 * @param {THREE.Camera} camara
 * @returns {THREE.Plane}
 */
function planoVertical(base, camara) {
  const normal = new THREE.Vector3().subVectors(camara.position, base).setZ(0);
  if (normal.lengthSq() < 1e-6) normal.set(1, 0, 0);
  return new THREE.Plane().setFromNormalAndCoplanarPoint(normal.normalize(), base);
}

/**
 * Las guías de la selección: un anillo sobre el piso, la línea vertical y el punto de la altura.
 *
 * @param {THREE.Scene} escena
 */
function crearGuias(escena) {
  const material = new THREE.MeshBasicMaterial({ color: COLOR_GUIA, side: THREE.DoubleSide });
  const anillo = new THREE.Mesh(new THREE.RingGeometry(0.3, 0.45, 32), material);
  const vertical = new THREE.Line(
    new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(0, 0, 0), new THREE.Vector3(0, 0, ALTURA_MAXIMA)]),
    new THREE.LineDashedMaterial({ color: COLOR_GUIA, dashSize: 0.4, gapSize: 0.25 }),
  );
  vertical.computeLineDistances();
  const punto = new THREE.Mesh(new THREE.SphereGeometry(0.25, 16, 12), material);
  escena.add(anillo, vertical, punto);

  const guias = {
    ocultar() {
      anillo.visible = vertical.visible = punto.visible = false;
    },
    /** Sigue al puntero sobre el piso antes del primer clic. */
    apuntar(sobrePiso) {
      anillo.visible = true;
      anillo.position.set(sobrePiso.x, sobrePiso.y, 0.01);
    },
    fijarBase(base) {
      anillo.position.set(base.x, base.y, 0.01);
      vertical.position.set(base.x, base.y, 0);
      punto.position.set(base.x, base.y, 0);
      anillo.visible = vertical.visible = punto.visible = true;
    },
    marcarAltura(z) {
      punto.position.z = z;
    },
  };
  guias.ocultar();
  return guias;
}

function redondear(valor) {
  return Math.round(valor * 100) / 100;
}

function texto(...valores) {
  return valores.map((v) => v.toFixed(2)).join(", ");
}
