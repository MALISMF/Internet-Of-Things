volatile bool interrupt_led_state = false;

int current_mode = 0; // 0=только прерывание, 1=millis, 2=delay
unsigned long prev_millis = 0;
bool millis_led_state = false;
const int millis_interval = 1000; // интервал для millis режима

// Пины для разных режимов
const int INTERRUPT_LED_PIN = 13; // пин для режима 0
const int MODE_LED_PIN = 12;      // пин для millis/delay режимов

void setup() {
  Serial.begin(9600);
  
  // Настройка пинов
  pinMode(INTERRUPT_LED_PIN, OUTPUT);
  pinMode(MODE_LED_PIN, OUTPUT);
  
  // Инициализация состояний
  digitalWrite(INTERRUPT_LED_PIN, LOW);
  digitalWrite(MODE_LED_PIN, LOW);
  
  // Настройка Timer2 для прерывания (всегда работает)
  setupTimer2Interrupt();
  
  Serial.println("Три вида blink");
  Serial.println("Команды: '1'=millis() режим, '2'=delay() режим, '0'=только прерывание");
  Serial.println("Прерывание всегда работает на пине 13, остальные режимы на пине 12");
}

void setupTimer2Interrupt() {
  cli(); // запрещаем глобальные прерывания для настройки таймера
  
  TCCR2A = 0;
  TCCR2B = 0;
  
  // Предделитель 1024 (максимальный для 8-битного таймера): CS22=1, CS21=1, CS20=1
  // Период переполнения: 256 * 1024 / 16_000_000 ≈ 16.384 мс
  TCCR2B = TCCR2B | ((1 << CS22) | (1 << CS21) | (1 << CS20));
  
  // Разрешаем прерывание по переполнению таймера 2
  TIMSK2 = TIMSK2 | (1 << TOIE2);
  
  sei(); // разрешаем глобальные прерывания
}

// Обработчик прерывания по переполнению Timer2 (всегда работает)
ISR(TIMER2_OVF_vect) {
  interrupt_led_state = !interrupt_led_state;
}

void loop() {
  // Обработка UART команд
  if (Serial.available()) {
    handleUARTCommands();
  }
  
  // Прерывание всегда работает на пине 13
  digitalWrite(INTERRUPT_LED_PIN, interrupt_led_state);
  
  // Выполнение текущего режима на пине 12
  if (current_mode == 0) {
    // Только прерывание - выключаем дополнительный светодиод
    digitalWrite(MODE_LED_PIN, LOW);
  }
  else if (current_mode == 1) {
    // Режим millis() - неблокирующий
    blinkWithMillis();
  }
  else if (current_mode == 2) {
    // Режим delay() - блокирующий
    blinkWithDelay();
  }
}

void handleUARTCommands() {
  char command = Serial.read();
  
  // игнорируем символы новой строки и возврата каретки после Enter
  if (command == '\n' || command == '\r') {
    return;
  }
  
  if (command == '0') {
    current_mode = 0;
    Serial.println("Режим: только прерывание");
  }
  else if (command == '1') {
    current_mode = 1;
    Serial.println("Режим: millis() + прерывание");
    prev_millis = millis(); // сброс таймера
    millis_led_state = false;
  }
  else if (command == '2') {
    current_mode = 2;
    Serial.println("Режим: delay() + прерывание");
  }
  else {
    Serial.println("Неизвестная команда. Используйте: 0, 1, 2");
  }
}

void blinkWithMillis() {
  unsigned long current_millis = millis();
  
  if (current_millis - prev_millis >= millis_interval) {
    millis_led_state = !millis_led_state;
    digitalWrite(MODE_LED_PIN, millis_led_state);
    prev_millis = current_millis;
  }
}

void blinkWithDelay() {

  digitalWrite(MODE_LED_PIN, HIGH);
  delay(1000);
  

  digitalWrite(MODE_LED_PIN, LOW);
  delay(1000);
}
