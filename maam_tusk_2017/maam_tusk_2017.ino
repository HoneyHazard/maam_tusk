#include "FastLED.h"

// *** uncomment below when running on Ma'aM ***
// #define TESTING
// *** uncomment above when running on Ma'aM ***

#define NEEDS_GREEN_BLUE_GRADIENT_SWAP

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

// utility #define
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// how fast are we running??
#define FRAMES_PER_SECOND  120

#ifndef TESTING

// Ma'aM configuration
#define DATA_PIN    3
#define CLK_PIN     4
#define LED_TYPE    LPD8806
#define COLOR_ORDER RGB
#define NUM_LEDS    300
#define BRIGHTNESS  96
#define GRAD_LENGTH 60 // length, in LEDs, of each gradient section

#else

// Sergey's setup for testing
#define DATA_PIN    3
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
#define NUM_LEDS    49
#define BRIGHTNESS  40
#define GRAD_LENGTH 20 // length, in LEDs, of each gradient section

#endif // TESTING

// *** Ma'aM Colors Stuff ***
CRGB maamColors[] = { CRGB::Blue, CRGB::Purple, CRGB::Pink, CRGB::White, CRGB::Cyan };
// CRGB maamColors[] = { CRGB::Blue, RGB::Red, CRGB::Green }; // good for testing.
const size_t numMaamColors = ARRAY_SIZE(maamColors);
const size_t colorArrayLen = GRAD_LENGTH * numMaamColors; 
CRGB * maamColorArray;
// *** End Ma'aM Colors Stuff ***

// utility functions
void addGlitter(fract8 chanceOfGlitter);
void applyFade(size_t i, CRGB rgb, uint8_t fadeFactor);
// end utility functions

// active "mode" functions
void maamRainbow(uint8_t fadeFactor);
void maamRainbowCompressed(uint8_t fadeFactor);
void maamFullGlow(uint8_t fadeFactor);
// end active "mode" functions

// functions from the 100DemoReel. Kept for reference...
void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void juggle();
void bpm();
// end not used "mode" functions

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*FunctionList[])(uint8_t fadeFactor);
//FunctionList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
//FunctionList gPatterns = { maamFullGlow, maamRainbow, maamFullGlow, maamRainbowCompressed };
FunctionList gPatterns = { maamFullGlow, maamRainbow, maamFullGlow, maamRainbowCompressed };
const size_t numPatterns = ARRAY_SIZE(gPatterns);

bool transitionActive = false;
uint8_t fadeCounter = 0; 
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
    // start empty
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    
    if (transitionActive) {
        // change is coming... use weighted transition ratios
        uint8_t nextPatternNumber = gCurrentPatternNumber + 1;
        if (nextPatternNumber == numPatterns) {
            nextPatternNumber = 0;
        }
        gPatterns[gCurrentPatternNumber](255-fadeCounter);
        gPatterns[nextPatternNumber](fadeCounter);
        ++fadeCounter;
        if (fadeCounter == 0) { // 256 and rolled over...
            transitionActive = false;
            gCurrentPatternNumber = nextPatternNumber;
        }        
    } else {
        // business as usual
        // Call the current pattern function once, updating the 'leds' array        
        gPatterns[gCurrentPatternNumber](255);
    }
    

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
  EVERY_N_SECONDS(11) {
      transitionActive = true;
  }

  // activate glitter every now and then
  EVERY_N_SECONDS(13) {
      doGlitter = (random8() < 100);
  } 
}

void addGlitter(fract8 chanceOfGlitter) 
{
    if( random8() < chanceOfGlitter) {
        leds[ random16(NUM_LEDS) ] += CRGB::White;
    }
}

void addRgb(size_t i, CRGB rgb, uint8_t fadeFactor)
{
    uint8_t r = ((uint16_t)rgb.r * fadeFactor) >> 8;
    uint8_t g = ((uint16_t)rgb.g * fadeFactor) >> 8;
    uint8_t b = ((uint16_t)rgb.b * fadeFactor) >> 8;
    leds[i] += CRGB(r, g, b);
}


void maamRainbow(uint8_t fadeCounter)
{
    size_t colorIdx = gHue % colorArrayLen;
    for (size_t i = 0; i < NUM_LEDS; ++i) {
        addRgb(i, maamColorArray[colorIdx], fadeCounter);        
        if (++colorIdx == colorArrayLen) {
            colorIdx = 0;
        }
    }
}

void maamRainbowCompressed(uint8_t fadeCounter)
{
    size_t colorIdx = gHue % colorArrayLen;
    for (size_t i = 0; i < NUM_LEDS; ++i) {
        uint8_t fadeFraction = fadeCounter >> 2;
        addRgb(i, maamColorArray[colorIdx], fadeFraction);
        if (++colorIdx == colorArrayLen) {
            colorIdx = 0;
        }
        addRgb(i, maamColorArray[colorIdx], fadeFraction);
        if (++colorIdx == colorArrayLen) {
            colorIdx = 0;
        }
        addRgb(i, maamColorArray[colorIdx], fadeFraction);
        if (++colorIdx == colorArrayLen) {
            colorIdx = 0;
        }
        addRgb(i, maamColorArray[colorIdx], fadeFraction);
        if (++colorIdx == colorArrayLen) {
            colorIdx = 0;
        }
    }
}


void maamFullGlow(uint8_t fadeCounter)
{
    size_t colorIdx = gHue % colorArrayLen;
    for (size_t i = 0; i < NUM_LEDS; ++i) {
        addRgb(i, maamColorArray[colorIdx], fadeCounter);        
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

