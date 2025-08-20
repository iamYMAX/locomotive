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

  // (Логика сглаживания остается без изменений)
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(EMG_PIN);
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;
  average = total / numReadings;

  // --- 2. Преобразование значения в угол ---

  // (Логика преобразования остается без изменений)
  int servoAngle = map(average, EMG_MIN_THRESHOLD, EMG_MAX_THRESHOLD, 0, 180);
  servoAngle = constrain(servoAngle, 0, 180);

  // --- 3. Управление сервоприводами ---

  // Отправляем команду на все сервоприводы одновременно
  thumbServo.write(servoAngle);
  indexServo.write(servoAngle);
  middleServo.write(servoAngle);
  ringServo.write(servoAngle);
  pinkyServo.write(servoAngle);

  // --- 4. Отладка ---

  Serial.print("Raw: ");
  Serial.print(analogRead(EMG_PIN));
  Serial.print("\t Smoothed: ");
  Serial.print(average);
  Serial.print("\t Angle: ");
  Serial.println(servoAngle);

  // Небольшая задержка для стабильности
  delay(10);
}
