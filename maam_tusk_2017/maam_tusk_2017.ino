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

// how fast are we running??
#define FRAMES_PER_SECOND  200
//#define FRAMES_PER_SECOND  12

#ifndef TESTING

// Ma'aM configuration
#define DATA_PIN    3
#define CLK_PIN     4
#define LED_TYPE    LPD8806
#define COLOR_ORDER RGB
#define NUM_LEDS    300

#define BRIGHTNESS  96
#define GRAD_SCALE_FACTOR 3

#else

// Sergey's setup for testing
#define DATA_PIN    3
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
#define NUM_LEDS    49

#define BRIGHTNESS  10
#define GRAD_SCALE_FACTOR 1

#endif // TESTING

#define GRAD_LENGTH (20 * GRAD_SCALE_FACTOR) // length, in LEDs, of each gradient section

// *** Ma'aM Colors Stuff ***
CRGB maamColors[] = { CRGB::Blue, CRGB::Purple, CRGB::Pink, CRGB::White, CRGB::Cyan };
//CRGB maamColors[] = { CRGB::Blue, CRGB::Red, CRGB::Green }; // good for testing.
const size_t numMaamColors = ARRAY_SIZE(maamColors);
const size_t colorArrayLen = GRAD_LENGTH * numMaamColors; 
CRGB * maamColorArray;
// *** End Ma'aM Colors Stuff ***

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
FunctionList gPatterns = { maamFullGlow, maamRainbow, maamFullGlow, maamRainbowCompressed };
const size_t numPatterns = ARRAY_SIZE(gPatterns);

bool transitionActive = false;
uint8_t fadeCounter = 0; 
bool doGlitter = false;

#define MAX_COLOR_SPOTS 5
struct SpotInfo
{
    int16_t pos;
    int16_t radius;
    int16_t vel;   
    CRGB color;
    uint8_t fadeFactor;
} gColorSpots[MAX_COLOR_SPOTS];
uint8_t numColorSpots = 0;

void addSpot()
{
    if (numColorSpots > MAX_COLOR_SPOTS) {
        // cannot handle any more color spots ;_;
        return;
    }
    
    // test a hue spot
    SpotInfo spot;
    spot.pos = 0;
    spot.vel = 1;
    spot.radius = 5;
    spot.color = CRGB::Red;
    spot.fadeFactor = 0;
    gColorSpots[num] = spot;
    numColorSpots = 1;
}

void removeColorSpot(uint8_t index)
{
    --numColorSpots;
    // shift all left...
    
    for (uint8_t i = index; i < numColorSpots; ++i) {
        gColorSpots[i] = gColorSpots[i+1];
    }
}

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

    addSpot();
}


uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint16_t gHue = 0; // rotating "base color" used by many of the patterns

// utility functions
void addGlitter(fract8 chanceOfGlitter);
void applyFade(size_t i, CRGB rgb, uint8_t fadeFactor);
void applySpot(const SpotInfo &spot);
// end utility functions

void loop()
{
    // start empty
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    CHSV test = rgb2hsv_approximate(CRGB::Blue);
    
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

  // apply hue spots
  for (size_t i = 0; i < numColorSpots; ++i) {
      applySpot(gColorSpots[i]);
  }   
  
  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // gHue is used mostly as a rotating color index
  EVERY_N_MILLISECONDS(200) {
      if (gPatterns[gCurrentPatternNumber] == maamFullGlow) {
          gHue += GRAD_SCALE_FACTOR;
      } else {
          ++gHue;
      }

      if (gHue >= colorArrayLen) {
          gHue -= colorArrayLen;
      }
  }

  EVERY_N_MILLISECONDS(577) {
      // hue spots
      for (size_t i = 0; i < numColorSpots; ++i) {
          SpotInfo &si = gColorSpots[i];
          si.pos += si.vel * GRAD_SCALE_FACTOR;
          if (si.pos < 0 || si.pos >= NUM_LEDS) {
              si.vel = -si.vel;
          }
      }
  }

  // change patterns periodically, with some chance
  EVERY_N_SECONDS(11) {
      if (random8() < 128) {
          transitionActive = true;
      }
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

void applySpot(const SpotInfo &spot)
{
    for (int16_t i = -spot.radius; i <= spot.radius; ++i) {        
        int16_t idx = spot.pos + i;
        if (idx < 0) {
            continue;
        } else if (idx >= NUM_LEDS) {
            continue;
        }
        
#if false
        CHSV hsv = rgb2hsv_approximate(leds[(size_t)idx]);
        hsv.hue += spot.hueDeltaMax * ((uint16_t)(spot.radius - abs(i)) * 255)/spot.radius/256; 
        CRGB rgb;
        hsv2rgb_raw(hsv, rgb);
#endif
        CRGB rgb = leds[(size_t)idx];
        rgb.g = 255;
        leds[(size_t)idx] = CRGB::Blue;//rgb;

        //hsv2rgb_spectrum(color, leds[(size_t)idx]);
        //ds[(size_t)idx] = CRGB::Blue;
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

