/**
 * @file Punto de entrada de la página: une la escena, el brazo, las esferas, el panel y los pedidos.
 *
 * Archivo nuevo de 2026 (grupo J1). Se dibujan tres brazos: la pose actual con sus colores, la
 * pose a la que va mientras se mueve (azul transparente) y la silueta gris del mejor intento
 * cuando no llega. Cada uno tiene sus esferas de colisión.
 */
import { crearBrazo } from "./brazo.js";
import { detener, mandarObjetivo, pedirDatosCada } from "./conexion.js";
import { actualizarEsferas, crearEsferas } from "./esferas.js";
import {
  agregarControles,
  agregarGrilla,
  ajustarTamano,
  crearEscena,
  crearMarcaObjetivo,
  dibujarSiempre,
} from "./escena.js";
import { avisar, mostrarConexion, mostrarDatos, OCUPADO, olvidarAviso } from "./panel.js";
import { activarSeleccion } from "./seleccion.js";

const $ = (id) => document.getElementById(id);

const vista = crearEscena($("vista"));
agregarGrilla(vista.escena);
const controles = agregarControles(vista.camara, vista.renderer.domElement);
ajustarTamano(vista, $("vista"), $("panel"));

const AZUL = 0x3d8bfd;
const GRIS = 0x9aa5b1;
const capas = {
  actual: { brazo: crearBrazo(vista.escena), esferas: crearEsferas(vista.escena) },
  final: {
    brazo: crearBrazo(vista.escena, { conBase: false, color: AZUL, opacidad: 0.3 }),
    esferas: crearEsferas(vista.escena, { color: AZUL, opacidad: 0.12 }),
  },
  silueta: {
    brazo: crearBrazo(vista.escena, { conBase: false, color: GRIS, opacidad: 0.4 }),
    esferas: crearEsferas(vista.escena, { color: GRIS, opacidad: 0.15 }),
  },
};
const mostrarObjetivo = crearMarcaObjetivo(vista.escena);
let ultima = null;

/**
 * Dibuja una telemetría: los tres brazos con sus esferas, el objetivo y el panel.
 *
 * @param {object} t - El JSON de /data.
 */
function dibujar(t) {
  ultima = t;
  const radio = t.sphere_radius ?? 1;
  const verEsferas = $("esferas").checked || !t.marcos; // el firmware de 2025 no manda los marcos
  const poses = {
    actual: t,
    final: t.estado === "moviendo" ? t.pose_final : null,
    silueta: t.silueta ?? null,
  };
  for (const [nombre, { brazo, esferas }] of Object.entries(capas)) {
    brazo.actualizar(poses[nombre]);
    actualizarEsferas(esferas, poses[nombre], radio, verEsferas);
  }
  mostrarObjetivo(t.objetivo);
  mostrarDatos(t);
}

const pedirAhora = pedirDatosCada(100, dibujar, mostrarConexion);

/**
 * Manda un objetivo y avisa si el servidor lo rechaza o si el brazo está ocupado.
 *
 * @param {number} x
 * @param {number} y
 * @param {number} z
 */
async function mover(x, y, z) {
  try {
    const { ok, codigo, datos } = await mandarObjetivo(x, y, z);
    if (!ok) avisar(datos.error ?? `No se pudo mandar el objetivo (HTTP ${codigo})`, "error");
    else if (datos.resultado === "ocupado") avisar(OCUPADO, "alerta");
    else olvidarAviso();
    pedirAhora();
  } catch {
    avisar("No se pudo mandar el objetivo: sin conexión", "error");
  }
}

$("formulario").addEventListener("submit", (evento) => {
  evento.preventDefault();
  const valores = $("objetivo").value.split(/[\s,;]+/).filter(Boolean).map(Number);
  if (valores.length !== 3 || !valores.every(Number.isFinite)) {
    avisar("Escribí tres números separados por comas: x, y, z", "error");
    return;
  }
  mover(...valores);
});

$("detener").addEventListener("click", async () => {
  try {
    await detener();
    pedirAhora();
  } catch {
    avisar("No se pudo detener: sin conexión", "error");
  }
});

$("esferas").addEventListener("change", () => {
  if (ultima) dibujar(ultima);
});

activarSeleccion({
  escena: vista.escena,
  camara: vista.camara,
  lienzo: vista.renderer.domElement,
  alCambiar: (texto) => {
    $("objetivo").value = texto;
  },
  alElegir: mover,
});

dibujarSiempre(vista, () => controles.update());
