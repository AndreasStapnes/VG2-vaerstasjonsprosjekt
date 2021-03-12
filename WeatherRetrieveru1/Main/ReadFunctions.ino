void PassiveFunction() {
  //static uint16_t maxima;
  uint8_t OneMinSubdivision;
  uint32_t TTime = 0;

  //if (mill() - TwentySecMillis >= TimeToNextTwentySec) { // Alt som skjer hvert 20. sek-----------------------------------------------------------------------------------------------------------
  //noInterrupts();
  //TwentySecMillis = mill();
  if (!RegisteredPrevTwentyS) {
    TTime = (Winds[0] << 8) + Winds[1];
    TTime += (mill() - MillisWindToAdd) / 10;
    Winds[0] = (TTime >> 8);
    Winds[1] = (TTime & 0xFF);
    MillisWindToAdd = mill(); //Serial.println("0,0,0"); // Registrer MillisWindToAdd for at neste tidsmåling ikke vil inkludere forrige tidsområde, til tross for at den inkluderer forrige tidsområdes gjennomsnitts fart
  }
  RegisteredPrevTwentyS = false;
  //GetUnixTime();



  if (TwentySecToTenMinCounter % 3 == 0) { // Alt som skjer hvert min-------------------------------------------------------------------------------------------------------------------
    OneMinSubdivision = TwentySecToTenMinCounter / 3;
    ReadBMEValues(OneMinSubdivision);
    ReadLightValues(OneMinSubdivision);
    ReadBatteryValues(OneMinSubdivision);
    WindDir[(TwentySecToTenMinCounter / 3) >> 1] &= ~(0b00001111 << (4 * (TwentySecToTenMinCounter % 2)));
    WindDir[(TwentySecToTenMinCounter / 3) >> 1] |= (ReadWindDir() << (4 * (TwentySecToTenMinCounter % 2)));
  }


  if (TwentySecToTenMinCounter == 27) { // Alt som skjer hvert 10. min---------------------------------------------------------------------------------------------------------
    if (ClaimI2C(120000)) {
      if (!CheckInternalComm()) {
        Addr = EEPROMSTARTVAL + EEPROMINTERNALSTORAGE;
      }
      if (ClaimI2C(60000)) {
        Serial.println("StartDump"); delay(100);
        //noInterrupts();
        Addr = DataDump(Addr);
        LeaveI2C();
        //interrupts();
        for (int i = 0; i < 32 * 32; i += 32) {
          for (int j = 0; j < 32; j++) {
            uint8_t Val[1] = {i + j};
            ReadBlock(EEPROMSTARTVAL + i + j, Val, 1);
            if (Val[0] < 10) Serial.print(" ");
            if (Val[0] < 100) Serial.print(" ");
            Serial.print(Val[0]); Serial.print(" ");
          }
          Serial.println();
        }
      }
      /*maxima = 0;
        writing = 1;
        for (int i = 1; i < 200; i++) {
        if (maxima < Winds[i]) {
          maxima = Winds[i];
        }
        }
        for (int i = 2; i < 200; i++) {
        Serial.print(((double)Winds[i] + (double)Winds[i - 1] + (double)Winds[i - 2] + (double)Winds[i + 1] + (double)Winds[i + 2]) / 5 * 25 / ((double)maxima));
        Serial.print(",");
        Serial.println((double)Winds[i] * 25 / ((double)maxima));
        }
        writing = 0;*/

      //FRAMDump();
      //FJERNER DATA FOR Å SJEKKE---------------------------------------------------------------------------------------------------------------------------------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
      /*for (int i = 0; i < 16; i++) {
        uint8_t nulls[64] = {};
        WriteBlock(EEPROMSTARTVAL + EEPROMINTERNALSTORAGE + i * 64, nulls, 64);
      }*/
    } 
    else {
      Serial.print("Could not claim I2C");
    }
  }
  TwentySecToTenMinCounter ++;
  TwentySecToTenMinCounter %= 30;
  //interrupts();
  //}
}



uint8_t ReadWindDir() {
  uint16_t Read;
  uint8_t Dir;
  Read = analogRead(AnalogDirectionPin);
  if (Read < 75) {
    Dir = 5;
  } else if (Read < 88) {
    Dir = 3;
  } else if (Read < 110) {
    Dir = 4;
  } else if (Read < 156) {
    Dir = 7;
  } else if (Read < 214) {
    Dir = 6;
  } else if (Read < 265) {
    Dir = 9;
  } else if (Read < 346) {
    Dir = 8;
  } else if (Read < 433) {
    Dir = 1;
  } else if (Read < 530) {
    Dir = 2;
  } else if (Read < 615) {
    Dir = 11;
  } else if (Read < 666) {
    Dir = 10;
  } else if (Read < 744) {
    Dir = 15;
  } else if (Read < 807) {
    Dir = 0;
  } else if (Read < 857) {
    Dir = 13;
  } else if (Read < 916) {
    Dir = 14;
  } else if (Read < 985) {
    Dir = 12;
  } else {
    Dir = 16; // Feilavlesning
  }
  return Dir;
}


