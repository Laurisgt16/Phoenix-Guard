#include <Wire.h>
#include <U8g2lib.h>
#include <SoftwareSerial.h>

// Configuración de la pantalla OLED
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Configuración del módulo LoRa
SoftwareSerial lora(6, 7); // RX en 6, TX en 7

const int buzzer = 2; // Pin del buzzer

// Variables para el temporizador de actualización de la pantalla OLED y cambio de vista
unsigned long previousMillisOLED = 0; // Renombrado para claridad
const long intervalPantalla = 1000;   // Refresco pantalla (1 segundo)

unsigned long lastSwitchMillis = 0;
const long intervalSwitch = 8000; // Cambia vista cada 8 segundos (como se pidió inicialmente)

// Variables para mostrar datos organizados
String lat_data = "", lon_data = "", sat_data = "", temp_data = "", hum_data = "", fecha_data = "", hora_data = "", fuego_status = "";

// Estado
bool pantallaPrincipal = true; // true = principales, false = secundarios
bool fireAlertReceived = false; // Flag para manejar la alerta de fuego por mensaje directo

// Prototipo
void actualizarPantalla();
String extraerCampo(const String& data, const String& prefijo, const String& delimitador);

void setup() {
    Serial.begin(9600);
    u8g2.begin();
    lora.begin(9600); 

    pinMode(buzzer, OUTPUT);

    // Inicializar variables de datos
    lat_data = "N/A";
    lon_data = "N/A";
    sat_data = "N/A";
    temp_data = "N/A";
    hum_data = "N/A";
    fecha_data = "N/A";
    hora_data = "N/A";
    fuego_status = "N/A"; // Default to N/A for fire status

    digitalWrite(buzzer, HIGH); // Assuming HIGH means OFF for your buzzer
}

void loop() {
    // Leer datos del LoRa línea por línea
    while (lora.available()) {
        String recibido = lora.readStringUntil('\n');
        recibido.trim(); // Elimina espacios y saltos de línea

        if (recibido.length() > 0) {
            Serial.println("LoRa Raw: " + recibido); // Para depuración

            // --- Filtrado y procesamiento de mensajes ---
            // Primero, manejar la respuesta del módulo LoRa (+RCV=...)
            int rcvIndex = recibido.indexOf("+RCV=");
            if (rcvIndex != -1) {
                // Formato esperado: +RCV=<addr>,<len>,<data>
                int firstComma = recibido.indexOf(',', rcvIndex);
                int secondComma = recibido.indexOf(',', firstComma + 1);
                if (secondComma != -1 && secondComma + 1 < recibido.length()) {
                    recibido = recibido.substring(secondComma + 1);
                    recibido.trim();
                    Serial.println("Payload: " + recibido);
                } else {
                    Serial.println("Malformed +RCV message: " + recibido);
                    continue; // Skip processing if not a valid +RCV payload
                }
            } else if (recibido.startsWith("OK") || recibido.startsWith("+ERR")) {
                Serial.println("LoRa module response (Ignored): " + recibido);
                continue; // Ignorar respuestas de comando LoRa
            }

            // Ahora, procesar el payload recibido
            if (recibido.startsWith("ALERTA_FUEGO")) {
                Serial.println("!!! ALERTA_FUEGO RECIBIDA !!!");
                fireAlertReceived = true;
                fuego_status = "0"; // For display purposes, treat as fire detected
            } else if (recibido.startsWith("Lat:")) {
                // Es un paquete de datos normal
                fireAlertReceived = false; // Reset flag if data is received
                Serial.println("Processing data packet: " + recibido);

                lat_data = extraerCampo(recibido, "Lat:", ",");
                lon_data = extraerCampo(recibido, "Lon:", ",");
                sat_data = extraerCampo(recibido, "Sat:", ",");
                temp_data = extraerCampo(recibido, "Temp:", "C"); // Delimiter is 'C'
                hum_data = extraerCampo(recibido, "Hum:", "%");   // Delimiter is '%'
                fecha_data = extraerCampo(recibido, "Fecha:", ",");
                hora_data = extraerCampo(recibido, "Hora:", ",");
                fuego_status = extraerCampo(recibido, "Fuego:", ""); // No delimiter for the last field
            } else {
                Serial.println("Unrecognized data format: " + recibido);
            }
        }
    }

    // --- Lógica de buzzer y display (0=fuego, 1=no fuego) ---
    // El buzzer debe sonar si fuego_status es "0" o si fireAlertReceived es true
    if (fuego_status == "0" || fireAlertReceived) {
        // Buzzer titilando (4Hz = 125ms ON, 125ms OFF)
        static unsigned long lastBuzzerToggle = 0;
        if (millis() - lastBuzzerToggle >= 125) {
            digitalWrite(buzzer, !digitalRead(buzzer)); // Invert current state
            lastBuzzerToggle = millis();
        }
    } else {
        digitalWrite(buzzer, LOW); // Buzzer apagado (asumiendo HIGH es apagado)
    }

    // --- Lógica de cambio de vista ---
    // Si hay fuego (fuego_status == "0" o fireAlertReceived), siempre en vista principal
    if (fuego_status == "0" || fireAlertReceived) {
        pantallaPrincipal = true;
        lastSwitchMillis = millis(); // Reset timer to stay on principal
    } else {
        // No hay fuego, alterna vistas
        if (millis() - lastSwitchMillis >= intervalSwitch) {
            pantallaPrincipal = !pantallaPrincipal;
            lastSwitchMillis = millis();
        }
    }

    actualizarPantalla(); // Siempre actualizar la pantalla
}

