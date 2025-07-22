#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_PCF8574.h>

// --- ПИНЫ ---
// RS-485
#define RS485_RX_PIN 2
#define RS485_TX_PIN 4
#define RS485_DE_PIN 3 // Пин управления передачей

// Двигатель
#define MOTOR_RPWM_PIN 10
#define MOTOR_LPWM_PIN 11
#define MOTOR_R_EN_PIN 9
#define MOTOR_L_EN_PIN 12

// Датчики
#define HALL_DEPO_PIN A1
#define HALL_STOP_PIN A2
#define CURRENT_SENSOR_PIN A3

// Периферия
#define BUZZER_PIN 13
#define LED_GREEN_PIN 6
#define LED_RED_PIN 5

// --- КОНСТАНТЫ ---
#define MASTER_ID 0
#define MAX_SLAVES 5
#define BAUD_RATE 9600
#define RETURN_TIMEOUT 45000 // 45 секунд
#define OVERCURRENT_THRESHOLD 700 // Пороговое значение для датчика тока (подбирается экспериментально)

// --- СОСТОЯНИЯ ---
enum State {
  READY_TO_START,
  MOVING_TO_TABLE,
  ON_TABLE,
  RETURNING_TO_DEPO,
  PAUSE
};

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);
Adafruit_PCF8574 pcf;

State currentState = READY_TO_START;
int targetTable = 0;
unsigned long onTableTimestamp = 0;

// --- ФУНКЦИИ ---

void setup() {
  Serial.begin(9600);
  rs485.begin(BAUD_RATE);
  pinMode(RS485_DE_PIN, OUTPUT);
  digitalWrite(RS485_DE_PIN, LOW); // Режим приема

  pinMode(MOTOR_RPWM_PIN, OUTPUT);
  pinMode(MOTOR_LPWM_PIN, OUTPUT);
  pinMode(MOTOR_R_EN_PIN, OUTPUT);
  pinMode(MOTOR_L_EN_PIN, OUTPUT);

  pinMode(HALL_DEPO_PIN, INPUT_PULLUP);
  pinMode(HALL_STOP_PIN, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);

  if (!pcf.begin(0x20, &Wire)) {
    Serial.println("Couldn't find PCF8574");
    while (1);
  }

  // Начальная инициализация
  setMotorSpeed(0);
  digitalWrite(LED_GREEN_PIN, HIGH);
  digitalWrite(LED_RED_PIN, LOW);
  beep(200);
}

void loop() {
  handleSerial();
  handleKeyboard();

  switch (currentState) {
    case READY_TO_START:
      // Ожидание выбора стола
      break;
    case MOVING_TO_TABLE:
      if (digitalRead(HALL_STOP_PIN) == LOW) {
        setMotorSpeed(0);
        currentState = ON_TABLE;
        onTableTimestamp = millis();
        sendCommand(targetTable, "LED_ON", "GREEN");
        beep(500);
      }
      break;
    case ON_TABLE:
      if (millis() - onTableTimestamp > RETURN_TIMEOUT) {
        startReturn();
      }
      break;
    case RETURNING_TO_DEPO:
      if (digitalRead(HALL_DEPO_PIN) == LOW) {
        setMotorSpeed(0);
        currentState = READY_TO_START;
        digitalWrite(LED_GREEN_PIN, HIGH);
        digitalWrite(LED_RED_PIN, LOW);
        beep(200);
      }
      break;
    case PAUSE:
      // Двигатель остановлен, ожидание команды
      break;
  }

  checkCurrent();
}

void handleSerial() {
  if (rs485.available()) {
    String message = rs485.readStringUntil('\n');
    parseMessage(message);
  }
}

void parseMessage(String msg) {
  int firstSep = msg.indexOf('|');
  int secondSep = msg.indexOf('|', firstSep + 1);
  int thirdSep = msg.indexOf('|', secondSep + 1);

  int srcId = msg.substring(0, firstSep).toInt();
  String cmd = msg.substring(secondSep + 1, thirdSep);
  String data = msg.substring(thirdSep + 1);

  if (cmd == "CONFIRM" && data == "SWITCH" && srcId == targetTable) {
    // Ведомый подтвердил переключение стрелки, можно начинать движение
    if (currentState == MOVING_TO_TABLE) {
      setMotorSpeed(200);
    } else if (currentState == RETURNING_TO_DEPO) {
      setMotorSpeed(-200);
    }
  } else if (cmd == "SENSOR") {
    // Обработка данных с датчиков ведомых (пока не требуется)
  }
}

void handleKeyboard() {
  uint16_t buttons = pcf.read16();

  for (int i = 0; i < 8; i++) {
    if (!(buttons & (1 << i))) {
      if (i < 5) { // Кнопки 1-5
        if (currentState == READY_TO_START) {
          startMovement(i + 1);
        }
      } else if (i == 5) { // Кнопка 6 - возврат
        if (currentState == ON_TABLE) {
          startReturn();
        }
      } else if (i == 6) { // Кнопка 7 - пауза
        if (currentState != PAUSE) {
          setMotorSpeed(0);
          currentState = PAUSE;
          digitalWrite(LED_RED_PIN, HIGH);
          beep(500);
        } else {
          // Выход из паузы (пока не реализовано)
        }
      }
      delay(100); // Антидребезг
    }
  }
}

void sendCommand(int dest, String cmd, String data) {
  digitalWrite(RS485_DE_PIN, HIGH); // Режим передачи
  delay(10);
  rs485.print(String(MASTER_ID) + "|" + String(dest) + "|" + cmd + "|" + data + "\n");
  rs485.flush();
  delay(10);
  digitalWrite(RS485_DE_PIN, LOW); // Режим приема
}

void setMotorSpeed(int speed) {
  if (speed > 0) {
    analogWrite(MOTOR_RPWM_PIN, speed);
    analogWrite(MOTOR_LPWM_PIN, 0);
  } else {
    analogWrite(MOTOR_RPWM_PIN, 0);
    analogWrite(MOTOR_LPWM_PIN, -speed);
  }
}

void startMovement(int table) {
  targetTable = table;
  sendCommand(targetTable, "SWITCH", "1");
  // дождаться подтверждения
  currentState = MOVING_TO_TABLE;
  setMotorSpeed(200); // начальная скорость
}

void startReturn() {
  currentState = RETURNING_TO_DEPO;
  sendCommand(targetTable, "LED_OFF", "GREEN");
  sendCommand(targetTable, "SWITCH", "0");
  // дождаться подтверждения
  setMotorSpeed(-200); // движение назад
}

void checkCurrent() {
  int current = analogRead(CURRENT_SENSOR_PIN);
  if (current > OVERCURRENT_THRESHOLD) {
    setMotorSpeed(0);
    currentState = PAUSE;
    digitalWrite(LED_RED_PIN, HIGH);
    beep(1000);
  }
}

void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}
