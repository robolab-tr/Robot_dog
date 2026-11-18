/*
  Robot Dog Eyes - TFT_eSPI, 320x240 display (ESP32 / ESP8266)

  - Flicker-free: draws everything into an 8-bit sprite (~75 KB RAM), then pushes it.
  - Smooth animation: every emotion only sets "targets", the eyes glide toward them.
  - Auto blinking and random glances while idle.

  Make sure your TFT_eSPI User_Setup.h matches your display and wiring.
*/

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

// ---------- Layout ----------
const int SCREEN_W = 320;
const int SCREEN_H = 240;
const int EYE_Y    = 120;
const int EYE_LX   = 88;
const int EYE_RX   = 232;
const int EYE_R    = 58;          // base eye radius
const uint16_t BG  = TFT_BLACK;

// ---------- Eye state ----------
struct EyeState {
  float open;    // 0 = closed ... 1 = fully open
  float slant;   // + angry (inner side lowered), - sad (outer side lowered), in pixels
  float cheek;   // 0..1 lower lid pushed up (happy squint)
  float pupil;   // pupil size multiplier
  float size;    // eye size multiplier
};

EyeState curL = {1, 0, 0, 1, 1}, curR = {1, 0, 0, 1, 1};
EyeState tgtL = {1, 0, 0, 1, 1}, tgtR = {1, 0, 0, 1, 1};

float curLookX = 0, curLookY = 0;   // -1..1
float tgtLookX = 0, tgtLookY = 0;

// Whole-eye movement (shifts both eyes across the screen), -1..1
float curOffX = 0, curOffY = 0;
float tgtOffX = 0, tgtOffY = 0;
const int MAX_OFF_X = 20;   // max pixels left/right
const int MAX_OFF_Y = 40;   // max pixels up/down

uint16_t scleraColor = TFT_WHITE;
uint16_t pupilColor  = TFT_BLACK;

enum Mode { MODE_NORMAL, MODE_HEART, MODE_DIZZY };
Mode mode = MODE_NORMAL;

// ---------- Timing ----------
unsigned long lastFrame = 0;
unsigned long nextBlink = 0;
unsigned long blinkStart = 0;
unsigned long nextGlance = 0;
const unsigned long BLINK_MS = 180;
bool autoIdle = true;

// =====================================================================
//                      EMOTION FUNCTIONS
// =====================================================================

void setEyes(float open, float slant, float cheek, float pupil, float size,
             float lookX, float lookY) {
  tgtL = tgtR = {open, slant, cheek, pupil, size};
  tgtLookX = lookX;
  tgtLookY = lookY;
}

void eyesNeutral() {
  mode = MODE_NORMAL;
  scleraColor = TFT_WHITE;
  setEyes(1.0, 0, 0, 1.0, 1.0, 0, 0);
}

void eyesHappy() {
  mode = MODE_NORMAL;
  scleraColor = TFT_WHITE;
  setEyes(1.0, 0, 0.85, 1.0, 1.0, 0, -0.1);
}

void eyesSad() {
  mode = MODE_NORMAL;
  scleraColor = TFT_WHITE;
  setEyes(0.85, -24, 0, 1.15, 1.0, 0, 0.5);
}

void eyesAngry() {
  mode = MODE_NORMAL;
  scleraColor = tft.color565(255, 190, 190);
  setEyes(0.85, 26, 0, 0.8, 1.0, 0, 0.1);
}

void eyesSurprised() {
  mode = MODE_NORMAL;
  scleraColor = TFT_WHITE;
  setEyes(1.0, 0, 0, 0.45, 1.18, 0, -0.1);
}

void eyesSleepy() {
  mode = MODE_NORMAL;
  scleraColor = TFT_WHITE;
  setEyes(0.3, -6, 0, 1.0, 1.0, 0, 0.4);
}

void eyesSuspicious() {
  mode = MODE_NORMAL;
  scleraColor = TFT_WHITE;
  setEyes(0.55, 12, 0, 0.9, 1.0, 0.7, 0);
}

