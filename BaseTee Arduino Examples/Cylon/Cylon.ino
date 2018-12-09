#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 660 

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806, define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 2
#define CLOCK_PIN 14
#define DATA_PIN2 8
#define CLOCK_PIN2 6
#define DATA_PIN3 21
#define CLOCK_PIN3 5
#define DATA_PIN4 48
#define CLOCK_PIN4 55
#define DATA_PIN5 52
#define CLOCK_PIN5 51

// Define the array of leds
CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS];
CRGB leds3[NUM_LEDS];
CRGB leds4[NUM_LEDS];
CRGB leds5[NUM_LEDS];

void setup() { 
  pinMode(13, OUTPUT);
  digitalWrite(13,HIGH);
	Serial.begin(57600);
	Serial.println("resetting");
  LEDS.addLeds<APA102,DATA_PIN,CLOCK_PIN,RGB,DATA_RATE_MHZ(8)>(leds,NUM_LEDS);
  LEDS.addLeds<APA102,DATA_PIN2,CLOCK_PIN2,RGB,DATA_RATE_MHZ(8)>(leds2,NUM_LEDS);
  LEDS.addLeds<APA102,DATA_PIN3,CLOCK_PIN3,RGB,DATA_RATE_MHZ(8)>(leds3,NUM_LEDS);
	LEDS.setBrightness(84);
}

void fadeall() 
{ 
  for(int i = 0; i < NUM_LEDS; i++) 
  { 
    leds[i].nscale8(250); 
    leds2[i] = leds[i];
    leds3[i] = leds[i];
    leds4[i] = leds[i];
    leds5[i] = leds[i];
  } 
}

void loop() { 
	static uint8_t hue = 0;
	Serial.print("x");
	// First slide the led in one direction
	for(int i = 0; i < NUM_LEDS; i++) {
		// Set the i'th led to red 
		leds[i] = CHSV(hue++, 255, 255);
    leds2[i] = leds[i];
    leds3[i] = leds[i];
    leds4[i] = leds[i];
    leds5[i] = leds[i];
		// Show the leds
		FastLED.show(); 
		// now that we've shown the leds, reset the i'th led to black
		// leds[i] = CRGB::Black;
		fadeall();
		// Wait a little bit before we loop around and do it again
		delay(10);
	}
	Serial.print("x");

	// Now go in the other direction.  
	for(int i = (NUM_LEDS)-1; i >= 0; i--) {
		// Set the i'th led to red 
		leds[i] = CHSV(hue++, 255, 255);
    leds2[i] = leds[i];
    leds3[i] = leds[i];
    leds4[i] = leds[i];
    leds5[i] = leds[i];
		// Show the leds
		FastLED.show();
		// now that we've shown the leds, reset the i'th led to black
		// leds[i] = CRGB::Black;
		fadeall();
		// Wait a little bit before we loop around and do it again
		delay(10);
	}
}
