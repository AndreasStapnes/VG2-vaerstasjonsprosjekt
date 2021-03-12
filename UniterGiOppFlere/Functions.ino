void SpreadData(uint8_t SD_DataPack[len]) {
  rf69SafeMgr.setHeaderFlags(0, 0x80);
  rf69.setPreambleLength(200);
  rf69SafeMgr.sendto((0xaa, 0xaa, 0xaa, 0xaa), 4, 0x00);
  rf69.setPreambleLength(4);
  delay(50);

  for (int i = 0; i < 6; i++) {
    if (nearbyNodes[i]) {
      Serial.print("SPREAD-"); Serial.print(SD_DataPack[0]); Serial.print("-TO-");
      Serial.println(nearbyNodes[i]);
      delay(30); // -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------Lite teknisk
      rf69SafeMgr.sendto((uint8_t *)SD_DataPack, len, nearbyNodes[i]);
    }
  }
}

void DataTransmissionFinished() {
  DataID++;
  DataID |= (1<<7); // Alltid hold DataID > 128. Dette er for å forhindre at DataID == Address, som medfører feil data
  //Dataoverførings tilkopling er dannet
}

//Sted å lagre data, formål, ID, -tomt-, Sender, Mottaker, LengdeX3, RuteX12
void PrepareData(uint8_t PD_DataStorage[len], uint8_t PD_Type, uint8_t PD_DataID, uint8_t PD_Sender, uint8_t PD_Receiver = 0, uint32_t PD_DataLength = 0, uint8_t PD_Route[12] = 0, uint32_t PD_Mistakes = 0) {
  uint8_t PD_LengthBytes[3];
  PD_LengthBytes[0] = PD_DataLength;
  PD_LengthBytes[1] = PD_DataLength >> 8;
  PD_LengthBytes[2] = PD_DataLength >> 16;
  uint8_t PD_MistakeBytes[3];
  PD_MistakeBytes[0] = PD_Mistakes;
  PD_MistakeBytes[1] = PD_Mistakes >> 8;
  PD_MistakeBytes[2] = PD_Mistakes >> 16;
  memset(PD_DataStorage, 0, len);
  switch (PD_Type) {
    case Connection:
      PD_DataStorage[0] = Connection;
      PD_DataStorage[1] = PD_DataID;
      PD_DataStorage[3] = PD_Sender;
      PD_DataStorage[4] = PD_Receiver;
      for (int i = 0; i < 3; i++) {
        PD_DataStorage[5 + i] = PD_LengthBytes[i];
      }
      for (int i = 0; i < 12; i++) {
        PD_DataStorage[8 + i] = PD_Route[i];
      }
      break;
    case Opening:
      PD_DataStorage[0] = Opening;
      PD_DataStorage[1] = PD_DataID;
      PD_DataStorage[3] = PD_Sender;
      PD_DataStorage[4] = PD_Receiver;
      for (int i = 0; i < 3; i++) {
        PD_DataStorage[5 + i] = PD_LengthBytes[i];
      }
      break;
    case Disconnection:
      PD_DataStorage[0] = Disconnection;
      PD_DataStorage[1] = PD_DataID;
      PD_DataStorage[3] = PD_Sender;
      PD_DataStorage[4] = PD_Receiver;
      for (int i = 0; i < 3; i++) {
        PD_DataStorage[5 + i] = PD_LengthBytes[i];
        PD_DataStorage[8 + i] = PD_MistakeBytes[i];
      }
      break;
    case Finished:
      PD_DataStorage[0] = Finished;
      PD_DataStorage[1] = PD_DataID;
      PD_DataStorage[3] = PD_Sender;
      PD_DataStorage[4] = PD_Receiver;
      for (int i = 0; i < 3; i++) {
        PD_DataStorage[5 + i] = PD_LengthBytes[i];
      }
      break;
    case Command:
      PD_DataStorage[0] = Command;
      PD_DataStorage[1] = PD_DataID;
      PD_DataStorage[3] = PD_Sender;
      PD_DataStorage[4] = PD_Receiver;
      break;
  }
}

uint8_t PreviousResSender;

