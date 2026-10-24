"""servidor.py: el emulador del ESP32, con los mismos endpoints que el firmware.

El test levanta el servidor en un puerto libre y corre su bucle en un hilo del test; el servidor
mismo no usa hilos.
"""
import http.client
import json
import threading
import time
import unittest
import urllib.error
import urllib.parse
import urllib.request

import centralizador
import servidor

OBJ_OK = (5.0, -5.0, 7.0)


class TestServidor(unittest.TestCase):
    """El emulador del ESP32, con pedidos HTTP de verdad."""
    def setUp(self):
        self.c = centralizador.Centralizador()
        self.srv = servidor.crear_servidor(self.c, puerto=0, registrar=False)
        self.parar = threading.Event()
        self.hilo = threading.Thread(target=servidor.correr, args=(self.srv, self.c, lambda: not self.parar.is_set()),
                                     daemon=True)
        self.hilo.start()
        self.base = f"http://127.0.0.1:{self.srv.server_address[1]}"

    def tearDown(self):
        self.parar.set()
        self.hilo.join(timeout=5)
        self.srv.server_close()

    def _pedir(self, ruta, campos=None):
        datos = urllib.parse.urlencode(campos).encode() if campos is not None else None
        pedido = urllib.request.Request(self.base + ruta, data=datos, method="POST" if campos is not None else "GET")
        try:
            with urllib.request.urlopen(pedido, timeout=10) as r:
                return r.status, dict(r.headers), r.read()
        except urllib.error.HTTPError as e:
            with e:
                return e.code, dict(e.headers), e.read()

    def _data(self):
        return json.loads(self._pedir("/data")[2])

    def _post(self, ruta, **campos):
        return json.loads(self._pedir(ruta, campos)[2])

    def _esperar(self, condicion, segundos):
        limite = time.monotonic() + segundos
        while time.monotonic() < limite:
            if condicion():
                return True
            time.sleep(0.05)
        return False

    def test_data(self):
        """GET /data devuelve la telemetría con CORS, como J5."""
        estado, cabeceras, cuerpo = self._pedir("/data")
        self.assertEqual(estado, 200)
        self.assertEqual(cabeceras["Access-Control-Allow-Origin"], "*")
        self.assertIn("angles", json.loads(cuerpo))

    def test_pagina(self):
        """GET / devuelve la página."""
        estado, cabeceras, _ = self._pedir("/")
        self.assertEqual(estado, 200)
        self.assertIn("text/html", cabeceras["Content-Type"])

    def test_set_y_movimiento(self):
        """POST /set mueve el brazo, responde «ocupado» mientras se mueve y el brazo termina quieto."""
        self.assertEqual(self._post("/set", x=OBJ_OK[0], y=OBJ_OK[1], z=OBJ_OK[2])["resultado"], "ok")
        self.assertEqual(self._post("/set", x=5, y=5, z=7)["resultado"], "ocupado")
        self.assertTrue(self._esperar(lambda: self._data()["estado"] == "quieto", segundos=10))

    def test_detener(self):
        """POST /detener frena el brazo."""
        self._post("/set", x=OBJ_OK[0], y=OBJ_OK[1], z=OBJ_OK[2])
        self.assertEqual(self._post("/detener")["estado"], "detenido")

    def test_entrada_invalida(self):
        """Un /set con campos faltantes, no numéricos, NaN, infinitos o bajo el piso da 400."""
        antes = self._data()["angles"]
        for campos in ({"x": 1, "y": 2}, {"x": "a", "y": 2, "z": 3}, {"x": "nan", "y": 2, "z": 3},
                       {"x": "inf", "y": 2, "z": 3}, {"x": 6, "y": -3, "z": -1}):
            estado, _, cuerpo = self._pedir("/set", campos)
            self.assertEqual(estado, 400, campos)
            self.assertIn("error", json.loads(cuerpo))
        self.assertEqual(self._data()["angles"], antes)

    def test_ruta_desconocida(self):
        """Una ruta que no existe da 404."""
        self.assertEqual(self._pedir("/nada")[0], 404)

    def test_pagina_del_gemelo(self):
        """La página del gemelo digital, con su estilo y sus módulos."""
        pagina = self._pedir("/")[2].decode("utf-8")
        for fragmento in ('type="importmap"', "three@0.160.0", 'id="mover"', 'id="detener"', 'id="esferas"',
                          'id="intento"', 'href="web/style.css"', 'src="web/principal.js"'):
            self.assertIn(fragmento, pagina)
        conexion = self._pedir("/web/conexion.js")[2].decode("utf-8")
        for fragmento in ('url("data")', 'url("set")', 'url("detener")'):
            self.assertIn(fragmento, conexion)

    def test_archivos_web(self):
        """La página está en módulos dentro de code/web/."""
        for ruta, tipo in (("/web/style.css", "text/css"), ("/web/principal.js", "text/javascript")):
            estado, cabeceras, cuerpo = self._pedir(ruta)
            self.assertEqual(estado, 200, ruta)
            self.assertIn(tipo, cabeceras["Content-Type"])
            self.assertTrue(cuerpo)

    def test_nada_fuera_de_la_carpeta_web(self):
        """No se sirve nada fuera de code/web/, aunque la ruta tenga «..»."""
        for ruta in ("/web/../simulacion/servidor.py", "/web/%2e%2e/index.html", "/web/no_existe.js", "/web/"):
            conexion = http.client.HTTPConnection("127.0.0.1", self.srv.server_address[1], timeout=10)
            try:
                conexion.request("GET", ruta)   # sin normalizar la ruta, como la mandaría cualquiera
                respuesta = conexion.getresponse()
                respuesta.read()
                self.assertEqual(respuesta.status, 404, ruta)
            finally:
                conexion.close()


if __name__ == "__main__":
    unittest.main()
