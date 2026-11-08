#include <Adafruit_NeoPixel.h>

#define LED_PIN 3
#define AUDIO_PIN 26
#define NUM_LEDS 30
#define BRIGHTNESS 90 // lowerr means less electricity. 255 max (i think)
#define MODE_SECONDS 20 // time it stays in a mode before switching to the next one

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

float level = 0.0f;
float peak = 0.05f;
float flash = 0.0f;
uint16_t hue = 0;
uint32_t lastSound = 0;
uint32_t modeStart = 0;
uint8_t mode = 0;

float readAmplitude() {
    int mn = 4095;
    int mx = 0;
    for (int i = 0; i < 512; i++) {
        int v = analogRead(AUDIO_PIN);
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    float amp = (mx - mn) / 4095.0f - 0.01f;
    return amp < 0 ? 0 : amp;
}

void drawIdle() {
    uint8_t v = 40 + 20 * sin(millis() / 800.0);
    for (int i = 0; i < NUM_LEDS; i++) {
        uint16_t h = hue + (uint32_t)i * 65535UL / NUM_LEDS;
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(h, 255, v)));
    }
}

void drawPulse() {
    uint32_t c = strip.gamma32(strip.ColorHSV(hue, 255, (uint8_t)(30 + level * 225)));
    for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, c);
}

void drawBar() {
    int n = level * NUM_LEDS;
    for (int i = 0; i < n; i++) {
        uint16_t h = 21845 - (uint32_t)i * 21845 / NUM_LEDS;
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(h, 255, 255)));
    }
}

void drawCenter() {
    int half = NUM_LEDS / 2;
    int n = level * half;
    for (int i = 0; i < n; i++) {
        uint32_t c = strip.gamma32(strip.ColorHSV(hue + i * 1500, 255, 255));
        strip.setPixelColor(half + i, c);
        strip.setPixelColor(half - i - 1, c);
    }
}

void drawFlash() {
    float f = (flash > level * 0.6f) ? flash : level * 0.6f;
    uint8_t sat = 255 - (uint8_t)(flash * 150);
    uint32_t c = strip.gamma32(strip.ColorHSV(hue, sat, (uint8_t)(20 + f * 235)));
    for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, c);
}

void setup() {
    analogReadResolution(12);
    strip.begin();
    strip.setBrightness(BRIGHTNESS);
    strip.show();
    modeStart = millis();
    lastSound = millis();
}

void loop() {
    float amp = readAmplitude();

    if (amp > peak) peak = amp;
    else peak *= 0.999f;
    if (peak < 0.03f) peak = 0.03f;
    float target = constrain(amp / peak, 0.0f, 1.0f);

    if (target > level + 0.25f) {
        flash = 1.0f;
        hue += 9000;
    }
    flash *= 0.85f;

    level = (target > level) ? level * 0.5f + target * 0.5f : level * 0.92f + target * 0.08f;
    hue += 40;

    if (amp > 0.02f) lastSound = millis();
    bool silent = (millis() - lastSound > 5000); // 5 seconds of silence

    if (millis() - modeStart > MODE_SECONDS * 1000UL) {
        mode = (mode + 1) % 4;
        modeStart = millis();
    }

    strip.clear();
    if (silent) drawIdle();
    else if (mode == 0) drawPulse();
    else if (mode == 1) drawBar();
    else if (mode == 2) drawCenter();
    else drawFlash();
    strip.show();
}
