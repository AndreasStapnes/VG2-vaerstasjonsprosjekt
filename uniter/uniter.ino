#include <Wire.h>
#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

uint8_t Address = 1; uint8_t nearbyNodes[6] = {2, 0, 0, 0, 0, 0}; // Burde lagres i EEPROM. Mottak må ikke nødv. skje av disse
//uint8_t Address = 2; uint8_t nearbyNodes[6] = {1, 0, 0, 0, 0, 0}; // Burde lagres i EEPROM. Mottak må ikke nødv. skje av disse
//uint8_t Address = 3; uint8_t nearbyNodes[6] = {2, 4, 0, 0, 0, 0}; // Burde lagres i EEPROM. Mottak må ikke nødv. skje av disse
//uint8_t Address = 4; uint8_t nearbyNodes[6] = {3, 0, 0, 0, 0, 0}; // Burde lagres i EEPROM. Mottak må ikke nødv. skje av disse
// dette er i praksis uint8_t, ikke 16

//double temp[3];
//uint8_t testerbool = 0;

#define DEV 1

#define TYPE 1

#if (TYPE == 0)
#define EEPROMAddrI2C 84
#define EEPROMDATA 512 // Antall kilobit lagring
#define EEPROMINTERNALSTORAGE 128 // antall bytes som kan skrives til EEPROM før chippen begynner å glemme ny data
#define THPSENSORI2C 0x40
#define ACTIVELED true // Active high LED
#define REDLED 3
#define GREENLED 5
#define BLUELED 6
#define I2CCLAIMNECCESARY 0
#define bitloadDataAddr 0 // Addressen der bitloadData begynner
#define EEPROMINTERNALCOMM 2 // Første 2 bytes for intern kommunikasjon
#define EEPROMSTARTVAL 8192 // addressen der værdata er lagret
#elif (TYPE == 1)
#define EEPROMAddrI2C 80 // 50 før -----------------XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXPROBLEM PROBLEM PROBLEM PROBLEM PROBLEM
#define EEPROMDATA 256 // Antall kilobit lagring
#define EEPROMINTERNALSTORAGE 16 // antall bytes som kan skrives til EEPROM før chippen begynner å glemme ny data
#define THPSENSORI2C 0 // Avlesning håndteres av en annen atmega328p
#define ACTIVELED false // Active low LED
#define REDLED 10
#define GREENLED 3
#define BLUELED 9
#define I2CCLAIMNECCESARY 1 // 1Før!!---------------------------------------------------------------------------------------------------
#define bitloadDataAddr 0 // Addressen der bitloadData begynner
#define EEPROMINTERNALCOMM 16 // Første 16 bytes for intern kommunikasjon
#define EEPROMSTARTVAL 8192 // addressen der værdata er lagret
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;
#define CLOCKAddrI2C 0x68
#endif

#define bitloadDataLen 2048

#define CLAIMBOOL 0
bool I2CClaimed = false;
bool Sleeping = false;
uint32_t Sleepyness = 0;

#define RF69_PUBL 433.54
#define RF69_DATA 433.44
// max +- 0.001
#define SendResponseTimeout 3000 // millisekund // 1000f
#define SeriesTransmissionDelay 100 // millisekund // 50f
#define PostConnectionEstablishedDelay 150 // millisekund
#define SeriesTransmissionTimeout 6000 // Må være lengre enn SendResponseTimeout??... hvis resend skal finne sted //5000f
#define LongTimeDefinition 3000 // tid for at samme meldingsID skal bli akseptert //5000f
#define ChangeFreqDelay 100
#define SeriesTransmissionDelayBlock 10 // millisekund
#define SeriesTransmissionDelayBlockIntermediate 3000 // millisekund 2000f
#define SleepTimeout 6000 // millisekund. Tid uten signal for at node sovner 16000F

#define powerLevel 14
#define RFM69_INT     2  // 
#define RFM69_CS      A1  //
#define RFM69_RST     A0  //

#define Sync1   0x2d
#define Sync2   0xd4

RH_RF69 rf69(RFM69_CS, RFM69_INT);
RHReliableDatagram rf69SafeMgr(rf69, Address);

#define len 52
uint8_t buf[len] = {};
unsigned long CurrentMillis;
uint8_t DataID;
uint8_t TransmissionData[len];

uint8_t Route[12];
uint8_t DataLen[3];
uint8_t ReceiverID;

boolean BlockSend; uint8_t NextBlockReqRepetitions; unsigned long NextBlockTimer; boolean AwaitingNextBlock; boolean FirstInRow; boolean ExpectingShortRequest;
boolean ExpectCommandRes;
uint32_t CommandTimeout;
boolean InSeries;
boolean BlockProcessDelay;
boolean AwaitingData;
uint8_t ConnectedSender, ConnectedID;
//unsigned long ConnectedMillisTimeout;
uint8_t ConnectedSendTo;
uint8_t ConnectedSendBack;
unsigned long LastComm;
boolean LongTimeSinceLast;
uint8_t HeaderID;

bool FirstData;
uint8_t FirstDataID;
uint8_t ExpectedDataID;

//Packets
#define DataPack 1
#define Opening 2
#define Response 3
#define Connection 4
#define Disconnection 5
#define Finished 6
#define CorrectMessage 8
#define FailedMessage 9
#define LoadOpening 10
#define LoadResponse 11
#define DataPackEndBlock 12
#define Command 13
#define CommandRes 14

//Shortrequests
#define BlockRequest 1

#define MaxStopsPerBroadcast 20

uint8_t packetsPerSwitch = 12; // kunne endres?

