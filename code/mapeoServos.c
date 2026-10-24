#include <ESP32Servo.h>

// ================== Pines ==================

const int POT_PIN   = 34;   // ADC1_CH6

const int SERVO_PIN = 4;    // PWM servo

// ================== Rango de pulsos ==================

const int SERVO_MIN_US = 500;   // ajustá a tu servo real (p.ej. 600..2400)

const int SERVO_MAX_US = 2500;

// ================== Filtros y zona muerta ==================

const float ALPHA = 0.25f;       // suavizado exponencial ADC

const int   DEADZONE_US = 10;    // cambio mínimo de pulso (µs) para actualizar

// ================== Config calibración ==================

// Modo barrido para capturar datos rápidamente desde Serial (false = control por potenciómetro)

#define CALIB_SWEEP false

const int SWEEP_STEP_US   = 20;   // paso de barrido en µs

const int SWEEP_HOLD_MS   = 300;  // tiempo para que el servo se estabilice y puedas leer/medir

// ---- Tabla de calibración (us -> grados) ----

// Reemplazá estos puntos por tus mediciones reales.

// Deben estar ORDENADOS por 'us' de menor a mayor.

struct CalibPoint {

int   us;

float deg;

};

// Ejemplo genérico (lineal aprox). EDITAR con tu curva real:

CalibPoint LUT[] = {

    {1000,   0.0},

    {1250,  45.0},

    {1500,  90.0},

    {1750, 135.0},

    {2000, 180.0}

};

const int LUT_SIZE = sizeof(LUT)/sizeof(LUT[0]);

// ================== Estado ==================

Servo servo;

float filteredAdc = 0.0f;

int   lastPulse   = -99999;

// ================== Utilitarios ==================

// Interpolación lineal por tramos sobre la LUT (us -> grados)

float angleFromPulseUS(int us) {

    if (us <= LUT[0].us)   return LUT[0].deg;

    if (us >= LUT[LUT_SIZE-1].us) return LUT[LUT_SIZE-1].deg;

    for (int i = 0; i < LUT_SIZE-1; i++) {

        int   x0 = LUT[i].us;

        int   x1 = LUT[i+1].us;

        float y0 = LUT[i].deg;

        float y1 = LUT[i+1].deg;

        if (us >= x0 && us <= x1) {

        float t = float(us - x0) / float(x1 - x0);

        return y0 + t * (y1 - y0);

        }

    }

    // No debería llegar acá si la LUT está ordenada

    return NAN;

}

// Opcional: imprimir la LUT (para verificar)

void printLUT() {

    Serial.println("Tabla de calibracion (us -> deg):");

    for (int i = 0; i < LUT_SIZE; i++) {

        Serial.print("  ");

        Serial.print(LUT[i].us);

        Serial.print(" us -> ");

        Serial.print(LUT[i].deg);

        Serial.println(" deg");

    }

}

// ================== Setup ==================

void setup() {

    Serial.begin(115200);

    delay(400);

    analogReadResolution(12);                      // 0..4095

    analogSetPinAttenuation(POT_PIN, ADC_11db);   // ~0..3.3V

    servo.setPeriodHertz(50); // servos a 50 Hz

    servo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);

    int raw = analogRead(POT_PIN);

    filteredAdc = (float)raw;

    Serial.println("--- Servo controlado por pulso (us) + mapeo a angulo via LUT ---");

    printLUT();

    #if CALIB_SWEEP

        Serial.println("Modo barrido activo (CSV: us,angulo_est)");

    #endif

}

// ================== Loop ==================

void loop() {

    #if CALIB_SWEEP

    // Barrido de pulsos para calibrar/medir

        for (int us = SERVO_MIN_US; us <= SERVO_MAX_US; us += SWEEP_STEP_US) {

            servo.writeMicroseconds(us);

            float angEst = angleFromPulseUS(us);

            // CSV fácil de copiar: us,angulo_est

            Serial.print(us);

            Serial.print(",");

            Serial.println(angEst);

            delay(SWEEP_HOLD_MS); // tiempo para estabilizar y medir con transportador

        }

        // Barrido inverso (opcional)

        for (int us = SERVO_MAX_US; us >= SERVO_MIN_US; us -= SWEEP_STEP_US) {

            servo.writeMicroseconds(us);

            float angEst = angleFromPulseUS(us);

            Serial.print(us);

            Serial.print(",");

            Serial.println(angEst);

            delay(SWEEP_HOLD_MS);

        }

    #else

    // Control por potenciómetro

    int raw = analogRead(POT_PIN);

    filteredAdc = ALPHA * raw + (1.0f - ALPHA) * filteredAdc;

    // Mapear ADC -> µs

    int pulseWidth = map((int)filteredAdc, 0, 4095, SERVO_MIN_US, SERVO_MAX_US);

    if (abs(pulseWidth - lastPulse) >= DEADZONE_US) {

        servo.writeMicroseconds(pulseWidth);

        lastPulse = pulseWidth;


        float angEst = angleFromPulseUS(pulseWidth);



        // Log legible

        Serial.print("ADC=");

        Serial.print((int)filteredAdc);

        Serial.print(" | us=");

        Serial.print(pulseWidth);

        Serial.print(" | ang_est=");

        Serial.print(angEst, 1);

        Serial.println(" deg");



        // Log CSV (para copiar a Excel): adc,us,ang_est

        // Serial.print((int)filteredAdc); Serial.print(",");

        // Serial.print(pulseWidth);       Serial.print(",");

        // Serial.println(angEst, 3);

    }

    delay(10);

    #endif

}