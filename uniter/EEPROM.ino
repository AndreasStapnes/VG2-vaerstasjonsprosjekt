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
  if (I2CCLAIMNECCESARY) {
    pinMode(8, INPUT); digitalWrite(8, HIGH);
  }
  return true;
}

bool StoreMessageEEPROM(uint8_t *Databuf) {
  if (ClaimI2C(SeriesTransmissionTimeout)) {
    if (PackPosition < MaxPosition) {
      if (WriteBlock(bitloadDataAddr + PackPosition * DataPackSize, Databuf, DataPackSize)) {
        PackPosition ++;
        return 1;
      } else {
        return 0;
      }
    }
    LeaveI2C();
  } else {
    return 0;
  }
}

bool WriteBlock(uint16_t StartAddr, uint8_t *Databuf, uint16_t DataLen) {
  //Serial.print("Wr:"); Serial.print(StartAddr); Serial.print(":"); Serial.print(DataLen); Serial.print(":");
  uint16_t BufAddr = 0;
  while (BufAddr < DataLen) {
    Wire.beginTransmission(EEPROMAddrI2C);
    Wire.write((uint16_t)((StartAddr + BufAddr) >> 8)); // Deklarere uint16_t?
    Wire.write((uint16_t)((StartAddr + BufAddr) & 0xFF));
    Wire.write(Databuf[BufAddr]);
    BufAddr ++;
    while (((BufAddr + StartAddr) % EEPROMINTERNALSTORAGE) && BufAddr < DataLen) {
      Wire.write(Databuf[BufAddr]);
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
        break;
      }
      if (i == 1) {
        return 0;
      }
    }
  }
  return 1;
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
      Serial.print(Databuf[BufPointer]); Serial.print("-");
      BufPointer ++;
    }
  }
  Wire.requestFrom(EEPROMAddrI2C, ReadLen);
  while (Wire.available()) {
    Databuf[BufPointer] = Wire.read();
    BufPointer ++;
  }
  if (BufPointer == DataLen) {
    return true;
  }
}
