#include <Wire.h>
//#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//#include "Adafruit_FRAM_I2C.h"

Adafruit_BME280 bme;
//RTC_PCF8523 rtc;
//Adafruit_FRAM_I2C fram     = Adafruit_FRAM_I2C();

#define AnalogDirectionPin A2
#define AnalogBatteryPin A0

#define I2CCLAIMNECCESARY true
bool I2CClaimed = false;
#define CLAIMBOOL 1 // ulikt sender, der CLAIMBOOL = 0

#define EEPROMENDVAL 32768
#define EEPROMSTARTVAL 8192
#define EEPROMAddrI2C 0x50 // 80?
#define EEPROMINTERNALSTORAGE 16 // storage for internal comm
//8192 bytes til data COMM. 16 bytes til internal storage (2 siste = tot len). 24561 bytes til data

#define CLOCKAddrI2C 0x68

#define RECALTIME 4 // hvor ofte tid skal rekalibreres. hvert 4x5s
#define DELAYMEASUREMENTS 5 // hvor mange recals mellom hver avlesning (20s med recal=4)

uint16_t TimeToNextTwentySec = 20000;

uint8_t RainBits[4]; // Signaliserer hvilken av regnbytes som er aktive
uint8_t Rain[30]; // 30 verdier
uint8_t Temp[20]; // 10 verdier, 2 bytes per
uint8_t Press[40]; // 10 verdier, 4 bytes per
uint8_t Humid[20]; // 10 verdier, 2 bytes per
uint8_t Light[40]; // 10 verdier, 4 bytes per
uint8_t Batt[20]; // 10 verdier, 2 bytes per
uint8_t Winds[512]; // Fra 0 til og med 31 er Winds[tall] målt i 10xmillisekunder. Fra 33-> er Winds[tall] målt i omdreininger- Dette gir optimal nøyaktighet. Består av 256 uint16_t verdier
uint8_t WindDir[15];

uint16_t Addr = EEPROMSTARTVAL + EEPROMINTERNALSTORAGE; // Byte 1 brukt av intern kommunikasjon mellom 2 atmega328p-er
// Byte 2 og 3 brukt for å kommunisere total datalengde

uint8_t TwentySecToTenMinCounter;
uint32_t TwentySecMillis;

uint32_t MillisWindToAdd = 0;
uint32_t MillisWindSpeed = 0;

uint32_t OffsetTime = 0;
uint8_t WakeupCorrect = 0;

bool RegisteredPrevTwentyS;

uint32_t prevReg;
uint64_t begRTCCheck;
uint64_t currRTCCheck;

void setup() {
  SMCR &= ~(1); // Våkne
  ADCSRA |= (1 << 7); // Slå på ADC
  PRR &= ~(1 << 0);
  pinMode(8, INPUT); digitalWrite(8, HIGH);
  pinMode(7, INPUT);


  Wire.begin();
  Wire.setClock(100000);
  attachInterrupt(digitalPinToInterrupt(2), AddWind, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), AddRain, FALLING);




  Serial.begin(115200);
  //if (! rtc.begin()) {
  //  Serial.println("Couldn't find RTC");
  //  while (1);
  //}
  if (!bme.begin()) {
    Serial.println("Couldn't find BME280");
    while (1);
  }
  /*  if (!fram.begin()) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
      Serial.println("Couldn't find FRAM");
      while (1);
    }*/
  //rtc.adjust(DateTime(2018, 9, 24, 17, 2, 0));
  //Serial.println("Datetimen");
  //if(!rtc.initialized()) {
  //  Serial.print("F");
  //} else {
  //  Serial.print("Y");
  //}
  //DateTime now = rtc.now();
  //UnixTime = now.unixtime();
  //FRAMDump();
  //Standby();
  delay(100);
  Wire.beginTransmission(CLOCKAddrI2C);
  Wire.write(0x0F);
  Wire.write((0b00000010 << 3));
  Wire.endTransmission();
  delay(100);
  /*uint32_t testval;

    Serial.println("Written");
    for (int j = 0; j < 10; j++) {
    ReadBMEValues(j);
    delay(300);
    }
    testval = micros();
    DataDump(1);
    Serial.println(micros() - testval);
    delay(1000);*/


  /*Wire.beginTransmission(CLOCKAddrI2C);
    Wire.write(0x01);
    Wire.endTransmission();

    Wire.requestFrom(CLOCKAddrI2C, 12);
    while(Wire.available()) Serial.println(Wire.read(), BIN);

    Serial.print("Failed");*/
  if (ClaimI2C(6000)) {
    RTCStartup(30, 48, 17, 20, 7, 19);
    begRTCCheck = unixtime();
    prevReg = mill();
    LeaveI2C();
  } else {
    Serial.println("FailedInConn");
  }

  Serial.print("Startup succ");
  Serial.print(digitalRead(8));
  delay(100);
}

//uint32_t message;

void loop() {
  //Standby();
  //PassiveFunction();
  /*for(int i = 0; i<60; i++) {
    Serial.print(RTCReadSec());
    Serial.print(":"); delayMicroseconds(400); Standby();
    }
    Serial.println();*/
  //if(message) {Serial.print((double)3600*279.4/message); Serial.print("mm/h during T = "); Serial.println(message); delay(10); message = 0;}
  WakeupSequence();
}