bool WaitForRes(uint8_t WFR_DataStorage[len], uint8_t WFR_DataID, uint8_t WFR_ReceiverID, uint32_t WFR_DataLen, bool Block = false) {
  uint8_t WFR_MeasuredLength = len;
  uint8_t WFR_From, WFR_To, WFR_ID, WFR_Flags;


  while ((unsigned long)(millis() - CurrentMillis) < SendResponseTimeout) {
    if (rf69SafeMgr.available()) {
      //Serial.println("-");
      rf69SafeMgr.recvfrom(WFR_DataStorage, &WFR_MeasuredLength, &WFR_From, &WFR_To, &WFR_ID, &WFR_Flags);
      PreviousResSender = WFR_From;
      //Serial.println(PreviousResSender);
      if (Block) {
        if (WFR_DataStorage[0] == LoadResponse && WFR_DataStorage[4] == Address) { //venter på respons. Når dette er oppnådd blir while-loop brutt.
          if (WFR_Flags != 0x80 && WFR_To == Address) { // Så lenge meldingen ikke er en Ack og den er dirigert mot denne addressen, aksepter
            rf69SafeMgr.acknowledge(WFR_ID, WFR_From);

            for (int i = 0; i < 12; i++) {
              Route[i] = WFR_DataStorage[8 + i];
            }
            Serial.println("SConnPck"); //Sending connection package
            //PrepareData(WFR_DataStorage, Connection, WFR_DataID, Address, WFR_ReceiverID, WFR_DataLen, Route); // INKLUDER LOAD/BLOCK I CONNECTION MELDING-----------------------------------------------------
            //SpreadData(WFR_DataStorage);
            return true;
          }
        }
      } else {
        if (WFR_DataStorage[0] == Response && WFR_DataStorage[4] == Address) { //venter på respons. Når dette er oppnådd blir while-loop brutt.
          if (WFR_Flags != 0x80 && WFR_To == Address) { // Så lenge meldingen ikke er en Ack og den er dirigert mot denne addressen, aksepter
            rf69SafeMgr.acknowledge(WFR_ID, WFR_From);

            for (int i = 0; i < 12; i++) {
              Route[i] = WFR_DataStorage[8 + i];
            }
            //Serial.println("Sending Connection pack");
            PrepareData(WFR_DataStorage, Connection, WFR_DataID, Address, WFR_ReceiverID, WFR_DataLen, Route);
            SpreadData(WFR_DataStorage);
            return true;
          }
        }
      }
    }
  }
  return false;
}

void CorrectResponseMistakes(uint32_t resTimeout) {
  uint32_t resStartMillis = millis();
  uint8_t CRM_DataStorage[len]; // Mulig å sette til en enkelt byte?--------------------------------------------------------------------------------------------------------
  uint8_t CRM_MeasuredLength, CRM_From, CRM_To, CRM_ID, CRM_Flags;
  while(millis() - resStartMillis < resTimeout) {
    if (rf69SafeMgr.available()) {
      //Serial.println("Into available");
      rf69SafeMgr.recvfrom(CRM_DataStorage, &CRM_MeasuredLength, &CRM_From, &CRM_To, &CRM_ID, &CRM_Flags);
      Serial.print(CRM_DataStorage[0]); Serial.print(" , "); Serial.print(CRM_To); Serial.print("-"); Serial.print(Address); Serial.print(" , "); Serial.print(CRM_From); Serial.print("-"); Serial.println(PreviousResSender);
      if ((CRM_DataStorage[0] == LoadResponse || CRM_DataStorage[0] == Response || CRM_DataStorage[0] == DataPackEndBlock) && CRM_To == Address && PreviousResSender == CRM_From) {
        rf69SafeMgr.acknowledge(CRM_ID, CRM_From);
        //Serial.println("Acked for correction");
      }
    }
  }
}

