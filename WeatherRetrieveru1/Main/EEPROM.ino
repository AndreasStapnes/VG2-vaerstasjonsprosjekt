bool ClaimI2C(uint32_t timeout) {
  if (I2CCLAIMNECCESARY && !I2CClaimed) {
    uint32_t StartupT = millis(); // I stor grad forbeholdt Server, der 2 atmegaer må dele I2C
    bool IterationSkip = false;
    pinMode(8, INPUT); digitalWrite(8, HIGH);
    while ((millis() - StartupT) < timeout) {
      if (!IterationSkip) {
        if (digitalRead(7) != CLAIMBOOL) {
          IterationSkip = true;
        }
      } else {
        if (digitalRead(7) == CLAIMBOOL) {
          if (!digitalRead(8)) { // Lav val på felles pin - I2C er claimet.
            IterationSkip = false;
          } else {
            I2CClaimed = true;
            pinMode(8, OUTPUT); digitalWrite(8, LOW);
            return true;
          }
        }
      }
    }
    return false;
  } else {
    I2CClaimed = true;
    return true;
  }
}

bool LeaveI2C() {
  I2CClaimed = false;
#if (I2CCLAIMNECCESARY)
  pinMode(8, INPUT); digitalWrite(8, HIGH);
#endif
  return true;
}

bool WriteBlock(uint16_t StartAddr, uint8_t *Databuf, uint16_t DataLen, uint16_t BufStartOffset = 0) {
  if (StartAddr + DataLen < EEPROMENDVAL) {
    //Serial.print("Wr:"); Serial.print(StartAddr); Serial.print(":"); Serial.print(DataLen); Serial.print(":");
    uint16_t BufAddr = BufStartOffset;
    while (BufAddr < DataLen) {
      //Serial.println();
      Wire.beginTransmission(EEPROMAddrI2C);
      Wire.write((uint16_t)((StartAddr + BufAddr) >> 8)); // Deklarere uint16_t?
      Wire.write((uint16_t)((StartAddr + BufAddr) & 0xFF));
      Wire.write(Databuf[BufAddr]);
      //Serial.print(Databuf[BufAddr]); Serial.print(":");
      BufAddr ++;
      while (((BufAddr + StartAddr) % EEPROMINTERNALSTORAGE) && BufAddr < DataLen) {
        Wire.write(Databuf[BufAddr]);
        //Serial.print(Databuf[BufAddr]); Serial.print(":");
        BufAddr ++;
      }
      Wire.endTransmission();
      for (int i = 100; i; i--) {
        delayMicroseconds(500);
        Wire.beginTransmission(EEPROMAddrI2C);
        Wire.write((uint16_t)((StartAddr + BufAddr) >> 8)); // Deklarere uint16_t?
        Wire.write((uint16_t)((StartAddr + BufAddr) & 0xFF));
        if (!Wire.endTransmission()) {
          //Serial.print("W");
          //Serial.print(i);
          break;
        }
        if (i == 1) {
          //Serial.println("F");
          return 0;
        }
      }

    }
    //Serial.println();
    return 1;
  }
  else {
    Serial.println("Not enough space");
    return 0;
  }
}

bool ReadBlock(uint16_t StartAddr, uint8_t *Databuf, uint16_t DataLen, uint8_t BufStartOffset = 0) {
  uint8_t ReadLen;
  uint8_t SeriesLen;
  uint16_t BufPointer;
  BufPointer = BufStartOffset;
  Wire.beginTransmission(EEPROMAddrI2C);
  Wire.write((uint16_t)(StartAddr >> 8)); // Deklarere uint16_t?
  Wire.write((uint16_t)(StartAddr & 0xFF));
  Wire.endTransmission(false);
  SeriesLen = (DataLen - (DataLen % 8)) / 8;
  ReadLen = DataLen % 8;
  for (int i = 0; i < SeriesLen; i++) {
    Wire.requestFrom(EEPROMAddrI2C, 8);
    while (Wire.available()) {
      Databuf[BufPointer] = Wire.read();
      //Serial.print(Databuf[BufPointer]); Serial.print("-");
      BufPointer ++;
    }
  }
  Wire.requestFrom(EEPROMAddrI2C, ReadLen);
  while (Wire.available()) {
    Databuf[BufPointer] = Wire.read();
    //Serial.print(Databuf[BufPointer]); Serial.print("-");
    BufPointer ++;
  }
  if (BufPointer == DataLen) {
    return true;
  }
}

bool CheckInternalComm() {
  uint8_t InternalComm[1] = {0};
  if (!ReadBlock(EEPROMSTARTVAL, InternalComm, 1)) { //MISLYKTES************************************
  } // Byte 1 betyr restart. Data har blitt avlest og sendt
  uint8_t InterCommReplacement[6] = {0,0,0,0,0,0};
  if (InternalComm[0] & 1) {
    WriteBlock(EEPROMSTARTVAL, InterCommReplacement, 6); // Erstatt kommbyte og registerdatalengde med 0x000000;
    uint8_t InternalLengthReplacement[2] = {0,0};
    WriteBlock(EEPROMSTARTVAL + EEPROMINTERNALSTORAGE - 2, InternalLengthReplacement, 2);
    Serial.println("Data read. Returning");
    return 0;
  }
  Serial.println("Going ahead");
  return 1;
}

