
/*============================================================================
=  GSR/EDA reading module, interfacing with an external op-amp connected to  =
=  and ADC pin on the MCU, and queried using an internal software interrupt  =
=  configured in this file.                                                  =
==============================================================================*/

bool EDADataAvailable = false;

/**
Set the interrupt flag to false and return the value read by the ADC
**/
int getEDAData() {
  EDADataAvailable = false;
  int val = analogRead(A6);
  return val;
}

/**
Return interrupt flag status
**/
bool isEDADataAvailable() {
  return EDADataAvailable;
}

/**
Setup internal interrupt for EDA reading using Atmel ARM's TC interrupt on channel 5
Use a 16-bit  counter  with  a  prescaler  value  of  64 to set the interrupt frequency
to roughly 12Hz, sufficient for EDA sampling
**/
void setupInternalInterrupts() {

  REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID ( GCM_TC4_TC5 ) ) ;
  while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // wait for sync 

  TcCount16* TC_ = (TcCount16*) TC5; // get timer struct

  TC_->CTRLA.reg &= ~TC_CTRLA_ENABLE;   // Disable TCx
  while (TC_->STATUS.bit.SYNCBUSY == 1); // wait for sync 

  TC_->CTRLA.reg |= TC_CTRLA_WAVEGEN_NFRQ; // Set TC as normal Normal Frq
  while (TC_->STATUS.bit.SYNCBUSY == 1); // wait for sync 

  TC_->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV64;   // Set perscaler
  while (TC_->STATUS.bit.SYNCBUSY == 1); // wait for sync 
  
  TC_->CC[0].reg = 0xfff;
  while (TC_->STATUS.bit.SYNCBUSY == 1); // wait for sync 
  
  TC_->INTENSET.reg = 0;              // disable all interrupts
  TC_->INTENSET.bit.OVF = 1;          // enable overfollow

  NVIC_EnableIRQ(TC5_IRQn);

  TC_->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC_->STATUS.bit.SYNCBUSY == 1); // wait for sync 

}

/**
Interrupt service routing (ISR) for EDA sampling
**/
void TC5_Handler()
{
  noInterrupts();
  TcCount16* TC = (TcCount16*) TC5; 
  if (TC->INTFLAG.bit.OVF == 1) {  
    // overflow, clear interrupt flag
    TC->INTFLAG.bit.OVF = 1; 
    EDADataAvailable = true; 
  }
  interrupts();
}


