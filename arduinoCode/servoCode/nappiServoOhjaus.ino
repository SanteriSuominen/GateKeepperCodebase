#include <Servo.h>

Servo ovi;

bool oviAuki = false;

void setup() {
  Serial.begin(115200);

  // --- Servo ---
  ovi.attach(9);
  ovi.write(90); // kiinni
  delay(500);
  Serial.println("SERVO VALMIS");

  // --- Nappien asetus INPUT_PULLUP ---
  DDRD &= ~(1 << DDD2); // D2 input
  PORTD |= (1 << PORTD2); // pull-up D2

  DDRB &= ~(1 << DDB0); // D8 input (PORTB0)
  PORTB |= (1 << PORTB0); // pull-up D8
}

void aukaiseOvi() {
  if (!oviAuki) {
    for (int p = 90; p >= 0; p -= 2) {
      ovi.write(p);
      delay(15);
    }
    oviAuki = true;
  }
}

void suljeOvi() {
  if (oviAuki) {
    for (int p = 0; p <= 90; p += 2) {
      ovi.write(p);
      delay(15);
    }
    oviAuki = false;
  }
}

void loop()
{
  bool nappiKiinni = !(PIND & (1 << PIND2)); // D2 LOW = painettu
  bool nappiAuki = !(PINB & (1 << PINB0));   // D8 LOW = painettu
  if (nappiKiinni) {
    suljeOvi();
  }

  if (nappiAuki) {
    aukaiseOvi();
  }
}
