#include <displaylib.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Wire.h>

// Pantalla OLED SH1106
display_SH1106_128X64_NONAME_1_HW_I2C display(display_R0, U8X8_PIN_NONE);

// Variables globales esenciales
TinyGPS gps;
SoftwareSerial ss(4, 3); // pin 3(tx) y 4(rx)

// Constantes de texto en PROGMEM para ahorrar RAM
const char text_lat[] PROGMEM = "Lat:";
const char text_lon[] PROGMEM = "Lon:";
const char text_sat[] PROGMEM = "Sat:";
const char text_fecha[] PROGMEM = "Fecha:";
const char text_hora[] PROGMEM = "Hora:";

void setup()
{
  Serial.begin(9600);
  ss.begin(9600);

  // Inicialización del display OLED
  display.begin();
  display.setFont(display_font_ncenB08_tr);
}

void loop()
{
  // Variables locales para reducir uso de RAM global
  bool newData = false;
  float latitud = 0.0, longitud = 0.0;
  int sat = 0;
  uint16_t year = 0;
  uint8_t month = 0, day = 0, hour = 0, minute = 0, second = 0;

  // Durante 800ms analizamos los datos del GPS
  for (unsigned long start = millis(); millis() - start < 800;)
  {
    while (ss.available())while (ss.available() > 0) {
    gps.encode(ss.read());
  }
    {
      char c = ss.read();
      if (gps.encode(c)) // ¿Ha entrado una nueva sentencia válida?
        newData = true;
    }
  }

  if (newData)
  {
    // Obtener latitud, longitud y número de satélites disponibles
    gps.f_get_position(&latitud, &longitud);
    sat = gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites();

    // Obtener fecha y hora
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second);

    // Mostrar datos en el monitor serie
    Serial.print(F("LAT="));
    Serial.print(latitud == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : latitud, 6);
    Serial.print(F(" LON="));
    Serial.print(longitud == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : longitud, 6);
    Serial.print(F(" SAT="));
    Serial.println(sat);
    Serial.print(F("Fecha: ")); Serial.print(day); Serial.print(F("/"));
    Serial.print(month); Serial.print(F("/")); Serial.print(year);
    Serial.print(F(" Hora: ")); Serial.print(hour); Serial.print(F(":"));
    Serial.print(minute); Serial.print(F(":")); Serial.println(second);
  }

  // Actualizar el contenido del display
  display.clearBuffer(); // Borra el buffer interno
  draw(latitud, longitud, sat, year, month, day, hour, minute, second); // Pasar variables como parámetros
  display.sendBuffer();  // Envía el contenido del buffer al display
}

void draw(float lat, float lon, int sat, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
  // Imprimir en pantalla latitud, longitud y número de satélites disponibles
  display.setFont(display_font_ncenB08_tr); // Selección de fuente

  display.setCursor(0, 10);
  display.print(reinterpret_cast<const __FlashStringHelper *>(text_lat));
  display.print(lat, 6);

  display.setCursor(0, 20);
  display.print(reinterpret_cast<const __FlashStringHelper *>(text_lon));
  display.print(lon, 6);

  display.setCursor(0, 30);
  display.print(reinterpret_cast<const __FlashStringHelper *>(text_sat));
  display.print(sat);

  // Fecha y hora
  display.setCursor(0, 40);
  display.print(reinterpret_cast<const __FlashStringHelper *>(text_fecha));
  display.print(day);
  display.print(F("/"));
  display.print(month);
  display.print(F("/"));
  display.print(year);

  display.setCursor(0, 50);
  display.print(reinterpret_cast<const __FlashStringHelper *>(text_hora));
  display.print(hour);
  display.print(F(":"));
  display.print(minute);
  display.print(F(":"));
  display.print(second);
}