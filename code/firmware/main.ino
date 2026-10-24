// --- NUEVAS INCLUSIONES PARA WEB SERVER Y JSON ---
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h> // ¡Asegúrate de instalar esta librería!

// --- CONFIGURACIÓN DE RED ---
const char* ssid = "Personal-58C-2.4GHz";
const char* password = "9114DFC58C";
const IPAddress localIP(192, 168, 1, 184); // IP Fija sugerida para el ESP32
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);

// Objeto del Servidor en el puerto 80
WebServer server(80);

#include <ESP32Servo.h>
#include <cmath> 
#include <string.h> 
#include <stdlib.h> // Necesario para 'free' en el setup

// --- INCLUSIÓN DE MÓDULOS DE CINEMÁTICA ---
#include "numpy.h"         
#include "algebra_lineal.h" 
#include "configuracion.h" 
#include "cinematica.h"    

// ================== PINES Y CONFIGURACIÓN HARDWARE ==================

// ----------- Pines de Control -----------
const int BTN_NEXT = 13;   
const int BTN_PREV = 14;   
const int POT_PIN  = 34;   

// ----------- Configuración de Servos (5 Eslabones) -----------
const int SERVO_COUNT = 5;
const int servoPins[SERVO_COUNT]  = {18, 19, 21, 22, 23};

// CON ESTO SE PUEDE CONTROLAR LOS VALORES FISICOS
const int servoMinUs[SERVO_COUNT] = {400, 400, 400, 400, 400};
const int servoMaxUs[SERVO_COUNT] = {2600,1800, 2600,2000,2600};
// --- PULSOS DE HOME PARA CADA SERVO (FÁCILMENTE CONFIGURABLES) ---
int servoHomePulse[SERVO_COUNT] = {
    400,   // Servo 0
    1400,   // Servo 1
    1200,   // Servo 2
    960,   // Servo 3
    400    // Servo 4
};

// ================== CONSTANTES DE CONTROL ==================
const float ALPHA = 0.25f;       // suavizado exponencial ADC
const int   DEADZONE_US = 150;    
const int SYNC_TOLERANCE_US = 200;
const int   DEBOUNCE_MS = 50;   
const int STEP_SIZE_US = 20;

// ---- Tabla de calibración (us -> grados) ----
struct CalibPoint {
    int   us;
    float deg;
};

CalibPoint LUT_GENERICO[] = {
    {400,   0.0},   // Inicio
    {767,  30.0},
    {1134,  60.0},
    {1501,  90.0},  // Punto central (similar al 1500 original)
    {1868, 120.0},
    {2235, 150.0},
    {2600, 180.0}   // Final
};

const int LUT_SIZE = sizeof(LUT_GENERICO)/sizeof(LUT_GENERICO[0]);

CalibPoint LUT1[] = {
    {400,   0.0},   // Inicio
    {800,  30.0},
    {1000,  60.0},
    {1100,  90.0},  // Punto central (similar al 1500 original)
    {1200, 120.0},
    {1500, 150.0},
    {1800, 180.0}   // Final
};

const int LUT_SIZE1 = sizeof(LUT1)/sizeof(LUT1[0]);

CalibPoint LUT2[] = {
    {400,   180.0},   // Inicio
    {767,  150.0},
    {1134, 120.0},
    {1501,  90.0},  // Punto central (similar al 1500 original)
    {1868, 60.0},
    {2235, 30.0},
    {2600, 0.0}   // Final
};

const int LUT_SIZE2 = sizeof(LUT2)/sizeof(LUT2[0]);

CalibPoint LUT3[] = {
    {400,   0.0},   // Inicio
    {500,  30.0},
    {600,  60.0},
    {900, 90.0},   // Punto central: (400 + 2100) / 2 = 1250 us
    {1300, 120.0},
    {1700, 150.0},
    {2000, 180.0}   // Final ajustado a 2100 us
};

