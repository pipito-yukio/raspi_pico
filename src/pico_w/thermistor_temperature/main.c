#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

const uint8_t ADC_SAMPLES = 10;
const float ADC_VREF = 3.3;
// Thermister section
const float THERM_V = 3.3;
const float THERM_B = 3435.0;
const float THERM_R0 = 10000.0;
const float THERM_R1 = 10000.0; // Divide register 10k
const float THERM_READ_INVALID = -9999.0;
const unsigned long DELAY_TIME = 30000;

float getVolt(uint16_t adcVal) {
   return ADC_VREF * adcVal / (1 << 12);
}

float getAdcVoltage() {
   uint16_t adcTotal = 0;
   for (int i = 0; i < ADC_SAMPLES; i++) {
      adcTotal += adc_read();
      sleep_ms(30);
   }   
   uint16_t adcValue = round(1.0 * adcTotal / ADC_SAMPLES);
   printf("adcValue = %d\n", adcValue);
   return getVolt(adcValue);
}

float getThermTemp(float outVolt) {
   double rx, xa, temp;
   printf("Therm.outVolt = %.1f\n", outVolt);
   if (outVolt < 0.001 || outVolt >= THERM_V) {
      return THERM_READ_INVALID;
   }

   rx = THERM_R1 * ((THERM_V - outVolt) / outVolt);
   xa = log(rx / THERM_R0) / THERM_B;
   printf("rx: %.5f, xa: %.5f\n", rx, xa);
   temp = (1 / (xa + 0.00336)) - 273.0;
   return (float)temp;
}

int main() {
   stdio_init_all();
   printf("ADC Example, measuring GPIO26\n");

   adc_init();

   // Make sure GPIO is high-impedance, no pullups etc
   adc_gpio_init(26);
   // Select ADC input 0 (GPIO26)
   adc_select_input(0);

   while (1) {
      // Measure Thermister
      float outVolt = getAdcVoltage();
      float temper = getThermTemp(outVolt);
      if (temper != THERM_READ_INVALID) {
         printf("Temper: %.1f ℃\n", temper);
      } else {
         printf("Thermister read invalid!\n");
      }
      sleep_ms(DELAY_TIME);
   }
}