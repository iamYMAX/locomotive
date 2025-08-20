/*
 * Прототип управления одним пальцем кисти-протеза
 *
 * Этот код считывает данные с аналогового ЭМГ-датчика,
 * сглаживает их и преобразует в угол поворота сервопривода,
 * который управляет движением одного пальца.
 *
 * Платформа: ESP32
 */

#include <ESP32Servo.h>
#include "config.h"

// Создаем объекты сервоприводов для каждого пальца
Servo thumbServo;
Servo indexServo;
Servo middleServo;
Servo ringServo;
Servo pinkyServo;

// Переменные для сглаживания сигнала (простое скользящее среднее)
const int numReadings = 10;
int readings[numReadings];      // Массив для хранения последних показаний
int readIndex = 0;              // Индекс текущего показания
long total = 0;                 // Сумма показаний
int average = 0;                // Среднее значение

// Перечисление для хранения текущего жеста
enum Gesture { OPEN, PINCH, FIST };
Gesture currentGesture = OPEN;

void setup() {
  // Инициализация последовательного порта для отладки
  Serial.begin(115200);
  Serial.println("--- Prosthetic Hand 5-Finger Prototype ---");

  // Подключение сервоприводов к пинам
  thumbServo.attach(SERVO_THUMB_PIN);
  indexServo.attach(SERVO_INDEX_PIN);
  middleServo.attach(SERVO_MIDDLE_PIN);
  ringServo.attach(SERVO_RING_PIN);
  pinkyServo.attach(SERVO_PINKY_PIN);

  // Инициализация пина датчика
  pinMode(EMG_PIN, INPUT);

  // Заполняем массив начальными значениями
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }

  Serial.println("Setup complete. Ready to read EMG signals.");
}

void loop() {
  // --- 1. Чтение и сглаживание сигнала ---
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(EMG_PIN);
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;
  average = total / numReadings;

  // --- 2. Определение и выполнение жеста ---
  if (average < EMG_THRESHOLD_PINCH) {
    if (currentGesture != OPEN) {
      currentGesture = OPEN;
      setOpenPalm();
      Serial.println("Gesture: OPEN");
    }
  } else if (average < EMG_THRESHOLD_FIST) {
    if (currentGesture != PINCH) {
      currentGesture = PINCH;
      setPinch();
      Serial.println("Gesture: PINCH");
    }
  } else {
    if (currentGesture != FIST) {
      currentGesture = FIST;
      setFist();
      Serial.println("Gesture: FIST");
    }
  }

  // --- 3. Отладка ---
  Serial.print("Smoothed: ");
  Serial.println(average);

  // Небольшая задержка для стабильности
  delay(50); // Увеличим задержку, т.к. команды отправляются реже
}

// --- Функции для управления жестами ---

// Устанавливает все пальцы в одно положение
void setAllFingers(int angle) {
  thumbServo.write(angle);
  indexServo.write(angle);
  middleServo.write(angle);
  ringServo.write(angle);
  pinkyServo.write(angle);
}

// Жест: Открытая ладонь
void setOpenPalm() {
  setAllFingers(SERVO_ANGLE_OPEN);
}

// Жест: Кулак
void setFist() {
  setAllFingers(SERVO_ANGLE_CLOSED);
}

// Жест: Щипок
void setPinch() {
  thumbServo.write(SERVO_ANGLE_PINCH_THUMB);
  indexServo.write(SERVO_ANGLE_PINCH_INDEX);
  // Остальные пальцы остаются открытыми
  middleServo.write(SERVO_ANGLE_OPEN);
  ringServo.write(SERVO_ANGLE_OPEN);
  pinkyServo.write(SERVO_ANGLE_OPEN);
}
