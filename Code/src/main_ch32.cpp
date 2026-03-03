#include "TM1637Display.h"
#include <Logs.h>
#include <SysClock.h>
#include <ch32Pins.hpp>
#include <debug.h>

// Module connection pins (Digital Pins)
#define CLK PC5
#define DIO PC6

// The amount of time (in milliseconds) between tests
#define TEST_DELAY 1000

const uint8_t SEG_DONE[] = {
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,         // d
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, // O
    SEG_C | SEG_E | SEG_G,                         // n
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G          // E
};

void ADC1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
/*********************************************************************
 * @fn      ADC1_IRQHandler
 *
 * @brief   This function handles analog wathdog exception.
 *
 * @return  none
 */
void ADC1_IRQHandler(void) {
  if (ADC_GetITStatus(ADC1, ADC_IT_AWD)) {
    printf("Enter AnalogWatchdog Interrupt\r\n");
  }

  ADC_ClearITPendingBit(ADC1, ADC_IT_AWD);
}

/*********************************************************************
 * @fn      ADC_Function_Init
 *
 * @brief   Initializes ADC collection.
 *
 * @return  none
 */
void ADC_Function_Init(void) {
  ADC_InitTypeDef ADC_InitStructure = {0};
  GPIO_InitTypeDef GPIO_InitStructure = {0};
  // NVIC_InitTypeDef NVIC_InitStructure = {0};

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  RCC_ADCCLKConfig(RCC_PCLK2_Div8);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  ADC_DeInit(ADC1);
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);

  ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_241Cycles);

  // /* Higher Threshold:900, Lower Threshold:500 */
  // ADC_AnalogWatchdogThresholdsConfig(ADC1, 900, 500);
  // ADC_AnalogWatchdogSingleChannelConfig(ADC1, ADC_Channel_2);
  // ADC_AnalogWatchdogCmd(ADC1, ADC_AnalogWatchdog_SingleRegEnable);

  // NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
  // NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  // NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  // NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  // NVIC_Init(&NVIC_InitStructure);

  ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);
  ADC_ITConfig(ADC1, ADC_IT_AWD, ENABLE);
  ADC_Cmd(ADC1, ENABLE);

  ADC_ResetCalibration(ADC1);
  while (ADC_GetResetCalibrationStatus(ADC1))
    ;
  ADC_StartCalibration(ADC1);
  while (ADC_GetCalibrationStatus(ADC1))
    ;
}

/*********************************************************************
 * @fn      Get_ADC_Val
 *
 * @brief   Returns ADCx conversion result data.
 *
 * @param   ch - ADC channel.
 *            ADC_Channel_0 - ADC Channel0 selected.
 *            ADC_Channel_1 - ADC Channel1 selected.
 *            ADC_Channel_2 - ADC Channel2 selected.
 *            ADC_Channel_3 - ADC Channel3 selected.
 *            ADC_Channel_4 - ADC Channel4 selected.
 *            ADC_Channel_5 - ADC Channel5 selected.
 *            ADC_Channel_6 - ADC Channel6 selected.
 *            ADC_Channel_7 - ADC Channel7 selected.
 *            ADC_Channel_8 - ADC Channel8 selected.
 *            ADC_Channel_9 - ADC Channel9 selected.
 *
 * @return  none
 */
u16 Get_ADC_Val(u8 ch) {
  u16 val;

  ADC_SoftwareStartConvCmd(ADC1, ENABLE);

  while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
    ;

  val = ADC_GetConversionValue(ADC1);

  return val;
}

TM1637Display display(CLK, DIO);

int main() {

  SystemCoreClockUpdate();
#ifdef LOG_ENABLE
  USART_Printf_Init(115200);
#endif
  logs("SystemClk:%ld\r\n", SystemCoreClock);
  logs("   ChipID:0x%08lx\r\n", DBGMCU_GetCHIPID());

  // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  ADC_Function_Init();
  u16 ADC_val;

  // Display test
  // int k;
  uint8_t data[] = {0xff, 0xff, 0xff, 0xff};
  display.setBrightness(0x0f);

  // All segments on
  display.setSegments(data);
  delay(TEST_DELAY);
  /*
    // Selectively set different digits
    data[0] = display.encodeDigit(0);
    data[1] = display.encodeDigit(1);
    data[2] = display.encodeDigit(2);
    data[3] = display.encodeDigit(3);
    display.setSegments(data);
    delay(TEST_DELAY);

    display.clear();
    display.setSegments(data + 2, 2, 2);
    delay(TEST_DELAY);

    display.clear();
    display.setSegments(data + 2, 2, 1);
    delay(TEST_DELAY);

    display.clear();
    display.setSegments(data + 1, 3, 1);
    delay(TEST_DELAY);

    // Show decimal numbers with/without leading zeros
    display.showNumberDec(0, false); // Expect: ___0
    delay(TEST_DELAY);
    display.showNumberDec(0, true); // Expect: 0000
    delay(TEST_DELAY);
    display.showNumberDec(1, false); // Expect: ___1
    delay(TEST_DELAY);
    display.showNumberDec(1, true); // Expect: 0001
    delay(TEST_DELAY);
    display.showNumberDec(301, false); // Expect: _301
    delay(TEST_DELAY);
    display.showNumberDec(301, true); // Expect: 0301
    delay(TEST_DELAY);
    display.clear();
    display.showNumberDec(14, false, 2, 1); // Expect: _14_
    delay(TEST_DELAY);
    display.clear();
    display.showNumberDec(4, true, 2, 2); // Expect: __04
    delay(TEST_DELAY);
    display.showNumberDec(-1, false); // Expect: __-1
    delay(TEST_DELAY);
    display.showNumberDec(-12); // Expect: _-12
    delay(TEST_DELAY);
    display.showNumberDec(-999); // Expect: -999
    delay(TEST_DELAY);
    display.clear();
    display.showNumberDec(-5, false, 3, 0); // Expect: _-5_
    delay(TEST_DELAY);
    display.showNumberHexEx(0xf1af); // Expect: f1Af
    delay(TEST_DELAY);
    display.showNumberHexEx(0x2c); // Expect: __2C
    delay(TEST_DELAY);
    display.showNumberHexEx(0xd1, 0, true); // Expect: 00d1
    delay(TEST_DELAY);
    display.clear();
    display.showNumberHexEx(0xd1, 0, true, 2); // Expect: d1__
    delay(TEST_DELAY);

    // Run through all the dots
    for (k = 0; k <= 4; k++) {
      display.showNumberDecEx(0, (0x80 >> k), true);
      delay(TEST_DELAY);
    }

    // Brightness Test
    for (k = 0; k < 4; k++)
      data[k] = 0xff;
    for (k = 0; k < 7; k++) {
      display.setBrightness(k);
      display.setSegments(data);
      delay(TEST_DELAY);
    }

    // On/Off test
    for (k = 0; k < 4; k++) {
      display.setBrightness(7, false); // Turn off
      display.setSegments(data);
      delay(TEST_DELAY);
      display.setBrightness(7, true); // Turn on
      display.setSegments(data);
      delay(TEST_DELAY);
    }
   */
  // Done!
  display.setSegments(SEG_DONE);

  while (1) {
    ADC_val = Get_ADC_Val(ADC_Channel_2);
    delay(500);
    logs("%04d\r\n", ADC_val);
    display.showNumberDec(ADC_val, false, 4, 0);
    delay(2);
  }
}
