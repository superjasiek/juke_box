#include "LightEffects.h"

CRGB leds[NUM_LEDS];

LightEffects::LightEffects()
  : currentEffect(RAINBOW)
  , prevEffect(RAINBOW)
  , isLocked(true)
  , unlockExpires(0)
  , rainbowHue(0)
  , pulseBright(0)
  , pulseDelta(8)
  , lastBlink(0)
  , lastReactive(0)
{}

void LightEffects::begin() {
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);
}

void LightEffects::lockLeds() {
  isLocked = true;
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
}

void LightEffects::unlockLeds() {
  isLocked = false;
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  unlockExpires = millis() + 1000;
  // keep currentEffect as it was before lock
}

void LightEffects::update() {
  if (isLocked) return;
  if (millis() < unlockExpires) return; // still green flash

  switch (currentEffect) {
    case RAINBOW:         updateRainbow();   break;
    case PULSE:           updatePulse();     break;
    case RANDOM_BLINK:    updateBlink();     break;
    case VOLUME_REACTIVE: updateReactive();  break;
  }
  FastLED.show();
}

void LightEffects::setEffect(LightEffect e) {
  currentEffect = prevEffect = e;
}

void LightEffects::onStartPlay() {
  isLocked = false;
  setEffect(RAINBOW);
}

void LightEffects::onStopPlay() {
  lockLeds();
}

void LightEffects::onTrackChange() {
  setEffect(PULSE);
}

void LightEffects::onStationChange() {
  setEffect(RANDOM_BLINK);
}

// ——— effect implementations ———

void LightEffects::updateRainbow() {
  fill_rainbow(leds, NUM_LEDS, rainbowHue, 7);
  rainbowHue++;
}

void LightEffects::updatePulse() {
  pulseBright += pulseDelta;
  if (pulseBright == 0 || pulseBright == 255) pulseDelta = -pulseDelta;
  fill_solid(leds, NUM_LEDS, CRGB(pulseBright, pulseBright, pulseBright));
}

void LightEffects::updateBlink() {
  uint32_t now = millis();
  if (now - lastBlink > 200) {
    for (int i = 0; i < NUM_LEDS; i++) {
      if (random8() < 60) leds[i] = CHSV(random8(), 255, random8());
      else               leds[i] = CRGB::Black;
    }
    lastBlink = now;
  }
}

void LightEffects::updateReactive() {
  uint32_t now = millis();
  if (now - lastReactive > 50) {
    uint8_t active = random8(NUM_LEDS);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = (i < active) ? CRGB::Green : CRGB::Black;
    }
    lastReactive = now;
  }
}
