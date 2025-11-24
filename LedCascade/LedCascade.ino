
// --- Константы для пинов ---
#define LED1_PIN PB0 // Пин 8
#define LED2_PIN PB1 // Пин 9
#define LED3_PIN PB2 // Пин 10
#define LED4_PIN PB3 // Пин 11
#define LED5_PIN PB4 // Пин 12

// --- Константы для периодов ---
const uint8_t PERIOD_LED1 = 1; // 250 мс
const uint8_t PERIOD_LED2 = 2; // 500 мс
const uint8_t PERIOD_LED3 = 3; // 750 мс
const uint8_t PERIOD_LED4 = 4; // 1000 мс
const uint8_t PERIOD_LED5 = 5; // 1250 мс


void setup() {
  // 1. Отключаем прерывания на время настройки
  cli();

  // 2. Настраиваем каждый пин (PB0-PB4) как ВЫХОД 
  // Устанавливаем биты 0, 1, 2, 3, 4 в регистре DDRB
  DDRB |= (1 << LED1_PIN); // Пин PB0 (D8) как ВЫХОД
  DDRB |= (1 << LED2_PIN); // Пин PB1 (D9) как ВЫХОД
  DDRB |= (1 << LED3_PIN); // Пин PB2 (D10) как ВЫХОД
  DDRB |= (1 << LED4_PIN); // Пин PB3 (D11) как ВЫХОД
  DDRB |= (1 << LED5_PIN); // Пин PB4 (D12) как ВЫХОД

  // 3. Устанавливаем начальное состояние светодиодов (ВЫКЛ) 
  // Сбрасываем биты 0, 1, 2, 3, 4 в регистре PORTB
  PORTB &= ~(1 << LED1_PIN); // LED 1 ВЫКЛ
  PORTB &= ~(1 << LED2_PIN); // LED 2 ВЫКЛ
  PORTB &= ~(1 << LED3_PIN); // LED 3 ВЫКЛ
  PORTB &= ~(1 << LED4_PIN); // LED 4 ВЫКЛ
  PORTB &= ~(1 << LED5_PIN); // LED 5 ВЫКЛ

  // 4. Настройка Timer1 
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  
  // Устанавливаем значение для сравнения (250 мс)
  // (16,000,000 / 256) / 4 Гц - 1 = 15624
  OCR1A = 15624;
  
  // Включаем режим CTC (WGM12) и предделитель 256 (CS12)
  TCCR1B |= (1 << WGM12) | (1 << CS12);
  
  // Включаем прерывание по совпадению A для Timer1
  TIMSK1 |= (1 << OCIE1A);

  // 5. Включаем глобальные прерывания
  sei();
}

// 6. Обработчик прерывания (ISR) для Timer1 
ISR(TIMER1_COMPA_vect) {
  
  static uint8_t counter_led1 = 0;
  static uint8_t counter_led2 = 0;
  static uint8_t counter_led3 = 0;
  static uint8_t counter_led4 = 0;
  static uint8_t counter_led5 = 0;

  // --- LED 1 (PB0) ---
  counter_led1++;
  if (counter_led1 >= PERIOD_LED1) {
    PORTB ^= (1 << LED1_PIN); 
    counter_led1 = 0;         
  }

  // --- LED 2 (PB1) ---
  counter_led2++;
  if (counter_led2 >= PERIOD_LED2) {
    PORTB ^= (1 << LED2_PIN); // Toggle
    counter_led2 = 0;         // Reset
  }

  // --- LED 3 (PB2) ---
  counter_led3++;
  if (counter_led3 >= PERIOD_LED3) {
    PORTB ^= (1 << LED3_PIN); // Toggle
    counter_led3 = 0;         // Reset
  }

  // --- LED 4 (PB3) ---
  counter_led4++;
  if (counter_led4 >= PERIOD_LED4) {
    PORTB ^= (1 << LED4_PIN); // Toggle
    counter_led4 = 0;         // Reset
  }

  // --- LED 5 (PB4) ---
  counter_led5++;
  if (counter_led5 >= PERIOD_LED5) {
    PORTB ^= (1 << LED5_PIN); // Toggle
    counter_led5 = 0;         // Reset
  }
}

void loop() {

}