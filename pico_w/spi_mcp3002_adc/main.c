#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

/* 
   Measurement thermistor1 temperature from MCP3002 ADC
               thermistor2 temperature by builtin ADC0
   [Connecting to MCP3002]
   GPIO 4 (pin 6) MISO/spi0_rx  to MCP3002 Dout (6) <- thermistor1
   GPIO 5 (pin 7) Chip select   to MCP3002 CS (1)
   GPIO 6 (pin 9) SCK/spi0_sclk to MCP3002 CLK (7)
   GPIO 7 (pin 10) MOSI/spi0_tx to MCP3002 Din (5)
   3.3v (pin 36) -> VDD/VREF
   GND (pin 38)  -> GND
                                   MCP3002 CH0
                                   MCP3002 CH1
   [Connecting builtin ADC0]
   ADC0 GPIO26 (pin 31)  <- Thermistor2 divide output
   3.3v        (pin 36)  -> Thermistor VDD
   GND (pin 38)          -> GND -> 10kΩ
*/

#define PIN_MISO 4
#define PIN_CS   5
#define PIN_SCK  6
#define PIN_MOSI 7
#define PIN_ADC0 26

#define SPI_PORT spi0

const uint8_t ADC_SAMPLES = 10;
// Builtin ADC0
const float ADC_VREF = 3.3;
// MCP3002 section
const uint8_t MCP3002_CMD_1 = 0b01101000;
const uint8_t MCP3002_CMD_2 = 0x00;
const uint8_t MCP_CH0 = 0;
const float MCP_VREF = 3.3;
// Thermister section
const float THERM_V = 3.3;
const float THERM_B = 3435.0;
const float THERM_R0 = 10000.0;
const float THERM_R1 = 10000.0; // Divide register 10k
const float THERM_READ_INVALID = -9999.0;
const unsigned long DELAY_TIME = 30000;

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);
    asm volatile("nop \n nop \n nop");
}

// Only MCP3002 ADC
static uint16_t analogRead(int8_t ch) {
   uint8_t cmd_first = MCP3002_CMD_1 | (ch << 4);
   uint8_t highByte;
   uint8_t lowByte;
   cs_select();
   spi_write_read_blocking(SPI_PORT, &cmd_first, &highByte, 1);
   spi_write_read_blocking(SPI_PORT, &MCP3002_CMD_2, &lowByte, 1);
   cs_deselect();
   return ((highByte & 0x03) << 8) | lowByte;
}

float getAdcVoltage() {
   uint16_t adcTotal = 0;
   for (int i = 0; i < ADC_SAMPLES; i++) {
      adcTotal += adc_read();
      sleep_ms(30);
   }   
   uint16_t adcValue = round(1.0 * adcTotal / ADC_SAMPLES);
   printf("adc0.adcValue = %d\n", adcValue);
   return ADC_VREF * adcValue / (1 << 12);
}

float getMcpAdcVoltage(uint8_t ch) {
   uint16_t adcTotal = 0;
   for (int i = 0; i < ADC_SAMPLES; i++) {
      adcTotal += analogRead(ch);
      sleep_ms(30);
   }
   uint16_t adcValue = round(1.0 * adcTotal / ADC_SAMPLES);
   printf("mcp.adcValue = %d\n", adcValue);
   return MCP_VREF * adcValue / 1024;
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

   // Initialize ADC HW
   adc_init();
   // Make sure GPIO is high-impedance, no pullups etc
   adc_gpio_init(PIN_ADC0);
   // Select ADC input 0 (GPIO26)
   adc_select_input(0);
   printf("ADC0 init.\n");

   printf("MCP3002 ADC Initialize with SPI...\n");
   // MCP3002: 3.3v use SPI0 at 100KHz.
   spi_init(SPI_PORT, 100 * 1000);
   gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
   gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
   gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
   // Make the SPI pins available to picotool
   bi_decl(bi_3pins_with_func(PIN_MISO, PIN_MOSI, PIN_SCK, GPIO_FUNC_SPI));

   // Chip select is active-low, so we'll initialise it to a driven-high state
   gpio_init(PIN_CS);
   gpio_set_dir(PIN_CS, GPIO_OUT);
   gpio_put(PIN_CS, 1);
   // Make the CS pin available to picotool
   bi_decl(bi_1pin_with_name(PIN_CS, "SPI CS"));
   printf("MCP3002 ADC Initialize with SPI...\n");

   while (1) {
      // Measure Thermister1 with MCP3002
      float outVolt = getMcpAdcVoltage(MCP_CH0);
      float temper = getThermTemp(outVolt);
      if (temper != THERM_READ_INVALID) {
         printf("MCP3002.Temper: %.1f ℃\n", temper);
      } else {
         printf("MCP3002 read invalid!\n");
      }
      sleep_ms(50);
      // Measure Thermister2 by ADC0
      outVolt = getAdcVoltage();
      temper = getThermTemp(outVolt);
      if (temper != THERM_READ_INVALID) {
         printf("ADC0.Temper: %.1f ℃\n", temper);
      } else {
         printf("ADC0 read invalid!\n");
      }
      sleep_ms(DELAY_TIME);
   }
}
