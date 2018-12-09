# Ackalyte Basetee

Repository folder description:

* **BaseTee Arduino Examples**: Contains various basic Teensy Arduino examples to test different functionailties of the [Ackalyte BaseTee](https://ackalyte.com/products/basetee).
* **library**: Ackalyte's LED (**AckaLED**) library to drive addressable LEDS accross 15 channels over DMA via [Ackalyte BaseTee](https://ackalyte.com/products/basetee).
* **Teensy modifed library**: Contains modified library for teensy to harmonise with [Ackalyte BaseTee](https://ackalyte.com/products/basetee).

## Install

### Teensy Modified Library
* Copy **Audio** and **Ethernet** directory from "_Teensy modified library_" and replace it with the original Teensy libraries (_PATH_TO_ARDUINO_/hardware/teensy/avr/libraries). Make sure you are running the latest teensyduino. 
* Overwrite the files when asked.

### Ackalyte LED (AckaLED) Library
Unlike OctoWS2811 library, AckaLED utilizes all the 15 channels for transmitting LED data. It also supports RGBW type LED.

* Copy the AckaLED directory from library folder and paste it under Arduino library folder.

## AckaLED Simple Example
Here's a simple RGBW blinky example:

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

## For more information

Check out the official site http://Ackalyte.com for links to datasheets, prices, and news