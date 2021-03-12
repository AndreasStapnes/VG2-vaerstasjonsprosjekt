#define sleeptime 17

void Standby() {
  TCCR2A |= 3; // Set fast PWM = sawtooth pattern
  TCCR2B |= 7; // Sett CLK til å måle CLKIO/1024 = 8ticks/millisec
  WakeupCorrect = TCNT2;
  PRR |= (1 << 0); // SLå av ADC i power register
  WDTCSR = 24;
  WDTCSR = 0; // sett 16ms oppvåkning ////EGT 0
  WDTCSR |= (1 << 6);
  ADCSRA &= ~(1 << 7); // slå av ADC

  SMCR |= (3 << 1); // velg power down modus
  SMCR |= 1; // Sov
  __asm__ __volatile__("sleep");
  //delay(16);
  //TCCR2B &= ~0b00000111; //// REAKTIVER PWM SIGNALER
  //TCCR2A &= ~0b00000011;
  PRR &= ~(1 << 0);
}

ISR(WDT_vect) {
  OffsetTime += sleeptime;
  SMCR &= ~(1); // Våkne
  ADCSRA |= (1 << 7); // Slå på ADC
  PRR &= ~(1 << 0);
  MCUSR &= ~(1 << 3); // turn off WDT
  WDTCSR |= (1 << 4) | (1 << 3);
  WDTCSR = 0x00;
}

uint32_t mill() {
  return (millis() + OffsetTime);
}

void InterruptWakeup() {
  SMCR &= ~(1); // Våkne
  ADCSRA |= (1 << 7); // Slå på ADC
  PRR &= ~(1 << 0);
  MCUSR &= ~(1 << 3);
  WDTCSR |= (1 << 4) | (1 << 3);
  WDTCSR = 0x00;

  OffsetTime += (uint8_t(TCNT2 - WakeupCorrect)) / 8; // TCNT2 = timer 2 = timer som kjører på 8khz. OffsetTime måler tid under søvn.
}


void WakeupSequence() {
  /*static uint16_t Timing;
    uint16_t ActTimerBase = RECALTIME * 1000 / sleeptime; // Våkner hvert 16ms (17). 1000/17 = antall oppvåk. per sek. RECALTIME gir antall oppvåkninger før RECAL er nødvendig
    static uint16_t ActTimer = ActTimerBase;
    static bool ReadNext;
    if (Timing % ActTimer == 0) {
    if (ReadNext) {
      ActTimer = ActTimerBase;
      ReadNext = 0;
      PassiveFunction();
      Serial.println();
      for(int i = 0; i<255; i++) {
        if(i < 32) {
          Serial.print((uint32_t(Winds[2*i]) << 8)+uint32_t(Winds[2*i+1]));
        }
        else {
          Serial.print(((uint32_t(Winds[2*i]) << 8) + uint32_t(Winds[2*i+1]))*600/i);
        }
        //Serial.print(",");
        //Serial.print((((uint32_t)(WindDir[(i-i%32)/32]) >> (4*(((i-i%16)/16) % 2))) & 0b00001111)*40);
        Serial.println();
      }
      for(int j = 255; j<500; j++) Serial.println("0");
      delay(100);
    }
    else {
      uint16_t Remainder = (RECALTIME * DELAYMEASUREMENTS) - (RTCReadSec() % (RECALTIME * DELAYMEASUREMENTS));
      //Serial.print(RTCReadSec()); Serial.print("-"); Serial.println(Remainder);
      if (Remainder > RECALTIME*5/4) {
        ActTimer = ActTimerBase;
      } else {
        ActTimer = Remainder * 1000 / sleeptime;
        ReadNext = 1;
      }
      //Serial.print(RTCReadSec());
      //Serial.print(OffsetTime);
      //Serial.print("-");
      //delayMicroseconds(300);
    }
    Timing = 0;
    }
    Timing ++;
    Standby();*/
  static uint8_t IntermediateCounter;
  static uint32_t MinuteCounter = 0;
  while ((int32_t)(mill() - prevReg) < 20000) {
    Standby();
  }
  SMCR &= ~(1); // Våkne
  ADCSRA |= (1 << 7); // Slå på ADC
  PRR &= ~(1 << 0);
  
  prevReg += 20000;
  IntermediateCounter ++;
  Serial.print("-"); Serial.println(mill());
  if (IntermediateCounter % 3 == 0) {
    IntermediateCounter = 0;
    MinuteCounter ++;
    if(ClaimI2C(8000)) {
      currRTCCheck = unixtime();
      prevReg = prevReg - ((currRTCCheck - begRTCCheck)-3*20*MinuteCounter)*1000;
      LeaveI2C();
      Serial.print(mill()); Serial.print(" & "); Serial.print(prevReg); Serial.print(" , "); Serial.println((uint32_t)currRTCCheck);
    } else {
      Serial.println("Could not claim");
    }
  }
  delay(30);
  PassiveFunction();
}
