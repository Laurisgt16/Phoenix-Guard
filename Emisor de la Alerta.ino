#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// GPS GT-U7 por SoftwareSerial
SoftwareSerial ss(3, 4); // RX = 3, TX = 4
TinyGPSPlus gps;

// LoRa por SoftwareSerial (pines 6, 7)
SoftwareSerial lora(7, 6); // RX = 6, TX = 7

#define DHTTYPE DHT11
#define DHTPin 5
DHT dht(DHTPin, DHTTYPE);

float t, h;
int sensorFuego = 0;
unsigned long Tiempo = 0;

void setup() {
    Serial.begin(9600);  // Para monitoreo
    ss.begin(9600);      // GPS
    lora.begin(9600);    // LoRa
    dht.begin();
    pinMode(2, INPUT);

    delay(1000); // Espera inicial para LoRa
    lora.listen();
    lora.print("AT+ADDRESS=16\r\n");
    delay(200);
    lora.print("AT+NETWORKID=10\r\n");
    delay(200);
    lora.print("AT+BAND=915000000\r\n");
    delay(200);
    Serial.println("Iniciando envío de datos...");
}

void loop() {
    // --- Lectura del GPS ---
    ss.listen();  // Activar GPS
    while (ss.available()) {
        char c = ss.read();
        gps.encode(c);
    }

    // Cada 3 segundos, leer sensores y enviar
    if (millis() - Tiempo > 3000) {
        Tiempo = millis();

        float latitud = gps.location.isValid() ? gps.location.lat() : 0.0;
        float longitud = gps.location.isValid() ? gps.location.lng() : 0.0;
        int sat = gps.satellites.isValid() ? gps.satellites.value() : 0;
        String fechaUTC = gps.date.isValid() ? String(gps.date.day()) + "/" + String(gps.date.month()) + "/" + String(gps.date.year()) : "N/A";
        String horaUTC = gps.time.isValid() ? String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) : "N/A";

        h = dht.readHumidity();
        t = dht.readTemperature();
        sensorFuego = digitalRead(2);

        String datos = "Lat:" + String(latitud, 6) + ",Lon:" + String(longitud, 6) +
                       ",Sat:" + String(sat) + ",Temp:" + String(t) + "C,Hum:" + String(h) +
                       "%,Fecha:" + fechaUTC + ",Hora:" + horaUTC + ",Fuego:" + String(sensorFuego);

        Serial.println("Enviando datos:");
        Serial.println(datos);

        // --- Enviar por LoRa ---
        lora.listen(); // Activar LoRa para enviar
        String comando = "AT+SEND=16," + String(datos.length()) + "," + datos + "\r\n";
        lora.print(comando);

        // Alerta de fuego si se detecta
        if (sensorFuego == LOW) {
            delay(1000);
            String alerta = "ALERTA_FUEGO";
            String comando_alerta = "AT+SEND=16," + String(alerta.length()) + "," + alerta + "\r\n";
            lora.print(comando_alerta);
            Serial.println("¡Fuego detectado! (enviado ALERTA_FUEGO)");
        }
    }
}