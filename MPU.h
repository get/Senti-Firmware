#ifndef MPU
#define MPU

const int MPUInterruptPin = 3;

void dmpDataReady();
void MPUinit();
String getMPUData(void);
bool isMPUDataAvailable();
void MPUPowerDown(void);
void MPUPowerUp(void);

#endif
