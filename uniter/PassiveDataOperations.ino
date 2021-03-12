void PassiveDataOperations() {
  uint8_t MeasuredLength = len;
  uint8_t PDO_From, PDO_To, PDO_ID, PDO_Flags;
  static uint8_t PrevID, PrevSender;
  static uint8_t PreviousDataID, PreviousDataSender, PreviousType;
  uint8_t SendToAddressResp;
  bool OpeningResponseChangeSuccess;
  static bool LTDetect, LTActive; // brukes for å skille bitload og bitstream
  // SJEKK OM ALT SOM ER STATIC FAKTISK ER STATIC-----------------------------------------------------------------------------------------------------------------------------------
  static uint32_t ReceivedData, ExpectedData;

  if (rf69SafeMgr.available()) {
    rf69SafeMgr.recvfrom(buf, &MeasuredLength, &PDO_From, &PDO_To, &PDO_ID, &PDO_Flags);
    //Serial.print("-");
    //Serial.print(buf[0]);
    /*Serial.print("-");
      Serial.print(buf[1]);
      Serial.print("-");
      Serial.print(buf[2]);
      Serial.print("-");
      Serial.print(buf[3]);
      Serial.print("-");
      Serial.print(buf[4]);*/
    /*if(!InSeries) {//Serial.println();
      for(int i = 0; i<len; i++) {
      //Serial.print(buf[i]);
      }
      //Serial.println();}*/
    if (PDO_Flags != 0x80 && PDO_Flags != 0xd0 && PDO_To == Address) { // Bare tillat hvis pakken ikke er ACK, er ment for denne addresse, og mottaker ikke er i serie med mindre det er data
      switch (buf[0]) {
        case DataPackEndBlock: //For slutten av en block for å signalisere at det er slutten.
          LTDetect = true;
        case DataPack:
          if (AwaitingNextBlock) { // Bryte alle shortrequests når data først blir mottatt
            AwaitingNextBlock = 0;
            NextBlockReqRepetitions = 0;
            //Serial.println("BLCK+");
          }
          //unsigned long test;
          //test = millis();
          if (LTDetect) { // for å sense om DataPackEndBlock har blitt åpnet
            LTActive = true;
            LTDetect = false;
          } else {
            LTActive = false;
          }
          if (InSeries) {
            digitalWrite(BLUELED, ACTIVELED);
            rf69SafeMgr.acknowledge(PDO_ID, PDO_From);
            rf69SafeMgr.registerMessage(PDO_ID, PDO_From);
            //rf69SafeMgr.setHeaderFlags(0, 0x80);
            digitalWrite(BLUELED, !ACTIVELED);
            // Serial.print(" ack ");
          }
          if (!(PDO_From == PrevSender && PDO_ID == PrevID) || FirstData) { // LONGTIMESINCELAST ERSTATTET SIST FIRSTDATA
            if (InSeries) {
              /*for (int i = 0; i < (len); i++) {
                Serial.print(buf[i]); Serial.print("-");
                }
                Serial.println();*/
              //Serial.print("Got data from "); Serial.print(PDO_From); Serial.print(" to "); Serial.println(ConnectedSendTo);
              //ConnectedMillisTimeout = millis();
              if (AwaitingData) {
                if (FirstData) {
                  FirstDataID = PDO_ID;
                  FirstData = false;
                }
                #if (DEV == 1)
                for(int i = 0; i<len; i++) {Serial.print(buf[i]); Serial.print(",");}
                Serial.println();
                #endif
                //Serial.print("I Received Data-"); Serial.print(PDO_ID); Serial.print("-"); Serial.print(PrevID); Serial.print("--"); Serial.print(PDO_From); Serial.print("-"); Serial.println(PrevSender);
                //Serial.print(PDO_ID);
                ReceivedData += len - 1;

              } else {
                rf69SafeMgr.resetRetransmissions();

                /*for (int i = 0; i < len; i++) {
                  Serial.print(buf[i]);
                  Serial.print("-");
                  }
                  Serial.println();*/
                // Serial.print("valid -");
                //  Serial.print(BlockSend); Serial.print("- ");
                if (BlockSend) { // Ikke send videre hvis meldingen sendes i blokker. Da må mottakelsen gjentas flere ganger.
                  if (!LTActive) {
                    if (StoreMessageEEPROM(buf)) {

                    } else {
                      // Mislyktes i å lagre i EEPROM
                      Serial.println("EERROORR");
                    }
                  }
                  else {
                    if (StoreMessageEEPROM(buf)) {
                      SendDataOnwards();
                      if (BlockProcessDelay) {
                        CorrectResponseMistakes((uint32_t)SeriesTransmissionDelayBlockIntermediate); // Dersom det er mer enn 3 nodes, vil det skapes et mellomrom mellom sendingene.
                      }
                    } else {
                      // Mislyktes i å lagre i EEPROM
                      Serial.println("EERROORR");
                    }
                    // SEND DATA VIDERE---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
                  }
                } else {
                  digitalWrite(BLUELED, ACTIVELED); digitalWrite(REDLED, ACTIVELED);
                  if (!rf69SafeMgr.sendtoWait(buf, len, ConnectedSendTo)) {
                    // Serial.print("Could not send data to node "); Serial.print(ConnectedSendTo); Serial.print(" - ");
                    Timeout();
                  }
                  digitalWrite(BLUELED, !ACTIVELED); digitalWrite(REDLED, !ACTIVELED);
                }

              }
            } else {
              //Serial.println("Recv failed, not in series");
            }
          }
          //Serial.println((unsigned long)(millis() - test));
          break;
        case LoadOpening:
          LTDetect = true;
        case Opening: // OPENING----------------------------------------------------------------------------------------------------------------------------------
          Serial.println("Into opening");
          if (LTDetect) { // for å sense om LoadOpening har blitt åpnet
            LTActive = true;
            LTDetect = false;
          } else {
            LTActive = false;
          }
          /*for (int i = 0; i < len; i++) {
            Serial.print(buf[i]);
            Serial.print("-");
            }*/
          if (!InSeries) { // Ikke aksepter "opening" hvis enheten allerede er i serie.
            //Serial.print(buf[1]); Serial.print("-"); Serial.print(PreviousDataID); Serial.print("-"); Serial.print(buf[3]); Serial.print("-"); Serial.print(PreviousDataSender); Serial.print("-"); Serial.print(buf[0]); Serial.print("-"); Serial.println(PreviousType);
            if (!(buf[1] == PreviousDataID && buf[3] == PreviousDataSender && buf[0] == PreviousType) || LongTimeSinceLast) {
              buf[8 + buf[2]] = Address;
              if (buf[4] == Address) {
                // SIKKERHETSSTOPP FOR Å FORHINDRE RARE GREIER. PASS PÅ AT SENDER OG MOTTAKER IKKE ER SAMME ENHET
                if (buf[3] == buf[4]) {
                  //Serial.print("Unknown opening from ");
                  //Serial.print(PDO_From);
                  //Serial.println(" terminated");
                  break;
                }
                FirstData = true;
                Serial.println("Creating response");
                ReceivedData = 0;
                //Serial.print("I expect "); Serial.print(ExpectedData); Serial.println(" bytes");
                if (buf[2]) {
                  ConnectedSendBack = buf[8 + buf[2] - 1];
                } else {
                  ConnectedSendBack = buf[3];
                }
                ConnectedSender = buf[3];
                ConnectedID = buf[1];
                //ConnectedMillisTimeout = millis();
                for (int i = 0; i < len; i++) {
                  TransmissionData[i] = buf[i];
                }
                if (LTActive) {
                  TransmissionData[0] = LoadResponse; // skille hvilken type respons, ut ifra hvilken opening
                  //Serial.println("LT ACTIVE AT OPENING");
                  BlockSend = true;
                  PackPosition = 0; // Tilbakestill EEPROM lagringsplass for blocksend
                } else {
                  TransmissionData[0] = Response;
                  BlockSend = false;
                }
                uint8_t SenderSwitch = TransmissionData[3];
                uint8_t ReceiverSwitch = TransmissionData[4];
                SenderSwitch = TransmissionData[3];
                ReceiverSwitch = TransmissionData[4];
                TransmissionData[3] = ReceiverSwitch;
                TransmissionData[4] = SenderSwitch;
                digitalWrite(BLUELED, ACTIVELED);
                OpeningResponseChangeSuccess = true;
                if (TransmissionData[2]) { // Hvis TD[2] finnes, er ikke 'Node' den første i rekken. Derfor må den sende til forrige 'Node' i rekken
                  TransmissionData[2] -= 1;
                  if (!rf69SafeMgr.sendtoWait(TransmissionData, len, TransmissionData[8 + TransmissionData[2]], true)) {
                    OpeningResponseChangeSuccess = false;
                    //Serial.println("Failed at sending");
                  }
                } else { // Hvis TD[2] ikke finnes, er 'Node' først i rekken. Da må data sendes til 'Sender'
                  if (!rf69SafeMgr.sendtoWait(TransmissionData, len, TransmissionData[4], true)) {
                    OpeningResponseChangeSuccess = false;
                    //Serial.println("Failed at sending");
                  }
                }
                InSeries = true;
                AwaitingData = true;
                digitalWrite(BLUELED, !ACTIVELED);
                if (OpeningResponseChangeSuccess) {
                  bool willdo; // NOE MÅ SKJE HER FØR FREKVENS KAN ENDRES, INGEN VET HVORFOR
                  ChangeFreq(RF69_DATA);
                  OpeningResponseChangeSuccess = false;
                }
              }
              else if (buf[2] < 12) {
                //Serial.println("Spreading opening");
                buf[2] ++;
                digitalWrite(BLUELED, ACTIVELED);
                SpreadData(buf);
              }
            }
          }
          break;
        case LoadResponse:
          LTDetect = true;
        case Response: //RESPONSE--------------------------------------------------------------------------------------------------------------------------------
          rf69SafeMgr.acknowledge(PDO_ID, PDO_From);
          rf69SafeMgr.registerMessage(PDO_ID, PDO_From);
          rf69SafeMgr.setHeaderFlags(0, 0x80);
          if (LTDetect) { // for å sense om LoadOpening har blitt åpnet
            LTActive = true;
            LTDetect = false;
          } else {
            LTActive = false;
          }
          if (!InSeries) {
            if (LTActive) {
              BlockSend = true;
              PackPosition = 0; // Tilbakestill EEPROM lagringsplass for blocksend
            } else {
              BlockSend = false;
            }
            InSeries = true;
            ConnectedSender = buf[3];
            ConnectedID = buf[1];
            //ConnectedMillisTimeout = millis();
            if (!(buf[1] == PreviousDataID && buf[3] == PreviousDataSender && buf[0] == PreviousType)) {
              for (int i = 0; i < len; i++) {
                TransmissionData[i] = buf[i];
              }
              if (TransmissionData[2]) { // For data som går bakover---------------------------------------------
                ConnectedSendBack = TransmissionData[8 - 1 + TransmissionData[2]];
                //Serial.println("Setting CSB to previous node");
                FirstInRow = false;
              } else {
                ConnectedSendBack = TransmissionData[4]; // 4 fordi i Response er SENDER og RECEIVER byttet
                //Serial.println("Setting CSB to TD[3]");
                Serial.println("First");
                FirstInRow = true;
              }
              ConnectedSendTo = TransmissionData[8 + TransmissionData[2] + 1];
              //Serial.print("ConnectedSendTo: "); Serial.println(ConnectedSendTo);
              digitalWrite(BLUELED, ACTIVELED);
              BlockProcessDelay = false;
              if (TransmissionData[2]) {
                TransmissionData[2] --;
                if (!rf69SafeMgr.sendtoWait(TransmissionData, len, TransmissionData[8 + TransmissionData[2]], true)) {
                  //Sending mislyktes
                }
              }
              else {
                if (BlockSend && (TransmissionData[8 + 2])) { // Dersom det er mer enn 3 nodes i queue, vent litt ekstra før videresend
                  BlockProcessDelay = true;
                }
                if (!rf69SafeMgr.sendtoWait(TransmissionData, len, TransmissionData[4], true)) {
                  //Sending mislyktes
                }
              }
              digitalWrite(BLUELED, !ACTIVELED);
              ChangeFreq(RF69_DATA);
            }
          }
          break;
        case Connection:
          if (buf[2] < MaxStopsPerBroadcast && !(buf[1] == PreviousDataID && buf[3] == PreviousDataSender && buf[0] == PreviousType)) {
            buf[2] ++;
            SpreadData(buf);
            //Serial.println("Connection Established");
          }
          break;
        case Disconnection:
          if (buf[2] < MaxStopsPerBroadcast && !(buf[1] == PreviousDataID && buf[3] == PreviousDataSender && buf[0] == PreviousType)) {
            buf[2] ++;
            digitalWrite(BLUELED, ACTIVELED);
            SpreadData(buf);
            digitalWrite(BLUELED, !ACTIVELED);
            /*if ((InSeries) && buf[3] == ConnectedSender && buf[1] == ConnectedID) { // FJERNE IGJEN???-----------------------------------------------------------------------------------------------------------------------------------XXXXXXXXXYYYYYYYYYYYYYYY
              InSeries = false;
              AwaitingData = false;
              ConnectedSender = 0;
              ConnectedID = 0;
              ConnectedSendTo = 0;
              ConnectedSendBack = 0;
              }*/
            //Serial.println("Disconnection Established");
          }
          break;
        case Finished:
          rf69SafeMgr.acknowledge(PDO_ID, PDO_From);
          rf69SafeMgr.registerMessage(PDO_ID, PDO_From);
          rf69SafeMgr.setHeaderFlags(0, 0x80);
          if (!(PDO_From == PrevSender && PDO_ID == PrevID) || LongTimeSinceLast) {
            if (InSeries) {
              if (AwaitingNextBlock) { // Bryte alle shortrequests når data først blir mottatt
                AwaitingNextBlock = 0;
                NextBlockReqRepetitions = 0;
                Serial.println("BA");
                /*for(int k = 0; k<len; k++) {
                  Serial.print(buf[k]); Serial.print(",");
                  }
                  Serial.println();*/
              }
              if (AwaitingData) {
                //Serial.println("Finisher pack");
                ExpectedData = (buf[5] + (buf[6] << 8) + (buf[7] << 16));
                ExpectedDataID = FirstDataID + (ExpectedData - ExpectedData % (len - 1)) / (len - 1) + 1;
                // Serial.print("FirstDataID "); Serial.print(FirstDataID); Serial.print(" -> "); Serial.print(ExpectedDataID);
                // Serial.print(", && "); Serial.println(PDO_ID);
                if (ExpectedDataID == PDO_ID) { // FUNKER IKKE IN THE LONGRUN; KAN IKKE SNAKKE MED ANDRE UNDER TRANSMISSION---------------------------------------------------------------------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                  // Serial.println("I got all the data i need!"); // Alt gikk korrekt, melding ble levert i full form-------------------------------------------------------------------------xxxxxxxxxxxxxxxxx
                  memset(TransmissionData, 0, len);
                  TransmissionData[0] = CorrectMessage;
                  rf69SafeMgr.sendtoWait(TransmissionData, len, ConnectedSendBack);
                  InSeries = false;
                  AwaitingData = false;
                  ConnectedSendTo = 0;
                  ConnectedSendBack = 0;
                  ConnectedSender = 0;
                  ConnectedID = 0;
                } else {
                  // Serial.println("Did not get all the data, please resend"); // Noe gikk galt, etterspør ny melding - fjern mottatt data, fjern all data.------------------------------------xxxxxxxxxxxxxxxxx
                  memset(TransmissionData, 0, len);
                  TransmissionData[0] = FailedMessage;
                  rf69SafeMgr.sendtoWait(TransmissionData, len, ConnectedSendBack);
                }
                ChangeFreq(RF69_PUBL);
              } else {
                //ConnectedMillisTimeout = millis();
                if (!rf69SafeMgr.sendtoWait(buf, len, ConnectedSendTo)) {
                  //Sending mislyktes
                }
              }
            } else {
              //Sending mislyktes, er ikke tilkoplet en serie
            }
          }
          break;
        case CorrectMessage:
          rf69SafeMgr.acknowledge(PDO_ID, PDO_From);
          rf69SafeMgr.registerMessage(PDO_ID, PDO_From);
          rf69SafeMgr.setHeaderFlags(0, 0x80);
          if (!(PDO_From == PrevSender && PDO_ID == PrevID) || LongTimeSinceLast) {
            if (InSeries) {
              // Serial.println("Messages appear to be right");
              //ConnectedMillisTimeout = millis();
              //Serial.print("Sending 'success' to "); Serial.println(ConnectedSendBack);
              if (!rf69SafeMgr.sendtoWait(buf, len, ConnectedSendBack)) {
                //Sending mislyktes
              }
            } else {
              //Sending mislyktes, er ikke tilkoplet en serie
            }
          }
          break;
        case FailedMessage:
          rf69SafeMgr.acknowledge(PDO_ID, PDO_From);
          rf69SafeMgr.registerMessage(PDO_ID, PDO_From);
          rf69SafeMgr.setHeaderFlags(0, 0x80);
          if (!(PDO_From == PrevSender && PDO_ID == PrevID) || LongTimeSinceLast) {
            if (InSeries) {
              //ConnectedMillisTimeout = millis();
              if (!rf69SafeMgr.sendtoWait(buf, len, ConnectedSendBack, true)) {
                //Sending mislyktes
              }
            } else {
              //Sending mislyktes, er ikke tilkoplet en serie
            }
          }
          break;
        case Command:
          //rf69SafeMgr.acknowledge(PDO_ID, PDO_From); ingen ack for cmd
          //Serial.println("CMD");
          rf69SafeMgr.registerMessage(PDO_ID, PDO_From);
          rf69SafeMgr.setHeaderFlags(0, 0x80);
          if (buf[3] != Address) {
            if (!(PDO_From == PrevSender && PDO_ID == PrevID && buf[0] == PreviousType) || LongTimeSinceLast) {
              if (buf[4] == Address) {
                if (ExecuteCommand(buf, 18)) { // cmd data ligger i buf val 18-(18+16). Dersom "sann" - send tilbake melding
                  buf[4] = buf[3]; buf[3] = Address;
                  buf[0] = CommandRes;
                  if (buf[2]) {
                    buf[2]--;
                    rf69SafeMgr.sendtoWait(buf, len, buf[6 + buf[2]], true);
                  } else {
                    rf69SafeMgr.sendtoWait(buf, len, buf[4], true);
                  }
                }
              }
              else if (buf[2] < 12) {
                //Serial.println("Spreading cmd");
                buf[6 + buf[2]] = Address;
                buf[2] ++;
                digitalWrite(BLUELED, ACTIVELED);
                SpreadData(buf);
                digitalWrite(BLUELED, !ACTIVELED);
              }
            }
          }
          break;
        case CommandRes:
          rf69SafeMgr.acknowledge(PDO_ID, PDO_From);
          rf69SafeMgr.registerMessage(PDO_ID, PDO_From);
          rf69SafeMgr.setHeaderFlags(0, 0x80);
          if (!(PDO_From == PrevSender && PDO_ID == PrevID && buf[0] == PreviousType) || LongTimeSinceLast) {
            if (buf[4] == Address) {
              //RETURNED COMMAND RESPONSE
              if (ExpectCommandRes) {
                RecvCommand();
                ExpectCommandRes = false;
              }
            } else {
              if (buf[2]) {
                buf[2]--;
                rf69SafeMgr.sendtoWait(buf, len, buf[6 + buf[2]], true);
              } else {
                rf69SafeMgr.sendtoWait(buf, len, buf[4], true);
              }
            }
          }
          break;
      }
      LastComm = millis();
      LongTimeSinceLast = false;
      PrevSender = PDO_From; PrevID = PDO_ID;
      PreviousDataID = buf[1];
      PreviousDataSender = buf[3];
      PreviousType = buf[0];
    }
    else if (PDO_Flags == 0x80) {
      Serial.println("Received signal was an ACK");
    }
    else if (PDO_Flags == 0xd0) {
      Serial.print("Received signal was a command: ");
      Serial.println(buf[0]);
    }
    Snooze(); // Hindre at node sovner
  }
}
