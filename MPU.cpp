#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>

// Initialize MPU on alternative I2C address (since RTC occupies 0x68)
MPU6050 mpu(0x69); 

/*============================================
=        MPU control/status variables        =
==============================================*/
volatile bool MPUPoweredDown = false;
volatile bool MPUDataAvailable = false;
volatile bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

/*============================================
=        Orientation/Motion variables        =
==============================================*/
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector

/**
MPU interrupt service routine
**/
void dmpDataReady() {
  if(!dmpReady) return;
  MPUDataAvailable = true;
}

bool isMPUDataAvailable() {
  return MPUDataAvailable;
}

/** 
MPU chip and DMP initialization
**/
void MPUinit() {

    mpu.initialize();
    devStatus = mpu.dmpInitialize();

    mpu.setXGyroOffset(0);
    mpu.setYGyroOffset(0);
    mpu.setZGyroOffset(0);
    mpu.setZAccelOffset(100); 

    if (devStatus == 0) {
        mpu.setDMPEnabled(true);

        attachInterrupt(3, dmpDataReady, FALLING);
        mpuIntStatus = mpu.getIntStatus();

        dmpReady = true;

        packetSize = mpu.dmpGetFIFOPacketSize();
    } else {
        SerialUSB.print(F("DMP Initialization failed (code "));
        SerialUSB.print(devStatus);
        SerialUSB.println(F(")"));
    }
}

void MPUPowerUp(void) {
  if(!MPUPoweredDown) return;
  MPUPoweredDown = false;
  mpu.setSleepEnabled(false);
}

void MPUPowerDown(void) {
  if(MPUPoweredDown) return;
  MPUPoweredDown = true;
  mpu.setSleepEnabled(true);
}

String getMPUData(void) {

  String output = "";
  MPUDataAvailable = false;
    mpuIntStatus = mpu.getIntStatus();

    // get current FIFO count
    fifoCount = mpu.getFIFOCount();

    // check for overflow (this should never happen unless our code is too inefficient)
    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
        // reset so we can continue cleanly
        mpu.resetFIFO();
        //SerialUSB.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } else if (mpuIntStatus & 0x02) {
        // wait for correct available data length, should be a VERY short wait
        while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

        // read a packet from FIFO
        mpu.getFIFOBytes(fifoBuffer, packetSize);
        
        // track FIFO count here in case there is > 1 packet available
        // (this lets us immediately read more without waiting for an interrupt)
        fifoCount -= packetSize;

        // get initial world-frame acceleration, adjusted to remove gravity
        // and rotated based on known orientation from quaternion
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetAccel(&aa, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
        mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);

        output = String(aaWorld.x) + ":" + String(aaWorld.y) + ":" + String(aaWorld.z);
    }

    return output;
}
