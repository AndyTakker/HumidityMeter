#include <Arduino.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "TM1637tiny.hpp"

// --- Пины ATtiny13 ---
// #define PIN_CLK PB3 // CLK Дисплей
// #define PIN_DIO PB4 // DIO Дисплей
#define ADC_NUM A1 // Номер аналогового входа на PB2

// --- Калибровка ---
#define RAW_MAX 190 // Вода (0%)
#define RAW_MIN 465 // Воздух (100%)
// Инверсия логики: чем меньше значение ADC, тем больше влаги
/* 
// --- Таблица сегментов (Строго во Flash) ---
const uint8_t digitToSegment[] PROGMEM = {
    0b00111111, 0b00000110, 0b01011011, 0b01001111,
    0b01100110, 0b01101101, 0b01111101, 0b00000111,
    0b01111111, 0b01101111};

// --- Минимальный драйвер TM1637 (Без классов, прямые регистры) ---
void tm_write_byte(uint8_t b) {
  for (uint8_t i = 0; i < 8; i++) {
    PORTB &= ~(1 << PIN_CLK); // CLK Low
    // Если бит 1 -> DIO Input (pull-up), если 0 -> DIO Output (Low)
    if (b & 0x01) {
      DDRB &= ~(1 << PIN_DIO);
      PORTB |= (1 << PIN_DIO);
    } else {
      DDRB |= (1 << PIN_DIO);
      PORTB &= ~(1 << PIN_DIO);
    }
    b >>= 1;
    PORTB |= (1 << PIN_CLK); // CLK High
    _delay_us(2);            // Минимальная задержка
  }
  // ACK
  DDRB &= ~(1 << PIN_DIO); // Input
  PORTB |= (1 << PIN_CLK);
  _delay_us(2);
  PORTB &= ~(1 << PIN_CLK);
  DDRB |= (1 << PIN_DIO); // Output
}

void tm_show_number(uint8_t num) {
  // 1. Команда записи данных
  DDRB |= (1 << PIN_DIO);
  PORTB &= ~(1 << PIN_DIO); // Start
  _delay_us(2);
  tm_write_byte(0x40); // Comm1
  PORTB |= (1 << PIN_DIO);
  DDRB &= ~(1 << PIN_DIO); // Stop
  _delay_us(2);

  // 2. Адрес
  DDRB |= (1 << PIN_DIO);
  PORTB &= ~(1 << PIN_DIO); // Start
  tm_write_byte(0xC0);      // Comm2 (Addr 0)

  // 3. Данные (2 цифры)
  uint8_t tens = num / 10;
  uint8_t ones = num % 10;

  tm_write_byte(pgm_read_byte(&digitToSegment[tens]));
  tm_write_byte(pgm_read_byte(&digitToSegment[ones]));

  // Очистка лишних цифр
  tm_write_byte(0);
  tm_write_byte(0);

  PORTB |= (1 << PIN_DIO);
  DDRB &= ~(1 << PIN_DIO); // Stop
  _delay_us(2);

  // 4. Дисплей ON + Яркость
  DDRB |= (1 << PIN_DIO);
  PORTB &= ~(1 << PIN_DIO); // Start
  tm_write_byte(0x8F);      // Comm3 + Max Brightness
  PORTB |= (1 << PIN_DIO);
  DDRB &= ~(1 << PIN_DIO); // Stop
  _delay_us(2);
}
 */

 void setup() {
  DDRB = 0; // Все входы
  // Подтяжка не нужна, TM1637 сам управляет линиями
}

void loop() {
  uint16_t val = analogRead(ADC_NUM);

  // Ограничение диапазона (вместо constrain)
  if (val > RAW_MIN)
    val = RAW_MIN;
  if (val < RAW_MAX)
    val = RAW_MAX;

  // Расчет процентов (вместо map)
  // Формула: (val - RAW_MIN) * 100 / (RAW_MAX - RAW_MIN)
  // Используем uint32_t для промежуточного умножения, чтобы не потерять точность
  uint8_t hum = (uint8_t)(((uint32_t)(val - RAW_MIN) * 100) / (RAW_MAX - RAW_MIN));

  tm_show_number(hum);

  _delay_ms(500);
}