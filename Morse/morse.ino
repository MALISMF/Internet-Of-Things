// ============================================================================
// НАСТРОЙКА ПИНОВ
// ============================================================================
const int BUTTON_PIN = 4;  // Кнопка для MANUAL режима (ввод точек/тире)
const int RX_PIN = 3;       // Вход для приема сигналов Морзе
const int TX_PIN = 2;       // Выход для передачи сигналов Морзе
const int LED_PIN = 13;     // Светодиод для визуальной индикации

// Пины для сдвигового регистра (7-сегментный индикатор)
const int LATCH_PIN = 8;    // Latch (ST_CP) - защелка
const int CLOCK_PIN = 9;    // Clock (SH_CP) - тактовый сигнал
const int DATA_PIN = 10;    // Data (DS) - данные

// ============================================================================
// ПАРАМЕТРЫ КОДА МОРЗЕ (в миллисекундах)
// ============================================================================
const int DOT_DURATION = 200;      // Длительность передачи точки
const int DASH_DURATION = 600;     // Длительность передачи тире
const int ELEM_PAUSE = 200;        // Пауза между элементами (точка/тире) в букве
const int LETTER_PAUSE = 600;      // Пауза между буквами в слове
const int WORD_PAUSE = 1400;       // Пауза между словами (отправка слова)
const int MIN_DISPLAY_TIME = 800;  // Минимальное время отображения символа на индикаторе (мс)

// ============================================================================
// ОПРЕДЕЛЕНИЕ РЕЖИМОВ РАБОТЫ
// ============================================================================
enum Mode {
  MODE_RX,        // Режим приема: постоянный мониторинг RX_PIN
  MODE_TX_AUTO,   // Режим передачи AUTO: текст через Serial -> Морзе
  MODE_TX_MANUAL  // Режим передачи MANUAL: кнопка -> точки/тире -> Морзе
};

Mode currentMode = MODE_RX;  // Текущий режим работы (по умолчанию - прием)

// ============================================================================
// ПЕРЕМЕННЫЕ ДЛЯ РЕЖИМА ПРИЕМА (RX)
// ============================================================================
int lastRxSignal = LOW;           // Предыдущее состояние сигнала на RX_PIN
unsigned long signalStart = 0;     // Время начала текущего импульса HIGH
unsigned long pauseStart = 0;      // Время начала текущей паузы (LOW)
String receivedMorse = "";         // Накопленная последовательность Морзе для текущей буквы
String decodedMessage = "";       // Полное декодированное сообщение

// ============================================================================
// ПЕРЕМЕННЫЕ ДЛЯ РЕЖИМА ПЕРЕДАЧИ (TX)
// ============================================================================
String currentMorse = "";          // Последовательность Морзе для передачи
int morseIndex = 0;                // Текущая позиция в последовательности Морзе
unsigned long txTimer = 0;         // Таймер для отсчета длительности элементов
bool txActive = false;             // Флаг активной передачи (true = идет передача)
bool sendingElement = false;       // Флаг отправки элемента (true = передается точка/тире)

// ============================================================================
// ПЕРЕМЕННЫЕ ДЛЯ MANUAL РЕЖИМА
// ============================================================================
String manualMorse = "";           // Текущая буква в формате Морзе (точки и тире)
String manualWord = "";           // Накопленное слово (буквы разделены пробелами)
unsigned long buttonPressStart = 0;   // Время начала нажатия кнопки
unsigned long buttonReleaseTime = 0;  // Время последнего отпускания кнопки
int lastButtonState = LOW;         // Предыдущее состояние кнопки (для pull-down: LOW = отпущена)
bool buttonPressed = false;        // Флаг активного нажатия кнопки

// ============================================================================
// АППАРАТНЫЙ ТАЙМЕР
// ============================================================================
// Используется Timer1 для точного измерения времени (1 мс точность)
volatile unsigned long timerCounter = 0;

