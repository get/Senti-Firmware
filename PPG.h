#ifndef PPGFNS
#define PPGFNS

const int PIN_SS_AFE = 4; 
const int PIN_ADC_RDY = 5;
const int PIN_AFE_PDN = 1;

// LED current in mA; 50mA max; typically <20mA
const float CURRENT_LED1 = 15.0;
const float CURRENT_LED2 = 15.0;

void AFE4400InitConfigs (void);
void AFE4400InitTimings100Hz (void);
void AFE4400Write (uint8_t address, uint32_t data);
uint32_t AFE4400Read (uint8_t address);
void enableAFE(void);
void disableAFE(void);
float convert_ADC_to_float(uint32_t data);
void sampleAFE(void);
bool isAFEDataAvailable(void);
void restAFEReady(void);
uint32_t getPPGData(void);
void AFEPowerUp(void);
void AFEPowerDown(void);


#endif