void AddWind() {
  noInterrupts();
  InterruptWakeup();
  //if (!writing) {
  if (!RegisteredPrevTwentyS) {
    RegisteredPrevTwentyS = true;
  }
  uint64_t DeltaMill;
  double WindSpeed;
  uint8_t WindByte;
  DeltaMill = mill() - MillisWindSpeed;
  WindSpeed = 1000 / ((double)DeltaMill) * 2.4;
  WindByte = round(WindSpeed * 4);
  //delayMicroseconds(50);

  uint32_t TempTime = 0;
  if (WindByte < 32) {
    TempTime = (Winds[WindByte * 2] << 8) + Winds[WindByte * 2 + 1];
    TempTime += (uint32_t(mill() - MillisWindToAdd)) / 10;
    Winds[WindByte * 2] = (((uint32_t)TempTime >> 8) & 0xFF); // Hvis vindfart er større enn nr31=8m/s lagres farten i omdreininger. Er den mindre -> i tid.
    Winds[WindByte * 2 + 1] = ((uint32_t)TempTime & 0xFF);
  }
  else {
    Winds[2 * WindByte + 1] += 1;
    if (Winds[2 * WindByte + 1] == 0) Winds[2 * WindByte]++;
  }
  //Bruk millistoadd for lavere luftfartsverdier

  MillisWindSpeed = mill();
  MillisWindToAdd = mill();
  //}

  interrupts();
}

uint32_t temprainmeas;

void AddRain() {
  noInterrupts();
  InterruptWakeup();
  if (mill() - temprainmeas > 1000) { // ungå retriggers. overse målinger mindre enn 1s etter hverandre
    Rain[TwentySecToTenMinCounter] ++;
    RainBits[(TwentySecToTenMinCounter - TwentySecToTenMinCounter % 8) / 8] |= (1 << (TwentySecToTenMinCounter % 8));
    Serial.print("R++");
    //message = (mill() - temprainmeas);
    temprainmeas = mill();
  } else {
    Serial.print("*");
  }
  interrupts();
}


/*void GetUnixTime() {
  DateTime now = rtc.now();
  UnixTime = now.unixtime();
  //EstimatedNextUnixTime += 20;
  //TimeToNextTwentySec = (EstimatedNextUnixTime - UnixTime) * 1000; // Dette holder arduinoen på rett klokkeslett, og divergerer aldri mer enn 1.5 sek.
  }*/

void ReadBMEValues(byte StorageSlot) {
  double Temperature, Pressure, Humidity;
  Temperature = bme.readTemperature();
  Pressure = bme.readPressure();
  Humidity = bme.readHumidity();
  for (int8_t i = 0; i < 4; i++) {
    Press[4 * StorageSlot + (3 - i)] = ((uint32_t)(Pressure * 100) >> (8 * i)) & 0xFF;
    if (i < 2) {
      Humid[2 * StorageSlot + (1 - i)] = ((uint16_t)(Humidity * 100) >> (8 * i)) & 0xFF;
      Temp[2 * StorageSlot + (1 - i)] = ((((uint16_t)abs((double)Temperature * 100)) << 1) >> (8 * i)) & 0xFF;
    }
  }
  if (Temperature > 0) {
    Temp[2 * StorageSlot + 1] |= 1; // 0bDDDDDDDD-DDDDDDD+, hvor + = +/- -> 1/0
  }
  //Serial.println(Temperature);
  //Serial.println((double)((Temp[2*StorageSlot + 1] & (~1)) >> 1)/100 + (double)(Temp[2*StorageSlot] << 7)/100);
}

void ReadLightValues(byte StorageSlot) {

}

void ReadBatteryValues(byte StorageSlot) {
  //Serial.println((double)analogRead(AnalogBatteryPin)*3.3/1024*2);
  uint16_t BattRead = analogRead(AnalogBatteryPin);
  Batt[2 * StorageSlot] = BattRead >> 8;
  Batt[2 * StorageSlot + 1] = BattRead & 0xFF;
}
