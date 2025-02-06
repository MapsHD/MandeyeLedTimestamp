/// @file    Fire2012WithPalette.ino
/// @brief   Simple one-dimensional fire animation with a programmable color palette
/// @example Fire2012WithPalette.ino

#include <FastLED.h>
#include <ArduinoJson.h>

#define LED_PIN     D4
#define COLOR_ORDER GRB
#define CHIPSET     WS2812
#define NUM_LEDS    24

#define BRIGHTNESS  10

bool gReverseDirection = false;

CRGB leds[NUM_LEDS];

// Allocate the JSON document
JsonDocument doc;

namespace Commands{
  const char* const Brightness = "brightness";
  const char* const ColorInactive = "active";
  const char* const ColorActive = "inactive";
  const char* const LedState = "ledState";
};

namespace Data
{
  uint8 brightness = 1; 
  CRGB colorInactive = CRGB::Blue;
  CRGB colorActive = CRGB::Red;
  uint32_t data = 0;
};


void setup() {
  delay(1000); // sanity delay
  Serial.begin(115200);
  Serial.setTimeout(1000);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );


  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Blue;
    FastLED.show();
    FastLED.delay(10);
  }
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
    FastLED.show();
    FastLED.delay(10);
  } 
}



void loop()
{
  for (int i = 0; i < NUM_LEDS; i++) {
    const bool active = Data::data & (1 << i);
    leds[i] = active ? Data::colorActive : Data::colorInactive;
  }
  FastLED.setBrightness(Data::brightness);
  FastLED.show();

  String msg = Serial.readStringUntil('\n');
  if (msg.length() == 0) {
    return;
  }
  DeserializationError error = deserializeJson(doc, msg);

  if (error) {
    if (error != DeserializationError::EmptyInput)
    {
      Serial.print(F("deserializeMsgPack() failed: "));
      Serial.println(error.f_str());
    }
    return;
  }
  if (doc.containsKey(Commands::Brightness)) {
    Data::brightness = doc[Commands::Brightness];
  }
  if (doc.containsKey(Commands::ColorInactive)) {
    uint32_t color = doc[Commands::ColorInactive].as<uint32_t>();
    Data::colorInactive = CRGB(color);
  }
  if (doc.containsKey(Commands::ColorActive)) {
    uint32_t color = doc[Commands::ColorActive].as<uint32_t>();
    Data::colorActive = CRGB(color);
  }
  if (doc.containsKey(Commands::LedState)) {
    const uint32_t data = doc[Commands::LedState].as<uint32_t>();
    Data::data = data;

  }
  StaticJsonDocument<200> doc1;
  doc1[Commands::Brightness] = Data::brightness;
  doc1[Commands::ColorInactive] = Data::colorInactive.as_uint32_t();
  doc1[Commands::ColorActive] = Data::colorActive.as_uint32_t();
  doc1[Commands::LedState] = Data::data;
  serializeJson(doc1, Serial);

}