void eyesWink() {
  mode = MODE_NORMAL;
  scleraColor = TFT_WHITE;
  setEyes(1.0, 0, 0.7, 1.0, 1.0, 0, 0);
  tgtR.open = 0.0;            // right eye closed
  tgtR.cheek = 0.0;
}

void eyesLove() {             // pulsing hearts
  mode = MODE_HEART;
  setEyes(1.0, 0, 0, 1.0, 1.0, 0, 0);
}

void eyesDizzy() {            // spinning spirals
  mode = MODE_DIZZY;
  scleraColor = TFT_WHITE;
  setEyes(1.0, 0, 0, 1.0, 1.0, 0, 0);
}

void lookAt(float x, float y) {   // -1..1 each
  tgtLookX = constrain(x, -1.0f, 1.0f);
  tgtLookY = constrain(y, -1.0f, 1.0f);
}

// Move the WHOLE eyes (not just the pupils). -1..1 each, 0,0 = centered.
void moveEyes(float x, float y) {
  tgtOffX = constrain(x, -1.0f, 1.0f);
  tgtOffY = constrain(y, -1.0f, 1.0f);
}

void blinkNow() {
  blinkStart = millis();
}

// =====================================================================
//                          DRAWING
// =====================================================================

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

void updateState(EyeState &c, const EyeState &t, float k) {
  c.open  = lerpf(c.open,  t.open,  k);
  c.slant = lerpf(c.slant, t.slant, k);
  c.cheek = lerpf(c.cheek, t.cheek, k);
  c.pupil = lerpf(c.pupil, t.pupil, k);
  c.size  = lerpf(c.size,  t.size,  k);
}

void drawHeart(int cx, int cy, int s, uint16_t color) {
  int r = s / 4;
  spr.fillCircle(cx - r, cy - r * 0.6, r, color);
  spr.fillCircle(cx + r, cy - r * 0.6, r, color);
  spr.fillTriangle(cx - 2 * r + 1, cy - r * 0.2,
                   cx + 2 * r - 1, cy - r * 0.2,
                   cx, cy + 2.3 * r, color);
}

void drawSpiral(int cx, int cy, int R, float rot, uint16_t color) {
  float px = cx, py = cy;
  for (float a = 0; a < 4 * PI * 1.5; a += 0.25) {
    float rad = (a / (4 * PI * 1.5)) * R;
    float x = cx + cos(a + rot) * rad;
    float y = cy + sin(a + rot) * rad;
    spr.drawLine(px, py, x, y, color);
    spr.drawLine(px + 1, py, x + 1, y, color);
    px = x; py = y;
  }
}

// isLeft decides which side is the "inner" side (toward the nose)
void drawEye(int cx, int cy, const EyeState &e, float blinkF, bool isLeft) {
  int R = EYE_R * e.size;
  float open = e.open * blinkF;

  // Closed eye -> simple curved line
  if (open < 0.1) {
    spr.fillRoundRect(cx - R, cy - 3, 2 * R, 7, 3, scleraColor);
    return;
  }

  // Eye white
  spr.fillCircle(cx, cy, R, scleraColor);

  // Pupil + highlight
  int pr = R * 0.5 * e.pupil;
  int px = cx + curLookX * R * 0.35;
  int py = cy + curLookY * R * 0.35;
  spr.fillCircle(px, py, pr, pupilColor);
  spr.fillCircle(px - pr * 0.4, py - pr * 0.4, max(2, (int)(pr * 0.25)), TFT_WHITE);

  // Upper lid (background colored polygon covering the top of the eye)
  float base = cy - R + (1.0 - open) * 2.0 * R;
  float yInner = base + e.slant;
  float yOuter = base - e.slant;
  int xl = cx - R - 2, xr = cx + R + 2;
  float yl = isLeft ? yOuter : yInner;
  float yr = isLeft ? yInner : yOuter;
  int top = cy - R - abs((int)e.slant) - 6;
  spr.fillTriangle(xl, top, xr, top, xr, yr, BG);
  spr.fillTriangle(xl, top, xr, yr, xl, yl, BG);

  // Lower lid / cheek (happy squint)
  if (e.cheek > 0.02) {
    int cr = R * 1.25;
    int ccy = cy + R + cr - e.cheek * R * 1.15;
    spr.fillCircle(cx, ccy, cr, BG);
  }
}

