/*
 * ARDUINO NANO - OVIYKSIKKÖ - FIX: 9600 BAUD
 * Kytkentä:
 * - Servo: Pin 9
 * - Nappi Avaa: Pin 2
 * - Nappi Sulje: Pin 3
 * - RX: Pin 0 (Kytke Master TX tähän)
 */

#include <Servo.h>

Servo ovi;

const int PIN_SERVO = 9;
const int PIN_BTN_OPEN = 2;
const int PIN_BTN_CLOSE = 3;

bool oviAuki = false;
unsigned long autoCloseTimer = 0;

void setup() {
  Serial.begin(9600); // <-- TÄRKEÄ: Synkassa muiden kanssa
  
  ovi.attach(PIN_SERVO);
  ovi.write(90); 
  
  pinMode(PIN_BTN_OPEN, INPUT_PULLUP);
  pinMode(PIN_BTN_CLOSE, INPUT_PULLUP);
}

void avaaOvi() {
  if (!oviAuki) {
    for (int p = 90; p >= 0; p -= 2) {
      ovi.write(p); delay(10);
    }
    oviAuki = true;
  }
  autoCloseTimer = millis(); 
}

void suljeOvi() {
  if (oviAuki) {
    for (int p = 0; p <= 90; p += 2) {
      ovi.write(p); delay(10);
    }
    oviAuki = false;
  }
}

void loop() {
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    if (msg == "IN" || msg == "OUT") avaaOvi();
  }

  if (digitalRead(PIN_BTN_OPEN) == LOW) {
    avaaOvi(); delay(50);
  }
  
  if (digitalRead(PIN_BTN_CLOSE) == LOW) {
    suljeOvi(); autoCloseTimer = 0; delay(50);
  }

  if (oviAuki && autoCloseTimer > 0 && millis() - autoCloseTimer > 1500) {
    suljeOvi(); autoCloseTimer = 0;
  }
}