const int LUT_SIZE3 = sizeof(LUT3) / sizeof(LUT3[0]);

// ================== ESTADO GLOBAL DEL ROBOT ==================
Servo servos[SERVO_COUNT];
float filteredAdc = 0.0f;
int currentPulse[SERVO_COUNT]; 
double angulos_grados[SERVO_COUNT] = {0.0}; 

// Índice del servo actualmente controlado por el potenciómetro
int activeServo = 0; 

// Variables de estado para el Debounce
int lastRawNext = HIGH;
int lastStableNext = HIGH;
unsigned long lastDebounceNext = 0;
int lastRawPrev = HIGH;
int lastStablePrev = HIGH;
unsigned long lastDebouncePrev = 0;


// ================== UTILITARIOS ==================

float getAngleForServo(int servo_idx, int us) {
    
    // 1. Declaración de Punteros (Deben estar fuera del if/else para ser accesibles)
    const CalibPoint* local_lut;
    int local_lut_size;
    
    // 2. Selección de la LUT (Corregida la lógica)
    if (servo_idx == 2) {
        // Servo 2 usa LUT2 (Invertida)
        local_lut = LUT2;
        local_lut_size = LUT_SIZE2;
        
    } else if (servo_idx == 1) { 
        // Servo 1 usa LUT1 (Calibración específica)
        local_lut = LUT1;
        local_lut_size = LUT_SIZE1;
        
    } else if (servo_idx == 3) { 
        // Servo 1 usa LUT1 (Calibración específica)
        local_lut = LUT3;
        local_lut_size = LUT_SIZE3;
        
    }else {
        // Todos los demás (0, 3, 4) usan la LUT original
        local_lut = LUT_GENERICO;
        local_lut_size = LUT_SIZE;
    }

    // --- Lógica de Interpolación (Unificada) ---
    
    // 3. Chequeo de límites (Extrapolación)
    if (us <= local_lut[0].us) {
        return local_lut[0].deg;
    }
    if (us >= local_lut[local_lut_size - 1].us) {
        return local_lut[local_lut_size - 1].deg;
    }
    
    // 4. Búsqueda por interpolación lineal
    for (int i = 0; i < local_lut_size - 1; i++) {
        // La condición de parada puede ser: si el pulso 'us' es menor
        // que el siguiente punto (x1), entonces está en este segmento.
        if (us < local_lut[i+1].us) {
            int   x0 = local_lut[i].us;
            int   x1 = local_lut[i+1].us;
            float y0 = local_lut[i].deg;
            float y1 = local_lut[i+1].deg;
            
            // Cálculo de interpolación lineal: y = y0 + t * (y1 - y0)
            float t = (float)(us - x0) / (float)(x1 - x0);
            return y0 + t * (y1 - y0);
        }
    }
    
    return NAN; // Debería ser inalcanzable si la LUT está completa y ordenada
}

// Función de Antirrebote (Debounce)
bool fallingEdgeDebounced(int pin, int &lastRaw, int &lastStable, unsigned long &tLast) {
    int raw = digitalRead(pin);
    unsigned long now = millis();

    if (raw != lastRaw) { lastRaw = raw; tLast = now; }
    if ((now - tLast) > DEBOUNCE_MS) {
        if (raw != lastStable) {
            lastStable = raw;
            if (lastStable == LOW) return true; // flanco de bajada (pulso a GND)
        }
    }
    return false;
}

// ================== FUNCIÓN DE AYUDA DE KINEMÁTICA ==================

// NOTA: Esta función DEBE liberar la memoria de los resultados devueltos después de usarlos.
// En el handler JSON, construiremos el JSON y luego liberaremos la memoria.
bool obtener_pose_global(EsferasGlobales esferas_resultado, ColisionSet* centros_resultado) {
    
    // Ejecutar Sistema de Colisión con la pose global actual
    return calcular_configuracion_modular(
        angulos_grados, // Usa la pose actual validada
        esferas_local, 
        RADIO_ESFERA, 
        esferas_resultado, 
        centros_resultado
    );
}

