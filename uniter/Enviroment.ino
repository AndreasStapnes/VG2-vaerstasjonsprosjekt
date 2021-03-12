void Sense(uint16_t &TemperatureR, uint16_t &HumidityR, uint32_t &PressureR, uint16_t &BatteryR) {
  if (ClaimI2C(SeriesTransmissionTimeout)) {
#if (TYPE == 0)
    Wire.beginTransmission(THPSENSORI2C);
    Wire.write(0xE5);
    Wire.endTransmission();
    Wire.requestFrom(THPSENSORI2C, 2);
    HumidityR = 0;
    for (int i = 0; i < 2; i++) {
      if (Wire.available()) {
        HumidityR |= Wire.read() << (8 * (1 - i));
      }
      else {
        Serial.println("F");
      }
    }
    Wire.beginTransmission(THPSENSORI2C);
    Wire.write(0xE0); // E0 = register for siste T-måling ved RH måling
    Wire.endTransmission();
    Wire.requestFrom(THPSENSORI2C, 2);
    TemperatureR = 0;
    for (int i = 0; i < 2; i++) {
      if (Wire.available()) {
        TemperatureR |= Wire.read() << (8 * (1 - i));
      }
      else {
        Serial.println("F");
      }
    }
    BatteryR = analogRead(A2);
    Serial.println(BatteryR);
#elif (TYPE == 1)
    TemperatureR = ((uint16_t)(((double)bme.readTemperature() + 46.85) * 65536 / 175.72));
    HumidityR = (uint32_t)(((double)bme.readHumidity() + 6) * 65536 / 125);
    PressureR = (uint32_t)((double)bme.readPressure() * 100);
    BatteryR = analogRead(A0);
    //inkluderer BME library

#endif
    LeaveI2C();
  }
}
