/**
 * Simple test program for Orpheus Pico - tests all functions of Sprig.
 */

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Arduino.h>
#include <FastLED.h>
#include <map>

#define RST 26
#define TFT_CS 20
#define DC 22
#define MOSI 19
#define SCK 18
#define BL 17

#define BTN_W p5
#define BTN_A p6
#define BTN_S p7
#define BTN_D p8
#define BTN_I p12
#define BTN_J p13
#define BTN_K p14
#define BTN_L p15

#define BTN_USR p25
#define LED_USR p23
#define LED_PIXEL p24

#define NUM_COLORS 7

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, DC, MOSI, SCK, RST);

CRGB pixel[1];

std::map<PinName, bool> lastVal;

long lastUpdate = 1;
long lastLEDUpdate = 0;

const char *LEDNames[] = {"RED    ", "ORANGE ", "YELLOW ", "GREEN  ",
                          "BLUE   ", "MAGENTA", "WHITE  "};

uint16_t LEDScreenColors[] = {ST7735_RED,   ST7735_ORANGE, ST7735_YELLOW,
                              ST7735_GREEN, ST7735_BLUE,   ST7735_MAGENTA,
                              ST7735_WHITE};

CRGB LEDPixelColors[] = {CRGB::Red,  CRGB::Orange,  CRGB::Yellow, CRGB::Green,
                         CRGB::Blue, CRGB::Magenta, CRGB::White};

uint16_t currentLED = 0;

void initialPrint(void);
void LEDCycle(void);

void setup(void) {
  Serial.begin(9600);
  Serial.print(F("Orpheus Pico quicktest!"));
  Serial.print(PICO_FLASH_SIZE_BYTES);

  pinMode(BL, OUTPUT);
  digitalWrite(BL, HIGH);

  pinMode(BTN_W, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_S, INPUT_PULLUP);
  pinMode(BTN_D, INPUT_PULLUP);
  pinMode(BTN_I, INPUT_PULLUP);
  pinMode(BTN_J, INPUT_PULLUP);
  pinMode(BTN_K, INPUT_PULLUP);
  pinMode(BTN_L, INPUT_PULLUP);

  pinMode(BTN_USR, INPUT_PULLDOWN);
  pinMode(LED_USR, OUTPUT);

  FastLED.addLeds<NEOPIXEL, LED_PIXEL>(pixel, 1);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  Serial.println(F("Initialized display"));

  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED);

  initialPrint();
}

void buttonIndicator(const char *text, PinName pin, int x, int y) {
  // Get current val of pin
  bool isPressed = digitalRead(pin);

  // Switch pulled down pin vals
  switch (pin) {
  case BTN_USR:
    break;
  default:
    isPressed = !isPressed;
  }
  // don't waste resources if value is unchanged
  if (!lastVal.count(pin) || isPressed != lastVal[pin]) {
    tft.setCursor(x, y);
    if (isPressed)
      tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
    else
      tft.setTextColor(ST7735_RED, ST7735_BLACK);
    tft.print(F(text));
  }
  lastVal[pin] = isPressed;
}

void initialPrint() {
  // Label settings
  tft.setTextColor(ST7735_WHITE);

  tft.setCursor(5, 5);
  tft.println("Orpheus Pico Quicktest :)");

  tft.setCursor(5, 30);
  tft.print("D-Pads");

  tft.setCursor(5, 65);
  tft.print("Orpheus Pico");

  tft.setTextColor(ST7735_ORANGE);
  tft.setCursor(5, 85);
  tft.print("RGBLED: ");

  tft.setCursor(5, 95);
  tft.print("User LED: ");

  tft.setTextColor(ST7735_CYAN);
  tft.setCursor(5, 118);
  tft.print("***adammakesthings.dev***");
}

void LEDCycle() {
  // Print current LED color
  int c = currentLED % NUM_COLORS;
  tft.setTextColor(LEDScreenColors[c], ST7735_BLACK);
  tft.setCursor(50, 85);
  tft.print(LEDNames[c]);

  // Toggle non-RGB LED
  tft.setTextColor(c % 2 == 0 ? ST7735_GREEN : ST7735_RED, ST7735_BLACK);
  tft.setCursor(60, 95);
  tft.print(c % 2 == 0 ? "On " : "Off");
  digitalWrite(LED_USR, !(c % 2));

  pixel[0] = LEDPixelColors[c];
  FastLED.show();

  // Reset counter
  currentLED = c + 1;
}

void loop() {
  // Calculate frames/second
  uint16_t deltaT = millis() - lastUpdate;
  lastUpdate = millis();
  if (millis() - lastLEDUpdate > 1000) {
    lastLEDUpdate = millis();
    LEDCycle();
  }

  tft.setCursor(5, 15);
  tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
  tft.print(F("FPS: "));
  tft.print(1000.0 / deltaT, 1);

  // D-Pad left buttons
  buttonIndicator("W", BTN_W, 15, 40);
  buttonIndicator("A", BTN_A, 5, 50);
  buttonIndicator("S", BTN_S, 15, 50);
  buttonIndicator("D", BTN_D, 25, 50);

  // D-Pad right buttons
  buttonIndicator("I", BTN_I, 50, 40);
  buttonIndicator("J", BTN_J, 40, 50);
  buttonIndicator("K", BTN_K, 50, 50);
  buttonIndicator("L", BTN_L, 60, 50);

  // Orph Pico onboard buttons
  buttonIndicator("User button", BTN_USR, 5, 75);
}