bool SendDataEEPROM(uint8_t SDE_DataStorage[len], uint8_t SDE_DataID, uint8_t SDE_ReceiverID, uint32_t SDE_DataLen, uint32_t SDE_DataLocation, bool Block) {
  rf69SafeMgr.resetRetransmissions();
  Serial.print(SDE_DataLocation); Serial.print(" - "); Serial.println(SDE_DataLen);
  CurrentMillis = millis();
  uint32_t SDE_PacketAmount = (SDE_DataLen + ((len - 1) - SDE_DataLen % (len - 1)) % (len - 1)) / (len - 1);
  uint8_t NextInRoute;
  bool Failed = false;
  uint8_t SwitchCount = 0;
  if (SDE_DataLen + SDE_DataLocation < EEPROMDATA * 128 && ClaimI2C(SeriesTransmissionTimeout)) { // Ta over I2C kanal
    Serial.println("EEPROMSend"); // Attempting eeprom send
    PrepareData(SDE_DataStorage, Opening, SDE_DataID, Address, SDE_ReceiverID, SDE_DataLen);
    if (Block) {
      Serial.println("D.op");
      SDE_DataStorage[0] = LoadOpening;
    }
    digitalWrite(BLUELED, ACTIVELED);
    SpreadData(SDE_DataStorage);
    digitalWrite(BLUELED, !ACTIVELED);
    Serial.println("Wait, res."); //Waiting for res
    if (WaitForRes(SDE_DataStorage, SDE_DataID, SDE_ReceiverID, SDE_DataLen, Block)) {
      Serial.print("RES:");
      NextInRoute = Route[0];
      //Serial.println(Route[0]);
      if (!NextInRoute) {
        NextInRoute = SDE_ReceiverID;
      }
      ExpectingShortRequest = true;
      if(NextInRoute == SDE_ReceiverID) ExpectingShortRequest = false; // Dersom ingen mellomsenden kan sende shortrequest, er det ikke vits i å bry seg med det
      CorrectResponseMistakes(PostConnectionEstablishedDelay); // garantere at ingen response-meldinger forblir uten respons og sys kresjer
      //delay(PostConnectionEstablishedDelay); // trenger litt dødtid for at motparten skal videresende "Connect" meldingen. erstattet av correctresponsemistakes
      ChangeFreq(RF69_DATA);
      Failed = false;
      for (int i = 0; i < SDE_PacketAmount; i++) {
        rf69SafeMgr.resetRetransmissions();
        memset(SDE_DataStorage, 0, len);
        SDE_DataStorage[0] = DataPack;
        /*for (int j = 0; j < len - 1; j++) {
          if ((len - 1) * i + j < SDE_DataLen) {
            SDE_DataStorage[j + 1] = EEPROM.read(SDE_DataLocation + i * (len - 1) + j);
          }
          }*/
        ReadBlock((len - 1)*i + SDE_DataLocation, SDE_DataStorage, len - 1, 1);
        /*for (int k = 0; k < len; k++) {
          Serial.print(SDE_DataStorage[k]); Serial.print("-");
          }*/
        //Serial.println();
        digitalWrite(BLUELED, ACTIVELED);
        if (!Block) {
          if (!rf69SafeMgr.sendtoWait(SDE_DataStorage, len, NextInRoute, true)) {
            //Overføring mislyktes
            //Serial.println("Messages were not received. Transmit terminated");
            digitalWrite(BLUELED, !ACTIVELED);
            Failed = true;
            Serial.print("FAILED");
            break;
          }
          delay(SeriesTransmissionDelay);
        } else {
          if (SwitchCount % packetsPerSwitch != (packetsPerSwitch - 1) && (i + 1) < SDE_PacketAmount) { // Gjør standard så lenge neste send ikke er %8 lik 7 eller siste pakke
            if (!rf69SafeMgr.sendtoWait(SDE_DataStorage, len, NextInRoute, true)) {
              //Overføring mislyktes
              //Serial.println("Messages were not received. Transmit terminated");
              digitalWrite(BLUELED, !ACTIVELED);
              Failed = true;
              Serial.print("FAILED");
              break;
            }
            delay(SeriesTransmissionDelayBlock);
          }
          else {
            SDE_DataStorage[0] = DataPackEndBlock;
            if (!rf69SafeMgr.sendtoWait(SDE_DataStorage, len, NextInRoute, true)) {
              //Overføring mislyktes
              //Serial.println("Messages were not received. Transmit terminated");
              digitalWrite(BLUELED, !ACTIVELED);
              Failed = true;
              Serial.print("F"); // FAILED
              break;
            }
            static unsigned long BreakingTimer;
            BreakingTimer = millis();
            if (ExpectingShortRequest) {
              while (1) { // Også når det er siste melding, fordi da sendes finished-melding;
                if (rf69SafeMgr.available()) {
                  uint8_t BLCKFlags, BLCKID, BLCKBuf, BLCKFrom, BLCKTo;
                  uint8_t BLCKLen = 1;
                  if (rf69SafeMgr.recvfrom(&BLCKBuf, &BLCKLen, &BLCKFrom, &BLCKTo, &BLCKID, &BLCKFlags)) {
                    if (BLCKFlags == 0xd0 && BLCKBuf == BlockRequest && BLCKTo == Address) {
                      Serial.println("+");
                      break;
                    }
                    Serial.print(BLCKLen); Serial.println(":"); Serial.print(BLCKFlags); Serial.println(":"); Serial.print(BLCKBuf);
                    Serial.println("-");
                  }
                  //Serial.println("WasThere");
                }
                if (millis() - BreakingTimer > SeriesTransmissionTimeout) {
                  Failed = true;
                  Serial.println("F1"); // Failed1
                  break;
                }
              }
            }
            delay(SeriesTransmissionDelayBlock);
          }
          SwitchCount ++;
        }
        digitalWrite(BLUELED, !ACTIVELED);
        //Serial.print("Pack"); Serial.print(i); Serial.print(" ");
        //Serial.print(rf69SafeMgr.retransmissions()); Serial.print(" ");
      }
      if (!Failed) {
        PrepareData(SDE_DataStorage, Finished, SDE_DataID, Address, SDE_ReceiverID, SDE_DataLen);
        delay(SeriesTransmissionDelay);
        digitalWrite(BLUELED, ACTIVELED);

        if (!rf69SafeMgr.sendtoWait(SDE_DataStorage, len, NextInRoute, true)) {
          //Slutt mislyktes
          Serial.println("FSndFin");
          Failed = true;
        }
        Serial.println("WForRes");
        digitalWrite(BLUELED, !ACTIVELED);
        if (WaitForResult(SDE_DataStorage)) {
          //Alt gikk bra, fullført meldingstransaksjon-----------------------------------------------------------------------------------------------------------xxxxxxxxxxxxxxxxxxxxx
          Serial.println("V");
          //PrepareData(SDE_DataStorage, Disconnection, SDE_DataID, Address, SDE_ReceiverID, (uint32_t)((unsigned long)(millis() - CurrentMillis)), NULL, rf69SafeMgr.retransmissions());
          //digitalWrite(GREENLED, ACTIVELED);
          //SpreadData(SDE_DataStorage);
          //digitalWrite(GREENLED , !ACTIVELED);
        } else {
          //Noe gikk galt, send melding på nytt------------------------------------------------------------------------------------------------------------------xxxxxxxxxxxxxxxxxxxxx
          Serial.println("X");
          Failed = true;
        }

      } else {
        Serial.println("f");
        Failed = true;
      }
      ChangeFreq(RF69_PUBL);
    } else {
      Serial.println("Nores");
      Failed = true;
      //Mottok aldri respons for datasending
    }
    DataTransmissionFinished();
  }
  else {
    //Data påkrevd strekker seg utfor data tilgjengelig i EEPROM
    Serial.print("EEPX");
    Failed = true;
  }
  LeaveI2C();
  rf69SafeMgr.setHeaderFlags(0, 0x80);
  Snooze(); // Hindre at node sovner
  return !Failed;
}


