#include <EEPROM.h>

const int trig1 = 9, echo1 = 8;
const int trig2 = 7, echo2 = 6;

const int vihreaLED = 3;
const int punainenLED = 4;

const int MAX_DISTANCE = 10;
const int AIKA_IKKUNA = 800;

int ihmiset = 0;
int tila = 0;
unsigned long ekaAika = 0;

void setup() {
    EEPROM.get(0, ihmiset);
    Serial.begin(115200);

    pinMode(trig1, OUTPUT);
    pinMode(echo1, INPUT);
    pinMode(trig2, OUTPUT);
    pinMode(echo2, INPUT);

    pinMode(vihreaLED, OUTPUT);
    digitalWrite(vihreaLED, LOW);

    pinMode(punainenLED, OUTPUT);
    digitalWrite(punainenLED, LOW);

    Serial.println("SENSORI-ARDUINO VALMIS");
}

float lueEtaisyys(int trig, int echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    unsigned long kesto = pulseIn(echo, HIGH, 10000);
    return (kesto == 0) ? 999.0 : kesto * 0.0343 / 2.0;
}

void loop() {
    float s1 = lueEtaisyys(trig1, echo1);
    float s2 = lueEtaisyys(trig2, echo2);

    bool lahella1 = (s1 < MAX_DISTANCE);
    bool lahella2 = (s2 < MAX_DISTANCE);

    if (lahella1 && lahella2) {
        tila = 0;
        return;
    }

    if (tila == 0) {
        if (lahella1) {
            tila = 1;
            ekaAika = millis();
        } else if (lahella2) {
            tila = 2;
            ekaAika = millis();
        }
    } else if (millis() - ekaAika < AIKA_IKKUNA) {
        if (tila == 1 && lahella2) {
            // SISÄÄN
            ihmiset++;
            EEPROM.put(0, ihmiset);
            digitalWrite(vihreaLED, HIGH);
            delay(300);
            digitalWrite(vihreaLED, LOW);
            tila = 0;
            return;
        }
        if (tila == 2 && lahella1) {
            // ULOS
            if (ihmiset > 0) ihmiset--;
            EEPROM.put(0, ihmiset);
            digitalWrite(punainenLED, HIGH);
            delay(300);
            digitalWrite(punainenLED, LOW);
            tila = 0;
            return;
        }
    } else {
        tila = 0;
    }

    // Lähetetään nykyinen ihmismäärä 2 kertaa sekunnissa
    static unsigned long edellinen = 0;
    if (millis() - edellinen > 500) {
        Serial.print("COUNT:");
        Serial.println(ihmiset);
        edellinen = millis();
    }

    delay(70);
}
