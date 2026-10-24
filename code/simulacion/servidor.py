"""Emulador del ESP32: los mismos endpoints y el mismo JSON que va a tener el firmware.

    GET  /         la página del gemelo digital (code/index.html)
    GET  /web/...  el estilo y los módulos de JavaScript de la página (code/web/)
    GET  /data     la telemetría
    POST /set      el objetivo, con los campos de  x, y y z
    POST /detener  frena el brazo

El bucle es el mismo que va a tener loop() en el ESP32: atiende una petición si hay una y, si
pasaron 20 ms desde el último paso, avanza uno. Si se atrasa no recupera el tiempo: entre dos
pasos siempre hay por lo menos 20 ms, así el brazo nunca va más rápido que lo planificado.
Se corre desde code/simulacion/ con: python servidor.py
Implementación J1.
"""
import argparse
import json
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import parse_qs, unquote

import configuracion as cfg
from centralizador import Centralizador

PAGINA = Path(__file__).resolve().parent.parent / "index.html"
CARPETA_WEB = PAGINA.parent / "web"
TIPOS_WEB = {".css": "text/css; charset=utf-8", ".js": "text/javascript; charset=utf-8"}


def _archivo_web(ruta):
    """El archivo de code/web/ que pide una ruta.

    Args:
        ruta: empieza con /web/.

    Returns:
        El Path del archivo, o None si no existe.
    """
    archivo = (CARPETA_WEB / unquote(ruta[len("/web/"):])).resolve()
    if archivo.parent != CARPETA_WEB or archivo.suffix not in TIPOS_WEB or not archivo.is_file():
        return None
    return archivo


class _Manejador(BaseHTTPRequestHandler):
    """Atiende un pedido HTTP."""

    def _responder(self, codigo, cuerpo, tipo="application/json; charset=utf-8", cors=False):
        """Manda la respuesta.

        Args:
            codigo: código HTTP.
            cuerpo: bytes, o un objeto que se manda como JSON.
            tipo: el Content-Type.
            cors: si agrega Access-Control-Allow-Origin: *, como /data en J5.
        """
        datos = cuerpo if isinstance(cuerpo, bytes) else json.dumps(cuerpo).encode("utf-8")
        self.send_response(codigo)
        self.send_header("Content-Type", tipo)
        self.send_header("Content-Length", str(len(datos)))
        if cors:
            self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(datos)

    def do_GET(self):
        """GET /, /web/..., /data; cualquier otra ruta da 404."""
        ruta = self.path.split("?")[0]
        if ruta == "/":
            self._responder(200, PAGINA.read_bytes(), "text/html; charset=utf-8")   # se lee en cada pedido
        elif ruta == "/data":
            self._responder(200, self.server.centralizador.telemetria(), cors=True)
        elif ruta.startswith("/web/") and (archivo := _archivo_web(ruta)) is not None:
            self._responder(200, archivo.read_bytes(), TIPOS_WEB[archivo.suffix])
        else:
            self._responder(404, {"error": f"no existe {ruta}"})

    def do_POST(self):
        """POST /set con x, y y z (400 si faltan o no son válidos) y POST /detener."""
        largo = int(self.headers.get("Content-Length", 0))
        campos = parse_qs(self.rfile.read(largo).decode("utf-8"))
        ruta = self.path.split("?")[0]
        if ruta == "/set":
            try:
                x, y, z = (float(campos[clave][0]) for clave in ("x", "y", "z"))
            except KeyError:
                return self._responder(400, {"error": "Faltan los campos x, y o z."})
            except ValueError:
                return self._responder(400, {"error": "Los valores de x, y y z tienen que ser números."})
            try:
                respuesta = self.server.centralizador.pedir_objetivo(x, y, z)
            except ValueError as e:
                return self._responder(400, {"error": str(e)})
            self._responder(200, respuesta)
        elif ruta == "/detener":
            self.server.centralizador.detener()
            self._responder(200, {"estado": self.server.centralizador.estado})
        else:
            self._responder(404, {"error": f"no existe {ruta}"})

    def log_message(self, formato, *args):
        """Registra los pedidos en la consola, salvo /data y salvo que registrar sea False."""
        if self.server.registrar and not self.path.startswith("/data"):   # /data llega 10 veces por segundo
            super().log_message(formato, *args)


def crear_servidor(centralizador, puerto=8000, registrar=True):
    """Crea el servidor HTTP en localhost.

    Args:
        centralizador: el Centralizador que atiende los pedidos.
        puerto: el puerto; con 0 el sistema elige uno libre.
        registrar: si muestra los pedidos en la consola.

    Returns:
        El HTTPServer, todavía sin atender pedidos.
    """
    servidor = HTTPServer(("127.0.0.1", puerto), _Manejador)
    servidor.centralizador = centralizador
    servidor.registrar = registrar
    return servidor


def correr(servidor, centralizador, seguir=lambda: True):
    """El bucle de loop(): una petición si hay, y un paso del movimiento cada DT segundos como mínimo.

    Args:
        servidor: el HTTPServer de crear_servidor.
        centralizador: el mismo Centralizador del servidor.
        seguir: función que dice si el bucle sigue; los tests la usan para cortarlo.
    """
    proximo = time.monotonic() + cfg.DT
    while seguir():
        servidor.timeout = max(0.0, proximo - time.monotonic())   # espera pedidos hasta el próximo paso
        servidor.handle_request()
        ahora = time.monotonic()
        if ahora >= proximo:
            centralizador.avanzar()
            proximo = ahora + cfg.DT


def main(argv=None):
    """Arranca el emulador.

    Args:
        argv: los argumentos de la línea de comandos; con None, los de sys.argv.
    """
    parser = argparse.ArgumentParser(description="Emulador del ESP32 para el gemelo digital")
    parser.add_argument("--puerto", type=int, default=8000)
    argumentos = parser.parse_args(argv)
    centralizador = Centralizador()
    servidor = crear_servidor(centralizador, argumentos.puerto)
    print(f"Gemelo digital en http://localhost:{servidor.server_address[1]}", flush=True)
    try:
        correr(servidor, centralizador)
    except KeyboardInterrupt:
        pass
    finally:
        servidor.server_close()


if __name__ == "__main__":
    main()
