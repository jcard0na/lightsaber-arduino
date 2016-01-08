#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <JQ6500_Serial.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

// Create the mp3 module object, 
//   Arduino Pin 4 is connected to TX of the JQ6500
//   Arduino Pin 5 is connected to one end of a  1k resistor, 
//     the other end of the 1k resistor is connected to RX of the JQ6500
//   If your Arduino is 3v3 powered, you can omit the 1k series resistor
JQ6500_Serial mp3(4,5);

#define PIN 13

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(28, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.
uint32_t c;
uint32_t c_saved;
int color_addr = 0;
bool blade_out = false;

int const RED_BUTTON = 9;
int const YLW_BUTTON = 10;
int const TILT_SENSOR = 11;

int tilt_sensor;

uint32_t load_saved_color() {
  byte r = EEPROM.read(color_addr);
  byte g = EEPROM.read(color_addr+1);
  byte b = EEPROM.read(color_addr+2);
  if (r + g + b == 0)
    b = 255;  // default to blue if no saved color
  return (uint32_t) r << 16 | (uint32_t) g << 8 | b;
}


void save_color(uint32_t c) {
  EEPROM.write(color_addr+2, c & 0xff);
  EEPROM.write(color_addr+1, (c >> 8) & 0xff);
  EEPROM.write(color_addr, (c >> 16) & 0xff);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
    for(uint16_t i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

// Fill the dots one after the other with a color
void colorWipeRev(uint32_t c, uint8_t wait) {
  for(int16_t i=strip.numPixels() - 1; i >= 0; i--) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

// Which one will turn on?
void randomLight() {
  for(uint16_t i=0; i < strip.numPixels(); i++) {
    int heads = random(2);
    uint32_t c = random(0x1000000);
    if (heads) 
      strip.setPixelColor(i, c);
    else
      strip.setPixelColor(i, 0);
    strip.show();
  }
}

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  mp3.begin(9600);
  mp3.reset();
  mp3.setVolume(20);
  mp3.setLoopMode(MP3_LOOP_NONE);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  pinMode(RED_BUTTON, INPUT_PULLUP);
  pinMode(YLW_BUTTON, INPUT_PULLUP);
  pinMode(TILT_SENSOR, INPUT_PULLUP);

  randomSeed(analogRead(0));
  c = load_saved_color();
  c_saved = c;
}

void loop() {

  if (digitalRead(YLW_BUTTON) == HIGH) {
    if (!blade_out && digitalRead(RED_BUTTON) == LOW) {
      colorWipe(c_saved, 1); //
      c = c_saved; 
      blade_out = true;
      mp3.playFileByIndexNumber(2);
    } else if (blade_out && digitalRead(RED_BUTTON) == LOW) {
      mp3.playFileByIndexNumber(1);
      colorWipeRev(strip.Color(0, 0, 0), 50); // off
      if (c != c_saved) {
        save_color(c);
        c_saved = c;
      }
      blade_out = false;
    }
  } else if (digitalRead(YLW_BUTTON) == LOW) {
    mp3.playFileByIndexNumber(3);
    if (digitalRead(RED_BUTTON) == HIGH) {
      int r = random(0x100);
      int b = random(0x100);
      int g = max(0, 255 - (r + b));
      c = strip.Color(r,g,b);
      colorWipe(strip.Color(r,g,b), 10);
      blade_out = true;
    } else if (digitalRead(RED_BUTTON) == LOW) {
      // TBD
    }
  }
  
  int tilt_sensor_tmp = digitalRead(TILT_SENSOR);
  if (tilt_sensor != tilt_sensor_tmp && blade_out) {
    tilt_sensor = tilt_sensor_tmp;
    mp3.playFileByIndexNumber(1);
  }
 
}
