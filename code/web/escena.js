/**
 * @file La escena 3D: cámara, luces, piso, bucle de dibujo y marca del objetivo.
 *
 * Las dos primeras funciones vienen de la página del grupo J5 (2025). La escena usa las
 * coordenadas del robot, con z hacia arriba.
 */

/**
 * Crea la escena, la cámara, el renderer, las luces y el piso dentro de un contenedor.
 *
 * @param {HTMLElement} contenedor - Elemento donde va el lienzo de WebGL.
 * @returns {{escena: THREE.Scene, camara: THREE.PerspectiveCamera, renderer: THREE.WebGLRenderer}}
 */
function crearEscena(contenedor) {
  THREE.Object3D.DEFAULT_UP.set(0, 0, 1);
  const escena = new THREE.Scene();
  escena.background = new THREE.Color(0x1d2126);
  const camara = new THREE.PerspectiveCamera(40, 1, 0.1, 500);
  camara.position.set(21, -28, 20);
  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  contenedor.appendChild(renderer.domElement);

  escena.add(new THREE.AmbientLight(0xffffff, 0.6));
  const sol = new THREE.DirectionalLight(0xffffff, 1.2);
  sol.position.set(12, -16, 24);
  escena.add(sol);

  const piso = new THREE.Mesh(
    new THREE.PlaneGeometry(50, 50),
    new THREE.MeshStandardMaterial({ color: 0x272c33, roughness: 0.95 }),
  );
  piso.position.z = -0.01; // apenas debajo de la grilla, para que no se mezclen
  escena.add(piso);
  return { escena, camara, renderer };
}

/**
 * Dibuja la escena en cada cuadro.
 *
 * @param {{escena: THREE.Scene, camara: THREE.Camera, renderer: THREE.WebGLRenderer}} vista
 * @param {() => void} [antes] - Se llama antes de cada cuadro, por ejemplo para los controles.
 */
function dibujarSiempre({ escena, camara, renderer }, antes = () => {}) {
  renderer.setAnimationLoop(() => {
    antes();
    renderer.render(escena, camara);
  });
}

// Para abajo es implementado por el grupo J1.

/**
 * Controles de órbita: arrastrar gira la vista, la rueda la acerca y el botón derecho la corre.
 *
 * @param {THREE.Camera} camara
 * @param {HTMLCanvasElement} lienzo
 * @returns {OrbitControls}
 */
function agregarControles(camara, lienzo) {
  const controles = new OrbitControls(camara, lienzo);
  controles.target.set(2, 0, 7);
  controles.enableDamping = true;
  controles.update();
  return controles;
}

/**
 * Grilla de 1 cm sobre el piso (z = 0) y los ejes: x en rojo, y en verde y z en azul.
 *
 * @param {THREE.Scene} escena
 */
function agregarGrilla(escena) {
  const grilla = new THREE.GridHelper(50, 50, 0x5b6672, 0x3a414a);
  grilla.rotation.x = Math.PI / 2; // GridHelper viene en el plano xz
  escena.add(grilla);
  escena.add(new THREE.AxesHelper(4));
}

/**
 * Ajusta la imagen al tamaño del contenedor cada vez que cambia.
 *
 * También vuelve a leer la densidad de píxeles de la pantalla, porque el zoom del navegador la
 * cambia junto con el tamaño. Si el panel flota sobre la vista, corre el centro de la imagen a la
 * derecha para que el panel no tape el brazo.
 *
 * @param {{camara: THREE.PerspectiveCamera, renderer: THREE.WebGLRenderer}} vista
 * @param {HTMLElement} contenedor
 * @param {HTMLElement} panel
 */
function ajustarTamano({ camara, renderer }, contenedor, panel) {
  new ResizeObserver(() => {
    const { clientWidth: ancho, clientHeight: alto } = contenedor;
    if (!ancho || !alto) return;
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.setSize(ancho, alto, false);
    camara.aspect = ancho / alto;
    const flota = getComputedStyle(panel).position === "absolute";
    if (flota) camara.setViewOffset(ancho, alto, -(panel.offsetWidth + 16) / 2, 0, ancho, alto);
    else camara.clearViewOffset();
    camara.updateProjectionMatrix();
  }).observe(contenedor);
}

/**
 * Crea la marca del objetivo: una esfera roja con una línea punteada hasta el piso.
 *
 * La telemetría llega cada 100 ms, pero la línea se rearma solo cuando cambia el objetivo. Se le
 * da una geometría nueva y se libera la anterior: si solo se cambiaran sus puntos, three.js
 * seguiría usando la esfera envolvente del primer objetivo para decidir si la línea está a la
 * vista, y la ocultaría al mirar de cerca otro objetivo.
 *
 * @param {THREE.Scene} escena
 * @returns {(objetivo: number[] | null) => void} Función que muestra la marca en un punto
 *   (x, y, z) o la oculta si recibe null.
 */
function crearMarcaObjetivo(escena) {
  const marca = new THREE.Mesh(
    new THREE.SphereGeometry(0.3, 16, 12),
    new THREE.MeshBasicMaterial({ color: 0xff5252 }),
  );
  const caida = new THREE.Line(
    new THREE.BufferGeometry(),
    new THREE.LineDashedMaterial({ color: 0xff5252, dashSize: 0.3, gapSize: 0.2 }),
  );
  marca.visible = caida.visible = false;
  escena.add(marca, caida);
  let dibujado = null; // el objetivo que tiene la línea, como texto

  return (objetivo) => {
    marca.visible = caida.visible = Boolean(objetivo);
    if (!objetivo || String(objetivo) === dibujado) return;
    dibujado = String(objetivo);
    marca.position.set(...objetivo);
    caida.geometry.dispose();
    caida.geometry = new THREE.BufferGeometry().setFromPoints([
      new THREE.Vector3(objetivo[0], objetivo[1], 0),
      new THREE.Vector3(...objetivo),
    ]);
    caida.computeLineDistances();
  };
}