// ================== HANDLER WEB ==================

int get_sphere_row_count(int eslabon_idx) {
    if (eslabon_idx == 0) return 1;
    if (eslabon_idx == 1) return 2;
    if (eslabon_idx == 2) return 3;
    if (eslabon_idx == 3) return 1;
    if (eslabon_idx == 4) return 2;
    return 0; // Error
}

void handleDataRequest() {
    // 1. Ejecutar Cinemática para obtener la pose actual
    EsferasGlobales esferas_globales_temp;
    ColisionSet centros_colisionantes;
    
    // Inicializar punteros a NULL (CRÍTICO)
    centros_colisionantes.coords = NULL;
    centros_colisionantes.count = 0;
    for(int j = 0; j < SERVO_COUNT; j++) esferas_globales_temp[j] = NULL;
    
    bool colision_detectada = obtener_pose_global(esferas_globales_temp, &centros_colisionantes);

    // 2. Crear Documento JSON (Tamaño estimado)
    // El tamaño depende del número de esferas: 5 eslabones * (2 o 3 esferas por eslabón) * 3 coordenadas
    const size_t CAPACITY = JSON_OBJECT_SIZE(4) + // 4 campos principales (angles, collision, spheres, centers)
                            JSON_ARRAY_SIZE(SERVO_COUNT) + // Array de ángulos (5 doubles)
                            SERVO_COUNT * JSON_ARRAY_SIZE(5) + // 5 eslabones
                            300; // Buffer de seguridad
                            
    DynamicJsonDocument doc(CAPACITY);
    
    // --- 3. Llenar el JSON ---
    
    // a) Ángulos
    JsonArray anglesArray = doc.createNestedArray("angles");
    for (int i = 0; i < SERVO_COUNT; i++) {
        anglesArray.add(angulos_grados[i]);
    }
    
    // b) Estado de Colisión
    doc["collision_detected"] = colision_detectada;
    doc["active_servo_index"] = activeServo;

    // c) Centros Colisionantes (si los hay)
    JsonArray collidingCentersArray = doc.createNestedArray("colliding_centers");
    if (centros_colisionantes.count > 0 && centros_colisionantes.coords != NULL) {
        // Los centros colisionantes son una lista aplanada de coordenadas (x1, y1, z1, x2, y2, z2...)
        // La lista centros_colisionantes.coords contiene (x, y, z) de cada centro
        for (int i = 0; i < centros_colisionantes.count; i++) {
            JsonArray center = collidingCentersArray.createNestedArray();
            center.add(centros_colisionantes.coords[i][0]);
            center.add(centros_colisionantes.coords[i][1]);
            center.add(centros_colisionantes.coords[i][2]);
        }
    }
    
    // d) Esferas Globales (La pose completa)
    doc["sphere_radius"] = RADIO_ESFERA;
    JsonArray spheresArray = doc.createNestedArray("spheres");
    
    for (int i = 0; i < SERVO_COUNT; i++) {
        JsonArray eslabon = spheresArray.createNestedArray();
        
        // --- Solución al Error 1: Usar helper para obtener rows_i ---
        int rows_i = get_sphere_row_count(i);
        
        if (esferas_globales_temp[i] != NULL) {
            
            // --- Solución al Error 2: Casting explícito a Vector3D* ---
            const Vector3D *esf_global_i = (const Vector3D *)esferas_globales_temp[i];
            
            for (size_t k = 0; k < rows_i; k++) {
                JsonArray sphereCenter = eslabon.createNestedArray();
                
                // Ahora la indexación es correcta: esf_global_i[k] es el Vector3D (double[3])
                sphereCenter.add(esf_global_i[k][0]); // x
                sphereCenter.add(esf_global_i[k][1]); // y
                sphereCenter.add(esf_global_i[k][2]); // z
            }
        }
    }

    // 4. Serializar JSON a String
    String response;
    serializeJson(doc, response);

    // 5. Enviar Respuesta HTTP
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", response);

    // 6. Liberar Memoria (CRÍTICO)
    if (centros_colisionantes.coords != NULL) free(centros_colisionantes.coords);
    for(int j = 0; j < SERVO_COUNT; j++) {
        if (esferas_globales_temp[j] != NULL) free((void*)esferas_globales_temp[j]);
    }
}


