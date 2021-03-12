void RTCStartup(uint8_t ss, uint8_t mm, uint8_t hh, uint8_t dd, uint8_t mo, uint8_t yy) {
  Wire.beginTransmission(CLOCKAddrI2C);
  Wire.write(0x0F);
  Wire.write((3 << 3));
  Wire.endTransmission();
  
  Wire.beginTransmission(CLOCKAddrI2C);
  Wire.write(0x03);
  Wire.write(binToBCD(ss));
  Wire.write(binToBCD(mm));
  Wire.write(binToBCD(hh));
  Wire.write(binToBCD(dd));
  Wire.write(binToBCD(0));
  Wire.write(binToBCD(mo));
  Wire.write(binToBCD(yy));
  Wire.endTransmission();

  /*Wire.beginTransmission(CLOCKAddrI2C);
  Wire.write(0x02);
  Wire.write(0x00);
  Wire.endTransmission();*/
}

uint8_t binToBCD(uint8_t binval) {
  return (binval%10)+(((binval-(binval%10))/10)<<4);
}

uint8_t BCDToBin(uint8_t bcdval) {
  return (bcdval & 0x0F) + (bcdval >> 4) * 10;
}

uint8_t RTCVal[7];
uint8_t DaysInMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
#define SECONDS_FROM_1970_TO_2000 946684800

void RTCRead() {
  Wire.beginTransmission(CLOCKAddrI2C);
  Wire.write(0x03);
  Wire.endTransmission();

  Wire.requestFrom(CLOCKAddrI2C, 7);
  for(int i = 0; i<7; i++) {
    if(Wire.available()) {
      RTCVal[i] = BCDToBin(Wire.read());
      //Serial.print("-");
      //Serial.println(RTCVal[i]);
    } else {
      Serial.println("F");
      return 255;
    }
  }
}

uint8_t RTCReadSec() {
  Wire.beginTransmission(CLOCKAddrI2C);
  Wire.write(0x03);
  Wire.endTransmission();

  Wire.requestFrom(CLOCKAddrI2C, 1);
  if(Wire.available()) {
    return BCDToBin(Wire.read());
  } else {
    return 255;
  }
}

uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += DaysInMonth[i-1];
    if (m > 2 && y % 4 == 0)
        ++days;
    for (int i = 0; i < y; i++) {
      if(i % 4 == 0 && !(i%100 == 0) || i%400 == 0) {
        days ++;
      }
    }
    return days + 365 * y - 1;
}

uint64_t time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return (((uint64_t)days * 24 + h) * 60 + m) * 60 + s;
}

uint64_t unixtime() {
  uint64_t t;
  RTCRead();
  uint16_t days = date2days(RTCVal[6], RTCVal[5], RTCVal[3]);
  //Serial.print(RTCVal[0]); Serial.print("-"); Serial.print(RTCVal[1]); Serial.print("-"); Serial.print(RTCVal[2]); Serial.print("-"); Serial.print(RTCVal[3]); Serial.print("-");
  //Serial.print(RTCVal[5]); Serial.print("-"); Serial.print(RTCVal[6]); Serial.print("-");
  t = time2long(days, RTCVal[2], RTCVal[1], RTCVal[0]);
  t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

  return t;
}
