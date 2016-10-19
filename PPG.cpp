#include <SPI.h>
#include <Time.h> 
#include <TimeLib.h>  
#include <Wire.h>
#include "PPG.h"
#include "AFE4400regs.h"

volatile bool adc_ready = false;
volatile bool afe_powered_down = false;

SPISettings AFE_SPI_Settings(20000000, MSBFIRST, SPI_MODE0);

bool isAFEDataAvailable(void) {
  return adc_ready;
}

void restAFEReady(void) {
  adc_ready = false;
}

void sampleAFE(void) {
  adc_ready = true;
}

void AFEPowerDown(void) {
  if(afe_powered_down) return;
  // Power down AFE (using pin, can also do using CONTROL2 register)
  afe_powered_down = true;
  digitalWrite(PIN_AFE_PDN, LOW);
}

void AFEPowerUp(void) {
  if(!afe_powered_down) return;
  // Power down AFE (using pin, can also do using CONTROL2 register)
  digitalWrite(PIN_AFE_PDN, HIGH);
  afe_powered_down = false;
}

void disableAFE(void) {
  // Tri-State SPI, Push-Pull Driver
  AFE4400Write(CONTROL2, (1L << 17) | (1L << 11) | (1L << 10) | (1L << 8));

  // Deselect AFE
  digitalWrite(PIN_SS_AFE, HIGH);
}

void enableAFE(void) {
  // Normal operation SPI, Push-Pull Driver
  AFE4400Write(CONTROL2,(1L << 17) | (1L << 11) | (1L << 8));
}

uint32_t getPPGData(void) {
  uint32_t data_led1 = AFE4400Read(LED1ABSVAL);
  float reading_led1 = convert_ADC_to_float(data_led1);
  
  uint32_t data_led2 = AFE4400Read(LED2ABSVAL);
  float reading_led2 = convert_ADC_to_float(data_led2);

  float finalPPGValue = (reading_led1 + reading_led2)/2.0;
  uint32_t finalPPGValueHex = (data_led1 + data_led2)/0x2;

  return finalPPGValueHex;
}

/*=========================================
=            AFE4400 Functions            =
===========================================*/

void AFE4400Diagnostics (void) {
  AFE4400Write(CONTROL0, B100);
  delay(20);
  uint32_t results = AFE4400Read(DIAG);
  SerialUSB.println(results, BIN);
}

/**
Initialize device operational and transimpeadance amplifier registers.
See datasheet for default values.
**/
void AFE4400InitConfigs (void) {

  pinMode(PIN_SS_AFE, OUTPUT);
  pinMode(PIN_ADC_RDY, INPUT);

  AFE4400Write(CONTROL0, (uint32_t) B1010); // reset registers and clear timers

  // TIA Parameters (datasheet pg. 24-26)
  long AMBDAC = B0100;  // AMBDAC[3:0]: Ambient DAC value
  long STAGE2EN = B1;   // STAGE2EN: Stage 2 enable for LED 2
  long STG2GAIN = B010; //B000; // STG2GAIN[2:0]: Stage 2 gain setting
  long CF_LED = B10000; //B00000; // CF_LED[4:0]: Program CF for LEDs
  long RF_LED = B110;   //B101; // RF_LED[2:0]: Program RF for LEDs
  AFE4400Write(TIA_AMB_GAIN, (AMBDAC << 16) | (STAGE2EN << 14) | (STG2GAIN << 8) | (CF_LED << 3) | RF_LED);

  // Tri-State SPI, Push-Pull Driver
  AFE4400Write(CONTROL2,(1L << 17) | (1L << 11) | (1L << 8));
  // LED Current
  long drive_led1 = (long) (CURRENT_LED1 / 50.0 * 256.0);
  long drive_led2 = (long) (CURRENT_LED2 / 50.0 * 256.0);
  AFE4400Write(LEDCNTRL, (1L << 16) | (drive_led1 << 8) | (drive_led2));
  // internal timer ON
  AFE4400Write(CONTROL1, (1L << 8) | (1L << 1));
}

