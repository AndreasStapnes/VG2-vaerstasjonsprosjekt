void Standby() {
  //Serial.println("Entering Sleep");  delay(2);
  Sleeping = true;
  ADCSRA &= ~(1<<7); // slå av ADC
  PRR |= (1<<0);
 
  SMCR |= (3<<1); // velg power down modus
  SMCR |= 1; // Sov
  __asm__ __volatile__("sleep");
}
