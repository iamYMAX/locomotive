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

// Создаем объект сервопривода
Servo fingerServo;

// Переменные для сглаживания сигнала (простое скользящее среднее)
const int numReadings = 10;
int readings[numReadings];      // Массив для хранения последних показаний
int readIndex = 0;              // Индекс текущего показания
long total = 0;                 // Сумма показаний
int average = 0;                // Среднее значение

void setup() {
  // Инициализация последовательного порта для отладки
  Serial.begin(115200);
  Serial.println("--- Prosthetic Hand Finger Prototype ---");

  // Подключение сервопривода к пину
  fingerServo.setPeriodHertz(50); // Стандартная частота для сервоприводов
  fingerServo.attach(SERVO_PIN, 500, 2500); // Подключаем пин, указываем диапазон ширины импульса

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

  // Вычитаем последнее значение
  total = total - readings[readIndex];
  // Считываем новое значение с датчика
  readings[readIndex] = analogRead(EMG_PIN);
  // Добавляем новое значение к сумме
  total = total + readings[readIndex];
  // Переходим к следующему индексу
  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  // Рассчитываем среднее
  average = total / numReadings;

  // --- 2. Преобразование значения в угол ---

  // Преобразуем среднее значение (от EMG_MIN до EMG_MAX) в угол (от 0 до 180)
  // EMG_MIN - значение в покое, EMG_MAX - при максимальном напряжении мышцы
  int servoAngle = map(average, EMG_MIN_THRESHOLD, EMG_MAX_THRESHOLD, 0, 180);

  // Ограничиваем угол, чтобы избежать выхода за пределы
  servoAngle = constrain(servoAngle, 0, 180);

  // --- 3. Управление сервоприводом ---

  fingerServo.write(servoAngle);

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