uint16_t DataDump(uint16_t StartAddr) { // returnerer avslutningsaddr
  uint16_t retries = 0;
  uint64_t UnixTime = unixtime();
  // CAPTURE DATA BUS!!!! (gjort i readfunctions)
  uint8_t MaxWrittenLen[2] = {0, 0}; // MWL Array for read
  uint16_t MWL = 0; // MWL uint16_t for math
  while (retries < 6) {
    while (1) {
      uint16_t InternalAddr = StartAddr;
      InternalAddr += 2; // Avsette rom for datalengde
      //Serial.println("Time"); // Skrive ned tid
      uint8_t CLKBytes[8] = {
                        (UnixTime >> 8 * 7) & 0xFF, (UnixTime >> 8 * 6) & 0xFF, (UnixTime >> 8 * 5) & 0xFF, (UnixTime >> 8 * 4) & 0xFF,
                        (UnixTime >> 8 * 3) & 0xFF, (UnixTime >> 8 * 2) & 0xFF, (UnixTime >> 8 * 1) & 0xFF, (UnixTime >> 8 * 0) & 0xFF
      };
      if (!WriteBlock(InternalAddr, CLKBytes, 8)) { // TID -------------------------------------------------------MSB
        break;
      }
      InternalAddr += 8;

      uint8_t WDistributionLen, WDistributionStart = 0, WDistributionEnd = 0; // Skrive ned vind
      bool WDistributionStarted = 0;
      for (uint16_t i = 0; i < 256; i++) {
        if (uint16_t(Winds[2 * i]) << 8 + Winds[2 * i + 1]) {
          if (!WDistributionStarted) {
            WDistributionStarted = true;
            WDistributionStart = i;
          }
          WDistributionEnd = i;
        }
      }
      WDistributionLen = WDistributionEnd - WDistributionStart + 1;
      //Serial.println("Wind");
      uint8_t Wdists[2] = {WDistributionLen, WDistributionStart};
      if (!WriteBlock(InternalAddr, Wdists, 2)) {
        break;
      } InternalAddr += 2;
      if (!WriteBlock(InternalAddr, Winds, WDistributionLen * 2, WDistributionStart)) {
        break;
      } InternalAddr += WDistributionLen * 2;
      //Serial.println("Rain");
      if (!WriteBlock(InternalAddr, RainBits, 4)) {
        break;
      } InternalAddr += 4; // Skrive ned regn
      bool RainBreaker = 0;
      for (int i = 0; i < 30; i++) {
        if (Rain[i]) {
          uint8_t tempRainArray[1];
          tempRainArray[0] = Rain[i];
          if (!WriteBlock(InternalAddr, tempRainArray, 1)) {
            RainBreaker = 1;
            break;
          } InternalAddr ++;
        }
      }
      if (RainBreaker) {
        break;
      }
      //Serial.println("Misc");
      if (!WriteBlock(InternalAddr, Temp, sizeof(Temp))) {
        break;
      } InternalAddr += sizeof(Temp);
      if (!WriteBlock(InternalAddr, Press, sizeof(Press))) {
        break;
      } InternalAddr += sizeof(Press);
      if (!WriteBlock(InternalAddr, Humid, sizeof(Humid))) {
        break;
      } InternalAddr += sizeof(Humid);
      if (!WriteBlock(InternalAddr, Light, sizeof(Light))) {
        break;
      } InternalAddr += sizeof(Light);
      if (!WriteBlock(InternalAddr, Batt, sizeof(Batt))) {
        break;
      } InternalAddr += sizeof(Batt);
      uint8_t InternalAddresses[2] = {((uint16_t)InternalAddr - StartAddr) >> 8, (InternalAddr-StartAddr) & 0xFF};
      if (!WriteBlock(StartAddr, InternalAddresses, 2)) {
        break; // skriv ned lengden på meldingen
      }
      //Serial.println("Success!");
      if (!ReadBlock(EEPROMSTARTVAL + EEPROMINTERNALSTORAGE - 2, MaxWrittenLen, 2)) {
        break;
      }
      MWL = (MaxWrittenLen[0] << 8) + MaxWrittenLen[1];
      //Serial.println(InternalAddr);
      Serial.print("Old MWL was: ");
      Serial.println(MWL);
      if (InternalAddr > MWL) {
        MaxWrittenLen[0] = InternalAddr >> 8; MaxWrittenLen[1] = InternalAddr & 0xFF;
        if (!WriteBlock(EEPROMSTARTVAL + EEPROMINTERNALSTORAGE - 2, (MaxWrittenLen), 2)) {
          break;
        }
        Serial.print("New MWL: "); Serial.println(InternalAddr);
      }
      return InternalAddr;
    }
    delay(25 * pow(2, retries));
    retries ++;
  }
  Serial.println("Broken");
  // MISLYKTES*****************************************************
  return 0;
}
