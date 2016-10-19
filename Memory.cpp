#include <Wire.h>
#include <SerialFlash.h>
#include <SPI.h>
#include "PPG.h"
#include "MPU.h"
#include "Memory.h"
#include "EDA.h"

char bufferedData[fileSizeInBytes] = "";
int bufferIndex = 0;
int memFileCounter = 0;
bool memoryChipReachedCapacity = false;
volatile bool shouldRecordData = false;

void setShouldRecordData(bool val) {
  if(val) {
    // Bring back to life all powered down devices
    MPUPowerUp();
  } else {
    MPUPowerDown();
  }
  
  shouldRecordData = val;
}

void memEnable() {
  /* 
  Disable OTHER memory module. Note that SS is automatically pulled
  high in the next call to SerialFlash.
  */
  digitalWrite(FlashChipSelect2, HIGH);
}

void memDisable() {
  digitalWrite(FlashChipSelect1, HIGH);
  digitalWrite(FlashChipSelect2, HIGH);
}

void memInit() {
  pinMode(FlashChipSelect1, OUTPUT);
  pinMode(FlashChipSelect2, OUTPUT);

  if (!SerialFlash.begin(FlashChipSelect1)) {
    memError("Unable to access SPI Flash chip");
  }

  SerialFlash.printStatus();

  SerialFlash.opendir();
  while (1) {
    char filename[64];
    unsigned long filesize;

    if (SerialFlash.readdir(filename, sizeof(filename), filesize)) {
      memFileCounter++;
    } else {
      break; // no more files
    }
  }
}

void memCreateFileNameFromCounter(char *fileName) {
  String constructedFileName = (memFileNameDefault + memFileCounter + ".txt");
  constructedFileName.toCharArray(fileName, 64);
}

void memWrite(const char *s) {  
  if(!shouldRecordData) return;
  if(strlen(bufferedData) < fileSizeInBytes*0.9) {
    strcat(bufferedData, s);
    strcat(bufferedData, "\n");
  } else {
    memCreateNewFile();
    // clear the buffer
    memset(bufferedData, 0, sizeof(bufferedData));
    memWrite(s);
  }
}

void memCreateNewFile() {
  noInterrupts();
  disableAFE();
  memEnable();

  if(memoryChipReachedCapacity) {
    memDisable();
    enableAFE();
    interrupts();
    return;
  }

  if(maxFilesToLog < memFileCounter + 1) {
    memoryChipReachedCapacity = true;
    memDisable();
    enableAFE();
    interrupts();
    memCapacityReachedChangePowerLED();
    return;
  }
  
  SerialFlashFile file;
  char fileName[64];
  memCreateFileNameFromCounter(fileName);

  while (SerialFlash.exists(fileName)) {
    memFileCounter++;
    memCreateFileNameFromCounter(fileName);
  }

  if (!SerialFlash.create(fileName, fileSizeInBytes)) {
    memError("File could not be created!");
  } else {
    memFileCounter++;
  }

  while (!SerialFlash.ready());

  file = SerialFlash.open(fileName);
  if (file) {  // true if the file exists
    file.write(bufferedData, fileSizeInBytes);
    SerialUSB.println("Wrote to new file!");
  } else memError("File could not be opened!");

  while (!SerialFlash.ready());

  memDisable();
  enableAFE();
  interrupts();
}

void memError(const char *message) {
  while (1) {
    SerialUSB.println(message);
    SerialFlash.printStatus();
    delay(2500);
  }
}

void memOutputListOfExistingFiles(void) {
  noInterrupts();
  disableAFE();
  
  SerialFlash.opendir();
  while (1) {
    char filename[64];
    unsigned long filesize;

    if (SerialFlash.readdir(filename, sizeof(filename), filesize)) {
      SerialUSB.print("  ");
      SerialUSB.print(filename);
      for (int i = 0; i < 20 - strlen(filename); i++) {
        SerialUSB.print(" ");
      }
      SerialUSB.print("  ");
      SerialUSB.print(filesize);
      SerialUSB.print(" bytes");
      SerialUSB.println();
    } else {
      break; // no more files
    }
  }

  enableAFE();
  interrupts();
}

void memCapacityReachedChangePowerLED() {
  setupInternalInterrupts(false);
}

