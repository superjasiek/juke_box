// main.cpp
#include "Arduino.h"
#include "core/options.h"
#include "core/LightEffects.h"
#include "core/RFIDAuth.h"
#include "core/config.h"
#include "core/telnet.h"
#include "core/player.h"
#include "core/display.h"
#include "core/network.h"
#include "core/netserver.h"
#include "core/controls.h"
#include "core/mqtt.h"
#include "core/optionschecker.h"
#include "pluginsManager/pluginsManager.h"
#include "core/audiohandlers.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// --- WS2812B strip -----------------------------------------------------------
LightEffects lightEffects;

// --- RFID setup --------------------------------------------------------------
// RC522 CS on GPIO4
#define RFID_CS_PIN 4
static RFIDAuth rfid(RFID_CS_PIN);

// Task handle (do ewentualnego kill/inspect)
static TaskHandle_t rfidTaskHandle = nullptr;

// prototypy callbacków odtwarzacza
void on_start_play();
void on_stop_play();
void on_track_change();
void on_station_change();

// podpinamy je pod YoRadio
void player_on_start_play()     { on_start_play(); }
void player_on_stop_play()      { on_stop_play(); }
void player_on_track_change()   { on_track_change(); }
void player_on_station_change() { on_station_change(); }

#if DSP_HSPI || TS_HSPI || VS_HSPI
SPIClass SPI2(HOOPSENb);
#endif

// -----------------------------------------------------------------------------
// Zadanie RFID (Core 0): co 1s odczytuje karty, utrzymuje 10-min „strefę odblokowania”
// i wysyła do playera PR_PLAY/PR_STOP, a diody zamienia w czerwone (lock) lub
// robią 1s zielony flash (unlock).
// -----------------------------------------------------------------------------
void rfidTask(void* pvParameters) {
  rfid.begin();
  bool lastUnlocked = false;

  for (;;) {
    rfid.update();
    bool unlocked = rfid.allowAction();

    if (unlocked && !lastUnlocked) {
      // właśnie odblokowano
      lightEffects.setEffect(RAINBOW); // ← dodano: ustaw efekt tylko tutaj
      lightEffects.unlockLeds();       // zielony flash 1s
      player.sendCommand({ PR_PLAY, (uint16_t)config.lastStation() });
    }
    else if (!unlocked && lastUnlocked) {
      // właśnie zablokowano
      lightEffects.lockLeds();         // pełne czerwone
      player.sendCommand({ PR_STOP, 0 });
    }

    lastUnlocked = unlocked;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);
  if (REAL_LEDBUILTIN != 255) pinMode(REAL_LEDBUILTIN, OUTPUT);

  extern void yoradio_on_setup() __attribute__((weak));
  if (yoradio_on_setup) yoradio_on_setup();

  pm.on_setup();
  config.init();
  display.init();

  // spawn RFID task on Core 0, 4 KB stack, prio 1
  xTaskCreatePinnedToCore(
    rfidTask,
    "RFID Task",
    4096,
    nullptr,
    1,
    &rfidTaskHandle,
    0
  );

  player.init();
  network.begin();

  if (network.status != CONNECTED && network.status != SDREADY) {
    netserver.begin();
    initControls();
    SPI.begin(18, 19, 23);
    display.putRequest(DSP_START);
    while (!display.ready()) delay(10);
    return;
  }

  if (SDC_CS != 255) {
    display.putRequest(WAITFORSD, 0);
    Serial.print("##[BOOT]#\tSD search\t");
  }

  config.initPlaylistMode();
  netserver.begin();
  telnet.begin();
  initControls();
  display.putRequest(DSP_START);
  while (!display.ready()) delay(10);

  #ifdef MQTT_ROOT_TOPIC
  mqttInit();
  #endif

  if (config.getMode() == PM_SDCARD) player.initHeaders(config.station.url);
  player.lockOutput = false;
  pm.on_end_setup();

  // diody
  lightEffects.begin();
  // startujemy w stanie zablokowanym:
  lightEffects.lockLeds();
}

void loop() {
  // uwaga: już nie wywołujemy tu rfid.update()
  telnet.loop();
  if (network.status == CONNECTED || network.status == SDREADY) {
    player.loop();
  }
  loopControls();
  netserver.loop();
  lightEffects.update();
}

// --- callbacki playera ------------------------------------------------------
void on_start_play() {
  if (!rfid.allowAction()) {
    Serial.println("🚫 RFID lock: playback blocked");
    player.sendCommand({ PR_STOP, 0 });
    return;
  }
  Serial.println("▶️ Playback started");

  // usunięto: lightEffects.setEffect(RAINBOW);
}

void on_stop_play() {
  Serial.println("🛑 Playback stopped");
  // bez zmiany diodek — stan jest trzymany przez RFID task
}

bool isManualAction = false;
uint8_t currentEffectIndex = 0;

void on_track_change() {
  currentEffectIndex = (currentEffectIndex + 1) % 4;
  switch (currentEffectIndex) {
    case 0: lightEffects.setEffect(RAINBOW);        Serial.println("🌈 Effect: Rainbow"); break;
    case 1: lightEffects.setEffect(PULSE);          Serial.println("💡 Effect: Pulse");   break;
    case 2: lightEffects.setEffect(RANDOM_BLINK);   Serial.println("🌟 Blink");          break;
    case 3: lightEffects.setEffect(VOLUME_REACTIVE);Serial.println("🎚️ Volume");         break;
  }
}

void on_station_change() {
  if (isManualAction) {
    if (!rfid.allowAction()) {
      Serial.println("🚫 RFID lock: station change blocked");
      isManualAction = false;
      return;
    }
  }
  isManualAction = false;

  currentEffectIndex = (currentEffectIndex + 1) % 4;
  switch (currentEffectIndex) {
    case 0: lightEffects.setEffect(RAINBOW);        Serial.println("🌈 Effect: Rainbow"); break;
    case 1: lightEffects.setEffect(PULSE);          Serial.println("💡 Effect: Pulse");   break;
    case 2: lightEffects.setEffect(RANDOM_BLINK);   Serial.println("🌟 Blink");          break;
    case 3: lightEffects.setEffect(VOLUME_REACTIVE);Serial.println("🎚️ Volume");         break;
  }
}
