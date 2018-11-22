//Vekotin Lapset töihin-päivää varten. IR-sensorilla ohjataan lediefektien väriä ja enkooderilla
//vaihdetaan efektiä. Enkooderin painonapilla kytketään efektien automaattinen vaihto päälle/pois.


#include "settings.h"
#include <FastLED.h>
#include <Encoder.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_Sensor.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

Encoder myEnc (D1, D2);
const int buttonPin = D3;     // the number of the pushbutton pin
int buttonState;         // variable for reading the pushbutton status
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int efekti=0;
int switchEffect=0;
float ObjTemp = mlx.readObjectTempC();
float AmbTemp = mlx.readAmbientTempC();
uint8_t HueCtrlValInt = 0;


FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define SDA D7
#define SCL D6
#define DATA_PIN    D5
//#define CLK_PIN   4
#define LED_TYPE    WS2812
#define COLOR_ORDER RGB
#define NUM_LEDS    4
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          128
#define FRAMES_PER_SECOND  120

void setup() {
  delay(3000); // 3 second delay for recovery
  mlx.begin();
  Wire.begin(SDA, SCL);
  pinMode(buttonPin, INPUT);
  Serial.begin(115200);
  Serial.println("Enkooderin testaus");
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}




long oldPosition = 0;

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void loop()
{

  long newPosition = myEnc.read();
  if (newPosition <= (oldPosition-4)) {
    efekti++;
    if(efekti > 5 ) efekti=0;
    oldPosition = newPosition;
    gCurrentPatternNumber = efekti;
    Serial.println(efekti);
  }
else if (newPosition >= (oldPosition+4)) {
    efekti--;
    if(efekti < 0 ) efekti=5;
    oldPosition = newPosition;
    gCurrentPatternNumber = efekti;
    Serial.println(efekti);
  }
    
  int reading = !digitalRead(buttonPin);
  
  if (reading != lastButtonState)
  { 
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime)> debounceDelay) {
   
    if (reading != buttonState) {
      buttonState = reading;
      Serial.println(buttonState); 
      if (buttonState == HIGH) {
  switchEffect = !switchEffect;
}
    }
  }
  
lastButtonState = reading;

  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();
  

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  //EVERY_N_MILLISECONDS(20) { gHue++; } // slowly cycle the "base color" through the rainbow
  if (switchEffect == 1) {
  EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}
  //Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC()); 
  //Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC()); Serial.println("*C");
  float ObjTemp = mlx.readObjectTempC();
  float AmbTemp = mlx.readAmbientTempC();
  float HueCtrlValFloat = 0;
  
  float RefVal = 37;
  
  if (ObjTemp >= AmbTemp) {
    HueCtrlValFloat = ((ObjTemp-AmbTemp)/((RefVal-AmbTemp)/256));}

  HueCtrlValInt = static_cast<int>(HueCtrlValFloat);
    
    if (ObjTemp > RefVal) {
      HueCtrlValInt = 255;
    }
    
  
  
  gHue = HueCtrlValInt;
  Serial.println(HueCtrlValInt);
  Serial.println("ObjTemp");
  Serial.println(ObjTemp);
  Serial.println("AmbTemp");
  Serial.println(AmbTemp);
  
}



#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

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

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
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

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
