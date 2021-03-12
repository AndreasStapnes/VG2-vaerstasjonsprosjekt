/*uint32_t FRAMAddrByte; // Byte-nr som skal bli skrevet
uint16_t FRAMLength; // Lengde mellom slutt og start av dump
uint64_t FRAMAddrBit; // Bit-nr som skal bli skrevet
uint64_t FRAMAddrBitDump; // Start-bit for en dump

uint8_t FRAMByteToSend; // Inneholder byte som skal bli skrevet til FRAM

void FRAMDump() {
  static uint8_t WDistributionLen;
  static uint8_t WDistributionStart;
  static bool WDistributionStarted;
  static uint8_t WDistributionEnd;
  
  FRAMSetPointBit(FRAMAddrBitDump + 16);

  Serial.print("TIME! f.o.m.: "); Serial.print((uint32_t)FRAMAddrBit); Serial.println(" Til, men uten "); 
  FRAMAdd(UnixTime, 64); Serial.println((uint32_t)FRAMAddrBit);

  Serial.print("VIND! f.o.m.: "); Serial.print((uint32_t)FRAMAddrBit); Serial.println(" Til, men uten "); 
  WDistributionLen = 0; WDistributionStart = 0; WDistributionStarted = 0; WDistributionEnd = 0;
  for (uint16_t i = 0; i < 256; i++) {
    if (Winds[i]) {
      if (!WDistributionStarted) {
        WDistributionStarted = true;
        WDistributionStart = i;
      }
      WDistributionEnd = i;
    }
  } 
  WDistributionLen = WDistributionEnd - WDistributionStart + 1;
  FRAMAdd(WDistributionLen, 8);
  FRAMAdd(WDistributionStart, 8);
  for (uint16_t i = WDistributionStart; i < (uint16_t)WDistributionEnd + 1; i++) {
    FRAMAdd(Winds[i], 16);
  }
  for(uint8_t i = 0; i<10; i++) {
    FRAMAdd(WindDir[i], 8);
  }
  Serial.println((uint32_t)FRAMAddrBit);
  Serial.print("REGN! f.o.m.: "); Serial.print((uint32_t)FRAMAddrBit); Serial.println(" Til, men uten "); 
  if (RainBits) {
    FRAMAdd(1, 1);
    FRAMAdd(RainBits, 30);
    for (int i = 0; i < 30; i++) {
      if (bitRead(RainBits, i)) {
        FRAMAdd(Rain[i], 4);
      }
    }
  } else {
    FRAMAdd(0, 1);
  } Serial.println((uint32_t)FRAMAddrBit);
  Serial.print("TEMP! f.o.m.: "); Serial.print((uint32_t)FRAMAddrBit); Serial.println(" Til, men uten "); 
  for (int i = 0; i < 10; i++) {
    FRAMAdd(Temp[i], 14);
  } Serial.println((uint32_t)FRAMAddrBit); Serial.print("TRYK! f.o.m.: "); Serial.print((uint32_t)FRAMAddrBit); Serial.println(" Til, men uten "); 
  for (int i = 0; i < 10; i++) {
    FRAMAdd(Press[i], 24);
  } Serial.println((uint32_t)FRAMAddrBit); Serial.print("FUKT! f.o.m.: "); Serial.print((uint32_t)FRAMAddrBit); Serial.println(" Til, men uten ");
  for (int i = 0; i < 10; i++) {
    FRAMAdd(Humid[i], 14);
  } Serial.println((uint32_t)FRAMAddrBit); Serial.print("LYSS! f.o.m.: "); Serial.print((uint32_t)FRAMAddrBit); Serial.println(" Til, men uten ");
  for (int i = 0; i < 10; i++) {
    FRAMAdd(Light[i], 30);
  } Serial.println((uint32_t)FRAMAddrBit); Serial.print("BATT! f.o.m.: "); Serial.print((uint32_t)FRAMAddrBit); Serial.println(" Til, men uten ");
  for (int i = 0; i < 10; i++) {
    FRAMAdd(Batt[i], 10);
  }Serial.println((uint32_t)FRAMAddrBit);

  FRAMLength = FRAMAddrBit - FRAMAddrBitDump;
  //Serial.println(FRAMLength, BIN);
  //Fullfør siste skriving, til tross for at den ikke er full.
  FRAMFinish();
  FRAMSetPointBit(FRAMAddrBitDump);
  FRAMAdd(1, 1);
  FRAMAdd(FRAMLength, 15);
  FRAMFinish();
  //Serial.println(fram.read8(2), BIN);
}

void FRAMSetPointBit(uint64_t ADDRESS) {
  FRAMAddrBit = ADDRESS;
  FRAMByteToSend = (uint8_t)(fram.read8(ADDRESS>>3) << (8-ADDRESS%8)) >> (8-ADDRESS%8);
 // Serial.println(FRAMByteToSend, BIN);
}

void FRAMFinish() {
  FRAMAdd(0, 7-(FRAMAddrBit+7)%8);
}

void FRAMAdd(uint64_t AddToByte, uint8_t UsableBits) {
  static uint8_t RemainingBits; // Gjenværende bits av usablebits
  static uint8_t BitUsageToSend; // 0-8  Omtaler hvor i neste sendebyte det er rom for mer data
  static uint8_t ProgressInUsableBits; // Omtaler hvor mange bits av info sendt til FRAMADD som er skrevet
  ProgressInUsableBits = 0;
  while (ProgressInUsableBits < UsableBits) {
    RemainingBits = UsableBits - ProgressInUsableBits;
    FRAMAddrByte = FRAMAddrBit >> 3;
    BitUsageToSend = FRAMAddrBit%8;
    FRAMByteToSend |= ((AddToByte >> ProgressInUsableBits) << BitUsageToSend);
    if (8 - BitUsageToSend < RemainingBits) { // Hvis det er mindre plass i sendebyte enn det er i gjenværende bits til FRAMADD
      ProgressInUsableBits += (8 - BitUsageToSend);
      FRAMAddrBit += (8 - BitUsageToSend);
      fram.write8(FRAMAddrByte, FRAMByteToSend);
      /*Serial.print("Writing: ");
      Serial.print(FRAMByteToSend, BIN);
      Serial.print(" To Addr: ");
      Serial.print(FRAMAddrByte);
      Serial.print(" Curr. bitaddr: ");
      Serial.println((uint32_t)FRAMAddrBit);
      FRAMByteToSend = 0;
    } else if (8 - BitUsageToSend > RemainingBits) { // Hvis det er mer plass i sendebyte enn det er i gjenværende bits til FRAMADD
      ProgressInUsableBits = UsableBits;
      BitUsageToSend += RemainingBits;
      FRAMAddrBit += RemainingBits;
    } else {
      ProgressInUsableBits = UsableBits;
      FRAMAddrBit += 8;
      fram.write8(FRAMAddrByte, FRAMByteToSend);
      /*Serial.print("Writing: ");
      Serial.print(FRAMByteToSend, BIN);
      Serial.print(" To Addr: ");
      Serial.print(FRAMAddrByte);
      Serial.print(" Curr. bitaddr: ");
      Serial.println((uint32_t)FRAMAddrBit);
      FRAMByteToSend = 0;
    }
  }
}*/

//Fullstendig lengde    Len=16          Tas ved slutten av åpenbare grunner
//Dato                  Len=64
//Vind                  Len=Ukj opp til 512
//Regn                  Len=Ukj opp til 34
//Temp                  Len=140
//Trykk                 Len=240
//Fukt                  Len=140
//Lys                   Len=300
//Batteri               Len=100
