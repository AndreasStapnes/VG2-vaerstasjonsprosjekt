bool ExecuteCommand(uint8_t *data, uint8_t offset) {
  uint8_t *Sv[7];
  uint8_t (*Mv[1])[6];
  uint8_t LocationSpecifier = data[offset + 1];

  switch (data[offset + 0]) {
    case 0:
      uint32_t ReturningValue;
      ReturningValue = returnValue(LocationSpecifier);
      for (int i = 0; i < 16; i++) {
        data[i + offset] = 0;
      }
      for (int i = 0; i < 4; i++) {
        data[offset + i] = ((uint32_t)ReturningValue >> (8 * (3 - i))) & 0xFF;
      }
      return 1;
      break;
    case 1:
      // RIMELIG USIKKERT. MÅ OGSÅ SKRIVE TIL EEPROM-----------------------------------------------------------
      Sv[4] = &Address; // Definerer Sv4
      Sv[6] = &packetsPerSwitch;
      if ((*Sv[data[offset + 1]]) != NULL) {
        (*Sv[data[offset + 1]]) = data[offset + 2];
      }
      for (int i = 0; i < 16; i++) {
        data[i + offset] = 0;
      }
      data[offset] = 1;
      return 1;
      break;
    case 2:
      Mv[0] = &nearbyNodes;
      if ((*Mv[data[offset + 1]]) != NULL) {
        for (int i = 0; i < 16; i++) {
          //if (i < sizeof(*Mv[data[offset + 1]])) {
          data[offset + i] = (*Mv[data[offset + 1]])[i] & 0xFF;
          //} else {
          //data[offset + i] = 0;
          //}
        }
      }
      return 1;
      break;
    case 3: // Val0 = CMD, VAL1 = ARRAY NR, VAL2 = ARRAY LOCATION, VAL3 = INSERTVAL
      Mv[0] = &nearbyNodes;
      if (*Mv[data[offset + 1]] != NULL) {
        (*Mv[data[offset + 1]])[data[offset + 2]] = data[offset + 3];
      }
      return 1;
      break;
    case 4:
      Serial.println("EEPROMSEND");
      uint8_t WeatherLengths[2];
      ReadBlock(EEPROMSTARTVAL + EEPROMINTERNALCOMM - 2, WeatherLengths, 2);
      uint16_t WeatherLength = (WeatherLengths[0] << 8) + WeatherLengths[1];
      Serial.print("WeatherLength = ");
      Serial.println(WeatherLength);
      while (rf69SafeMgr.available()) { // Fjern alt i buffer
        rf69SafeMgr.recvfrom(0, 0, 0, 0, 0, 0);
      }
      if (SendDataEEPROM(TransmissionData, DataID, data[offset + 1], WeatherLength - (EEPROMSTARTVAL + EEPROMINTERNALCOMM), (EEPROMSTARTVAL + EEPROMINTERNALCOMM), true)) {
        WeatherLength = EEPROMSTARTVAL + EEPROMINTERNALCOMM;
        WeatherLengths[0] = WeatherLength >> 8; WeatherLengths[1] = WeatherLength & 0xFF; // SKAL VÆRE I EEPROMSTARTVAL+EEPROMINTERNALCOMM VED AVSLUTNING -------------------------------------------------
        WriteBlock(EEPROMINTERNALCOMM + EEPROMSTARTVAL - 2, WeatherLengths, 2);
        uint8_t TrueVerdi[1] = {1};
        WriteBlock(EEPROMSTARTVAL, TrueVerdi, 1); // Signalisere til andre atmega at det er lest
      }
      return 0;
      break;
  }
}

uint32_t returnValue(uint8_t Location) {
  uint16_t SensT, SensH, SensB;
  uint32_t SensP;
  switch (Location) {
    case 0:
      Sense(SensT, SensH, SensP, SensB);
      return SensT;
      break;
    case 1:
      Sense(SensT, SensH, SensP, SensB);
      return SensH;
      break;
    case 2:
      Sense(SensT, SensH, SensP, SensB);
      return SensP;
      break;
    case 3:
      Sense(SensT, SensH, SensP, SensB);
      return SensB;
      break;
    case 4:
      return Address;
      break;
    case 5:
      rf69.setModeStandby();
      return ((uint16_t)((double)(rf69.temperatureRead() + 46.85) * 65536 / 175.72)); // Burde settes automatisk inn i RX?
      break;
    case 6:
      return packetsPerSwitch;
      break;
  }
}


void SendCommand(uint8_t *data, uint16_t target) {
  memset(TransmissionData, 0, len);
  PrepareData(TransmissionData, Command, DataID, Address, target);
  //Serial.print("Preparing:");
  for (int i = 0; i < 16; i++) {
    //Serial.print(data[i]); Serial.print("-");
    TransmissionData[i + 18] = data[i];
  }
  //Serial.println();
  if (data[0] == 4) { // FORBEDRINGER-----------------------------------------------------
    //Ikke forvent cmdres dersom det forventes data
  } else {
    ExpectCommandRes = true;
    CommandTimeout = millis();
  }
  SpreadData(TransmissionData);
  Snooze(); // Hindre at node sovner
}

void RecvCommand() { // data ligger i buf[]
  Serial.print("delay:"); Serial.print(millis() - CommandTimeout); Serial.print("ms,");
  for (int i = 0; i < 16; i++) {
    Serial.print(buf[18 + i]); Serial.print("-");
  }
  Serial.println();
  /*uint32_t tempRe = 0;
    for(int i = 0; i<4; i++) {
    tempRe |= (((uint32_t)buf[18+i] << (8*(3-i))));
    }
    //temp[testerbool] = tempRe*175.72/65536 - 46.85;
    Serial.print(tempRe*175.72/65536 - 46.85); Serial.println("*C");*/
}

void CommandFailed() {
  Serial.println("CMDFail");
}
/*
  |0|: Cmd type                     |1|: Location                                    |2|: Value
    0 - ReadVal (Single value)        0:Temp 1:Humi 2:Press 3:Batt 4:Addr 5:TeRF 6:PackPerSw
    1 - WriteVal (Single value)       0:     1:     2:      3      4:Addr 5:     6:PackPerSw
    2 - ReadValues of Array           0:Near 1:
    3 - Change Array value            0:Near 1:
    4 - Unload data
*/