bool WaitForResult(uint8_t WFRE_DataStorage[len]) {
  unsigned long TimerMillisResult = millis();
  uint8_t WFRE_MeasuredLength = len;
  uint8_t WFRE_From, WFRE_To, WFRE_ID, WFRE_Flags;
  while ((unsigned long)(millis() - TimerMillisResult) < SendResponseTimeout) {
    if (rf69SafeMgr.available()) {
      rf69SafeMgr.recvfrom(WFRE_DataStorage, &WFRE_MeasuredLength, &WFRE_From, &WFRE_To, &WFRE_ID, &WFRE_Flags);
      Serial.print(WFRE_Flags); Serial.print(":"); Serial.print(WFRE_To); Serial.print(":"); Serial.print(WFRE_MeasuredLength); Serial.print(":"); Serial.println(WFRE_DataStorage[0]);
      if (WFRE_DataStorage[0] == CorrectMessage) { //venter på respons. Når dette er oppnådd blir while-loop brutt.
        if (WFRE_Flags != 0x80 && WFRE_Flags != 0xd0 && WFRE_To == Address) { // Så lenge meldingen ikke er en Ack og den er dirigert mot denne addressen, aksepter
          rf69SafeMgr.acknowledge(WFRE_ID, WFRE_From);

          return true;
        }
      } else if (WFRE_DataStorage[0] == FailedMessage) {
        if (WFRE_Flags != 0x80 && WFRE_Flags != 0xd0 && WFRE_To == Address) { // Så lenge meldingen ikke er en Ack og den er dirigert mot denne addressen, aksepter
          rf69SafeMgr.acknowledge(WFRE_ID, WFRE_From);

          return false;
        }
      }
    }
  }
  return false;
}


