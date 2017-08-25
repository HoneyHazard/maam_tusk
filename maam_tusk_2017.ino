#include "FastLED.h"

// *** uncomment below when running on Ma'aM ***
#define TESTING
// *** uncomment above when running on Ma'aM ***

#define NEEDS_GREEN_BLUE_GRADIENT_SWAP

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

// utility #define
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#ifndef TESTING

// Ma'aM configuration
#define DATA_PIN    3
#define CLK_PIN     4
#define LED_TYPE    LPD8806
#define COLOR_ORDER RGB
#define NUM_LEDS    300
#define BRIGHTNESS  96

#else

// Sergey's setup for testing
#define DATA_PIN    3
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
#define NUM_LEDS    49
#define BRIGHTNESS  40

#endif // TESTING

// how fast are we running??
#define FRAMES_PER_SECOND  120
//#define FRAMES_PER_SECOND  3

// *** Ma'aM Colors Stuff ***
#define GRAD_LENGTH 20 // length, in LEDs, of each gradient section
CRGB maamColors[] = { CRGB::Blue, CRGB::Purple, CRGB::Pink, CRGB::White, CRGB::Cyan };
// CRGB maamColors[] = { CRGB::Blue, RGB::Red, CRGB::Green }; // good for testing.
const size_t numMaamColors = ARRAY_SIZE(maamColors);
const size_t colorArrayLen = GRAD_LENGTH * numMaamColors; 
CRGB * maamColorArray;
// *** End Ma'aM Colors Stuff ***

// utility functions
void addGlitter(fract8 chanceOfGlitter);
void nextPattern();
// end utility functions

// active "mode" functions
void maamRainbow();
void maamRainbowWithGlitter();
void maamFullGlow();
// end active "mode" functions

// functions from the 100DemoReel. Kept for reference...
void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void juggle();
void bpm();
// end not used "mode" functions

enum MetaState { Pattern, FadeOut, FadeIn };

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*FunctionList[])();
//FunctionList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
FunctionList gPatterns = { maamFullGlow, maamRainbow };
MetaState metaState = FadeIn;

bool doGlitter = false;


// the "output" array
CRGB leds[NUM_LEDS];

void setup()
{
    maamColorArray = new CRGB[colorArrayLen];
    for (uint16_t i = 0; i < numMaamColors; ++i) {
        uint16_t iNext = i + 1;
        if (iNext == numMaamColors) {
            iNext = 0;
        }
        uint16_t startPos = i * GRAD_LENGTH;
        CRGB startColor = maamColors[i];
        CRGB endColor = maamColors[iNext];
#ifdef NEEDS_GREEN_BLUE_GRADIENT_SWAP
        endColor = CRGB(endColor.r, endColor.b, endColor.g); // correct it?!
        startColor = CRGB(startColor.r, startColor.b, startColor.g); // correct it?!
#endif
        fill_gradient_RGB(maamColorArray,
                          startPos, startColor,
                          startPos + GRAD_LENGTH-1, endColor);
                          
    }
    
    delay(3000); // 3 second delay for recovery
  
    // tell FastLED about the LED strip configuration
#ifndef TESTING
    // actual Ma'aM setup used LPD8806, which needs data and clock pins
    FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#else
    // Sergey's setup at home only needs the data pin
    FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#endif

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);
}


uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // lets make glitter independent, and applied on top of the main "modes"
  if (doGlitter) {
      addGlitter(80);
  }
  

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // gHue is used mostly as a rotating color index
  EVERY_N_MILLISECONDS(200) {
      if (++gHue == colorArrayLen) {
          gHue = 0;
      }
  }

  // change patterns periodically
  EVERY_N_SECONDS(10) {
      nextPattern();
  }

  // activate glitter every now and then
  EVERY_N_SECONDS(13) {
      doGlitter = (random8() < 100);
  } 
}

void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void addGlitter(fract8 chanceOfGlitter) 
{
    if( random8() < chanceOfGlitter) {
        leds[ random16(NUM_LEDS) ] += CRGB::White;
    }
}

void maamRainbow()
{
    size_t colorIdx = gHue % colorArrayLen;
    for (size_t i = 0; i < NUM_LEDS; ++i) {
        leds[i] = maamColorArray[colorIdx];
        if (++colorIdx == colorArrayLen) {
            colorIdx = 0;
        }
    }
}

void maamRainbowWithGlitter()
{
    maamRainbow();
    addGlitter(80);
}

void maamFullGlow()
{
    size_t colorIdx = gHue % colorArrayLen;
    for (size_t i = 0; i < NUM_LEDS; ++i) {
        leds[i] = maamColorArray[colorIdx];
    }
}

// ***************

// *** The rest are not actively used; just kept here for reference ***

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014


void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}


void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle()
{
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

