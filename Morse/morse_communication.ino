// Пины
const int TX_PIN = 2;
const int RX_PIN = 3;
const int SENSOR_PIN = 4;
const int LATCH_PIN = 8;
const int CLOCK_PIN = 9;
const int DATA_PIN = 10;
const int LED_PIN = 13;

// Параметры Морзе
const int DOT_DURATION = 200;
const int DASH_DURATION = 600;
const int ELEM_PAUSE = 200;
const int LETTER_PAUSE = 600;
const int WORD_PAUSE = 1400;

// Маркеры START и END 
const String START_MARKER = "---.";   
const String END_MARKER = "....-";    

// Режимы
enum Mode { MODE_AUTO, MODE_MANUAL, MODE_RAW };
Mode currentMode = MODE_AUTO;

// Передача
String txBuffer = "";
String currentMorse = "";
int morseIndex = 0;
unsigned long txTimer = 0;
bool txActive = false;
bool sendingElement = false;

// MANUAL режим - формирование Морзе с кнопки
String manualMorse = "";
unsigned long buttonPressStart = 0;
unsigned long buttonReleaseTime = 0;
int lastButtonState = HIGH;
bool buttonPressed = false;

// Прием
int lastRxSignal = LOW;
unsigned long signalStart = 0;
unsigned long pauseStart = 0;
String receivedMorse = "";
String rxMessage = "";
bool gotStart = false;
bool receivingActive = false;

// Код Морзе
const char* morseCode[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
  "..-", "...-", ".--", "-..-", "-.--", "--..",
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

// 7-сегментный индикатор
// Распиновка: Q1=A, Q2=B, Q3=C, Q4=D, Q5=E, Q6=F, Q7=G, Q0=не используется
// Порядок битов в массиве: [бит7=G, бит6=F, бит5=E, бит4=D, бит3=C, бит2=B, бит1=A, бит0=0]
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
  {0,0,0,1,1,0,0,0},  // C (12): D E (упрощенная C)
  {0,1,1,1,1,0,1,0},  // d (13): B C D E G
  {1,0,0,1,1,1,1,0},  // E (14): A D E F G
  {1,0,0,0,1,1,1,0},  // F (15): A E F G
  {1,0,1,1,1,1,1,0},  // G (16): A C D E F G
  {0,1,1,0,1,1,1,0},  // H (17): B C E F G
  {0,1,1,0,0,0,0,0},  // I (18): B C
  {0,1,1,1,0,0,0,0},  // J (19): B C D
  {0,1,1,0,1,1,1,0},  // K (20): B C E F G (как H)
  {0,0,0,1,1,1,0,0},  // L (21): D E F
  {1,1,1,0,1,0,1,0},  // M (22): A B C E G
  {0,0,1,1,1,0,1,0},  // N (23): C D E G
  {1,1,1,1,1,1,0,0},  // O (24): A B C D E F
  {1,1,0,0,1,1,1,0},  // P (25): A B E F G
  {1,1,1,0,0,1,1,0},  // Q (26): A B C F G
  {1,1,0,0,1,0,1,0},  // R (27): A B E G
  {1,0,1,1,0,1,1,0},  // S (28): A C D F G (как 5)
  {0,0,0,1,1,1,1,0},  // T (29): D E F G
  {1,1,1,1,1,1,0,0},  // U (30): A B C D E F
  {1,1,1,1,0,0,0,0},  // V (31): A B C D
  {1,1,1,0,1,1,0,0},  // W (32): A B C E F
  {0,1,1,0,1,1,1,0},  // X (33): B C E F G
  {0,1,1,1,0,1,1,0},  // Y (34): B C D F G
  {1,1,0,1,1,0,1,0}   // Z (35): A B D E G
};

void setup() {
  Serial.begin(9600);
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  
  digitalWrite(TX_PIN, LOW);
  displayChar(' ');
  
  // Инициализация переменных приема
  lastRxSignal = digitalRead(RX_PIN);
  pauseStart = 0;
  receivingActive = false;
  
  Serial.println("Morse System Ready");
  Serial.println("A=Auto, M=Manual, R=Raw");
  
  // Инициализация приема
  lastRxSignal = digitalRead(RX_PIN);
  pauseStart = 0;
  receivingActive = false;
}

void loop() {
  handleSerial();
  handleReceive();
  handleTransmit();
}

