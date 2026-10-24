/**
 * @file El panel: conexión, mensaje del resultado y datos de la búsqueda.
 *
 * De la página del grupo J5 (2025) viene el cuadro de información con el estado y los ángulos.
 */

/**
 * Los ángulos con un decimal, separados por comas.
 *
 * @param {number[]} angulos - Ángulos del modelo en grados.
 * @returns {string}
 */
export function textoAngulos(angulos) {
  return angulos.map((a) => a.toFixed(1)).join(", ");
}

// Para abajo es implementado por el grupo J1.

/** Mensaje cuando llega un objetivo mientras el brazo se mueve. */
export const OCUPADO = "El brazo se está moviendo: apretá Detener antes de mandar otro objetivo";

/** Cuánto dura en pantalla el aviso de una acción (ms) antes de volver al mensaje del estado. */
const DURACION_AVISO = 4000;

const $ = (id) => document.getElementById(id);

let ultima = null; // la última telemetría
let aviso = null; // { texto, clase, hasta }

/**
 * Muestra un aviso sobre la última acción (por ejemplo, un error al mandar el objetivo) durante
 * unos segundos.
 *
 * @param {string} texto
 * @param {"ok" | "alerta" | "error" | ""} clase - Color del mensaje.
 */
export function avisar(texto, clase) {
  aviso = { texto, clase, hasta: performance.now() + DURACION_AVISO };
  pintarMensaje();
}

/** Borra el aviso, para que el mensaje vuelva a mostrar el estado. */
export function olvidarAviso() {
  aviso = null;
  pintarMensaje();
}

/**
 * Marca si llega la telemetría.
 *
 * @param {boolean} hay
 */
export function mostrarConexion(hay) {
  $("punto").className = "punto " + (hay ? "si" : "no");
  $("conexion").textContent = hay ? "Conectado" : "Sin conexión";
}

/**
 * Llena el panel con una telemetría de /data.
 *
 * @param {object} t - El JSON de /data.
 */
export function mostrarDatos(t) {
  ultima = t;
  const m = t.metricas;
  $("estado").textContent = t.estado ?? "—";
  $("error").textContent =
    t.error_cm != null
      ? `${numero(t.error_cm)} cm`
      : t.silueta?.error_cm != null
        ? `${numero(t.silueta.error_cm)} cm (mejor intento)`
        : "—";
  $("iteraciones").textContent = !m
    ? "—"
    : t.resultado === "ok"
      ? `${m.iteraciones} (${m.iteraciones_totales} en total)`
      : `${m.iteraciones_totales} en total`;
  $("posiciones").textContent = m ? String(m.posiciones) : "—";
  $("tiempo").textContent = m ? `${numero(m.tiempo_ms, 1)} ms` : "—";
  $("angulos").textContent = textoAngulos(t.angles);
  mostrarIntento(t.silueta);
  $("pulsos").textContent = t.pulsos_us ? t.pulsos_us.join(", ") : "—";
  pintarMensaje();
}

/**
 * Muestra los ángulos de la silueta cuando la búsqueda no llega, con los de los servos que no
 * dejan llegar en rojo.
 *
 * @param {object | null | undefined} silueta - La silueta de /data, o nada si no hay.
 */
function mostrarIntento(silueta) {
  $("intento-titulo").hidden = !silueta;
  $("intento").hidden = !silueta;
  if (!silueta) return;
  const fuera = silueta.servos_fuera_de_rango ?? [];
  $("intento").replaceChildren(
    ...silueta.angles.flatMap((angulo, i) => {
      const valor = document.createElement("span");
      valor.textContent = angulo.toFixed(1);
      if (fuera[i]) valor.className = "fuera-de-rango";
      return i === 0 ? [valor] : [", ", valor];
    }),
  );
}

function pintarMensaje() {
  const vigente = aviso && performance.now() < aviso.hasta;
  const [texto, clase] = vigente ? [aviso.texto, aviso.clase] : mensajeDe(ultima);
  $("mensaje").textContent = texto;
  $("mensaje").className = clase;
}

/**
 * El mensaje que corresponde al estado y al resultado de la última búsqueda.
 *
 * @param {object | null} t - La última telemetría.
 * @returns {[string, string]} El texto y la clase de color.
 */
function mensajeDe(t) {
  if (!t) return ["Esperando datos…", ""];
  if (!("resultado" in t)) return ["Firmware sin búsqueda de objetivos: solo se muestran las esferas", ""];
  switch (t.resultado) {
    case null:
      return ["Listo. Escribí un objetivo o elegilo con dos clics sobre la vista.", ""];
    case "ok":
      if (t.estado === "moviendo") return ["Moviendo hacia el objetivo", "ok"];
      if (t.estado === "detenido") return ["Detenido antes de llegar al objetivo", "alerta"];
      return [`Objetivo alcanzado (error ${numero(t.error_cm)} cm)`, "ok"];
    case "fuera_de_alcance":
      return ["Fuera de alcance: ninguna posición de la muñeca llega a ese punto", "error"];
    case "bloqueado_por_limites":
      return [
        `Bloqueado por límites: los topes de los servos no dejan llegar. Mejor intento a ${numero(t.silueta?.error_cm)} cm`,
        "alerta",
      ];
    case "colision":
      return ["Colisión: todas las poses que llegan chocan. En rojo, las esferas que chocan", "error"];
    default:
      return [String(t.resultado), ""];
  }
}

function numero(valor, decimales = 2) {
  return typeof valor === "number" ? valor.toFixed(decimales) : "—";
}
