#include <AckaLED.h>
const int ledsPerStrip = 10;
DMAMEM int displayMemory[ledsPerStrip*16];
int drawingMemory[ledsPerStrip*16];
#define GREEN_COLOR 0x0000FF00
#define RED_COLOR   0x00FF0000
#define BLUE_COLOR  0x000000FF
#define WHITE_COLOR 0xFF000000
#define BLACK_COLOR 0x00000000
const int config = SK6812_GRBW | SK6812_800kHz;
AckaLED leds(ledsPerStrip, displayMemory, drawingMemory, config);

void setup() {  leds.begin();}

void loop() {
leds.setPixel(0,GREEN_COLOR);   leds.show();  delay(1000);
leds.setPixel(0,RED_COLOR);     leds.show();  delay(1000);
leds.setPixel(0,BLUE_COLOR);    leds.show();  delay(1000);
leds.setPixel(0,WHITE_COLOR);   leds.show();  delay(1000);
leds.setPixel(0,BLACK_COLOR);   leds.show();  delay(1000);
}