void handleSerial() {
  if (!Serial.available()) return;
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toUpperCase();
  
  if (input.length() == 0) return;
  
  // Команды режима
  if (input == "A") {
    currentMode = MODE_AUTO;
    Serial.println("Mode: AUTO");
    return;
  }
  if (input == "M") {
    currentMode = MODE_MANUAL;
    manualMorse = "";
    Serial.println("Mode: MANUAL - Short press = dot, Long press = dash");
    return;
  }
  if (input == "R") {
    currentMode = MODE_RAW;
    Serial.println("Mode: RAW");
    return;
  }
  
  // Текст для передачи
  if (currentMode == MODE_AUTO) {
    if (!txActive) {
      startTransmission(input);
    } else {
      txBuffer = input;
    }
  }
}

void handleTransmit() {
  // RAW режим - прямая передача
  if (currentMode == MODE_RAW) {
    int sensor = digitalRead(SENSOR_PIN);
    digitalWrite(TX_PIN, sensor == LOW ? HIGH : LOW);
    digitalWrite(LED_PIN, sensor == LOW ? HIGH : LOW);
    return;
  }
  
  // MANUAL режим - кнопка формирует точки и тире
  if (currentMode == MODE_MANUAL && !txActive) {
    int btn = digitalRead(SENSOR_PIN);
    unsigned long now = millis();
    
    // Нажатие кнопки - только визуальная обратная связь (LED)
    if (btn == LOW && lastButtonState == HIGH) {
      buttonPressStart = now;
      buttonPressed = true;
      digitalWrite(LED_PIN, HIGH);  // Только LED, TX_PIN не трогаем
    }
    
    // Отпускание кнопки - определяем точку или тире
    if (btn == HIGH && lastButtonState == LOW && buttonPressed) {
      unsigned long pressDuration = now - buttonPressStart;
      buttonPressed = false;
      buttonReleaseTime = now;
      digitalWrite(LED_PIN, LOW);
      
      // Определение точки или тире (более гибкие пороги)
      if (pressDuration >= 400) {  // Тире: >= 400мс
        manualMorse += '-';
        Serial.print('-');
      } else if (pressDuration >= 100) {  // Точка: >= 100мс
        manualMorse += '.';
        Serial.print('.');
      }
    }
    
    // Длинная пауза после последнего отпускания - отправка символа
    if (btn == HIGH && manualMorse.length() > 0 && buttonReleaseTime > 0 && (now - buttonReleaseTime) > WORD_PAUSE) {
      // Декодирование и отправка
      char decoded = morseToChar(manualMorse);
      if (decoded != 0) {
        String text = String(decoded);
        startTransmission(text);  // Отправка через стандартную функцию с маркерами
        Serial.print(" -> ");
        Serial.println(decoded);
      } else {
        Serial.println(" -> ?");
      }
      manualMorse = "";
      buttonReleaseTime = 0;
    }
    
    lastButtonState = btn;
  }
  
  // Выполнение передачи Морзе
  if (txActive) {
    unsigned long now = millis();
    
    if (morseIndex >= currentMorse.length()) {
      txActive = false;
      digitalWrite(TX_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      return;
    }
    
    char elem = currentMorse[morseIndex];
    
    if (!sendingElement) {
      if (now < txTimer) return;
      
      if (elem == '.') {
        digitalWrite(TX_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        txTimer = now + DOT_DURATION;
        sendingElement = true;
      } else if (elem == '-') {
        digitalWrite(TX_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        txTimer = now + DASH_DURATION;
        sendingElement = true;
      } else if (elem == ' ') {
        txTimer = now + LETTER_PAUSE;
        morseIndex++;
      } else {
        morseIndex++;
      }
    } else {
      if (now >= txTimer) {
        digitalWrite(TX_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        morseIndex++;
        sendingElement = false;
        txTimer = now + ELEM_PAUSE;
      }
    }
  }
}

void startTransmission(String text) {
  if (txActive) return;
  
  // Формируем кадр: START маркер + пробел + текст в Морзе + пробел + END маркер
  // Маркеры уже в формате Морзе (---. и ....-)
  currentMorse = START_MARKER;  // Начинаем с START маркера
  currentMorse += " ";          // Пауза после START
  currentMorse += textToMorse(text);  // Текст в Морзе
  currentMorse += " ";          // Пауза перед END
  currentMorse += END_MARKER;   // END маркер
  
  morseIndex = 0;
  txActive = true;
  sendingElement = false;
  txTimer = 0;
  
  Serial.print("TX: ");
  Serial.print(text);
  Serial.print(" | Frame: [");
  Serial.print(START_MARKER);
  Serial.print("] [");
  Serial.print(text);
  Serial.print("] [");
  Serial.print(END_MARKER);
  Serial.print("] | Morse: ");
  Serial.println(currentMorse);
}

String textToMorse(String text) {
  String result = "";
  for (int i = 0; i < text.length(); i++) {
    char c = text.charAt(i);
    if (c == ' ') {
      result += " ";
    } else if (c >= 'A' && c <= 'Z') {
      result += morseCode[c - 'A'];
      result += " ";
    } else if (c >= '0' && c <= '9') {
      result += morseCode[26 + (c - '0')];
      result += " ";
    }
  }
  return result;
}

void handleReceive() {
  if (currentMode == MODE_RAW) return;
  
  int signal = digitalRead(RX_PIN);
  unsigned long now = millis();
  
  // Обнаружение изменения сигнала
  if (signal != lastRxSignal) {
    if (lastRxSignal == LOW && signal == HIGH) {
      // Начало импульса
      signalStart = now;
      receivingActive = true;
    } else if (lastRxSignal == HIGH && signal == LOW) {
      // Конец импульса - измеряем длительность
      unsigned long duration = now - signalStart;
      pauseStart = now;
      
      if (duration > 50) { // Фильтр шума
        if (duration >= 400) { // Тире (>= 400мс)
          receivedMorse += '-';
          Serial.print('-');
        } else if (duration >= 100) { // Точка (>= 100мс)
          receivedMorse += '.';
          Serial.print('.');
        }
      }
    }
    lastRxSignal = signal;
  }
  
  // Обработка длительной паузы (конец символа или маркера)
  static unsigned long lastProcessTime = 0;
  if (lastRxSignal == LOW && receivingActive && pauseStart > 0 && (now - pauseStart) > LETTER_PAUSE) {
    if (receivedMorse.length() > 0 && (now - lastProcessTime) > LETTER_PAUSE) {
      processMorse();
      receivedMorse = "";
      pauseStart = 0;
      lastProcessTime = now;
    }
  }
}

void processMorse() {
  Serial.print(" [");
  Serial.print(receivedMorse);
  Serial.print("] (len=");
  Serial.print(receivedMorse.length());
  Serial.print(")");
  
  // Проверка на START маркер (5 тире)
  if (receivedMorse == START_MARKER) {
    gotStart = true;
    rxMessage = "";
    Serial.println(" -> START");
    return;
  }
  
  // Проверка на END маркер (4 тире)
  if (receivedMorse == END_MARKER) {
    if (gotStart && rxMessage.length() > 0) {
      Serial.print(" -> END. Message: ");
      Serial.println(rxMessage);
      // Показать последний символ сообщения
      if (rxMessage.length() > 0) {
        displayChar(rxMessage.charAt(rxMessage.length() - 1));
      }
    } else {
      Serial.println(" -> END (no message)");
    }
    gotStart = false;
    rxMessage = "";
    return;
  }
  
  // Обработка символов внутри кадра
  if (gotStart) {
    char decoded = morseToChar(receivedMorse);
    if (decoded != 0) {
      rxMessage += decoded;
      Serial.print(" -> ");
      Serial.print(decoded);
      Serial.print(" -> Displaying...");
      displayChar(decoded);
      Serial.println(" [OK]");
    } else {
      Serial.print(" -> ? (unknown: ");
      Serial.print(receivedMorse);
      Serial.println(")");
    }
  } else {
    Serial.print(" -> (waiting for START, marker=");
    Serial.print(START_MARKER);
    Serial.println(")");
  }
}

char morseToChar(String morse) {
  for (int i = 0; i < 36; i++) {
    if (String(morseCode[i]) == morse) {
      if (i < 26) return 'A' + i;
      else return '0' + (i - 26);
    }
  }
  return 0;
}

void displayChar(char c) {
  int digitIndex = -1;
  if (c >= '0' && c <= '9') {
    digitIndex = c - '0';
  } else if (c >= 'A' && c <= 'Z') {
    digitIndex = 10 + (c - 'A');
  } else {
    // Пусто - все сегменты выключены
    digitalWrite(LATCH_PIN, LOW);
    for (int i = 7; i >= 0; i--) {
      shiftBit(0);
    }
    digitalWrite(LATCH_PIN, HIGH);
    return;
  }
  
  // Отправка паттерна: порядок битов [G F E D C B A 0] (биты 7-0)
  // Q7=G, Q6=F, Q5=E, Q4=D, Q3=C, Q2=B, Q1=A, Q0=не используется
  digitalWrite(LATCH_PIN, LOW);
  for (int i = 7; i >= 0; i--) {
    shiftBit(digitPatterns[digitIndex][i]);
  }
  digitalWrite(LATCH_PIN, HIGH);
}

void shiftBit(bool val) {
  digitalWrite(DATA_PIN, val ? HIGH : LOW);
  digitalWrite(CLOCK_PIN, HIGH);
  digitalWrite(CLOCK_PIN, LOW);
}