// ======================================================
// =========== FUNCIÓN PARA LLEVAR A POSICIÓN HOME =======
// ======================================================
void home() {
    Serial.println("\n--- Moviendo todos los servos a POSICIÓN HOME ---");

    for (int i = 0; i < SERVO_COUNT; i++) {

        int pulse = servoHomePulse[i];

        // Asegurar que cae dentro del rango del servo
        if (pulse < servoMinUs[i]) pulse = servoMinUs[i];
        if (pulse > servoMaxUs[i]) pulse = servoMaxUs[i];

        // Convertir a ángulo usando tu LUT
        float ang = getAngleForServo(i, pulse);

        if (i == 3) {
            ang -= 90.0f; // Aplica offset: 0..180 -> -90..90
        }

        if (i == 0 || i == 2) {
            ang = -1 * ang;   // Tu corrección especial
        }

        // Guardar en el estado global
        currentPulse[i] = pulse;
        angulos_grados[i] = ang;

        // Mover el servo
        servos[i].writeMicroseconds(pulse);
        Serial.println("--- POSICIÓN HOME PROCESOOOO ---\n");
        Serial.printf("Servo %d -> HomePulse=%dus | Ang=%.1f°\n", i, pulse, ang);
        delay(300);  // deja que ese servo llegue antes de pasar al siguiente

    }

    Serial.println("--- POSICIÓN HOME COMPLETADA ---\n");
}

// ================== SETUP ==================

