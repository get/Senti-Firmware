#ifndef MEMORY_H
#define MEMORY_H

const int fileSizeInBytes = 16384; // keep this well below 20K as the total sram is 32K
const int FlashChipSelect1 = 7;
const int FlashChipSelect2 = 6;
const String memFileNameDefault = "r";
const int maxChipCapacity = 67108864; // taken from SerialFlash.capacity(id)
const int maxFilesToLog = 3900; // maxChipCapacity/fileSizeInBytes

void memEnable();
void memDisable();
void memInit();
void memError(const char *message);

void memCreateFileNameFromCounter(char *fileName);
void memCreateNewFile();
void memOutputListOfExistingFiles(void);
void memWrite(const char *s);
void memCapacityReachedChangePowerLED();
void setShouldRecordData(bool val);

#endif