void drawFrame() {
  spr.fillSprite(BG);

  // Blink factor (1 = open, 0 = closed)
  float blinkF = 1.0;
  unsigned long t = millis() - blinkStart;
  if (t < BLINK_MS) blinkF = 1.0 - sin(PI * (float)t / BLINK_MS);

  // Eye positions including whole-eye movement
  int lx = EYE_LX + (int)(curOffX * MAX_OFF_X);
  int rx = EYE_RX + (int)(curOffX * MAX_OFF_X);
  int ey = EYE_Y  + (int)(curOffY * MAX_OFF_Y);

  if (mode == MODE_NORMAL) {
    drawEye(lx, ey, curL, blinkF, true);
    drawEye(rx, ey, curR, blinkF, false);
  }
  else if (mode == MODE_HEART) {
    float pulse = 1.0 + 0.12 * sin(millis() / 120.0);
    int s = EYE_R * 2.1 * pulse;
    drawHeart(lx, ey, s, TFT_RED);
    drawHeart(rx, ey, s, TFT_RED);
  }
  else if (mode == MODE_DIZZY) {
    float rot = millis() / 150.0;
    spr.drawCircle(lx, ey, EYE_R, scleraColor);
    spr.drawCircle(rx, ey, EYE_R, scleraColor);
    drawSpiral(lx, ey, EYE_R - 3, rot, scleraColor);
    drawSpiral(rx, ey, EYE_R - 3, -rot, scleraColor);
  }

  spr.pushSprite(0, 0);
}

// =====================================================================
//                        IDLE BEHAVIOUR
// =====================================================================

void idleBehaviour() {
  if (!autoIdle) return;
  unsigned long now = millis();

  if (now > nextBlink) {
    blinkNow();
    nextBlink = now + random(2000, 5500);
  }
  if (mode == MODE_NORMAL && now > nextGlance) {
    float gx = random(-100, 101) / 100.0;
    float gy = random(-60, 61) / 100.0;
    lookAt(gx, gy);
    moveEyes(gx * 0.6, gy * 0.5);   // whole eyes drift a little too; remove if unwanted
    nextGlance = now + random(1500, 4000);
  }
}

// =====================================================================
//                        SETUP / LOOP
// =====================================================================

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

  tft.init();
  tft.setRotation(1);            // landscape 320x240
  tft.fillScreen(BG);

  spr.setColorDepth(8);          // 8-bit to fit in RAM on a plain ESP32
  if (!spr.createSprite(SCREEN_W, SCREEN_H)) {
    Serial.println("Sprite allocation failed!");
    while (true) delay(1000);
  }

  eyesNeutral();
}

// Demo: cycle through all emotions
typedef void (*EmotionFn)();
EmotionFn emotions[] = {
  eyesNeutral, eyesHappy, eyesSad, eyesAngry, eyesSurprised,
  eyesSleepy, eyesSuspicious, eyesWink, eyesLove, eyesDizzy
};
const int NUM_EMOTIONS = sizeof(emotions) / sizeof(emotions[0]);
int emotionIndex = 0;
unsigned long nextEmotion = 0;

void loop() {
  unsigned long now = millis();

  // Demo cycling (remove this block and call the eyesXxx() functions yourself)
  if (now > nextEmotion) {
    emotions[emotionIndex]();
    emotionIndex = (emotionIndex + 1) % NUM_EMOTIONS;
    nextEmotion = now + 3500;
  }

  // ~60 fps animation
  if (now - lastFrame >= 16) {
    lastFrame = now;
    updateState(curL, tgtL, 0.25);
    updateState(curR, tgtR, 0.25);
    curLookX = lerpf(curLookX, tgtLookX, 0.2);
    curLookY = lerpf(curLookY, tgtLookY, 0.2);
    curOffX  = lerpf(curOffX,  tgtOffX,  0.15);
    curOffY  = lerpf(curOffY,  tgtOffY,  0.15);
    idleBehaviour();

    // TEST: sweep the whole eyes side to side (remove when done testing)
    moveEyes(sin(now / 1000.0), 0);

    drawFrame();
  }
}
