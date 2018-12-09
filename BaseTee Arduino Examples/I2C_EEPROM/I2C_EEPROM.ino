/*
 * A simple hardware test which reads MAC address
 * from EEPROM and prints it out on the Serial Monitor
 *
 * 
 *
 * This example code is in the public domain.
 */
#include <i2c_t3.h>

const byte EEPROM_I2C_ADDR = 0x50;
char mac_address[6];

void setup() 
{
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  while (!Serial) 
  {
    // Wait for serial port to connect.
  }

  Serial.println("Beginning EEPROM I2C transaction.");
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  
  delay(100);
  readMACAddress();

}

void loop() 
{

}

void readMACAddress(void)
{
  Wire.beginTransmission(EEPROM_I2C_ADDR);
  Wire.write(0xFA);  // Address of first byte of MAC address
  Wire.endTransmission(I2C_NOSTOP);
  // no STOP command

  int i = 0;
  Wire.requestFrom(EEPROM_I2C_ADDR, 6);
  while(Wire.available()) 
  {
    mac_address[i] = Wire.read();
    Serial.print(mac_address[i], HEX);
    i++;
  }
}

