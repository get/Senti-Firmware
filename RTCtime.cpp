#include <Wire.h>
#include <Time.h> 
#include "RTCtime.h"

// M41T62 Real Time Clock
// Datasheet: http://www.st.com/web/en/resource/technical/document/datasheet/CD00019860.pdf

const char *monthNameList[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

tmElements_t tm;

void RTCinit(const char * timeString, const char * dateString) {

  getTimeFromPC(timeString);
  getDateFromPC(dateString);
  setTime(makeTime(tm));

  rtc_centiseconds_write(millis() % 1000);
  rtc_seconds_write(tm.Second);
  rtc_minutes_write(tm.Minute);
  rtc_hours_write(tm.Hour);
  rtc_date_write(tm.Day);
  rtc_month_write(tm.Month);
  rtc_year_write(tm.Year);
}

void getTimeFromPC(const char * timeString) {
  int Hour, Min, Sec;

  if (sscanf(timeString, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return;
  tm.Hour = Hour % 24;
  tm.Minute = Min % 60;
  tm.Second = Sec % 60;
}

void getDateFromPC(const char * dateString) {
  char Month_[12];
  int Day_, Year_;
  uint8_t monthIndex;

  if (sscanf(dateString, "%s %d %d", Month_, &Day_, &Year_) != 3) return;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month_, monthNameList[monthIndex]) == 0) break;
  }
  tm.Day = Day_ % 31;
  tm.Month = (monthIndex + 1) % 12;
  tm.Year = Year_ + 12;
}

String getTimeData() {
  time_t t = now();
  return String(year(t) + 30) + ":" +
         String(month(t)) + ":" +
         String(day(t)) + ":" +
         String(hour(t)) + ":" + 
         String(minute(t)) + ":" + 
         String(second(t)) + ":" + 
         String(millis() % 1000);
}

void RTCsync() {
  setTime(RTCsyncProvider());
}

time_t RTCsyncProvider() {
  tmElements_t tm;
  tm.Hour   = (int)rtc_hours_read() % 24;
  tm.Minute = (int)rtc_minutes_read() % 60;
  tm.Second = (int)rtc_seconds_read() % 60;
  tm.Day    = (int)rtc_date_read() % 31;
  tm.Month  = (int)rtc_month_read() % 12;
  tm.Year   = (int)rtc_year_read();
  return makeTime(tm);
}

/*=====  End of Main Program  ======*/


/*==========================================
=            RTC I2C Read/Write            =
==========================================*/

void rtc_write (byte address, byte data) {
  Wire.beginTransmission(RTC_I2C_ADDR); // device address
  Wire.write(address); // register address
  Wire.write(data);
  Wire.endTransmission();
}

uint32_t rtc_read (byte address) {
  Wire.beginTransmission(RTC_I2C_ADDR); // device address
  Wire.write(address); // register address
  Wire.endTransmission();

  Wire.requestFrom(RTC_I2C_ADDR, 1, true); // request 3 bytes and STOP
  return Wire.read();
}

/*=====  End of RTC I2C Read/Write  ======*/

/*==============================================
=            Time Segment Registers            =
==============================================*/

/*----------  Centiseconds  ----------*/

int rtc_centiseconds_read() {
  return convert_bcd_to_dec(rtc_read(0x00));
}

void rtc_centiseconds_write(int centiseconds) {
  rtc_write(0x00, convert_dec_to_bcd(centiseconds));
}

/*----------  Seconds  ----------*/

int rtc_seconds_read() {
  return convert_bcd_to_dec(rtc_read(0x01) & 0x7F);
}

void rtc_seconds_write(int seconds) {
  rtc_write(0x01, convert_dec_to_bcd(seconds) & 0x7F);
}

/*----------  Minutes  ----------*/

int rtc_minutes_read() {
  return convert_bcd_to_dec(rtc_read(0x02));
}

void rtc_minutes_write(int minutes) {
  rtc_write(0x02, convert_dec_to_bcd(minutes));
}

/*----------  Hours  ----------*/

int rtc_hours_read() {
  return convert_bcd_to_dec(rtc_read(0x03) & 0x3F);
}

void rtc_hours_write(int hours) {
  rtc_write(0x03, convert_dec_to_bcd(hours) & 0x3F);
}

/*----------  Day  ----------*/

int rtc_day_read() {
  return convert_bcd_to_dec(rtc_read(0x04) & 0x07);
}

void rtc_day_write(int day) {
  rtc_write(0x04, convert_dec_to_bcd(day) & 0x07);
}

/*----------  Date  ----------*/

int rtc_date_read() {
  return convert_bcd_to_dec(rtc_read(0x05) & 0x3F);
}

void rtc_date_write(int date) {
  rtc_write(0x05, convert_dec_to_bcd(date) & 0x3F);
}

/*----------  Month  ----------*/

int rtc_month_read() {
  return convert_bcd_to_dec(rtc_read(0x06) & 0x3F);
}

void rtc_month_write(int month) {
  rtc_write(0x06, convert_dec_to_bcd(month) & 0x3F);
}

/*----------  Year  ----------*/

int rtc_year_read() {
  return convert_bcd_to_dec(rtc_read(0x07) & 0x1F);
}

void rtc_year_write(int year) {
  rtc_write(0x07, convert_dec_to_bcd(year) & 0x1F);
}

/*----------  Calibration  ----------*/

int rtc_calibration_read() {
  byte val = rtc_read(0x08);
  int offset = (int) (val & 0x1F);
  if (val & 0x20 == 0x00) {
    offset *= -1;
  }
  return offset;
}

void rtc_calibration_write(int offset) {
  byte val = offset & 0x1F;
  if (offset > 0) {
    val |= 0x20;
  }
  rtc_write(0x08, val);
}

/*=====  End of Time Segment Registers  ======*/

/*========================================
=            Helper Functions            =
========================================*/

byte convert_dec_to_bcd(int dec) {
  return ((dec / 10) << 4) | (dec % 10);
}

int convert_bcd_to_dec(byte bcd) {
  return (int) (10 * ((bcd & 0xF0) >> 4) + (bcd & 0x0F));
}

void print_pretty() {
  SerialUSB.print("T_Pretty:");
  SerialUSB.print(rtc_month_read());
  SerialUSB.print("-");
  SerialUSB.print(rtc_date_read());
  SerialUSB.print("-");
  SerialUSB.print(rtc_year_read());
  SerialUSB.print(" ");
  SerialUSB.print(rtc_hours_read());
  SerialUSB.print(":");
  SerialUSB.print(rtc_minutes_read());
  SerialUSB.print(":");
  SerialUSB.print(rtc_seconds_read());
  SerialUSB.print(":");
  SerialUSB.println(rtc_centiseconds_read());
}

/*=====  End of Helper Functions  ======*/