bool SendDataOnwards() {
  Serial.println("Sending onw");
  for (int i = 0; i < PackPosition; i++) {
    ReadBlock(bitloadDataAddr + i * DataPackSize, buf, DataPackSize);
    delay(SeriesTransmissionDelayBlock);
    digitalWrite(BLUELED, ACTIVELED); digitalWrite(REDLED, ACTIVELED);
    if (buf[0] == DataPack || buf[0] == DataPackEndBlock) {
      if (!rf69SafeMgr.sendtoWait(buf, len, ConnectedSendTo, true)) {
        Serial.print("Could not send data to node "); Serial.print(ConnectedSendTo); Serial.print(" - ");
        Timeout();
        PackPosition = 0;
        digitalWrite(BLUELED, !ACTIVELED); digitalWrite(REDLED, !ACTIVELED);
        return 0;
      }
    }
    else {
      //error with eeprom save.
      Serial.print("ERR "); Serial.print(buf[0]);
    }
    digitalWrite(BLUELED, !ACTIVELED); digitalWrite(REDLED, !ACTIVELED);
  }
  PackPosition = 0;

  if (FirstInRow) {
    AwaitingNextBlock = 1;
  }

  return 1;
}


void MillisOperations() {
  if (InSeries) {
    /*if ((unsigned long)(millis() - ConnectedMillisTimeout) > SeriesTransmissionTimeout) {
      //Serial.println("Timed out, severing all ties");
      //Serial.println("Did not get all the information, please resend");
      Timeout();
    }*/ // Trengs ikke når snooze finnes
    if (AwaitingNextBlock) {
      if ((unsigned long)(millis() - NextBlockTimer) > TimeRepeatNextBlock) {
        rf69SafeMgr.shortRequest(NextBlockReqRepetitions , BlockRequest , ConnectedSendBack);
        NextBlockTimer = millis();
        NextBlockReqRepetitions ++;
      }
      if (NextBlockReqRepetitions >= AmountRepeatNextBlock) {
        AwaitingNextBlock = 0;
        NextBlockReqRepetitions = 0;
        //Sending mislyktes Signaliser?--------------------
        Serial.println("Failed");
      }
    }
  }
  if (!LongTimeSinceLast) {
    if ((unsigned long)(millis() - LastComm) > LongTimeDefinition) {
      LongTimeSinceLast = true;
    }
  }
  if(ExpectCommandRes) {
    if(millis() - CommandTimeout > SendResponseTimeout) {
      ExpectCommandRes = false;
      CommandFailed();
    }
  }
  if(Sleeping) {
    Wakeup();
  } else {
    if(millis() - Sleepyness > SleepTimeout) {
      Serial.println("Falling asleep");
      Timeout();
      delayMicroseconds(100);
      Wakeup();
      Serial.println("Waking up");
    }
  }
}

void Snooze() {
  Sleepyness = millis();
}

void Timeout() {
  //Serial.println("Exiting");
  //Serial.println("Did not get all the information, please resend");
  InSeries = false;
  AwaitingData = false;
  ConnectedSendTo = 0;
  ConnectedSendBack = 0;
  ConnectedSender = 0;
  ConnectedID = 0;
  ChangeFreq(RF69_PUBL);

  rf69.setModeListen();
  Standby();
}

void Wakeup() {
  SMCR &= ~(1); // Våkne
  ADCSRA |= (1 << 7); // Slå på ADC
  PRR &= ~(1 << 0);
  Sleeping = false;
  Snooze();
}

void ChangeFreq(float Freq) {
  rf69.setFrequency(Freq);
  delay(ChangeFreqDelay);
  //Serial.print("Changing Freq to "); Serial.println(Freq);
}
