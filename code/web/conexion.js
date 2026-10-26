/**
 * @file Los pedidos al ESP32 o a su emulador: telemetría, objetivo y Detener.
 *
 * De la página del grupo J5 (2025) vienen pedir /data cada 100 ms y pasarle el JSON a quien dibuja,
 * y tener escrita la dirección del ESP32.
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
function pedirDatosCada(ms, alLlegar, alCambiarConexion) {
  let pidiendo = false;
  let fallosConsecutivos = 0;
  async function pedirDatos() {
    if (pidiendo) return;
    pidiendo = true;
    try {
      const respuesta = await fetch(url("data"), { cache: "no-store" });
      if (!respuesta.ok) throw new Error(`HTTP ${respuesta.status}`);
      const datos = await respuesta.json();
      fallosConsecutivos = 0;
      alLlegar(datos);
      alCambiarConexion(true);
    } catch {
      fallosConsecutivos++;
      if (fallosConsecutivos >= 3) {
        alCambiarConexion(false);
      }
    } finally {
      pidiendo = false;
    }
  }
  setInterval(pedirDatos, ms);
  pedirDatos();
  return pedirDatos;
}

// Para abajo es implementado por el grupo J1.

/** La dirección del ESP32: la IP que muestra el monitor serie cuando arranca la placa. */
const ESP32 = "http://192.168.0.167";

/**
 * A quién le pide la página. Abierta con doble clic, al ESP32; servida por un servidor, como el
 * emulador de code/simulacion, a ese servidor. `?servidor=http://IP` elige otra dirección.
 */
const SERVIDOR =
  new URLSearchParams(location.search).get("servidor") ?? ESP32;

/**
 * La URL de un pedido.
 *
 * @param {string} ruta - "data", "set" o "detener".
 * @returns {string}
 */
function url(ruta) {
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
async function mandarObjetivo(x, y, z) {
  const respuesta = await fetch(url("set"), { method: "POST", body: new URLSearchParams({ x, y, z }) });
  const datos = await respuesta.json().catch(() => ({}));
  return { ok: respuesta.ok, codigo: respuesta.status, datos };
}

/**
 * Frena el brazo con POST /detener.
 *
 * @throws {TypeError} Si no hay conexión.
 */
async function detener() {
  await fetch(url("detener"), { method: "POST" });
}

/**
 * Vuelve el brazo a la posición original con POST /home.
 *
 * @returns {Promise<{ok: boolean, codigo: number, datos: object}>}
 * @throws {TypeError} Si no hay conexión.
 */
async function irAHome() {
  const respuesta = await fetch(url("home"), { method: "POST" });
  const datos = await respuesta.json().catch(() => ({}));
  return { ok: respuesta.ok, codigo: respuesta.status, datos };
}