// ============================================================================
// ТАБЛИЦА КОДА МОРЗЕ
// ============================================================================
// Индексы 0-25: буквы A-Z
// Индексы 26-35: цифры 0-9
const char* morseCode[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
  "..-", "...-", ".--", "-..-", "-.--", "--..",
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

// ============================================================================
// ТАБЛИЦА ПАТТЕРНОВ ДЛЯ 7-СЕГМЕНТНОГО ИНДИКАТОРА
// ============================================================================
// Распиновка: Q0=A, Q1=B, Q2=C, Q3=D, Q4=E, Q5=F, Q6=G, Q7=не используется
bool digitPatterns[36][8] = {
  // Цифры 0-9: A B C D E F G 0
  {1,1,1,1,1,1,0,0},  // 0: A B C D E F 
  {0,1,1,0,0,0,0,0},  // 1: B C
  {1,1,0,1,1,0,1,0},  // 2: A B D E G
  {1,1,1,1,0,0,1,0},  // 3: A B C D G
  {0,1,1,0,0,1,1,0},  // 4: B C F G
  {1,0,1,1,0,1,1,0},  // 5: A C D F G
  {1,0,1,1,1,1,1,0},  // 6: A C D E F G
  {1,1,1,0,0,0,0,0},  // 7: A B C
  {1,1,1,1,1,1,1,0},  // 8: A B C D E F G 
  {1,1,1,1,0,1,1,0},  // 9: A B C D F G
  
  // Буквы A-Z: A B C D E F G 0
  {1,1,1,0,1,1,1,0},  // A (10): A B C E F G
  {0,0,1,1,1,1,1,0},  // b (11): C D E F G
  {0,0,0,1,1,0,0,0},  // C (12): D E
  {0,1,1,1,1,0,1,0},  // d (13): B C D E G
  {1,0,0,1,1,1,1,0},  // E (14): A D E F G
  {1,0,0,0,1,1,1,0},  // F (15): A E F G
  {1,0,1,1,1,1,1,0},  // G (16): A C D E F G
  {0,1,1,0,1,1,1,0},  // H (17): B C E F G
  {0,1,1,0,0,0,0,0},  // I (18): B C
  {0,1,1,1,0,0,0,0},  // J (19): B C D
  {0,1,1,0,1,1,1,0},  // K (20): B C E F G 
  {0,0,0,1,1,1,0,0},  // L (21): D E F
  {1,1,1,0,1,0,1,0},  // M (22): A B C E G
  {0,0,1,1,1,0,1,0},  // N (23): C D E G
  {1,1,1,1,1,1,0,0},  // O (24): A B C D E F
  {1,1,0,0,1,1,1,0},  // P (25): A B E F G
  {1,1,1,0,0,1,1,0},  // Q (26): A B C F G
  {1,1,0,0,1,0,1,0},  // R (27): A B E G
  {1,0,1,1,0,1,1,0},  // S (28): A C D F G 
  {0,0,0,1,1,1,1,0},  // T (29): D E F G
  {1,1,1,1,1,1,0,0},  // U (30): A B C D E F
  {1,1,1,1,0,0,0,0},  // V (31): A B C D
  {1,1,1,0,1,1,0,0},  // W (32): A B C E F
  {0,1,1,0,1,1,1,0},  // X (33): B C E F G
  {0,1,1,1,0,1,1,0},  // Y (34): B C D F G
  {1,1,0,1,1,0,1,0}   // Z (35): A B D E G
};

// ============================================================================
// ПРЕРЫВАНИЕ ТАЙМЕРА
// ============================================================================
// Вызывается каждую миллисекунду для точного измерения времени
ISR(TIMER1_COMPA_vect) {
  timerCounter++;
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ СИСТЕМЫ
// ============================================================================
void setup() {
  Serial.begin(9600);
  
  // Настройка пинов
  pinMode(BUTTON_PIN, INPUT);  // Кнопка с внешним pull-down резистором (HIGH = нажата)
  pinMode(RX_PIN, INPUT);       // Вход для приема сигналов
  pinMode(TX_PIN, OUTPUT);      // Выход для передачи сигналов
  pinMode(LED_PIN, OUTPUT);     // Светодиод для индикации
  
  // Настройка пинов для сдвигового регистра
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  
  // Инициализация состояний
  digitalWrite(TX_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LATCH_PIN, LOW);
  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(DATA_PIN, LOW);
  lastRxSignal = digitalRead(RX_PIN);
  lastButtonState = digitalRead(BUTTON_PIN);  // Инициализация для pull-down
  
  // Очистка индикатора при старте
  displayChar(' ');
  
  // Настройка аппаратного таймера Timer1 для точного измерения времени
  // Прескалер = 64, OCR1A = 249 -> частота прерывания = 1 кГц (1 мс)
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 249;
  TCCR1B |= (1 << WGM12);        // Режим CTC (Clear Timer on Compare)
  TCCR1B |= (1 << CS11) | (1 << CS10);  // Прескалер 64
  TIMSK1 |= (1 << OCIE1A);       // Разрешить прерывание по совпадению
  interrupts();
  
  // Информация о системе
  Serial.println("Morse2 System initialized");
  Serial.println("Mode: RX (always monitoring)");
  Serial.println("Commands: AUTO, MANUAL");
}

// ============================================================================
// ОСНОВНОЙ ЦИКЛ ПРОГРАММЫ
// ============================================================================
void loop() {
  // Прием сигналов всегда активен (кроме времени передачи)
  handleReceive();
  
  // Обработка передачи (если идет активная передача)
  if (txActive) {
    handleTransmit();
  }
  
  // Обработка MANUAL режима (ввод через кнопку)
  if (currentMode == MODE_TX_MANUAL && !txActive) {
    handleManualInput();
  }
  
  // Обработка команд через Serial порт
  handleSerial();
}

// ============================================================================
// ОБРАБОТКА ПРИЕМА СИГНАЛОВ (RX)
// ============================================================================
// Постоянный мониторинг RX_PIN для декодирования входящих сигналов Морзе
// Прием приостанавливается только во время активации передачи 
void handleReceive() {
  // Если идет передача - прием приостанавливается
  if (txActive) {
    // Обновляем состояние RX, но не декодируем сигналы
    lastRxSignal = digitalRead(RX_PIN);
    return;
  }
  
  // Обычная логика приема (только когда не передаем)
  int signal = digitalRead(RX_PIN);
  unsigned long now = timerCounter;
  static unsigned long lastDisplayTime = 0;  // Время последнего отображения символа
  
  // Обнаружение изменения сигнала (переход LOW->HIGH или HIGH->LOW)
  if (signal != lastRxSignal) {
    // Начало импульса HIGH (начало передачи точки или тире)
    if (lastRxSignal == LOW && signal == HIGH) {
      signalStart = now;
      
      // Если была длительная пауза, завершаем предыдущий символ
      if (pauseStart > 0 && (now - pauseStart) > LETTER_PAUSE) {
        if (receivedMorse.length() > 0) {
          char decoded = morseToChar(receivedMorse);
          if (decoded != 0) {
            decodedMessage += decoded;
            Serial.print("RX: ");
            Serial.print(receivedMorse);
            Serial.print(" -> ");
            Serial.println(decoded);
            Serial.print("Message: ");
            Serial.println(decodedMessage);
            // Вывод на 7-сегментный индикатор (только если прошло достаточно времени)
            if ((now - lastDisplayTime) >= MIN_DISPLAY_TIME) {
              displayChar(decoded);
              lastDisplayTime = now;
            }
          }
          receivedMorse = "";
        }
      }
    } 
    // Конец импульса HIGH (завершение передачи точки или тире)
    else if (lastRxSignal == HIGH && signal == LOW) {
      unsigned long duration = now - signalStart;
      pauseStart = now;
      
      // Фильтр шума: игнорируем очень короткие импульсы (< 50 мс)
      if (duration > 50) {
        // Определение типа элемента по длительности
        if (duration >= (DASH_DURATION - 100)) {
          // Длительность >= 500 мс -> это тире
          receivedMorse += '-';
          Serial.print('-');
        } else if (duration >= (DOT_DURATION - 100)) {
          // Длительность >= 100 мс -> это точка
          receivedMorse += '.';
          Serial.print('.');
        }
      }
    }
    
    lastRxSignal = signal;
  }
  
  // Обработка длительной паузы (если сигнал LOW и прошло достаточно времени)
  // Это означает завершение буквы
  if (lastRxSignal == LOW && pauseStart > 0 && (now - pauseStart) > LETTER_PAUSE) {
    if (receivedMorse.length() > 0) {
      static unsigned long lastLetterProcessTime = 0;
      if ((now - lastLetterProcessTime) > LETTER_PAUSE) {
        char decoded = morseToChar(receivedMorse);
        if (decoded != 0) {
          decodedMessage += decoded;
          Serial.print("RX: ");
          Serial.print(receivedMorse);
          Serial.print(" -> ");
          Serial.println(decoded);
          Serial.print("Message: ");
          Serial.println(decodedMessage);
          // Вывод на 7-сегментный индикатор 
          if ((now - lastDisplayTime) >= MIN_DISPLAY_TIME) {
            displayChar(decoded);
            lastDisplayTime = now;
          }
        }
        receivedMorse = "";
        lastLetterProcessTime = now;
      }
    }
  }
  
  // Обработка паузы между словами (WORD_PAUSE)
  // Если пауза достаточно длинная -> завершение слова, очистка сообщения
  if (lastRxSignal == LOW && pauseStart > 0 && (now - pauseStart) > WORD_PAUSE) {
    static unsigned long lastWordProcessTime = 0;
    // Предотвращаем повторную обработку в течение WORD_PAUSE
    if ((now - lastWordProcessTime) > WORD_PAUSE) {
      if (decodedMessage.length() > 0) {
        Serial.print("RX: Word complete: ");
        Serial.println(decodedMessage);
        decodedMessage = "";  // Очистка сообщения после приема слова
      }
      lastWordProcessTime = now;
    }
  }
}

// ============================================================================
// ОБРАБОТКА MANUAL РЕЖИМА
// ============================================================================
// Ввод кода Морзе через кнопку:
// - Короткое нажатие (100-400 мс) = точка
// - Длинное нажатие (>= 400 мс) = тире
// - Пауза 600 мс (LETTER_PAUSE) = завершение буквы, переход к следующей
// - Пауза 1400 мс (WORD_PAUSE) = автоматическая отправка накопленного слова
void handleManualInput() {
  int buttonState = digitalRead(BUTTON_PIN);
  unsigned long now = timerCounter;
  
  // Обнаружение нажатия кнопки (с pull-down: HIGH = нажато)
  if (buttonState == HIGH && lastButtonState == LOW) {
    buttonPressStart = now;
    buttonPressed = true;
    digitalWrite(LED_PIN, HIGH);  
  }
  
  // Обнаружение отпускания кнопки 
  // Определяем тип элемента (точка или тире) по длительности нажатия
  if (buttonState == LOW && lastButtonState == HIGH && buttonPressed) {
    unsigned long pressDuration = now - buttonPressStart;
    buttonPressed = false;
    buttonReleaseTime = now;
    digitalWrite(LED_PIN, LOW);
    
    if (pressDuration >= 400) {
      // Длительное нажатие (>= 400 мс) -> тире
      manualMorse += '-';
      Serial.print('-');
    } else if (pressDuration >= 100) {
      // Короткое нажатие (>= 100 мс) -> точка
      manualMorse += '.';
      Serial.print('.');
    }
  }
  
  // Обработка пауз между элементами (кнопка отпущена)
  if (buttonState == LOW && buttonReleaseTime > 0) {
    unsigned long pauseDuration = now - buttonReleaseTime;
    
    // Проверка паузы между словами (WORD_PAUSE = 1400 мс)
    // Если пауза достаточно длинная -> автоматическая отправка слова
    if (pauseDuration >= WORD_PAUSE) {
      if (manualWord.length() > 0) {
        startTransmissionMorse(manualWord);
        Serial.print("TX: Sending word: ");
        Serial.println(manualWord);
        manualWord = "";
        manualMorse = "";
        buttonReleaseTime = 0;
      }
    }
    // Проверка паузы между буквами (LETTER_PAUSE = 600 мс)
    // Если пауза средняя -> завершение буквы, добавление в слово
    else if (pauseDuration >= LETTER_PAUSE) {
      static unsigned long lastLetterProcessTime = 0;
      // Предотвращаем повторную обработку в течение LETTER_PAUSE
      if ((now - lastLetterProcessTime) >= LETTER_PAUSE) {
        if (manualMorse.length() > 0) {
          // Проверяем валидность символа Морзе
          char decoded = morseToChar(manualMorse);
          if (decoded != 0) {
            // Добавляем букву в слово (буквы разделяются пробелами)
            if (manualWord.length() > 0) {
              manualWord += " ";
            }
            manualWord += manualMorse;
            Serial.print(" -> ");
            Serial.print(decoded);
            Serial.print(" (word: ");
            Serial.print(manualWord);
            Serial.println(")");
          } else {
            Serial.println(" -> ? (invalid)");
          }
          manualMorse = "";
          lastLetterProcessTime = now;
          // НЕ сбрасываем buttonReleaseTime, чтобы можно было обработать WORD_PAUSE
        }
      }
    }
  }
  
  lastButtonState = buttonState;
}

// ============================================================================
// ОБРАБОТКА ПЕРЕДАЧИ СИГНАЛОВ (TX)
// ============================================================================
// Последовательная передача элементов Морзе (точки, тире, паузы)
// Управляется через currentMorse и morseIndex
void handleTransmit() {
  unsigned long now = timerCounter;
  
  // Проверка завершения передачи
  if (morseIndex >= currentMorse.length()) {
    txActive = false;
    digitalWrite(TX_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    Serial.println("TX: Transmission complete");
    return;
  }
  
  char elem = currentMorse[morseIndex];
  
  // Начало отправки элемента (точка или тире)
  if (!sendingElement) {
    if (now < txTimer) return;  // Ждем окончания предыдущей паузы
    
    if (elem == '.') {
      // Передача точки
      digitalWrite(TX_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);
      txTimer = now + DOT_DURATION;
      sendingElement = true;
    } else if (elem == '-') {
      // Передача тире
      digitalWrite(TX_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);
      txTimer = now + DASH_DURATION;
      sendingElement = true;
    } else if (elem == ' ') {
      // Пауза между буквами
      txTimer = now + LETTER_PAUSE;
      morseIndex++;
    } else {
      // Пропускаем неизвестные символы
      morseIndex++;
    }
  } 
  // Завершение отправки элемента
  else {
    if (now >= txTimer) {
      // Завершили передачу элемента -> выключаем сигнал
      digitalWrite(TX_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      morseIndex++;
      sendingElement = false;
      txTimer = now + ELEM_PAUSE;  // Пауза между элементами
    }
  }
}

// ============================================================================
// ЗАПУСК ПЕРЕДАЧИ
// ============================================================================

// Запуск передачи из текста (AUTO режим)
// Преобразует ASCII текст в код Морзе и начинает передачу
void startTransmission(String text) {
  if (txActive) {
    Serial.println("TX: Transmission already in progress");
    return;
  }
  
  currentMorse = textToMorse(text);
  morseIndex = 0;
  txActive = true;
  sendingElement = false;
  txTimer = 0;
  
  Serial.print("TX: ");
  Serial.print(text);
  Serial.print(" | Morse: ");
  Serial.println(currentMorse);
}

// Запуск передачи из последовательности Морзе (MANUAL режим)
// Принимает уже готовую последовательность Морзе и начинает передачу
void startTransmissionMorse(String morse) {
  if (txActive) {
    Serial.println("TX: Transmission already in progress");
    return;
  }
  
  currentMorse = morse;
  morseIndex = 0;
  txActive = true;
  sendingElement = false;
  txTimer = 0;
  
  Serial.print("TX: Morse: ");
  Serial.println(currentMorse);
}

// ============================================================================
// КОНВЕРТАЦИЯ МЕЖДУ ТЕКСТОМ И КОДОМ МОРЗЕ
// ============================================================================

// Преобразование ASCII текста в код Морзе
// Поддерживает буквы A-Z и цифры 0-9
// Буквы разделяются пробелами, слова - двойными пробелами
String textToMorse(String text) {
  String result = "";
  for (int i = 0; i < text.length(); i++) {
    char c = text.charAt(i);
    if (c == ' ') {
      // Пробел в тексте -> пробел в Морзе (пауза между словами)
      result += " ";
    } else if (c >= 'A' && c <= 'Z') {
      // Буквы A-Z -> код Морзе из таблицы
      result += morseCode[c - 'A'];
      result += " ";  // Пробел между буквами
    } else if (c >= '0' && c <= '9') {
      // Цифры 0-9 -> код Морзе из таблицы (индексы 26-35)
      result += morseCode[26 + (c - '0')];
      result += " ";  // Пробел между буквами
    }
  }
  return result;
}

// Декодирование кода Морзе в ASCII символ
// Возвращает символ или 0, если код не найден
char morseToChar(String morse) {
  for (int i = 0; i < 36; i++) {
    if (String(morseCode[i]) == morse) {
      if (i < 26) {
        // Индексы 0-25 -> буквы A-Z
        return 'A' + i;
      } else {
        // Индексы 26-35 -> цифры 0-9
        return '0' + (i - 26);
      }
    }
  }
  return 0;  // Код не найден
}

// ============================================================================
// ФУНКЦИИ ДЛЯ 7-СЕГМЕНТНОГО ИНДИКАТОРА
// ============================================================================

// Вывод символа на 7-сегментный индикатор
// Поддерживает цифры 0-9 и буквы A-Z
void displayChar(char c) {
  int digitIndex = -1;
  
  // Определение индекса в таблице паттернов
  if (c >= '0' && c <= '9') {
    digitIndex = c - '0';
  } else if (c >= 'A' && c <= 'Z') {
    digitIndex = 10 + (c - 'A');
  } else {
    // Неизвестный символ - очистка индикатора
    digitIndex = -1;
  }
  
  // Отправка паттерна в сдвиговый регистр
  // Порядок: Q0=A, Q1=B, Q2=C, Q3=D, Q4=E, Q5=F, Q6=G, Q7=0
  digitalWrite(LATCH_PIN, LOW);
  
  if (digitIndex >= 0 && digitIndex < 36) {
    // Отправка битов в порядке Q7, Q6, Q5, Q4, Q3, Q2, Q1, Q0
    // (сдвиговый регистр принимает старший бит первым)
    for (int i = 7; i >= 0; i--) {
      shiftBit(digitPatterns[digitIndex][i]);
    }
  } else {
    // Очистка индикатора (все сегменты выключены)
    for (int i = 0; i < 8; i++) {
      shiftBit(0);
    }
  }
  
  digitalWrite(LATCH_PIN, HIGH);  // Защелка - обновление выхода
}

// Отправка одного бита в сдвиговый регистр
void shiftBit(bool val) {
  digitalWrite(DATA_PIN, val ? HIGH : LOW);
  // Импульс на CLOCK для сдвига данных
  digitalWrite(CLOCK_PIN, HIGH);
  digitalWrite(CLOCK_PIN, LOW);
}

// ============================================================================
// ОБРАБОТКА КОМАНД ЧЕРЕЗ SERIAL ПОРТ
// ============================================================================
// Команды:
// - "AUTO" -> переключение в режим TX AUTO (текст через Serial)
// - "MANUAL" -> переключение в режим TX MANUAL (кнопка)
// - В режиме AUTO: любой текст -> передача в коде Морзе
void handleSerial() {
  if (!Serial.available()) return;
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toUpperCase();
  
  if (input.length() == 0) return;
  
  // Команды переключения режимов
  if (input == "AUTO") {
    currentMode = MODE_TX_AUTO;
    manualMorse = "";  // Сброс накопленной последовательности
    manualWord = "";
    Serial.println("Mode: TX AUTO");
    Serial.println("Enter text to transmit in Morse code");
    return;
  }
  
  if (input == "MANUAL") {
    currentMode = MODE_TX_MANUAL;
    manualMorse = "";  // Сброс накопленной последовательности
    manualWord = "";
    Serial.println("Mode: TX MANUAL");
    Serial.println("Short press (100-400ms) = dot");
    Serial.println("Long press (>=400ms) = dash");
    Serial.println("LETTER_PAUSE (600ms) = next letter");
    Serial.println("WORD_PAUSE (1400ms) = send word");
    return;
  }
  
  // В AUTO режиме: любой текст -> передача в коде Морзе
  if (currentMode == MODE_TX_AUTO && !txActive) {
    startTransmission(input);
  } else if (currentMode == MODE_TX_AUTO && txActive) {
    Serial.println("TX: Wait for transmission to complete");
  }
}
