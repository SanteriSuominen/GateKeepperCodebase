/*
 * ARDUINO NANO - SENSORIYKSIKKÖ (MASTER)
 * FIX: Baudrate 9600 (Vakaa tiedonsiirto)
 * * * KYTKENTÄ (Arduino Nano/Uno):
 * - Trig1 (Ulko): Pin 9  (PORTB PB1)
 * - Echo1 (Ulko): Pin 8  (PORTB PB0) -> PCINT0
 * - Trig2 (Sisä): Pin 7  (PORTD PD7)
 * - Echo2 (Sisä): Pin 6  (PORTD PD6) -> PCINT2
 * - Vihreä LED:   Pin 3  (PORTD PD3)
 * - Punainen LED: Pin 4  (PORTD PD4)
 * - BUZZER:       Pin 5  (PORTD PD5)
 * - TX (Data):    Pin 1  (PD1) -> Kytke ESP32 RX (16) & Servo RX (0)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <EEPROM.h>

// --- VAKIOT ---
#define MAX_DISTANCE_CM 40
#define TIMEOUT_MS 800

// --- GLOBAALIT MUUTTUJAT ---
volatile unsigned long systemTimeMs = 0; 
volatile unsigned long echoStart1 = 0, echoDuration1 = 0;
volatile unsigned long echoStart2 = 0, echoDuration2 = 0;
int ihmiset = 0;
int tila = 0;
unsigned long ekaAika = 0;

// --- REKISTERIOHJATTU USART (9600 BAUD) ---
void USART_Init(unsigned int baud) {
  // Lasketaan UBRR (16MHz kello)
  // 9600 baudilla UBRR on 103 -> Virhe vain 0.2% -> ERITTÄIN VAKAA
  unsigned int ubrr = F_CPU / 16 / baud - 1;
  
  UBRR0H = (unsigned char)(ubrr >> 8);
  UBRR0L = (unsigned char)ubrr;
  UCSR0B = (1 << TXEN0); // TX päällä
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1
}

void USART_Transmit(char data) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = data;
}

void USART_Print(const char* str) {
  while (*str) USART_Transmit(*str++);
}

void USART_PrintInt(int num) {
  char buffer[10];
  itoa(num, buffer, 10);
  USART_Print(buffer);
}

// --- TIMER2 KESKEYTYS (1ms) ---
void Timer2_Init() {
  TCCR2A = (1 << WGM21); // CTC
  OCR2A = 124; 
  TCCR2B = (1 << CS22) | (1 << CS20); // Prescaler 128
  TIMSK2 = (1 << OCIE2A);
}

ISR(TIMER2_COMPA_vect) {
  systemTimeMs++;
}

// --- PIN CHANGE INTERRUPTS ---
void PCINT_Init() {
  PCICR |= (1 << PCIE0) | (1 << PCIE2);
  PCMSK0 |= (1 << PCINT0);  // PB0 (Pin 8)
  PCMSK2 |= (1 << PCINT22); // PD6 (Pin 6)
}

ISR(PCINT0_vect) {
  if (PINB & (1 << PB0)) echoStart1 = micros(); 
  else echoDuration1 = micros() - echoStart1;
}

ISR(PCINT2_vect) {
  if (PIND & (1 << PD6)) echoStart2 = micros();
  else echoDuration2 = micros() - echoStart2;
}

// --- GPIO ---
void GPIO_Init() {
  DDRB |= (1 << PB1); // Out
  DDRB &= ~(1 << PB0); // In
  DDRD |= (1 << PD7) | (1 << PD5) | (1 << PD4) | (1 << PD3); // Out
  DDRD &= ~(1 << PD6); // In
}

void triggerSensor(bool sensor1) {
  if (sensor1) {
    PORTB |= (1 << PB1); _delay_us(10); PORTB &= ~(1 << PB1);
  } else {
    PORTD |= (1 << PD7); _delay_us(10); PORTD &= ~(1 << PD7);
  }
}

void beep(int duration_ms) {
  PORTD |= (1 << PD5);
  for(int i=0; i<duration_ms; i++) _delay_ms(1);
  PORTD &= ~(1 << PD5);
}

void setup() {
  EEPROM.get(0,ihmiset);
  cli();
  USART_Init(9600); // <-- TÄRKEÄ MUUTOS
  GPIO_Init();
  Timer2_Init();
  PCINT_Init();
  sei();
  USART_Print("MASTER START 9600\n");
}

void loop() {
  static int vaihe = 0;
  static unsigned long lastTrigger = 0;
  
  unsigned long currentMillis;
  cli(); currentMillis = systemTimeMs; sei();

  if (currentMillis - lastTrigger > 60) {
    lastTrigger = currentMillis;
    if (vaihe == 0) { triggerSensor(true); vaihe = 1; } 
    else { triggerSensor(false); vaihe = 0; }
  }

  float dist1 = echoDuration1 * 0.0343 / 2.0;
  float dist2 = echoDuration2 * 0.0343 / 2.0;
  
  bool active1 = (dist1 > 0 && dist1 < MAX_DISTANCE_CM);
  bool active2 = (dist2 > 0 && dist2 < MAX_DISTANCE_CM);

  if (active1 && active2) {
    tila = 0; return;
  }

  if (tila == 0) {
    if (active1) { tila = 1; ekaAika = currentMillis; } 
    else if (active2) { tila = 2; ekaAika = currentMillis; }
  } 
  else {
    if (currentMillis - ekaAika > TIMEOUT_MS) {
      tila = 0;
    } 
    else {
      if (tila == 1 && active2) { // SISÄÄN
        ihmiset++;
        USART_Print("IN\n");    
        USART_Print("COUNT:"); USART_PrintInt(ihmiset); USART_Print("\n");
        PORTD |= (1 << PD3); beep(50); _delay_ms(150); PORTD &= ~(1 << PD3);
        tila = 0; echoDuration1 = 0; echoDuration2 = 0; _delay_ms(500);
        EEPROM.put(0,ihmiset);
      }
      else if (tila == 2 && active1) { // ULOS
        if (ihmiset > 0) ihmiset--;
        USART_Print("OUT\n");
        USART_Print("COUNT:"); USART_PrintInt(ihmiset); USART_Print("\n");
        PORTD |= (1 << PD4); beep(150); _delay_ms(50); PORTD &= ~(1 << PD4);
        tila = 0; echoDuration1 = 0; echoDuration2 = 0; _delay_ms(500); 
        EEPROM.put(0,ihmiset);
      }
    }
  }
}