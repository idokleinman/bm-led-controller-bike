// This #include statement was automatically added by the Particle IDE.
#include <FastLED.h>

/*
 * Original code:
 * https://github.com/evilgeniuslabs/eclipse
 * Copyright (C) 2015 Jason Coon, Evil Genius Labs
 * Modified for Particle Photon by Ido Kleinman, 2019 for Burning Man LED controllers (c)
 * Hardware: Particle Photon + Battery shield, Neo pixel strip, button
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// #include "FastLED.h"
FASTLED_USING_NAMESPACE;

#include "GradientPalettes.h"
#include "application.h"
/// test

SYSTEM_MODE(SEMI_AUTOMATIC);

// allow us to use itoa() in this scope
extern char* itoa(int a, char* buffer, unsigned char radix);

#define NUM_LEDS    150
#define DATA_PIN    A5	// adjust this to the pin you've connected your LEDs to

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
#define LED_TYPE    WS2812B	// adjust this to the type of LEDS. This is for Neopixels

int buttonPin =	D3;	// Connect the button to GND and one of the pins.
// int offlinePin = D7;
int onboardLedPin = D7;

const int brightnessLevelsArr[] = {48, 96, 144, 255};
int brightnessLevelsCount = ARRAY_SIZE(brightnessLevelsArr);

int brightnessLevel = 0;
// unsigned long doublePressTime = 350;
unsigned long patternChangeTime = 10000; // change pattern every 15 sec
unsigned long longPressTime = 3000; // 3 sec press to shutdown

#define COLOR_ORDER GRB  // Try mixing up the letters (RGB, GBR, BRG, etc) for a whole new world of color combinations

uint8_t gHue = 0; // rotating "base color" used by many of the patterns



CRGB leds[NUM_LEDS];

#include "MeteorShower.h"

typedef uint8_t (*SimplePattern)();
typedef SimplePattern SimplePatternList[];
typedef struct { SimplePattern drawFrame;  String name; } PatternAndName;
typedef PatternAndName PatternAndNameList[];

// List of patterns to cycle through.  Each is defined as a separate function below.                                                                                                                                                                                                                    

const PatternAndNameList patterns =
{
  { colorWaves,             "Color Waves" },
  { pride,                  "Pride" },
  // { rainbowTwinkles,        "Rainbow Twinkles" },
  // { snowTwinkles,           "Snow Twinkles" },
  // { cloudTwinkles,          "Cloud Twinkles" },
  // { incandescentTwinkles,   "Incandescent Twinkles" },
  // { rainbow,                "Rainbow" },
  { rainbowWithGlitter,     "Rainbow With Glitter" },
  // { rainbowSolid,           "Solid Rainbow" },
  // { confetti,               "Confetti" },
  { sinelon,                "Sinelon" },
  { juggle,                 "Juggle" },
  { meteorShower,           "Meteor Shower" },
  { juggle2,                "Juggle 2" },
  { bpm,                    "Beat" },
  { water,                  "Water" },
  { fire,                   "Fire" },
  // { analogClock,            "Analog Clock" },
  // { showSolidColor,         "Solid Color" }
};

int patternCount = ARRAY_SIZE(patterns);

// variables exposed via Particle cloud API (Spark Core is limited to 10)
int brightness = 255;
int patternIndex = 0;
String patternNames = "";
int power = 1;
int r = 0;
int g = 0;
int b = 255;
int flipClock = 0;
int timezone = -6;

unsigned long lastTimeSync = millis();

CRGB solidColor = CRGB(r, g, b);

CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

uint8_t paletteIndex = 0;

// List of palettes to cycle through.
CRGBPalette16 palettes[] =
{
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  OceanColors_p,
  ForestColors_p,
  HeatColors_p,
  LavaColors_p,
  PartyColors_p,
  IceColors_p,
};

uint8_t paletteCount = ARRAY_SIZE(palettes);

CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 targetPalette = palettes[paletteIndex];

// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
#define SECONDS_PER_PALETTE 20

int lastButtonState = HIGH;   // the previous reading from the input pin
int buttonState;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


void setup()
{
    // FastLED.addLeds<APA102, D1, A3>(leds, NUM_LEDS);
    FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS);

    FastLED.setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(brightness);
    FastLED.setDither(false);
    fill_solid(leds, NUM_LEDS, solidColor);
    FastLED.show();

    
    pinMode(onboardLedPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    
    Serial.begin(9600);
    Serial.println("Hello - LED controller");

    // load settings from EEPROM
    brightness = 128; 
    
    FastLED.setBrightness(brightness);
    FastLED.setDither(brightness < 255);

    patternIndex = 0;
    
    r = 0;
    g = 0;
    b = 255;
    
    solidColor = CRGB(r, b, g);

    patternNames = "[";
    for(uint8_t i = 0; i < patternCount; i++)
    {
      patternNames.concat("\"");
      patternNames.concat(patterns[i].name);
      patternNames.concat("\"");
      if(i < patternCount - 1)
        patternNames.concat(",");
    }
    patternNames.concat("]");
    Particle.variable("patternNames", patternNames);

    Time.zone(timezone);
}

int s = 0;
unsigned long buttonTime = -1;
unsigned long patternTime = -1;

// boolean buttonPressed = false;
boolean goToSleep = false;

void loop()
{
  buttonState = digitalRead(buttonPin);

  // detected start of press
  if ((buttonState == LOW) && (lastButtonState == HIGH)) {
    buttonTime = millis(); 
  }

  if (buttonState == LOW) {
    if ((millis() - buttonTime) > longPressTime) {
      Serial.println("Sleep");
        // turn off the LEDs
      setBrightness(String(0));
      FastLED.show();
      goToSleep = true;
    }
  }
  
  if ((buttonState == HIGH) && (lastButtonState == LOW)) {
    // that's a regular press 
    if (goToSleep) { // end of a long press
      setBrightness(String(0)); // make sure LEDs are off
      FastLED.show();
      // wake up with previous brightness
      setBrightness(String(brightnessLevelsArr[brightnessLevel]));
      goToSleep = false;
      delay(100);
      System.sleep(buttonPin,RISING); // TODO: change button to a pin that wakes up (D1, D2, D3, D4, A0, A1, A3, A4, A6, A7)
    } else {
      brightnessLevel++;
      if (brightnessLevel >= brightnessLevelsCount) {
        brightnessLevel = 0;
      }
      int newBrightness = brightnessLevelsArr[brightnessLevel];
      setBrightness(String(newBrightness));
      Serial.println("Set brightness to "+String(newBrightness)); 
    }
  }

  if ((millis() - patternTime) > patternChangeTime) {
    patternIndex++;
    if (patternIndex >= patternCount) {
        patternIndex = 0;
    }
    // EEPROM.write(1, patternIndex);
    Serial.println("Pattern is "+patterns[patternIndex].name);
    patternTime = millis();   
  }

  lastButtonState = buttonState;

  if(power < 1) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    FastLED.delay(15);
    return;
  }

  uint8_t delay = patterns[patternIndex].drawFrame();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();

  // insert a delay to keep the framerate modest
  FastLED.delay(delay);

  // blend the current palette to the next
  EVERY_N_MILLISECONDS(40) {
    nblendPaletteTowardPalette(currentPalette, targetPalette, 16);
  }

  EVERY_N_MILLISECONDS( 40 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  // slowly change to a new palette
  EVERY_N_SECONDS(SECONDS_PER_PALETTE) {
    paletteIndex++;
    if (paletteIndex >= paletteCount) paletteIndex = 0;
    targetPalette = palettes[paletteIndex];
  };
}


int setBrightness(String args)
{
    brightness = args.toInt();
    if(brightness < 1)
        brightness = 1;
    else if(brightness > 255)
        brightness = 255;

    FastLED.setBrightness(brightness);
    FastLED.setDither(brightness < 255);

    // EEPROM.write(0, brightness);

    return brightness;
}


byte parseByte(String args) {
    int c = args.toInt();
    if(c < 0)
        c = 0;
    else if (c > 255)
        c = 255;

    return c;
}


// Patterns from FastLED example DemoReel100: https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino

uint8_t rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 255 / NUM_LEDS);
  return 8;
}

uint8_t rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
  return 8;
}

uint8_t rainbowSolid()
{
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
  return 8;
}

uint8_t confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  return 8;
}

uint8_t sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);
  return 8;
}

uint8_t bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(currentPalette, gHue+(i*2), beat-gHue+(i*10));
  }

  return 8;
}

uint8_t juggle()
{
  static uint8_t    numdots =   4; // Number of dots in use.
  static uint8_t   faderate =   2; // How long should the trails be. Very low value = longer trails.
  static uint8_t     hueinc =  255 / numdots - 1; // Incremental change in hue between each dot.
  static uint8_t    thishue =   0; // Starting hue.
  static uint8_t     curhue =   0; // The current hue
  static uint8_t    thissat = 255; // Saturation of the colour.
  static uint8_t thisbright = 255; // How bright should the LED/display be.
  static uint8_t   basebeat =   5; // Higher = faster movement.

  static uint8_t lastSecond =  99;  // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30; // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand) { // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    switch(secondHand) {
      case  0: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break; // You can change values here, one at a time , or altogether.
      case 10: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 8; basebeat =  3; hueinc =  0; faderate = 8; thishue=random8(); break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30: break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue; // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  for( int i = 0; i < numdots; i++) {
    //beat16 is a FastLED 3.1 function
    leds[beatsin16(basebeat+i+numdots,0,NUM_LEDS)] += CHSV(gHue + curhue, thissat, thisbright);
    curhue += hueinc;
  }

  return 0;
}

uint8_t juggle2()
{
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;

  const uint8_t bpm = 20;
  const uint8_t maxdotcount = 8;

  static uint8_t dotcount = 1;
  static uint8_t dotinc = 1;
  static bool handled = false;

  for( int i = 0; i < dotcount; i++)
  {
    uint8_t pos = beatsin8(bpm, 0, NUM_LEDS / 2, 0, i * (256 / maxdotcount));

    if(i % 2 == 0)
      pos = (NUM_LEDS - 1) - pos;

    leds[pos] |= CHSV(dothue, 255, 255);

    dothue += 256 / dotcount;
  }

  uint8_t newdotpos;

  if(dotinc == 1)
    newdotpos = beatsin8(bpm, 0, NUM_LEDS / 2, 0, dotcount * (256 / maxdotcount));
  else if (dotinc == -1)
    newdotpos = beatsin8(bpm, 0, NUM_LEDS / 2, 0, (dotcount - 1) * (256 / maxdotcount));

  if((newdotpos == NUM_LEDS / 2 || newdotpos == (NUM_LEDS / 2) - 1))
  {
    if(!handled)
    {
      handled = true;

      dotcount += dotinc;

      if(dotcount == maxdotcount - 1 || dotcount == 1)
        dotinc *= -1;
    }
  }
  else
  {
    handled = false;
  }

  return 0;
}

uint8_t fire()
{
    heatMap(HeatColors_p, true);

    return 30;
}

uint8_t water()
{
    heatMap(IceColors_p, false);

    return 30;
}

uint8_t analogClock()
{
    dimAll(220);

    drawAnalogClock(Time.second(), Time.minute(), Time.hourFormat12(), true, true);

    return 8;
}

uint8_t showSolidColor()
{
    fill_solid(leds, NUM_LEDS, solidColor);

    return 30;
}

// Pride2015 by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
uint8_t pride()
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;

  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;

    nblend( leds[pixelnumber], newcolor, 64);
  }

  return 15;
}

uint8_t radialPaletteShift()
{
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // leds[i] = ColorFromPalette( currentPalette, gHue + sin8(i*16), brightness);
    leds[i] = ColorFromPalette(currentPalette, i + gHue, 255, LINEARBLEND);
  }

  return 8;
}

void heatMap(CRGBPalette16 palette, bool up)
{
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    // Add entropy to random number generator; we use a lot of it.
    random16_add_entropy(random(256));

    uint8_t cooling = 55;
    uint8_t sparking = 120;

    // Array of temperature readings at each simulation cell
    static const uint8_t halfLedCount = NUM_LEDS / 2;
    static byte heat[2][halfLedCount];

    byte colorindex;

    for(uint8_t x = 0; x < 2; x++) {
        // Step 1.  Cool down every cell a little
        for( int i = 0; i < halfLedCount; i++) {
          heat[x][i] = qsub8( heat[x][i],  random8(0, ((cooling * 10) / halfLedCount) + 2));
        }

        // Step 2.  Heat from each cell drifts 'up' and diffuses a little
        for( int k= halfLedCount - 1; k >= 2; k--) {
          heat[x][k] = (heat[x][k - 1] + heat[x][k - 2] + heat[x][k - 2] ) / 3;
        }

        // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
        if( random8() < sparking ) {
          int y = random8(7);
          heat[x][y] = qadd8( heat[x][y], random8(160,255) );
        }

        // Step 4.  Map from heat cells to LED colors
        for( int j = 0; j < halfLedCount; j++) {
            // Scale the heat value from 0-255 down to 0-240
            // for best results with color palettes.
            colorindex = scale8(heat[x][j], 240);

            CRGB color = ColorFromPalette(palette, colorindex);

            if(up) {
                if(x == 0) {
                    leds[(halfLedCount - 1) - j] = color;
                }
                else {
                    leds[halfLedCount + j] = color;
                }
            }
            else {
                if(x == 0) {
                    leds[j] = color;
                }
                else {
                    leds[(NUM_LEDS - 1) - j] = color;
                }
            }
        }
    }
}

int oldSecTime = 0;
int oldSec = 0;

void drawAnalogClock(byte second, byte minute, byte hour, boolean drawMillis, boolean drawSecond)
{
    if(Time.second() != oldSec){
        oldSecTime = millis();
        oldSec = Time.second();
    }

    int millisecond = millis() - oldSecTime;

    int secondIndex = map(second, 0, 59, 0, NUM_LEDS);
    int minuteIndex = map(minute, 0, 59, 0, NUM_LEDS);
    int hourIndex = map(hour * 5, 5, 60, 0, NUM_LEDS);

    int millisecondIndex = map(secondIndex + int(millisecond * .06), 0, 60, 0, NUM_LEDS);

    if(millisecondIndex >= NUM_LEDS)
        millisecondIndex -= NUM_LEDS;

    hourIndex += minuteIndex / 12;

    if(hourIndex >= NUM_LEDS)
        hourIndex -= NUM_LEDS;

    // see if we need to reverse the order of the LEDS
    if(flipClock == 1) {
        int max = NUM_LEDS - 1;
        secondIndex = max - secondIndex;
        minuteIndex = max - minuteIndex;
        hourIndex = max - hourIndex;
        millisecondIndex = max - millisecondIndex;
    }

    if(secondIndex >= NUM_LEDS)
        secondIndex = NUM_LEDS - 1;
    else if(secondIndex < 0)
        secondIndex = 0;

    if(minuteIndex >= NUM_LEDS)
        minuteIndex = NUM_LEDS - 1;
    else if(minuteIndex < 0)
        minuteIndex = 0;

    if(hourIndex >= NUM_LEDS)
        hourIndex = NUM_LEDS - 1;
    else if(hourIndex < 0)
        hourIndex = 0;

    if(millisecondIndex >= NUM_LEDS)
        millisecondIndex = NUM_LEDS - 1;
    else if(millisecondIndex < 0)
        millisecondIndex = 0;

    if(drawMillis)
        leds[millisecondIndex] += CRGB(0, 0, 127); // Blue

    if(drawSecond)
        leds[secondIndex] += CRGB(0, 0, 127); // Blue

    leds[minuteIndex] += CRGB::Green;
    leds[hourIndex] += CRGB::Red;
}

// scale the brightness of all pixels down
void dimAll(byte value)
{
  for (int i = 0; i < NUM_LEDS; i++){
    leds[i].nscale8(value);
  }
}

void addGlitter( uint8_t chanceOfGlitter)
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

///////////////////////////////////////////////////////////////////////

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );


uint8_t colorWaves()
{
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  EVERY_N_MILLISECONDS(40) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16);
  }

  colorwaves( leds, NUM_LEDS, gCurrentPalette);

  return 20;
}

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;

  for( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds-1) - pixelnumber;

    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

// Alternate rendering function just scrolls the current palette
// across the defined LED strip.
void palettetest( CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette)
{
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( ledarray, numleds, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
}

// based on ColorTwinkles by Mark Kriegsman: https://gist.github.com/kriegsman/5408ecd397744ba0393e

uint8_t cloudTwinkles()
{
  gCurrentPalette = CloudColors_p; // Blues and whites!
  colortwinkles();
  return 20;
}

uint8_t rainbowTwinkles()
{
  gCurrentPalette = RainbowColors_p;
  colortwinkles();
  return 20;
}

uint8_t snowTwinkles()
{
  CRGB w(85,85,85), W(CRGB::White);

  gCurrentPalette = CRGBPalette16( W,W,W,W, w,w,w,w, w,w,w,w, w,w,w,w );
  colortwinkles();
  return 20;
}

uint8_t incandescentTwinkles()
{
  CRGB l(0xE1A024);

  gCurrentPalette = CRGBPalette16( l,l,l,l, l,l,l,l, l,l,l,l, l,l,l,l );
  colortwinkles();
  return 20;
}

#define STARTING_BRIGHTNESS 64
#define FADE_IN_SPEED       32
#define FADE_OUT_SPEED      20
#define DENSITY            255

enum { GETTING_DARKER = 0, GETTING_BRIGHTER = 1 };

void colortwinkles()
{
  // Make each pixel brighter or darker, depending on
  // its 'direction' flag.
  brightenOrDarkenEachPixel( FADE_IN_SPEED, FADE_OUT_SPEED);

  // Now consider adding a new random twinkle
  if( random8() < DENSITY ) {
    int pos = random16(NUM_LEDS);
    if( !leds[pos]) {
      leds[pos] = ColorFromPalette( gCurrentPalette, random8(), STARTING_BRIGHTNESS, NOBLEND);
      setPixelDirection(pos, GETTING_BRIGHTER);
    }
  }
}

void brightenOrDarkenEachPixel( fract8 fadeUpAmount, fract8 fadeDownAmount)
{
 for( uint16_t i = 0; i < NUM_LEDS; i++) {
    if( getPixelDirection(i) == GETTING_DARKER) {
      // This pixel is getting darker
      leds[i] = makeDarker( leds[i], fadeDownAmount);
    } else {
      // This pixel is getting brighter
      leds[i] = makeBrighter( leds[i], fadeUpAmount);
      // now check to see if we've maxxed out the brightness
      if( leds[i].r == 255 || leds[i].g == 255 || leds[i].b == 255) {
        // if so, turn around and start getting darker
        setPixelDirection(i, GETTING_DARKER);
      }
    }
  }
}

CRGB makeBrighter( const CRGB& color, fract8 howMuchBrighter)
{
  CRGB incrementalColor = color;
  incrementalColor.nscale8( howMuchBrighter);
  return color + incrementalColor;
}

CRGB makeDarker( const CRGB& color, fract8 howMuchDarker)
{
  CRGB newcolor = color;
  newcolor.nscale8( 255 - howMuchDarker);
  return newcolor;
}

// Compact implementation of
// the directionFlags array, using just one BIT of RAM
// per pixel.  This requires a bunch of bit wrangling,
// but conserves precious RAM.  The cost is a few
// cycles and about 100 bytes of flash program memory.
uint8_t  directionFlags[ (NUM_LEDS+7) / 8];

bool getPixelDirection( uint16_t i)
{
  uint16_t index = i / 8;
  uint8_t  bitNum = i & 0x07;

  uint8_t  andMask = 1 << bitNum;
  return (directionFlags[index] & andMask) != 0;
}

void setPixelDirection( uint16_t i, bool dir)
{
  uint16_t index = i / 8;
  uint8_t  bitNum = i & 0x07;

  uint8_t  orMask = 1 << bitNum;
  uint8_t andMask = 255 - orMask;
  uint8_t value = directionFlags[index] & andMask;
  if( dir ) {
    value += orMask;
  }
  directionFlags[index] = value;
}
