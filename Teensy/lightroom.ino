

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include "eeprom_addr.h"
#include "EEPROMAnything.h"



float MIN_DB;          // Audio intensity that maps to low LED brightness.
float MAX_DB;          // Audio intensity that maps to high LED brightness.
uint8_t LEDS_ENABLED;                  // Control if the LED's should display the spectrum or not.  1 is true, 0 is false.
                                       // Useful for turning the LED display on and off with commands from the serial port.
                                       // without running out of memory for buffers and other state.
const int AUDIO_INPUT_PIN = 14;        // Input ADC pin for audio data.                        
const int POWER_LED_PIN = 13;          // Output pin for power LED (pin 13 to use Teensy 3.0's onboard LED).
const int NEO_PIXEL_PIN = 2;           // Output pin for neo pixels.
const int NEO_PIXEL_COUNT = 24;        // Number of neo pixels.  You should be able to increase this without
                                       // any other changes to the program.
const int MAX_CHARS = 65;              // Max size of the input command buffer

const int UART_BITRATE = 38400;

const double OFFSET_VALUE = 0.2;

//////////////////////////////////////////////////////////////////////////////
// DAVE'S NEW STUFF
//////////////////////////////////////////////////////////////////////////////
const int BAND_COUNT = 3;
int BASS_FREQ;
int MID_FREQ;
int TREBLE_FREQ;
int BAND_FREQUENCY[BAND_COUNT];
const float GAMMA = 0.7;

uint8_t PIXEL_BRIGHTNESS;
uint8_t FIXED_RED;
uint8_t FIXED_GREEN;
uint8_t FIXED_BLUE;

int RAINBOW_DELAY;
int CO_SPREAD_DELAY;
int STROBE_DELAY;

int COLOR_CHANGE_CUTOFF;

enum light_mode {
  CO_ALL, // all pixel same color organ
  CO_SPREAD, // spread out from the center
  RAINBOW_ALL, // all pixels same rainbow shade
  RAINBOW_CHASE,
  FIXED_COLOR,
  STROBE,
  YO
};

light_mode MODE;
light_mode prev_mode;

HardwareSerial Uart = HardwareSerial();

static const int FFT_SIZE = 1024;
AudioInputAnalog         adc1(AUDIO_INPUT_PIN);
AudioAnalyzeFFT1024      fft;       
AudioConnection          patchCord1(adc1, fft);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NEO_PIXEL_COUNT, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
char commandBuffer[MAX_CHARS];

const uint32_t COLOR_WHITE = pixels.Color(255,255,255);
const uint32_t COLOR_BLACK = pixels.Color(0,0,0);
const uint32_t COLOR_PURPLE = pixels.Color(255,0,255);


////////////////////////////////////////////////////////////////////////////////
// MAIN SKETCH FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

void setup() {
  delay(5);  // see http://forum.pjrc.com/threads/24811-Driving-NeoPixel-144-with-Teensy-3

  //firstTime();
  readAllValues();

  // Set up serial port.
  Uart.begin(UART_BITRATE);
  
  // Turn on the power indicator LED.
  pinMode(POWER_LED_PIN, OUTPUT);
  digitalWrite(POWER_LED_PIN, HIGH);
  
  // Initialize neo pixel library and turn off the LEDs
  pixels.begin();
  pixels.show(); 
  
  // Clear the input command buffer
  memset(commandBuffer, 0, sizeof(commandBuffer));
  
  AudioMemory(12);
  fft.windowFunction(AudioWindowHanning256);
}

void loop() {

  switch (MODE) {
      case CO_ALL:
        spectrumLoop();
        break;
      case CO_SPREAD:
        spectrumLoop();
        break;
      case RAINBOW_ALL:
        rainbow(RAINBOW_DELAY, true);
        break;
      case RAINBOW_CHASE:
        rainbow(RAINBOW_DELAY, false);
        break;
      case FIXED_COLOR:
        setOneColor(pixels.Color(FIXED_RED, FIXED_GREEN, FIXED_BLUE));
        delay(5);
        break;
      case STROBE:
        setOneColor(COLOR_WHITE);
        delay(10);
        setOneColor(COLOR_BLACK);
        delay(STROBE_DELAY);
        break;
       case YO:
        setOneColor(COLOR_PURPLE);
        delay(1000);
        MODE = prev_mode;  // TODO: This doesn't get saved in eeprom.  maybe it shouldn't?
        break;
      default:
        // do something
        break;
  }
  
  // Parse any pending commands.
  parserLoop();
}


////////////////////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS
////////////////////////////////////////////////////////////////////////////////


// Convert a frequency to the appropriate FFT bin it will fall within.
int frequencyToBin(float frequency) {
  float binFrequency = float(44100) / float(FFT_SIZE);
  return int(frequency / binFrequency) + 1;
}


////////////////////////////////////////////////////////////////////////////////
// SPECTRUM DISPLAY FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

