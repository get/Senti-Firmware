#ifndef RTCtime
#define RTCtime

const int RTC_I2C_ADDR = 0x68;

void RTCinit(const char * timeString, const char * dateString);
void RTCsync();
time_t RTCsyncProvider();
void rtc_write (byte address, byte data);
uint32_t rtc_read (byte address);
String getTimeData();
void getTimeFromPC(const char * timeString);
void getDateFromPC(const char * dateString);

int rtc_centiseconds_read();
void rtc_centiseconds_write(int centiseconds);
int rtc_seconds_read();
void rtc_seconds_write(int seconds);
int rtc_minutes_read();
void rtc_minutes_write(int minutes);
int rtc_hours_read();
void rtc_hours_write(int hours);
int rtc_day_read();
void rtc_day_write(int day);
int rtc_date_read();
void rtc_date_write(int date);
int rtc_month_read();
void rtc_month_write(int month);
int rtc_year_read();
void rtc_year_write(int year);
int rtc_calibration_read();
void rtc_calibration_write(int offset);

byte convert_dec_to_bcd(int dec);
int convert_bcd_to_dec(byte bcd);
void print_pretty();

#endif
