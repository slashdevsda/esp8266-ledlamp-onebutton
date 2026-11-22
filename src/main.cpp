#include <Arduino.h>
#include <FastLED.h>

#define DATA_PIN 14
#define BUTTON_PIN 5
#define NUM_LEDS 11
#define LONG_PRESS_MS 500
#define DOUBLE_PRESS_MS 400
#define TRANSITION_MS 5000
#define BRIGHT_TRANS_MS 2000
#define FEEDBACK_MS 300
#define BTN_IGNORE_MS 80
#define UPDATE_MS 16
#define RGB_HOLD_MS 10000

#define NUM_MODES 3
const CRGB MODE_COLORS[2] = {
  CRGB(255, 110, 20),
  CRGB(255, 230, 200)
};

const uint8_t BRIGHT_LEVELS[3] = {20, 80, 225};
int brightLevel = 1;

CRGB leds[NUM_LEDS];
bool lampOn = false;
int currentMode = 0;
CRGB startColor = CRGB::Black;
CRGB targetColor = MODE_COLORS[0];
uint32_t lastTransition = 0;

float curBrightF = 40.0f;
float tgtBrightF = 40.0f;
float startBrightF = 40.0f;
uint32_t lastBrightT = 0;
bool brightTrans = false;

bool feedbackActive = false;
uint32_t feedbackStart = 0;
int feedbackMode = 0;
CRGB feedbackTarget = CRGB::Black;
CRGB savedColor = CRGB::Black;
CRGB feedbackColor = CRGB(0, 0, 255);

bool btnWas = false;
uint32_t btnTime = 0;
bool waitSecond = false;
uint32_t firstPress = 0;
uint32_t ignoreUntil = 0;
uint32_t rgbHoldStart = 0;

float smootherstep(float t) {
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("LAMPE-LED v11 (float blend)");
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
  FastLED.show();
  Serial.println("Pret.");
}

void modeName(int m) {
  if (m == 0) Serial.print("blanc chaud");
  else if (m == 1) Serial.print("blanc");
  else Serial.print("RGB");
}

void setTarget(CRGB c) {
  startColor = leds[0];
  targetColor = c;
  lastTransition = millis();
}

void getModeColor(int m, CRGB &c) {
  if (m == 0) c = MODE_COLORS[0];
  else if (m == 1) c = MODE_COLORS[1];
  else c = CRGB(CHSV(random(256), 255, 255));
}

void changeIntensity() {
  if (!lampOn) return;
  brightLevel = (brightLevel + 1) % 3;
  startBrightF = curBrightF;
  tgtBrightF = (float)BRIGHT_LEVELS[brightLevel];
  lastBrightT = millis();
  brightTrans = true;
  feedbackActive = true;
  feedbackStart = millis();
  feedbackColor = CRGB(255, 0, 0);
  Serial.print("Intensite: ");
  Serial.println(brightLevel==0?"faible":(brightLevel==1?"moyen":"max"));
}

void changeMode() {
  if (!lampOn) return;
  savedColor = leds[0];
  feedbackActive = true;
  feedbackStart = millis();
  feedbackColor = CRGB(0, 0, 255);
  feedbackMode = (currentMode + 1) % NUM_MODES;
  getModeColor(feedbackMode, feedbackTarget);
}

void toggleLamp() {
  lampOn = !lampOn;
  if (lampOn) {
    CRGB c;
    getModeColor(currentMode, c);
    for (int i = 0; i < NUM_LEDS; i++) leds[i] = c;
    targetColor = c;
    startColor = c;
    lastTransition = millis() + TRANSITION_MS;
    startBrightF = 0.0f;
    curBrightF = 0.0f;
    tgtBrightF = (float)BRIGHT_LEVELS[brightLevel];
    lastBrightT = millis();
    brightTrans = true;
    rgbHoldStart = 0;
    FastLED.show();
    Serial.println("Lampe ALLUMEE");
  } else {
    for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
    FastLED.show();
    Serial.println("Lampe ETEINTEE");
  }
}

void loop() {
  bool btn = (digitalRead(BUTTON_PIN) == LOW);
  if (millis() < ignoreUntil) {
    btnWas = btn;
  } else if (btn && !btnWas) {
    btnWas = true;
    btnTime = millis();
  } else if (!btn && btnWas) {
    btnWas = false;
    ignoreUntil = millis() + BTN_IGNORE_MS;
    uint32_t dur = millis() - btnTime;
    if (dur >= LONG_PRESS_MS) {
      toggleLamp();
    } else {
      if (waitSecond && (millis() - firstPress) < DOUBLE_PRESS_MS) {
        waitSecond = false;
        changeMode();
      } else {
        waitSecond = true;
        firstPress = millis();
      }
    }
  }

  if (waitSecond && (millis() - firstPress) >= DOUBLE_PRESS_MS) {
    waitSecond = false;
    changeIntensity();
  }

  if (feedbackActive) {
    uint32_t el = millis() - feedbackStart;
    if (el < FEEDBACK_MS) {
      float bScale = curBrightF / 255.0f;
      for (int i = 1; i < NUM_LEDS; i++) {
        leds[i].r = (uint8_t)(leds[i].r * bScale + 0.5f);
        leds[i].g = (uint8_t)(leds[i].g * bScale + 0.5f);
        leds[i].b = (uint8_t)(leds[i].b * bScale + 0.5f);
      }
      leds[0] = feedbackColor;
      FastLED.show();
    } else {
      feedbackActive = false;
      if (feedbackColor == CRGB(0, 0, 255)) {
        currentMode = feedbackMode;
        rgbHoldStart = 0;
        startColor = savedColor;
        targetColor = feedbackTarget;
        lastTransition = millis();
        Serial.print("Mode: ");
        modeName(currentMode);
        Serial.println();
      }
    }
    return;
  }

  if (brightTrans) {
    uint32_t el = millis() - lastBrightT;
    if (el < BRIGHT_TRANS_MS) {
      float t = (float)el / BRIGHT_TRANS_MS;
      float e = smootherstep(t);
      curBrightF = startBrightF + (tgtBrightF - startBrightF) * e;
    } else {
      brightTrans = false;
      curBrightF = tgtBrightF;
    }
  }

  static uint32_t lastUpd = 0;
  if (millis() - lastUpd >= UPDATE_MS) {
    lastUpd = millis();
    if (lampOn) {
      uint32_t el = millis() - lastTransition;
      if (el < TRANSITION_MS) {
        float t = (float)el / TRANSITION_MS;
        float e = smootherstep(t);
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i].r = (uint8_t)(startColor.r + (targetColor.r - startColor.r) * e + 0.5f);
          leds[i].g = (uint8_t)(startColor.g + (targetColor.g - startColor.g) * e + 0.5f);
          leds[i].b = (uint8_t)(startColor.b + (targetColor.b - startColor.b) * e + 0.5f);
        }
      } else {
        for (int i = 0; i < NUM_LEDS; i++) leds[i] = targetColor;
        if (currentMode == 2) {
          if (rgbHoldStart == 0) rgbHoldStart = millis();
          else if (millis() - rgbHoldStart >= RGB_HOLD_MS) {
            rgbHoldStart = 0;
            setTarget(CRGB(CHSV(random(256), 255, 255)));
          }
        } else {
          rgbHoldStart = 0;
        }
      }
      float bScale = curBrightF / 255.0f;
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].r = (uint8_t)(leds[i].r * bScale + 0.5f);
        leds[i].g = (uint8_t)(leds[i].g * bScale + 0.5f);
        leds[i].b = (uint8_t)(leds[i].b * bScale + 0.5f);
      }
      FastLED.show();
    }
  }
}
