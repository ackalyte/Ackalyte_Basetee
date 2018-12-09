/*
* Quad channel output test
* Play two WAV files on two audio shields.
*
* Connect BaseTee Line-in A/B 
* to computer’s audio output port. 
* Connect headphones, earphones or speakers to BaseTee either through 
* HEADPHONE A/B or LINE OUT A/B jack.
*
* Data files to put on the SD card can be downloaded here:
* http://www.pjrc.com/teensy/td_libs_AudioDataFiles.html
*
* This example code is in the public domain.
*/
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioPlaySdWav           playSdWav1;
AudioPlaySdWav           playSdWav2;
AudioOutputI2SQuad       audioOutput;
AudioConnection          patchCord1(playSdWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playSdWav1, 1, audioOutput, 1);
AudioConnection          patchCord3(playSdWav2, 0, audioOutput, 2);
AudioConnection          patchCord4(playSdWav2, 1, audioOutput, 3);
AudioControlSGTL5000     sgtl5000_1;
AudioControlSGTL5000     sgtl5000_2;

// Use these with the audio adaptor board
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

void setup() {
  Serial.begin(9600);
  AudioMemory(10);
  
  sgtl5000_1.setAddress(LOW);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  sgtl5000_2.setAddress(HIGH);
  sgtl5000_2.enable();
  sgtl5000_2.volume(0.5);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(BUILTIN_SDCARD))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
}

void loop() {
  if (playSdWav1.isPlaying() == false) {
    Serial.println("Start playing 1");
    playSdWav1.play("SDTEST1.WAV");
    delay(10); // wait for library to parse WAV info
  }
  if (playSdWav2.isPlaying() == false) {
    Serial.println("Start playing 2");
    playSdWav2.play("SDTEST3.WAV");
    delay(10); // wait for library to parse WAV info
  }
}

