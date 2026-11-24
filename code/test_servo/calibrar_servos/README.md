# Calibrador de Rango (Wide Range) y Relación Ángulo - Ancho de Pulso

Utilidad interactiva en C++ para ESP32 diseñada para probar servomotores MG90S / SG90 de a uno, encontrar su rango útil real sin forzar topes mecánicos y generar las tablas de calibración (Look-Up Table / LUT) para `configuracion.c` y `firmware.ino`.

---

## 🚀 Cómo usar

1. **Abrir o Flashear:**
   Desde terminal con `arduino-cli`:
   ```bash
   arduino-cli compile --upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 "code/test_servo/calibrar_servos"
   ```
   O abrir `calibrar_servos.ino` en el **Arduino IDE** y presionar **Subir**.

2. **Abrir Monitor Serie:**
   - Velocidad: **115200 baudios**.
   - Configuración de línea: **Nueva línea (`\n`)** o **Ambos NL & CR**.

---

## 🎮 Comandos interactivos por Monitor Serie

### Selección de Servo:
- `0` a `4`: Selecciona el servo a calibrar:
  - `0`: Base (GPIO 18)
  - `1`: Hombro (GPIO 19)
  - `2`: Codo (GPIO 21)
  - `3`: Muñeca (GPIO 22)
  - `4`: Pinza (GPIO 23)

### Ajuste de Pulso (µs):
- `+` / `-` : Ajuste fino de **10 µs**.
- `u` / `d` : Ajuste medio de **50 µs** (Up / Down).
- `]` / `[` : Ajuste rápido de **100 µs**.
- `c` : Retorna a pulso central (**1500 µs**).
- `<numero>` directo: Podés escribir cualquier pulso entre 450 y 2550 (ej: `1250`, `600`, `2200`) y presionar Enter; el servo se moverá suavemente hacia ese pulso.

### Registro de Puntos de Calibración:
- `a <grados>` (o `ang <grados>`): Registra el pulso actual con el ángulo físico medido.
  *Ejemplo:* Si pusiste el servo en posición física de 0° (medido con transportador) y el pulso actual es 540 µs, escribís `a 0` + Enter.
  Luego lo movés a 45° y escribís `a 45`. Así sucesivamente (`a 90`, `a 135`, `a 180`).
- `l` o `listar`: Muestra los puntos registrados hasta el momento para el servo activo.
- `del` o `borrar`: Borra los puntos del servo activo si te equivocaste y querés reiniciar.

### Generación automática de Código C:
- `t`: Imprime en la consola la tabla de calibración en C (`const PuntoCalibracion LUT_<Servo>`) lista para copiar y pegar.
- `T`: Imprime las tablas de **todos los servos**.

### Pruebas de movimiento y seguridad:
- `b`: **Barrido suave (Sweep):** Hace oscilar el servo de un extremo al otro para verificar la suavidad del recorrido y asegurarse de que no haya obstrucciones mecánicas. Se detiene enviando cualquier carácter.
- `w`: **Wiggle:** Hace una pequeña vibración para identificar físicamente qué motor estás moviendo.
- `m`: **Alternar modo de energía:**
  - *Modo Solo Activo (por defecto):* Solo el servo seleccionado tiene torque (`attach`). Los otros quedan libres (`detach`) para no consumir corriente ni zumbar en el banco.
  - *Modo Todos Activos:* Mantiene todos los servos energizados en su posición actual (útil si el brazo está armado y no querés que caiga por gravedad).

---

## ⚠️ Advertencia de Seguridad (Evitar quemar servos)

1. **Topes mecánicos internos:** Los servos micro tienen un tope plástico/metálico interno. Si al aumentar o disminuir el pulso el servo **deja de girar** y empieza a **emitir un zumbido agudo y a levantar temperatura**, **dejó de moverse porque chocó su tope**. Retrocedé inmediatamente 50 o 100 µs hacia el centro.
2. **Alimentación externa:** Recordá tener conectada la fuente externa de 5V a los servos, con la masa (GND) compartida con el ESP32.

