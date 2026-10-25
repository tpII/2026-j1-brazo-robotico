# 2026-j1-brazo-robotico 

**Taller de Proyecto II 2026 — Grupo J1**

Continuación del desarrollo de un brazo robótico manipulador de 5 grados de libertad controlado por un microcontrolador ESP32. El sistema elimina completamente el hardware analógico previo (eliminación de potenciómetros y pulsadores físicos) hacia una interfaz web que actúa como Gemelo Digital, integrando cinemática inversa iterativa con detección geométrica de autocolisiones por esferas para la planificación segura de movimientos antes de activar los servomotores.

---

## Equipo

| Integrantes |
|---|
| Armando Ezequiel Tarifa |
| Emma Guitarra Woida |
| Ana Justina Terruzzi |

Docente responsable: Julián Delekta.

---

### Componentes

- **Dispositivo (manipulador):** brazo mecánico articulado de **5 grados de libertad** accionado por servomotores SG90 (Base, Hombro, Codo, Muñeca, Pinza), controlado por un microcontrolador ESP32.
- **Cinemática y Seguridad preventiva:** cálculo de cinemática inversa iterativa para resolver los ángulos requeridos a partir de una coordenada objetivo en el espacio. Antes de mover los servos, el algoritmo evalúa la posición espacial mediante un **modelo geométrico de esferas delimitadoras**, verificando una matriz de autocolisiones 5×5 y la interferencia contra el plano base.
- **Suavizado de movimiento:** transición de articulaciones interpolada por incrementos angulares en cada ciclo de control para evitar movimientos bruscos.
- **Comunicación y Telemetría:** servidor HTTP embebido en el ESP32 que expone endpoints REST (`/data` y `/set`) intercambiando objetos JSON con el estado articular, coordenadas de esferas y banderas de seguridad.
- **Gemelo Digital (Frontend):** interfaz web interactiva que consume la telemetría periódicamente y renderiza en tiempo real la postura del robot en un entorno 3D mediante Three.js, visualizando tanto la pose real como las alertas visuales si una trayectoria es rechazada.


---

## Cómo se usa con el brazo

Como en 2025, el ESP32 levanta el servidor web y la página se abre desde la computadora. No hace falta Python: `code/simulacion/` es solo para probar sin el brazo.

1. Se abre `code/firmware/firmware.ino` en el Arduino IDE, con la placa *ESP32 Dev Module* y las librerías ESP32Servo y ArduinoJson instaladas. Se completan el nombre y la contraseña de la red WiFi y se sube a la placa.
2. En el monitor serie, a 115200 baudios, la placa muestra su IP al arrancar.
3. Esa IP se escribe en la constante `ESP32` de `code/web/conexion.js` y se abre `code/index.html` con doble clic. La computadora tiene que estar en la misma red WiFi que la placa y tener internet, porque la página baja three.js.

---

## Estructura del repositorio

```
.
├── README.md
├── code/
│   ├── firmware/          
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

---

## Bitácora

El registro cronológico de avances, experimentos y decisiones tomadas se documenta continuamente en:

**[Bitácora J1](https://docs.google.com/document/d/10rTo3Ey651mk7nShooTuPY7t9uNhYceF5nL0UofQub0/edit?usp=sharing)**