void spectrumLoop() {
  static uint8_t a = 0;
  static uint8_t b = 1;
  static uint8_t c = 2;

    // Calculate FFT if a full sample is available.
  if (fft.available()) {

    // Update each LED based on the intensity of the audio 
    // in the associated frequency window.
    float intensity;
    int rgb[BAND_COUNT];
    for (int i = 0; i < BAND_COUNT; ++i) {
      float offset = OFFSET_VALUE * BAND_FREQUENCY[i];
      intensity = fft.read(frequencyToBin(BAND_FREQUENCY[i] - offset), frequencyToBin(BAND_FREQUENCY[i] + offset));
      intensity *= 16384;
      // Convert intensity to decibels (sort of).
      intensity = 20.0*log10(intensity);
      // Scale the intensity and clamp between 0 and 1.0.
      intensity -= MIN_DB;
      intensity = intensity < 0.0 ? 0.0 : intensity;
      intensity /= (MAX_DB-MIN_DB);
      intensity = intensity > 1.0 ? 1.0 : intensity;

      rgb[i] = (int) pow(256, pow(intensity,GAMMA)) - 1;  // might want to move this to a look up table-like thing.
      rgb[i] = rgb[i] > 255 ? 255 : rgb[i];
    }

    if(MODE == CO_ALL){
      if( rgb[0] > COLOR_CHANGE_CUTOFF){
        a = (a + 1) % 3;
        b = (b + 1) % 3;
        c = (c + 1) % 3;
      }

      for( int j = 0; j < NEO_PIXEL_COUNT; ++j){
        pixels.setPixelColor(j, pixels.Color(rgb[a], rgb[b], rgb[c]));
      }
    }
    else if(MODE == CO_SPREAD){
      delay(CO_SPREAD_DELAY);

      if (NEO_PIXEL_COUNT % 2 == 0){
        for (int j = 0; j < NEO_PIXEL_COUNT / 2 - 1; j++){
          pixels.setPixelColor(j, pixels.getPixelColor(j + 1));
          pixels.setPixelColor(NEO_PIXEL_COUNT - j - 1, pixels.getPixelColor(NEO_PIXEL_COUNT - j - 2));
        }

        pixels.setPixelColor(NEO_PIXEL_COUNT/2 - 1, pixels.Color(rgb[0], rgb[1], rgb[2]));
        pixels.setPixelColor(NEO_PIXEL_COUNT/2, pixels.Color(rgb[0], rgb[1], rgb[2]));
      }
      else{
        for (int j = 0; j < NEO_PIXEL_COUNT / 2; j++){
          pixels.setPixelColor(j, pixels.getPixelColor(j + 1));
          pixels.setPixelColor(NEO_PIXEL_COUNT - j, pixels.getPixelColor(NEO_PIXEL_COUNT - j - 1));
        }

        pixels.setPixelColor(NEO_PIXEL_COUNT/2, pixels.Color(rgb[0], rgb[1], rgb[2]));
      }
    }
    
    //pixels.setBrightness(255);  // set back to mx brightness, might have been changed.....
    pixels.show();
  }
}

