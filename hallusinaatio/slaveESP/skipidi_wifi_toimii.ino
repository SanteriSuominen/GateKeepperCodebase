/*
 * ESP32 OLED NÄYTTÖYKSIKKÖ - FIX: 9600 BAUD
 * Kytkentä:
 * - OLED SDA -> GPIO 4
 * - OLED SCL -> GPIO 5
 * - RX -> GPIO 16 (Master TX)
 * - GND -> YHTEINEN MAA!
 */

#include <U8g2lib.h>
#include <WiFi.h>     // Vaaditaan Wi-Fi-toimintoihin
#include <WebServer.h> // Vaaditaan Web-palvelimeen

// U8G2 alustus, kuten aiemmin
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 5, 4);

float angle = 0;
int nykyinenLuku = -1;
bool dataSaapunut = false;

// --- WEB-PALVELIN ASETUKSET ---
const char* ssid = "ESP32_Laskuri";      // Wi-Fi-verkon nimi (SSID)
const char* password = "salasana123";  // Wi-Fi-verkon salasana (väh. 8 merkkiä)

// Määritä WebServer käyttämään porttia 80
WebServer server(80);

// Funktio, joka luo HTML-sivun
void handleRoot() {
  // LISÄTTIIN: <meta charset='UTF-8'> varmistaa ääkkösten oikean näyttämisen selaimessa.
  String html = "<html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='1'>"; 
  html += "<title>Ihmislaskuri</title>";
  html += "<style>body{font-family: Arial, Helvetica, sans-serif; text-align: center; margin-top: 50px;} h1{font-size: 80px; color: #0077b6;}</style>";
  html += "</head><body>";
  html += "<h2>Nykyinen Ihmismäärä</h2>"; // Ääkkösiä sisältävä teksti
  html += "<h1>";
  
  // Näytetään lukumäärä, jos dataa on saapunut
  if (nykyinenLuku >= 0) {
    html += nykyinenLuku;
  } else {
    html += "ODOTTAA..."; // Ääkkösiä sisältävä teksti
  }
  
  html += "</h1>";
  html += "</body></html>";
  
  // Varmistetaan, että selain ymmärtää vastauksen olevan UTF-8 (vaikka ei ole aivan pakollista HTML-meta-tagin kanssa)
  server.send(200, "text/html", html); 
}
// -----------------------------

void setup() {
  // Sarjaporttien alustus (molemmat 9600 baud)
  Serial.begin(9600); // Debug USB:lle
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // Kommunikaatio Masterin kanssa (RX=16)

  // OLED alustus
  u8g2.begin();
  u8g2.setContrast(255);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  
  // Huom: OLED-näyttö näyttää ääkköset oikein, kunhan ne on kirjoitettu koodiin oikein.
  u8g2.setCursor(20, 35);
  u8g2.print("ODOTTAA..."); 
  u8g2.sendBuffer();

  // --- WEB-PALVELIMEN SETUP ---
  Serial.println("\nAccess Pointin luominen..."); // Korjattu: Käytetään println()
  
  // Käynnistetään Access Point
  WiFi.softAP(ssid, password); 
  
  // Haetaan AP:n IP-osoite
  IPAddress IP = WiFi.softAPIP(); 
  Serial.print("AP IP osoite: ");
  Serial.println(IP);
  
  // Ilmoitetaan näytöllä IP-osoite
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(0, 10);
  u8g2.print("WiFi AP: ");
  u8g2.print(ssid);
  u8g2.setCursor(0, 25);
  u8g2.print("IP: ");
  u8g2.print(IP);
  u8g2.setCursor(0, 40);
  u8g2.print("Liity verkkoon");
  u8g2.setCursor(0, 55);
  u8g2.print("ja selaa IP:ta!");
  u8g2.sendBuffer();


  // Määritellään, mitä tapahtuu, kun juuriosoitteeseen (/) otetaan yhteyttä
  server.on("/", handleRoot);
  server.begin(); // Käynnistetään HTTP-palvelin
  Serial.println("HTTP-palvelin käynnistetty");
  // -----------------------------
}

void loop() {
  bool lukuMuuttui = false;

  // LUE SARJADATAA
  while (Serial2.available()) {
    String rivi = Serial2.readStringUntil('\n');
    rivi.trim();
    
    // Debuggaus: Tulosta kaikki mitä tulee ESP32:n sarjaporttiin
    // Serial Monitorin tulosteet toimivat yleensä UTF-8:na, kunhan itse kooditiedosto on tallennettu UTF-8-muodossa.
    Serial.print("RAW: "); 
    Serial.println(rivi); 

    if (rivi.startsWith("COUNT:")) {
      int saatu = rivi.substring(6).toInt();
      if (saatu >= 0 && saatu <= 999 && saatu != nykyinenLuku) {
        nykyinenLuku = saatu;
        lukuMuuttui = true;
        dataSaapunut = true;
        // Tulostetaan uusi luku myös debug-porttiin
        Serial.printf("Ihmisiä: %d\n", nykyinenLuku); // Ääkköset tässä OK, Serial.printf käyttää C-standardia.
      }
    }
  }

  // PÄIVITÄ NÄYTTÖ
  if (dataSaapunut) {
    static uint32_t edPiirto = 0;
    // Päivitä näyttö, jos luku muuttui TAI on kulunut yli 33ms (n. 30 FPS)
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
      u8g2.sendBuffer();
      edPiirto = millis();
    }
  }

  // --- KÄSITTELE WEB-PYYNNÖT ---
  server.handleClient(); // Kuuntelee ja vastaa HTTP-pyyntöihin.
  // -----------------------------

  delay(10);
}