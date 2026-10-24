/**
 * @file Los pedidos al ESP32 o a su emulador: telemetría, objetivo y Detener.
 *
 * De la página del grupo J5 (2025) viene pedir /data cada 100 ms y pasarle el JSON a quien dibuja.
 * La página lo pide a quien la sirvió, o a la dirección de `?servidor=http://IP`.
 */

/**
 * Pide la telemetría cada `ms` milisegundos, sin mandar un pedido nuevo mientras el anterior
 * no terminó.
 *
 * @param {number} ms - Período en milisegundos.
 * @param {(telemetria: object) => void} alLlegar - Recibe el JSON de /data.
 * @param {(hay: boolean) => void} alCambiarConexion - Recibe si el pedido salió bien.
 * @returns {() => Promise<void>} Una función para pedir la telemetría en el momento.
 */
export function pedirDatosCada(ms, alLlegar, alCambiarConexion) {
  let pidiendo = false;
  async function pedirDatos() {
    if (pidiendo) return;
    pidiendo = true;
    try {
      const respuesta = await fetch(url("data"), { cache: "no-store" });
      if (!respuesta.ok) throw new Error(`HTTP ${respuesta.status}`);
      alLlegar(await respuesta.json());
      alCambiarConexion(true);
    } catch {
      alCambiarConexion(false);
    } finally {
      pidiendo = false;
    }
  }
  setInterval(pedirDatos, ms);
  pedirDatos();
  return pedirDatos;
}

// Para abajo es implementado por el grupo J1.

/** La dirección de `?servidor=`, o null si la página le pide a quien la sirvió. */
const SERVIDOR = new URLSearchParams(location.search).get("servidor");

/**
 * La URL de un pedido.
 *
 * @param {string} ruta - "data", "set" o "detener".
 * @returns {string}
 */
export function url(ruta) {
  return SERVIDOR ? SERVIDOR.replace(/\/$/, "") + "/" + ruta : ruta;
}

/**
 * Manda un objetivo con POST /set.
 *
 * @param {number} x
 * @param {number} y
 * @param {number} z
 * @returns {Promise<{ok: boolean, codigo: number, datos: object}>} Si el servidor lo aceptó, el
 *   código HTTP y la respuesta (`resultado`, o `error` si lo rechazó).
 * @throws {TypeError} Si no hay conexión.
 */
export async function mandarObjetivo(x, y, z) {
  const respuesta = await fetch(url("set"), { method: "POST", body: new URLSearchParams({ x, y, z }) });
  const datos = await respuesta.json().catch(() => ({}));
  return { ok: respuesta.ok, codigo: respuesta.status, datos };
}

/**
 * Frena el brazo con POST /detener.
 *
 * @throws {TypeError} Si no hay conexión.
 */
export async function detener() {
  await fetch(url("detener"), { method: "POST" });
}