void rainbow(int wait, bool setAll) {
  static uint8_t j = 0;
    for(int i=0; i < NEO_PIXEL_COUNT; i++) {
      if (setAll) {
        pixels.setPixelColor(i, Wheel(j));
      }
      else {
        pixels.setPixelColor(i, Wheel(((i * 256 / NEO_PIXEL_COUNT) + j) & 255));
      }
    }

    j++;
    pixels.setBrightness(PIXEL_BRIGHTNESS);
    pixels.show();
    delay(wait);
  }

  void setOneColor(uint32_t color){
    for(int i = 0; i < NEO_PIXEL_COUNT; i++){
      pixels.setPixelColor(i, color);
    }

    pixels.setBrightness(PIXEL_BRIGHTNESS);
    pixels.show();
  }


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(uint8_t WheelPos) {
  if(WheelPos < 85) {
   return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


////////////////////////////////////////////////////////////////////////////////
// COMMAND PARSING FUNCTIONS
// These functions allow parsing simple commands input on the serial port.
// Commands allow reading and writing variables that control the device.
//
// All commands must end with a semicolon character.
// 
// Example commands are:
// GET MODE;
// - Get the mode of the device.
// SET MODE 2;
// - Set the mode of th device to 2.
// 
////////////////////////////////////////////////////////////////////////////////

void parserLoop() {
  // Process any incoming characters from the serial port
  //while (Serial.available() > 0) {
  while (Uart.available() > 0) {  
    //char c = Serial.read();
    char c = Uart.read();
    // Add any characters that aren't the end of a command (semicolon) to the input buffer.
    if (c != ';') {
      c = toupper(c);
      strncat(commandBuffer, &c, 1);
    }
    else
    {
      // Parse the command because an end of command token was encountered.
      parseCommand(commandBuffer);
      // Clear the input buffer
      memset(commandBuffer, 0, sizeof(commandBuffer));
    }
  }
}

// Macro used in parseCommand function to simplify parsing get and set commands for a variable
#define GET_AND_SET(variableName) \
  else if (strcmp(command, "GET " #variableName) == 0) { \
    Uart.println(variableName); \
  } \
  else if (strstr(command, "SET " #variableName " ") != NULL) { \
    variableName = (typeof(variableName)) atof(command+(sizeof("SET " #variableName " ")-1)); \
    EEPROM_writeAnything(variableName ## _ADDR , variableName); \
    Uart.println(variableName); \
  }

void parseCommand(char* command) {
  if (strstr(command, "SET MODE ") != NULL) { 
    prev_mode = MODE;
  }
  if(false){}  // awkward!
  GET_AND_SET(MODE)
  GET_AND_SET(LEDS_ENABLED)
  GET_AND_SET(MIN_DB)
  GET_AND_SET(MAX_DB)
  GET_AND_SET(BASS_FREQ)
  GET_AND_SET(TREBLE_FREQ)
  GET_AND_SET(MID_FREQ)
  GET_AND_SET(PIXEL_BRIGHTNESS)
  GET_AND_SET(RAINBOW_DELAY)
  GET_AND_SET(FIXED_RED)
  GET_AND_SET(FIXED_GREEN)
  GET_AND_SET(FIXED_BLUE)
  GET_AND_SET(STROBE_DELAY)
  GET_AND_SET(CO_SPREAD_DELAY)
  GET_AND_SET(COLOR_CHANGE_CUTOFF)
  else {
    Uart.println("ERROR");
  }

  BAND_FREQUENCY[0] = BASS_FREQ;
  BAND_FREQUENCY[1] = MID_FREQ;
  BAND_FREQUENCY[2] = TREBLE_FREQ;
  
  // Turn off the LEDs if the state changed.
  if (LEDS_ENABLED == 0) {
    for (int i = 0; i < NEO_PIXEL_COUNT; ++i) {
      pixels.setPixelColor(i, 0);
    }

    pixels.show();
  }
}

void firstTime(){
  EEPROM_writeAnything(MIN_DB_ADDR, (float) 45.0);
  EEPROM_writeAnything(MAX_DB_ADDR, (float) 65.0);
  EEPROM_writeAnything(LEDS_ENABLED_ADDR, (uint8_t) 1);
  EEPROM_writeAnything(BASS_FREQ_ADDR, (int) 100);
  EEPROM_writeAnything(MID_FREQ_ADDR, (int) 600);
  EEPROM_writeAnything(TREBLE_FREQ_ADDR, (int) 4000);
  EEPROM_writeAnything(PIXEL_BRIGHTNESS_ADDR, (uint8_t) 200);
  EEPROM_writeAnything(FIXED_RED_ADDR, (uint8_t) 0);
  EEPROM_writeAnything(FIXED_GREEN_ADDR, (uint8_t) 255);
  EEPROM_writeAnything(FIXED_BLUE_ADDR, (uint8_t) 255);
  EEPROM_writeAnything(RAINBOW_DELAY_ADDR, (int) 100);
  EEPROM_writeAnything(CO_SPREAD_DELAY_ADDR, (int) 10);
  EEPROM_writeAnything(STROBE_DELAY_ADDR, (int) 75);
  EEPROM_writeAnything(MODE_ADDR, RAINBOW_ALL);
  EEPROM_writeAnything(COLOR_CHANGE_CUTOFF_ADDR, 750);
}

void readAllValues(){
  EEPROM_readAnything(MIN_DB_ADDR, MIN_DB);
  EEPROM_readAnything(MAX_DB_ADDR, MAX_DB);
  EEPROM_readAnything(LEDS_ENABLED_ADDR, LEDS_ENABLED);
  EEPROM_readAnything(BASS_FREQ_ADDR, BASS_FREQ);
  EEPROM_readAnything(MID_FREQ_ADDR, MID_FREQ);
  EEPROM_readAnything(TREBLE_FREQ_ADDR, TREBLE_FREQ);
  EEPROM_readAnything(PIXEL_BRIGHTNESS_ADDR, PIXEL_BRIGHTNESS);
  EEPROM_readAnything(FIXED_RED_ADDR, FIXED_RED);
  EEPROM_readAnything(FIXED_GREEN_ADDR, FIXED_GREEN);
  EEPROM_readAnything(FIXED_BLUE_ADDR, FIXED_BLUE);
  EEPROM_readAnything(RAINBOW_DELAY_ADDR, RAINBOW_DELAY);
  EEPROM_readAnything(CO_SPREAD_DELAY_ADDR, CO_SPREAD_DELAY);
  EEPROM_readAnything(STROBE_DELAY_ADDR, STROBE_DELAY);
  EEPROM_readAnything(MODE_ADDR, MODE);
  EEPROM_readAnything(COLOR_CHANGE_CUTOFF_ADDR, COLOR_CHANGE_CUTOFF);

  BAND_FREQUENCY[0] = BASS_FREQ;
  BAND_FREQUENCY[1] = MID_FREQ;
  BAND_FREQUENCY[2] = TREBLE_FREQ;
}