void setup() {
    Serial.begin(115200);
    delay(400);

    // --- NUEVO: Conexión Wi-Fi ---
    // if (!WiFi.config(localIP, gateway, subnet)) {
    //   Serial.println("Falló la configuración STA");
    // }
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado.");
    Serial.print("IP del servidor: ");
    Serial.println(WiFi.localIP());

    // --- NUEVO: Inicialización del Web Server ---
    server.on("/", HTTP_GET, [](){
        server.send(200, "text/plain", "ESP32 Robot Arm Server. Endpoint /data para JSON.");
    });
    server.on("/data", HTTP_GET, handleDataRequest);
    server.begin();
    Serial.println("Servidor HTTP iniciado.");

    // Inicialización del hardware y cinemática
    analogReadResolution(12);    
    analogSetPinAttenuation(POT_PIN, ADC_11db); 
    pinMode(BTN_NEXT, INPUT_PULLUP);
    pinMode(BTN_PREV, INPUT_PULLUP);
    inicializar_matrices_offset(); 

    // Inicialización de Servos y ángulos
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].setPeriodHertz(50); 
        servos[i].attach(servoPins[i], servoMinUs[i], servoMaxUs[i]);
        currentPulse[i] = servoMinUs[i];
        angulos_grados[i] = getAngleForServo(i, currentPulse[i]);
    }
    
    // Inicialización del ADC para el filtro
    int raw = analogRead(POT_PIN);
    filteredAdc = (float)raw;

    Serial.println("--- Robot Modular: Control por Potenciómetro + Anticolisión ---");
    
    // ---------------------------------------------------------------------------------
    // --- LÓGICA DE INICIALIZACIÓN DE POSE BASE (Migrada de simulacion.py) ---
    // ---------------------------------------------------------------------------------
    
    // Definir N_eslabones (simula len(esferas_local))
    const int N_eslabones = SERVO_COUNT; 

    Serial.println("--- FASE 0: Calculando e Inicializando el Gráfico Base ---");
    
    // --> Pose de inicio (ángulos fijos).
    double angulos_base[SERVO_COUNT] = {
        ANGULO_FIJO_ESLABON_0, ANGULO_FIJO_ESLABON_1,
        ANGULO_FIJO_ESLABON_2, ANGULO_FIJO_ESLABON_3, ANGULO_FIJO_ESLABON_4
    };
    
    // --> Se lleva el brazo a la posición base, y se verifica si ya en la posicion base hay colisión.
    EsferasGlobales esferas_base;
    ColisionSet centros_colisionantes;
    
    // Inicializar punteros a NULL antes de la llamada (CRÍTICO para evitar el error 'free')
    centros_colisionantes.coords = NULL;
    centros_colisionantes.count = 0;
    for(int j = 0; j < N_eslabones; j++) esferas_base[j] = NULL;

    home();

    // =========================================================================
    // ** MODIFICACIÓN CLAVE PARA SINCRONIZAR EL POTENCIÓMETRO **
    // Después de home(), el pulso del servo activo está en currentPulse[activeServo].
    // Necesitamos que filteredAdc (y por lo tanto desiredPulse) esté cerca de ese valor.
    // =========================================================================
    int home_pulse_active = currentPulse[activeServo]; // Obtener el pulso Home del servo 0 (activo inicial)
    int min_us_active = servoMinUs[activeServo];
    int max_us_active = servoMaxUs[activeServo];

    // Mapeo inverso: Pulso (min_us -> max_us) -> ADC (0 -> 4095)
    // Esto establece el filtro a un valor que corresponde al pulso actual del servo.
    int adc_home = map(home_pulse_active, min_us_active, max_us_active, 0, 4095);
    filteredAdc = (float)adc_home;
    Serial.printf("Potenciómetro sincronizado con Home del Eslabón %d (GPIO%d): ADC inicial = %d\n", activeServo, servoPins[activeServo], adc_home);
    // =========================================================================

    Serial.println("Usa BTN_NEXT/PREV para cambiar de eslabón.");
    Serial.printf("Servo activo inicial -> %d (GPIO%d)\n", activeServo, servoPins[activeServo]);
}

// ================== LOOP ==================

