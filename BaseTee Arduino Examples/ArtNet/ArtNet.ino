#include <AckaLED.h>
#include <Artnet.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <i2c_t3.h>

/*****************************************************************************/
/*                            User Defined Values                            */
/*****************************************************************************/
//Comment the following line to use static IP address
#define USE_DHCP

//This is the static IP address or the fallback if DHCP fails, do not comment
IPAddress ip(192, 168, 1, 4);

const int ledsPerStrip = 170; //Maximum number of RGB LED that can be received 
                              //per Art-Net Universe (128 for RGBW)

/* The LED configuration contains two parts.
 * - Pixel configuration: WS2811_RGB
 *                        WS2811_RBG
 *                        WS2811_GRB
 *                        WS2811_GBR
 *                        SK6812_GRBW
 * 
 * - LED timing: WS2811_800kHz
 *               WS2811_400kHz
 *               WS2813_800kHz
 *               SK6812_800kHz
 */
const int config = WS2811_GRB | WS2811_800kHz;
/*****************************************************************************/
/*                         End of User Defined Values                        */
/*****************************************************************************/

const byte EEPROM_I2C_ADDR = 0x50;
byte mac[] =  {0x80, 0x1F, 0x12, 0x43, 0xE4, 0x01};
char hostname[12];

int drawingMemory[ledsPerStrip*16];
int numColours;

DMAMEM int displayMemory[ledsPerStrip*16]; // 16-bit int * num leds * num colours * 2 * 2 
AckaLED leds(ledsPerStrip, displayMemory, drawingMemory, config);

Artnet artnet;

#define EMPTY_UNIVERSE -1
int firstUniverse;

void setup()
{
  //First things first, LEDs should be initialised and blanked
  leds.begin();
  leds.show();
  
  Serial.begin(115200); delay(500);
  
  readMACAddress();  
  
  Ethernet.init(31); // CS pin 

#ifdef USE_DHCP
  sprintf(hostname, "BaseTee-%.2X%.2X", mac[4], mac[5]);
  if (Ethernet.begin(mac, hostname) == 0) {
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
    Serial.print("DHCP failed, static IP address used instead: ");
  } else {
    Serial.print("DHCP assigned IP: ");
  }
  Serial.println(Ethernet.localIP());
  Serial.print("Hostname: ");
  Serial.println(Ethernet.hostname());    
#else
  Ethernet.begin(mac, ip);
  Serial.print("Static IP: ");  
  Serial.println(Ethernet.localIP());
#endif
   
  artnet.begin();

  if ((config & 0x07) == SK6812_GRBW) 
    numColours = 4;
  else numColours = 3;  
  
  // this will be called for each packet received
  artnet.setArtDmxCallback(onDmxFrame);
  delay(2);
}

void loop()
{
  // we call the read function inside the loop
  artnet.read();
}

void readMACAddress(void)
{
  Wire.begin();  
  Wire.beginTransmission(EEPROM_I2C_ADDR);
  Wire.write(0xFA);  // Address of first byte of MAC address
  Wire.endTransmission(I2C_NOSTOP);
  // no STOP command

  Serial.print("MAC Address: ");
  int i = 0;
  Wire.requestFrom(EEPROM_I2C_ADDR, 6);
  while(Wire.available()) 
  {
    mac[i] = Wire.read();
    Serial.print(mac[i], HEX);
    Serial.print(" ");
    i++;
  }
  Serial.println("");
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
 /* To avoid the performance penalty of redundant LED refresh, we want to call the function
  * leds.show() only when we have a complete frame.
  * 
  * We keep record of the first Universe received and when it is received again, it is considered
  * a new frame. Thus, the LEDs are updated with the data so far before reading the first Universe
  * of the next frame.
  */
  if (universe == firstUniverse) {
    leds.show();
  } else {
    if (firstUniverse == EMPTY_UNIVERSE) firstUniverse = universe;
  }

  //If the Universe is larger than the number of channels, we stop.
  if (universe > 15) return;

  /* Channel 10 in BaseTee is not used, so we map channels 10-15 to 11-16 */
  if (universe > 9) universe++;

  int pixel_index = ledsPerStrip * universe;
  int strip_end = pixel_index + ledsPerStrip;
  int data_index = 0;

  while ((pixel_index < strip_end) && (data_index < length - numColours)) {
    if (numColours == 4) {
      leds.setPixel(pixel_index, data[data_index], data[data_index+1], data[data_index+2], data[data_index+3]);
    } else {
      leds.setPixel(pixel_index, data[data_index], data[data_index+1], data[data_index+2]);
    }
    data_index += numColours;
    pixel_index++;
  }
}