void AFE4400Write (uint8_t address, uint32_t data) {
  digitalWrite(PIN_SS_AFE, LOW);

  SPI.beginTransaction(AFE_SPI_Settings);
  SPI.transfer(address); // register address
  // transmit bytes in MSB-first order
  SPI.transfer((data >> 16) & 0xFF); // DATA[23:16]
  SPI.transfer((data >> 8) & 0xFF);  // DATA[15:8]
  SPI.transfer(data & 0xFF);         // DATA[7:0]
  SPI.endTransaction();

  digitalWrite(PIN_SS_AFE, HIGH);
}

uint32_t AFE4400Read (uint8_t address) {
  uint32_t data = 0x0;

  digitalWrite(PIN_SS_AFE, LOW);

  SPI.beginTransaction(AFE_SPI_Settings);
  SPI.transfer(address); // register address
  data |= ((uint32_t) SPI.transfer(0) << 16);  // DATA[23:16]
  data |= ((uint32_t) SPI.transfer(0) << 8);   // DATA[15:8]
  data |= (uint32_t) SPI.transfer(0);          // DATA[7:0]
  SPI.endTransaction();

  digitalWrite(PIN_SS_AFE, HIGH);

  return data;
}

/**
Converts 22-bit signed integer to float [-1.0, 1.0)
**/
float convert_ADC_to_float(uint32_t data) {
  signed long adc_s22 = (signed long) (data << 10);
  adc_s22 = adc_s22 >> 10; // fill left with 1s
  return float(adc_s22) / 2097152.0; // [-2^21, 2^21 - 1]
}

/**
Configure AFE4400 timing registers for 100Hz sample rate
**/
void AFE4400InitTimings100Hz (void) {
  // See Page 31-32 of AFE4400 Datesheet for Timings

  // Pulse Repetition Period Count (16-bits)
  // Sample Frequency = 4MHz / (PRPCOUNT + 1)
  // Must be Integral Sample Frequency
  AFE4400Write(PRPCOUNT, 0x009C3F); // 39999

  // Sample Phases
  AFE4400Write(ALED2STC,      0x000050);   // Sample ambient 2 start
  AFE4400Write(ALED2ENDC,     0x0007CE); // Sample ambient 2 end
  AFE4400Write(LED1STC,       0x002760);
  AFE4400Write(LED1ENDC,      0x002EDE);
  AFE4400Write(ALED1STC,      0x004E70);
  AFE4400Write(ALED1ENDC,     0x0055EE);
  AFE4400Write(LED2STC,       0x007580); // Sample LED2 start
  AFE4400Write(LED2ENDC,      0x007CFE); // Sample LED2 end
  
  // LED ON Phases
  AFE4400Write(LED1LEDSTC,    0x002710);
  AFE4400Write(LED1LEDENDC,   0x002EDF);
  AFE4400Write(LED2LEDSTC,    0x007530); // LED2 start
  AFE4400Write(LED2LEDENDC,   0x007CFF); // LED2 end

  // ADC Conversion Phases
  AFE4400Write(ALED2CONVST,   0x002716); // Ambient 2 convert phase start
  AFE4400Write(ALED2CONVEND,  0x004E1F); // Ambient 2 convert phase end
  AFE4400Write(LED1CONVST,    0x004E26);
  AFE4400Write(LED1CONVEND,   0x00752F);
  AFE4400Write(ALED1CONVST,   0x007536);
  AFE4400Write(ALED1CONVEND,  0x009C3F);
  AFE4400Write(LED2CONVST,    0x000006);  // LED2 convert phase start
  AFE4400Write(LED2CONVEND,   0x00270F); // LED2 convert phase end

  // ADC Reset Phases
  AFE4400Write(ADCRSTSTCT0,   0x000000);  
  AFE4400Write(ADCRSTENDCT0,  0x000005);  
  AFE4400Write(ADCRSTSTCT1,   0x002710);  
  AFE4400Write(ADCRSTENDCT1,  0x002715);  
  AFE4400Write(ADCRSTSTCT2,   0x004E20);  
  AFE4400Write(ADCRSTENDCT2,  0x004E25);   
  AFE4400Write(ADCRSTSTCT3,   0x007530);   
  AFE4400Write(ADCRSTENDCT3,  0x007535);   

  AFE4400Write(CONTROL0, 0x01); // read mode
}

