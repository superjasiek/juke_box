#ifndef LIGHT_EFFECTS_H
#define LIGHT_EFFECTS_H

#include <FastLED.h>

// ——— strip configuration ———
#define DATA_PIN    33
#define NUM_LEDS    10
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define BRIGHTNESS  128

// ——— available effects ———
enum LightEffect {
  RAINBOW,
  PULSE,
  RANDOM_BLINK,
  VOLUME_REACTIVE
};

class LightEffects {
public:
  LightEffects();
  void begin();
  void update();

  // called from your RFID task:
  void lockLeds();   // immediately go solid red
  void unlockLeds(); // 1 s green flash, then resume previous effect

  // callbacks from player
  void setEffect(LightEffect e);

  void onStartPlay();      // resets to RAINBOW
  void onStopPlay();       // lock
  void onTrackChange();    // PULSE
  void onStationChange();  // RANDOM_BLINK

private:
  LightEffect currentEffect, prevEffect;
  bool       isLocked;
  uint32_t   unlockExpires;

  // state for each effect
  uint8_t    rainbowHue;
  uint8_t    pulseBright;
  int8_t     pulseDelta;
  uint32_t   lastBlink;
  uint32_t   lastReactive;

  // per‐effect updates
  void updateRainbow();
  void updatePulse();
  void updateBlink();
  void updateReactive();
};

extern CRGB leds[NUM_LEDS];

#endif // LIGHT_EFFECTS_H