#define TimeRepeatNextBlock 100 //Forespør ny block hvert x millisekund
#define AmountRepeatNextBlock 10 //Forespør ny block 10 ganger.

#define DataPackSize len
uint8_t MaxPosition = (floor(bitloadDataLen / DataPackSize));
uint8_t PackPosition;

void setup()
{
  Wakeup(); // I tilfelle feil skjer og alt tilbakestilles, slå av søvn.
  Snooze();
  LeaveI2C();

  pinMode(BLUELED, OUTPUT); // Blå LED
  pinMode(REDLED, OUTPUT); // Rød LED
  pinMode(GREENLED, OUTPUT); // Grn LED
  digitalWrite(BLUELED, !ACTIVELED); digitalWrite(REDLED, !ACTIVELED); digitalWrite(GREENLED, !ACTIVELED);

#if (TYPE == 1)
  pinMode(8, INPUT); digitalWrite(8, HIGH); // inter comm for å overta I2C rail
  pinMode(7, INPUT); // avlesning sqw fra clk
  if (!bme.begin()) {
    Serial.println("BmeF");
  }
#elif (TYPE == 0)
  pinMode(7, INPUT); // FAULT PIN CHARGER
  pinMode(4, INPUT); // (anti) BATT MEASURE
#endif
  Serial.begin(115200);

  digitalWrite(RFM69_RST, LOW);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  if (!rf69SafeMgr.init()) {
    Serial.println("NO RF69HCW");
  }
  rf69.setFrequency(RF69_PUBL);
  rf69.setSyncWords((Sync1, Sync2), 2);
  rf69.setTxPower(powerLevel, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW
  Serial.print("RFM69at");  Serial.print((int)RF69_PUBL);  Serial.println("MHz");

  rf69SafeMgr.setTimeout(20);
  rf69SafeMgr.setRetries(40);
  rf69.setModemConfig(RH_RF69::FSK_Rb9_6Fd19_2);

  Wire.begin();
  Wire.setClock(100000); // SPI tregere ved Wire = 400khz


#if (TYPE == 0)
  Wire.beginTransmission(THPSENSORI2C);
  Wire.write(0xE7);
  Wire.endTransmission();
  Wire.requestFrom(THPSENSORI2C, 1);
  uint8_t HeaterCorrectVal = 0;
  if (Wire.available()) {
    HeaterCorrectVal |= Wire.read();
  }
  //Serial.print(HeaterCorrectVal, BIN);
  HeaterCorrectVal &= (~(1 << 2));
  //Serial.print(", Writing "); Serial.println(HeaterCorrectVal, BIN);
  Wire.beginTransmission(THPSENSORI2C);
  Wire.write(0xE6);
  Wire.write(HeaterCorrectVal);
  Wire.endTransmission();
#endif
}

void loop() {
  PassiveDataOperations();
  MillisOperations();
#if (DEV)
  uint8_t dump;
  Snooze();

  if (Serial.available()) {
    int Type = 0;
    Type = Serial.parseInt();
    Serial.print(Type);
    while (Serial.available()) dump = Serial.read();
    while (!Serial.available());
    int Loc = 0;
    Loc = Serial.parseInt();
    Serial.print(Loc);
    while (Serial.available()) dump = Serial.read();
    while (!Serial.available());
    int Node = 1;
    Node = Serial.parseInt();
    Serial.print(Node);
    Serial.print(Type); Serial.print(","); Serial.print(Loc); Serial.print(" to "); Serial.println(Node);
    if (Node == 0) {
      InSeries = false;
      AwaitingData = false;
      ConnectedSendTo = 0;
      ConnectedSendBack = 0;
      ConnectedSender = 0;
      ConnectedID = 0;
      rf69.setFrequency(RF69_PUBL);
      Serial.println("Freq=PUBL");
    } else if (!(Node == Address)) {
      uint8_t testarr[16] = {Type, Loc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      SendCommand(testarr, Node);
    } else {
      uint8_t longbit[16];
      longbit[0] = Type; longbit[1] = Loc;
      ExecuteCommand(longbit, 0);
      for (int i = 0; i < 16; i++) {
        Serial.print(longbit[i]); Serial.print("-");
      }
      Serial.println();
    }

    while (Serial.available()) {
      dump = Serial.read();
    }
  }
#endif

  //SendDataEEPROM(TransmissionData, DataID, 3, 10000, 0, true); delay(12000);
  /*uint8_t testarr[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    testarr[1] = 0;
    if ((millis() - 1000) % 12000 < 10) {
    testerbool = 0;
    SendCommand(testarr, 1);
    }
    else if((millis()-3000) % 12000 <10) {
    testerbool = 1;
    SendCommand(testarr, 2);
    }
    else if((millis()-5000) % 12000 <10) {
    testerbool = 2;
    testarr[1] = 5;
    SendCommand(testarr, 2);
    }
    else if((millis() - 7000) % 12000 < 10) {
      Serial.println();
    Serial.print(temp[0]); Serial.print(","); Serial.print(temp[1]); Serial.print(","); Serial.println(temp[2]);
    Snooze();
    delay(10);
    }*/
  /*
    |0|: Cmd type                     |1|: Location                                    |2|: Value
    0 - ReadVal (Single value)        0:Temp 1:Humi 2:Press 3:Batt 4:Addr 5:TempRF
    1 - WriteVal (Single value)       0:     1:     2:      3      4:Addr 5:TempRF
    2 - ReadValues of Array           0:Near 1:
    3 - Change Array value            0:Near 1:
    4 - Unload data                   DataReceiver                                     DataSender
  */
}
