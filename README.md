# 2026-j1-brazo-robotico 

**Taller de Proyecto II 2026, Grupo J1**

Continuación del desarrollo de un brazo robótico de 5 grados de libertad controlado por un ESP32. Los potenciómetros y pulsadores de la versión anterior se reemplazan por una página web, el gemelo digital, desde la que se elige el punto al que tiene que llegar la pinza. El ESP32 calcula los ángulos con cinemática inversa y, antes de mover los servos, revisa con un modelo de esferas que el brazo no choque.

## Equipo

| Integrantes |
|---|
| Armando Ezequiel Tarifa |
| Emma Guitarra Woida |
| Ana Justina Terruzzi |

Docente responsable: Julián Delekta.

## Componentes

- El brazo tiene 5 grados de libertad (base, hombro, codo, muñeca y pinza), cada uno con un servo MG90S, y lo controla un ESP32. Las medidas salen de los STL de las piezas y de la hoja de datos del MG90S, y los pulsos de los servos, de tablas medidas en el brazo. La simulación de Python y los informes usan los mismos valores que el firmware.
- Los ángulos se calculan con FABRIK, adaptado a un brazo con base giratoria y dos bisagras. Los límites de los servos se aplican en cada paso, con una regla para que el codo no quede doblado al revés. El usuario solo indica el punto, y la muñeca y la pinza las elige una búsqueda.
- Antes de mover los servos se revisa que el brazo no choque consigo mismo ni con la mesa. Para eso se usan 17 esferas de 1.5 cm a lo largo de los eslabones, un tope para el codo y las esquinas de la pala, y se revisan la pose final y cada paso del camino.
- El movimiento avanza un paso cada 20 ms, con la velocidad limitada a 60°/s en cada articulación y a 10 cm/s en cada punto del brazo, y con rampas de 0.3 s para arrancar y frenar.
- El ESP32 tiene un servidor HTTP con cuatro pedidos: `GET /data` devuelve la telemetría en JSON (ángulos, pulsos, esferas y el resultado de la última búsqueda), `POST /set` recibe el objetivo, `POST /detener` frena el brazo y `POST /home` lo lleva a la posición original.
- El gemelo digital es una página hecha con Three.js. Pide la telemetría cada 100 ms y dibuja el brazo con sus piezas reales. Si un objetivo se rechaza, muestra el motivo y dibuja en gris el mejor intento.

## Cómo se usa con el brazo

Como en 2025, el ESP32 levanta el servidor web y la página se abre desde la computadora. No hace falta Python: `code/simulacion/` es solo para probar sin el brazo.

1. Se montan los horns de los servos con `code/test_servo/centrar_servos/centrar_servos.ino`. Con la tecla `c`, cada servo va a 1500 µs, y su horn se monta con el brazo en L: el brazo vertical; el antebrazo, la muñeca y la pinza horizontales y en línea, y la base a 90° del frente. Después, la tecla `h` tiene que llevar el brazo al home.
2. Se abre `code/firmware/firmware.ino` en el Arduino IDE, con la placa *ESP32 Dev Module* y la librería ESP32Servo instalada. Se completan el nombre y la contraseña de la red WiFi y se sube a la placa. La pinza baja hasta 450 µs y la ESP32Servo del gestor de librerías no manda pulsos de menos de 500 µs: si en el home la pinza queda corta, hay que bajar `MIN_PULSE_WIDTH` a 450 en `ESP32Servo.h`.
3. En el monitor serie, a 115200 baudios, la placa muestra su IP al arrancar.
4. Esa IP se escribe en la constante `ESP32` de `code/web/conexion.js` y se abre `code/index.html` con doble clic. La computadora tiene que estar en la misma red WiFi que la placa y tener internet, porque la página baja three.js.

## Estructura del repositorio

```
.
├── README.md
├── code/
│   ├── firmware/          
│   ├── test_servo/        
│   ├── simulacion/        
│   │   ├── tests/         
│   │   ├── experimentos/  
│   │   ├── resultados/    
│   ├── index.html         
│   ├── web/               
│   └── mapeoServos.c      
└── docs/
    ├── entregas/          
    └── investigaciones/   
```

## Bitácora

Los avances, los experimentos y las decisiones se van registrando en la [Bitácora J1](https://docs.google.com/document/d/10rTo3Ey651mk7nShooTuPY7t9uNhYceF5nL0UofQub0/edit?usp=sharing).
