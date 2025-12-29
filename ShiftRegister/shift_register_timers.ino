const bool COMMON_CATHODE = true; 

bool digits[10][8] = {
  {1,1,1,1,1,1,0,0},  // 0
  {0,1,1,0,0,0,0,0},  // 1
  {1,1,0,1,1,0,1,0},  // 2
  {1,1,1,1,0,0,1,0},  // 3
  {0,1,1,0,0,1,1,0},  // 4
  {1,0,1,1,0,1,1,0},  // 5
  {1,0,1,1,1,1,1,0},  // 6
  {1,1,1,0,0,0,0,0},  // 7
  {1,1,1,1,1,1,1,0},  // 8
  {1,1,1,1,0,1,1,0}   // 9
};
volatile uint8_t counter = 0;

void updateDisplay() {
  uint8_t tens = counter / 10;
  uint8_t units = counter % 10;
  uint8_t nums[2] = {tens, units};

  for (int n = 0; n < 2; n++) { 
    uint8_t digitId = nums[n];
    
    for (int i = 0; i < 8; i++) {
      bool bitValue = digits[digitId][i];
      
      if (COMMON_CATHODE == false) {
        bitValue = !bitValue;
      }

      // dataPin (PB2, pin 10)
      if (bitValue) PORTB |= (1 << 2);
      else PORTB &= ~(1 << 2);
    
      // clockPin (PB3, pin 11) - импульс
      PORTB |= (1 << 3);
      PORTB &= ~(1 << 3);
    }
  }

  // latchPin (PB4, pin 12) - импульс
  PORTB |= (1 << 4);
  PORTB &= ~(1 << 4);
}

void setup() {
  // Инициализация пинов как выходы (PB2, PB3, PB4 соответствуют пинам 10, 11, 12)
  DDRB |= (1 << 2) | (1 << 3) | (1 << 4);
  
  // Установка начального состояния пинов в LOW
  PORTB &= ~((1 << 2) | (1 << 3) | (1 << 4));
  
  Serial.begin(9600);

  // Timer1 (1 секунда)
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 62499; 
  TCCR1B |= (1 << WGM12); 
  TCCR1B |= (1 << CS12);  
  TIMSK1 |= (1 << OCIE1A);
  sei();

  updateDisplay();
}


ISR(TIMER1_COMPA_vect) {
  counter++;
  if (counter > 59) {
    counter = 0;
  }
  updateDisplay();
}

void loop() {
  // Чтение начального значения через Serial
  if (Serial.available() > 0) {
    int input = Serial.parseInt();
    if (input >= 0 && input <= 59) {
      cli(); 
      counter = (uint8_t)input;
      updateDisplay();
      sei();
    }
  }
}