// Función auxiliar para extraer un campo entre un prefijo y un delimitador
String extraerCampo(const String& data, const String& prefijo, const String& delimitador) {
    int start = data.indexOf(prefijo);
    if (start == -1) return "N/A"; // Return N/A if prefix not found
    start += prefijo.length();

    int end;
    if (delimitador.length() > 0) {
        end = data.indexOf(delimitador, start);
    } else {
        // If no delimiter, read to the end of the string, but check for other fields
        // This is tricky for the last field. Let's find the next "key" instead.
        int nextKeyStart = data.length(); // Assume it's the end of the string initially
        if (prefijo == "Fuego:") { // Special handling for "Fuego:"
            // No next key to search for, so it's truly the end
        } else {
            // Find the next possible key to act as a pseudo-delimiter
            int possibleNext = data.indexOf("Lat:", start);
            if (possibleNext != -1 && possibleNext < nextKeyStart) nextKeyStart = possibleNext;
            possibleNext = data.indexOf("Lon:", start);
            if (possibleNext != -1 && possibleNext < nextKeyStart) nextKeyStart = possibleNext;
            possibleNext = data.indexOf("Sat:", start);
            if (possibleNext != -1 && possibleNext < nextKeyStart) nextKeyStart = possibleNext;
            possibleNext = data.indexOf("Temp:", start);
            if (possibleNext != -1 && possibleNext < nextKeyStart) nextKeyStart = possibleNext;
            possibleNext = data.indexOf("Hum:", start);
            if (possibleNext != -1 && possibleNext < nextKeyStart) nextKeyStart = possibleNext;
            possibleNext = data.indexOf("Fecha:", start);
            if (possibleNext != -1 && possibleNext < nextKeyStart) nextKeyStart = possibleNext;
            possibleNext = data.indexOf("Hora:", start);
            if (possibleNext != -1 && possibleNext < nextKeyStart) nextKeyStart = possibleNext;
            possibleNext = data.indexOf("Fuego:", start);
            if (possibleNext != -1 && possibleNext < nextKeyStart) nextKeyStart = possibleNext;
        }
        end = nextKeyStart; // Use the found next key or end of string as delimiter
    }

    if (end == -1 || end < start) { // If delimiter not found or invalid range
        end = data.length(); // Read till the end of the string
    }

    String extracted = data.substring(start, end);
    extracted.trim();
    // Special handling for the last field, if it happens to be followed by another key without a comma
    if (prefijo == "Fuego:" && extracted.indexOf(',') != -1) {
        extracted = extracted.substring(0, extracted.indexOf(','));
    }
    return extracted;
}


void actualizarPantalla() {
    unsigned long currentMillis = millis();
    // Only refresh the OLED at the specified interval
    if (currentMillis - previousMillisOLED >= intervalPantalla) {
        previousMillisOLED = currentMillis;

        u8g2.firstPage();
        do {
            u8g2.setFont(u8g2_font_5x8_tr); // Smallest font

            if (fuego_status == "0" || fireAlertReceived) { // Fire detected
                u8g2.setFont(u8g2_font_unifont_t_symbols); // Larger for alert
                u8g2.drawStr(0, 30, "!!! ALERTA DE FUEGO !!!");

            } else if (pantallaPrincipal) { // Main data view
                u8g2.drawStr(0, 10, "Lat:"); u8g2.drawStr(35, 10, lat_data.c_str());
                u8g2.drawStr(0, 20, "Lon:"); u8g2.drawStr(35, 20, lon_data.c_str());
                u8g2.drawStr(0, 30, "Temp:"); u8g2.drawStr(35, 30, temp_data.c_str());
                u8g2.drawStr(0, 40, "Hum:"); u8g2.drawStr(35, 40, hum_data.c_str());

            } else { // Secondary data view (date/time, etc.)
                u8g2.drawStr(0, 10, "Sat:"); u8g2.drawStr(35, 10, sat_data.c_str());
                u8g2.drawStr(0, 20, "Fecha:"); u8g2.drawStr(35, 20, fecha_data.c_str());
                u8g2.drawStr(0, 30, "Hora:"); u8g2.drawStr(35, 30, hora_data.c_str());
                u8g2.drawStr(0, 40, "Fuego: No"); // Redundant but consistent
                // You can add other data here if you wish, or make the font larger
            }
        } while (u8g2.nextPage());
    }
}