#include <SoftwareSerial.h>

// --- ID ВЕДОМОГО ---
#define SLAVE_ID 1 // Уникальный ID для каждого ведомого (1-5)

// --- ПИНЫ ---
// RS-485
#define RS485_RX_PIN 10
#define RS485_TX_PIN 11
#define RS485_DE_PIN 9 // Пин управления передачей

// Датчики Холла
#define HALL_U1_PIN 2
#define HALL_U2_PIN 4

// Управление стрелкой
#define SWITCH_PIN_1 7
#define SWITCH_PIN_2 8

// Светодиоды
#define LED1_PIN 3
#define LED2_PIN 5
#define LED3_PIN 6
#define LED4_PIN 12


// --- КОНСТАНТЫ ---
#define BAUD_RATE 9600
#define MASTER_ID 0

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

// --- ФУНКЦИИ ---

void setup() {
  Serial.begin(9600);
  rs485.begin(BAUD_RATE);
  pinMode(RS485_DE_PIN, OUTPUT);
  digitalWrite(RS485_DE_PIN, LOW); // Режим приема

  pinMode(HALL_U1_PIN, INPUT_PULLUP);
  pinMode(HALL_U2_PIN, INPUT_PULLUP);

  pinMode(SWITCH_PIN_1, OUTPUT);
  pinMode(SWITCH_PIN_2, OUTPUT);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED4_PIN, OUTPUT);
}

void loop() {
  handleSerial();
  checkSensors();
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
  int dstId = msg.substring(firstSep + 1, secondSep).toInt();
  String cmd = msg.substring(secondSep + 1, thirdSep);
  String data = msg.substring(thirdSep + 1);

  if (dstId == SLAVE_ID || dstId == 255) { // 255 - широковещательный адрес
    if (cmd == "SWITCH") {
      setSwitch(data.toInt());
      sendCommand(MASTER_ID, "CONFIRM", "SWITCH");
    } else if (cmd == "LED_ON") {
      setLed(data, HIGH);
    } else if (cmd == "LED_OFF") {
      setLed(data, LOW);
    } else if (cmd == "PING") {
      sendCommand(MASTER_ID, "PONG", "");
    }
  }
}

void sendCommand(int dest, String cmd, String data) {
  digitalWrite(RS485_DE_PIN, HIGH); // Режим передачи
  delay(10);
  rs485.print(String(SLAVE_ID) + "|" + String(dest) + "|" + cmd + "|" + data + "\n");
  rs485.flush();
  delay(10);
  digitalWrite(RS485_DE_PIN, LOW); // Режим приема
}

void checkSensors() {
  if (digitalRead(HALL_U1_PIN) == LOW) {
    sendCommand(MASTER_ID, "SENSOR", "U1");
    delay(200); // Антидребезг
  }
  if (digitalRead(HALL_U2_PIN) == LOW) {
    sendCommand(MASTER_ID, "SENSOR", "U2");
    delay(200); // Антидребезг
  }
}

void setSwitch(int position) {
  if (position == 1) {
    digitalWrite(SWITCH_PIN_1, HIGH);
    digitalWrite(SWITCH_PIN_2, LOW);
  } else {
    digitalWrite(SWITCH_PIN_1, LOW);
    digitalWrite(SWITCH_PIN_2, HIGH);
  }
}

void setLed(String led, int state) {
  if (led == "GREEN") {
    digitalWrite(LED1_PIN, state);
  } else if (led == "RED") {
    digitalWrite(LED2_PIN, state);
  }
}
