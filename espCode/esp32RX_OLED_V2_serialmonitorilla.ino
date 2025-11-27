#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 5, 4);

float angle = 0;
int nykyinenLuku = -1;
bool dataSaapunut = false;

void setup() {
  Serial.begin(115200);                    // debug USB:lle
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17 (TX ei tarvita)

  u8g2.begin();
  u8g2.setContrast(255);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.setCursor(20, 35);
  u8g2.print("HAETAAN...");
  u8g2.sendBuffer();
}

void loop() {
  bool lukuMuuttui = false;

  while (Serial2.available()) {
    String rivi = Serial2.readStringUntil('\n');
    rivi.trim();

    if (rivi.startsWith("COUNT:")) {
      int saatu = rivi.substring(6).toInt();
      if (saatu >= 0 && saatu <= 999 && saatu != nykyinenLuku) {
        nykyinenLuku = saatu;
        lukuMuuttui = true;
        dataSaapunut = true;
        Serial.printf("Ihmisiä: %d\n", nykyinenLuku); // näkyy Serial Monitorissa
      }
    }
  }

  if (dataSaapunut) {
    static uint32_t edPiirto = 0;
    if (lukuMuuttui || millis() - edPiirto > 33) {
      u8g2.clearBuffer();

      // ISO LUKU
      u8g2.setFont(u8g2_font_ncenB24_tr);
      char buf[10];
      sprintf(buf, "%d", nykyinenLuku);
      int w = u8g2.getStrWidth(buf);
      u8g2.setCursor((128 - w) / 2, 42);
      u8g2.print(buf);

      // ANIMAATIO
      angle += 0.08;
      int x = 64 + 30 * cos(angle);
      int y = 48 + 26 * sin(angle);
      u8g2.drawDisc(64, 60, 14, U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT);
      u8g2.drawDisc(x, y, 12, U8G2_DRAW_ALL);
      u8g2.drawBox(x-12, y-12, 24, 24);
      u8g2.drawDisc(x-6, y-6, 4, U8G2_DRAW_ALL);

      u8g2.sendBuffer();
      edPiirto = millis();
    }
  }

  delay(10);
}