#include <U8g2lib.h>
#include <Wire.h>

// Inicializa el display SH1106 en I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
    Serial.begin(9600);

    // Inicializa el display
    display.begin();
    display.clearBuffer(); // Limpia el buffer interno
    display.sendBuffer();  // Muestra un contenido vacío (pantalla limpia)

    // Mensaje inicial en el monitor serial
    Serial.println(F("Pantalla SH1106 inicializada."));
}

void loop() {
    // Limpia el buffer de la pantalla
    display.clearBuffer();

    // Configura el tamaño y posición del texto
    display.setFont(u8g2_font_ncenB08_tr); // Fuente de texto
    display.setCursor(0, 10);
    display.print("Hija_de");

    // Ajusta el texto debajo
    display.setCursor(0, 30);
    display.print("Puta");

    // Envía el contenido del buffer a la pantalla
    display.sendBuffer();

    // Espera 2 segundos antes de volver a actualizar
    delay(2000);
}