volatile unsigned long millis_counter = 0; 

void initMyMillis() {
  cli();

  // Timer2 
  TCCR2A = 0;               // сброс конфигурации
  TCCR2B = 0;               // сброс конфигурации
  TCCR2A |= (1 << WGM21);   

  OCR2A = 249;              // 16MHz / 64 = 250kHz; 250 тиков = 1мс -> OCR2A=249
  TCNT2 = 0;                // сброс счётчика

  TIMSK2 |= (1 << OCIE2A);  // разрешить прерывание по совпадению A

  // Предделитель 64: CS22=1, CS21=0, CS20=0
  TCCR2B |= (1 << CS22);

  sei();
}

ISR(TIMER2_COMPA_vect) {
  millis_counter++;
}

unsigned long my_millis() {
  cli();
  unsigned long ms = millis_counter;
  sei();
  return ms;
}

void setup() {
    Serial.begin(9600);
    initMyMillis();
}


void loop() {
    Serial.println(my_millis());
    delay(1000);
}