void loop() {
    
    // NUEVO: Manejar las peticiones HTTP (debe ser lo primero)
    server.handleClient();

    // 1. Leer Botones (Navegación de servo activo con Antirrebote)
    if (fallingEdgeDebounced(BTN_NEXT, lastRawNext, lastStableNext, lastDebounceNext)) {
        activeServo = (activeServo + 1) % SERVO_COUNT;
        Serial.printf("\n--- Servo activo -> Eslabón %d (GPIO%d) ---\n", activeServo, servoPins[activeServo]);
    }
    if (fallingEdgeDebounced(BTN_PREV, lastRawPrev, lastStablePrev, lastDebouncePrev)) {
        activeServo = (activeServo - 1 + SERVO_COUNT) % SERVO_COUNT;
        Serial.printf("\n--- Servo activo -> Eslabón %d (GPIO%d) ---\n", activeServo, servoPins[activeServo]);
    }

    // 2. Leer y Filtrar Potenciómetro
    int raw = analogRead(POT_PIN);
    filteredAdc = ALPHA * raw + (1.0f - ALPHA) * filteredAdc;

    // 3. Mapear ADC -> µs (Ancho de pulso deseado para el servo ACTUAL)
    int i = activeServo;
    int min_us = servoMinUs[i];
    int max_us = servoMaxUs[i];

    int desiredPulse = map((int)filteredAdc, 0, 4095, min_us, max_us);

    int check = abs(desiredPulse - currentPulse[i]);

    // CHEQUEO DE SINCRONIZACIÓN BASADA EN PULSOS (µs)
    // ----------------------------------------------------
    // Compara el pulso deseado (lectura del POT) con el último pulso actual (servo)
    if (check > SYNC_TOLERANCE_US) {
        
        Serial.printf("Eslabón %d: DESINCRONIZADO! Mover POT a %dus\n", i, currentPulse[i]);
        
        return; 
    }

    // 4. Chequear Zona Muerta
    if (check >= DEADZONE_US) {
        // 5. Mapear Pulso -> Ángulo Propuesto
        float angulo_propuesto = getAngleForServo(i, desiredPulse);

        if (i == 3) {
            angulo_propuesto -= 90.0f; // Aplica offset: 0..180 -> -90..90
        }

        if (i == 0 || i == 2){
            angulo_propuesto = -1*angulo_propuesto;
        }

        Serial.print("Eslabón "); Serial.print(i);
        Serial.print(": F_ADC="); Serial.print((int)filteredAdc);
        Serial.print(" | us="); Serial.print(desiredPulse);
        Serial.print(" | Angulo_Prop="); Serial.print(angulo_propuesto, 1);
        Serial.print(" deg | ");
        
        // 6. Crear Configuración Propuesta (Array de 5 ángulos)
        double angulos_propuestos[SERVO_COUNT];
        memcpy(angulos_propuestos, angulos_grados, sizeof(angulos_grados)); 
        angulos_propuestos[i] = angulo_propuesto; // Aplica el cambio a la junta 'i'

        // 7. Ejecutar Sistema de Colisión
        EsferasGlobales esferas_globales_result; 
        ColisionSet centros_colisionantes;
        
        // Inicializar punteros a NULL antes de la llamada (CRÍTICO)
        centros_colisionantes.coords = NULL;
        centros_colisionantes.count = 0;
        for(int j = 0; j < SERVO_COUNT; j++) esferas_globales_result[j] = NULL;
        
        bool colision_detectada = calcular_configuracion_modular(
            angulos_propuestos, 
            esferas_local, 
            RADIO_ESFERA, 
            esferas_globales_result, 
            &centros_colisionantes
        );

        // 8. ACTUALIZACIÓN DEL ESTADO SIEMPRE (SE ENVÍA AL FRONTEND)
        angulos_grados[i] = angulo_propuesto;  // SIEMPRE actualizar, haya o no colisión

        // 8. ACTUACIÓN (Si NO hay colisión, mover el servo)
        if (!colision_detectada) {
            // MOVER SERVO
            servos[i].writeMicroseconds(desiredPulse);
            currentPulse[i] = desiredPulse;
            // Lógica OK
            angulos_grados[i] = angulo_propuesto; // Actualiza el estado global
            Serial.print("Eslabón "); Serial.print(i);
            Serial.print(": OK. Ang: ");
            Serial.print(angulo_propuesto, 1);
            Serial.println(" deg");
            
        } else {
            // COLISIÓN DETECTADA: NO mover el servo, advertir.
            Serial.print("Eslabón "); Serial.print(i);
            Serial.print("!! COLISIÓN PREVENIDA: Angulo ");
            Serial.print(angulo_propuesto, 1);
            Serial.println("° resulta en choque.");
        }

        // 9. ACTUALIZACIÓN CRÍTICA DEL ESTADO:
        // El desiredPulse pasó la deadzone, por lo tanto, es el nuevo pulso *actual*
        // para el chequeo de la próxima Deadzone, sin importar si el servo se movió.
        currentPulse[i] = desiredPulse; // <--- MOVER AQUÍ

        // ... (Tu código de log y liberación de memoria) ...
        Serial.print("TERMINE CHEQUEO");

        // 9. Liberar memoria (Crítico en ESP32) 
        if (centros_colisionantes.coords != NULL) free(centros_colisionantes.coords);
        for(int j = 0; j < SERVO_COUNT; j++) {
            if (esferas_globales_result[j] != NULL) free((void*)esferas_globales_result[j]);
        }
    }
}