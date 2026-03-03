//============================================================= (c) A.Kolesov ==
// Измерение влажности почвы с помощью аналогового датчика HW-390
// Поскольку точность датчика - лаптем по карте, цифровая индикация смысла не имеет
// Будем отображать светодиодами три уровня: низкий/в норме/высокий.
// Техническая реализация:
// - делаем на Attiny85, отлаживаемся и калибруем через SoftwareSerial и прошиваем
//   через сервисный интерфейс Arduino IDE
// - затем все переносим  на Attiny13
//
// Напоминалка, как работать с Digispark + Micronucleus
//  1. Компилируем и загружаем код в VSCode+PlatformIO
//  2. В момент загузки, по подсказке VSCode, подключаем Digispark к USB
//  3. После прошивки отключаем от USB.
//  4. Подключаем конвертор USB-UART к пинам Digispark (RX/TX SoftwareSerial)
//  5. Отлаживаемся через Serial Monitor и print(). Одновременно USB и конвертор
//     не работают.
//  6. Итоговую прошивку записываем в контроллер програматором.
// Если используется LED_DISPLAY_ENABLED, на момент прошивки его надо отключать.
// Светодиоды на шинах дисплея не мешают дисплею.
//
// Для большинства растений в обычном грунте актуальны следующие значения:
// 0% – 15%:    Критически сухо. Почва пылеобразная, растения в стадии завядания. Срочный полив.
// 15% – 30%:   Сухо. Почва рассыпается, не держит форму. Подходит только для кактусов и суккулентов.
// 30% – 60%:   Оптимально (зона комфорта). Почва влажная на ощупь, легко лепится в комок. Идеально для большинства комнатных растений и овощей.
// 60% – 85%:   Очень влажно. Почва мажется, комок при нажатии выделяет влагу. Подходит для влаголюбивых культур (например, огурцы или капуста в период роста).
// 85% – 100%:  Переувлажнение (болото). Корни могут начать гнить из-за нехватки кислорода.
//
// Рекомендации для конкретных групп растений:
// Суккуленты	      15% – 25%	Алоэ, кактусы, толстянка
// Декоративные	    35% – 50%	Фикусы, монстера, пальмы
// Овощные культуры 45% – 70%	Томаты, перец, зелень
// Влаголюбивые	    60% – 85%	Огурцы, циперус, папоротники
//------------------------------------------------------------------------------
#include <Arduino.h>
#ifndef ATtiny13
#include <Soft_Serial.h>
#endif
#ifdef LED_DISPLAY_ENABLED
#include <TM1637Display.h>
#endif

// Конфигурируем железо
#define pinRx PB0     // Вывод RX для serial
#define pinTx PB1     // Вывод TX для serial
#define pinSensor PB2 // Вход датчика влажности
#ifdef ATtiny13
#define ADC_NUM A1 // Номер аналогового входа на PB2
#else
#define ADC_NUM 1 // Номер аналогового входа на PB2
#endif

#ifdef LED_DISPLAY_ENABLED
#define CLK PB3
#define DIO PB4
#else
#define pinLedLow PB3  // Светодиод низкого уровня влажности
#define pinLedHigh PB4 // Светодиод высокого уровня влажности
#endif

// Зададим пороговые значения влажности
#define RAW_SENSOR_100 190 // Абсолютные показания датчика влажности при 100% (в воде)
#define RAW_SENSOR_0 465   // Абсолютные показания датчика влажности при 0% (в воздухе)
#define HUMIDITY_LOW 30    // Нижняя граница влажности (%)
#define HUMIDITY_HIGH 65   // Верхняя граница влажности (%)

#define SCAN_PERIOD 500 // Период сканирования датчика (ms)

// Управление светодиодными индикаторам верхней и нижней границы
#define LOW_ON() digitalWrite(pinLedLow, HIGH)
#define LOW_OFF() digitalWrite(pinLedLow, LOW)
#define HIGH_ON() digitalWrite(pinLedHigh, HIGH)
#define HIGH_OFF() digitalWrite(pinLedHigh, LOW)

#ifndef ATtiny13
// Создадим последовательный порт, через который будем работать
SoftwareSerial softSerial(pinRx, pinTx);
#endif

#ifdef LED_DISPLAY_ENABLED
TM1637Display display(CLK, DIO);
#endif

void printLog(uint16_t hm) {
#ifndef ATtiny13
  char str[10];
  sprintf(str, "Humidity: %d%%\r\n", hm);
  softSerial.print(str);
#endif

#ifdef LED_DISPLAY_ENABLED
  display.showNumberDec(hm, false, 4, 0);
#endif
}

//------------------------------------------------------------------------------
void setup() {
#ifndef ATtiny13
  softSerial.begin(9600);
#endif

  pinMode(pinSensor, INPUT);

#ifdef LED_DISPLAY_ENABLED
  display.setBrightness(0x0f);
#else
  pinMode(pinLedLow, OUTPUT);
  pinMode(pinLedHigh, OUTPUT);

  for (int i = 0; i < 3; i++) { // Помигаем как признак жизни
    LOW_ON();
    delay(500);
    LOW_OFF();
    HIGH_ON();
    delay(500);
    HIGH_OFF();
  }
  LOW_OFF();
  HIGH_OFF();
#endif
}

//==============================================================================
// Каждые N миллисекунд читаем показания датчика влажности и выводим
// уровень светодиодами.
//------------------------------------------------------------------------------
void loop() {
  uint16_t hm = 0;                                    // Значение влажности
  hm = analogRead(ADC_NUM);                           // Абсолютные показания датчика
  hm = constrain(hm, (uint16_t)RAW_SENSOR_100, (uint16_t)RAW_SENSOR_0);   // Ограничим возможные всплески выхода за диапазон
  hm = map(hm, RAW_SENSOR_0, RAW_SENSOR_100, 0, 100); // Конвертируем в %
  printLog(hm);

#ifndef LED_DISPLAY_ENABLED
  // Покажем светодиодами уровень влажности
  if (hm < HUMIDITY_LOW) { // Влажность ниже нормы
    LOW_ON();
    HIGH_OFF();
  } else if (hm > HUMIDITY_HIGH) { // Влажность выше нормы
    LOW_OFF();
    HIGH_ON();
  } else { // Влажность в норме
    LOW_ON();
    HIGH_ON();
  }
#endif

  delay(SCAN_PERIOD);
}