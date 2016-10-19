#include <SPI.h>
#include <Time.h> 
#include <Wire.h>
#include "AFE4400regs.h"
#include "RTCtime.h"
#include "MPU.h"
#include "EDA.h"
#include "PPG.h"
#include "Memory.h"

void setup() {
  analogReadResolution(12);
  
  Serial.begin(9600);
  SPI.begin();
  Wire.begin();
  // Disable PPG AFE
  digitalWrite(PIN_SS_AFE, HIGH);
  // Initialize memory chip and release SPI bus
  memInit();
  memDisable();
  // Initialize RTC with time from computer, set time sync
  RTCinit(__TIME__, __DATE__);
  setSyncProvider(RTCsyncProvider);
  // Initialize accelerometer
  MPUinit();
  // Initialize PPG AFE registers to sample at 100Hz
  AFE4400InitConfigs();
  AFE4400InitTimings100Hz();
  // Setup AFE interrupt
  attachInterrupt(PIN_ADC_RDY, sampleAFE, RISING);

  setupInternalInterrupts(true);
}

void loop() { 
  String toWrite;

  bool wrote = false;

  if(isEDADataAvailable()) {
    toWrite = "E:" + String(getEDAData());
    memWrite(toWrite.c_str()); 
    wrote = true;
  }
  
  if (isMPUDataAvailable()) {
    toWrite = "A:" + getMPUData();
    memWrite(toWrite.c_str()); 
    wrote = true;
  }
  
  if (isAFEDataAvailable()) { 
    restAFEReady();
    toWrite = "P:" + String(getPPGData());
    memWrite(toWrite.c_str()); 
    wrote = true;
  }

  if(wrote) {
    toWrite = "T:" + getTimeData();
    memWrite(toWrite.c_str());
  }
}
