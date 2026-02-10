/**
 * Etch-a-Sketch for Orpheus Pico
 */

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Arduino.h>
#include <FastLED.h>

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

// Drawing settings
#define BG_COLOR ST7735_BLACK
#define MOVE_SPEED 2 // Pixels to move per button press
#define NUM_COLORS 8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, DC, MOSI, SCK, RST);
CRGB pixel[1];

// Color arrays
uint16_t drawColors[] = {ST7735_WHITE,  ST7735_RED,   ST7735_GREEN,
                         ST7735_BLUE,   ST7735_CYAN,  ST7735_MAGENTA,
                         ST7735_YELLOW, ST7735_ORANGE};

const char *colorNames[] = {"WHITE", "RED",     "GREEN",  "BLUE",
                            "CYAN",  "MAGENTA", "YELLOW", "ORANGE"};

int currentColorIndex = 0;

// Cursor position
int cursorX = 80;
int cursorY = 64;

// Old cursor location
int oldX = 80;
int oldY = 64;

// Drawing state
bool isDrawing = true; // Start in drawing mode
bool bigDot = false;

// Button debouncing
unsigned long lastMoveTime = 0;
unsigned long moveDelay = 50; // ms between moves

// Screen bounds (adjusted for rotation 3: landscape)
const int MIN_X = 2;
const int MAX_X = 158;
const int MIN_Y = 2;
const int MAX_Y = 126;

// Function declarations
void updateLEDStatus();
bool buttonPressed(PinName pin);

void setup(void) {
  Serial.begin(9600);
  Serial.println(F("Etch-a-Sketch Starting!"));

  // Setup backlight
  pinMode(BL, OUTPUT);
  digitalWrite(BL, HIGH);

  // Setup buttons
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

  // Setup RGB LED
  FastLED.addLeds<NEOPIXEL, LED_PIXEL>(pixel, 1);

  // Initialize display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3); // Landscape mode

  // Clear screen and draw border
  tft.fillScreen(BG_COLOR);
  tft.drawRect(0, 0, 160, 128, ST7735_CYAN);

  // Show initial instructions briefly
  tft.setTextColor(ST7735_GREEN);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.print("WASD: Move");
  tft.setCursor(10, 25);
  tft.print("I: Toggle Draw");
  tft.setCursor(10, 40);
  tft.print("J: Clear");
  tft.setCursor(10, 55);
  tft.print("K: Change Color");
  tft.setCursor(10, 70);
  tft.print("L: Thick/Thin");

  delay(2000);
  tft.fillScreen(BG_COLOR);
  tft.drawRect(0, 0, 160, 128, ST7735_BLUE);

  // Set LED to show drawing mode
  updateLEDStatus();

  Serial.println(F("Ready to draw!"));
}

void updateLEDStatus() {
  if (isDrawing) {
    pixel[0] = CRGB::Green;
    digitalWrite(LED_USR, HIGH);
  } else {
    pixel[0] = CRGB::Red;
    digitalWrite(LED_USR, LOW);
  }
  FastLED.show();
}

bool buttonPressed(PinName pin) {
  // Most buttons are pull-up (LOW when pressed)
  if (pin == BTN_USR) {
    return digitalRead(pin) == HIGH;
  }
  return digitalRead(pin) == LOW;
}

void loop() {
  unsigned long currentTime = millis();
  bool moved = false;

  // Handle movement (with debouncing)
  if (currentTime - lastMoveTime > moveDelay) {
    if (buttonPressed(BTN_W)) { // Up
      cursorY -= MOVE_SPEED;
      moved = true;
    }
    if (buttonPressed(BTN_S)) { // Down
      cursorY += MOVE_SPEED;
      moved = true;
    }
    if (buttonPressed(BTN_A)) { // Left
      cursorX -= MOVE_SPEED;
      moved = true;
    }
    if (buttonPressed(BTN_D)) { // Right
      cursorX += MOVE_SPEED;
      moved = true;
    }

    // Constrain to screen bounds
    cursorX = constrain(cursorX, MIN_X, MAX_X);
    cursorY = constrain(cursorY, MIN_Y, MAX_Y);

    if (moved) {
      lastMoveTime = currentTime;

      if (isDrawing) {
        // Draw line from old position to new position using current color
        tft.drawLine(oldX, oldY, cursorX, cursorY,
                     drawColors[currentColorIndex]);
        if (bigDot) {
          // Draw thick line by drawing parallel lines
          tft.drawLine(oldX + 1, oldY, cursorX + 1, cursorY,
                       drawColors[currentColorIndex]);
          tft.drawLine(oldX, oldY + 1, cursorX, cursorY + 1,
                       drawColors[currentColorIndex]);
          tft.drawLine(oldX + 1, oldY + 1, cursorX + 1, cursorY + 1,
                       drawColors[currentColorIndex]);
        }
      }

      // Update old position for next line segment
      oldX = cursorX;
      oldY = cursorY;
    }
    // Handle toggle drawing mode (I button)
    // static keyword initializes only once
    static bool lastIState = false;
    bool currentIState = buttonPressed(BTN_I);
    if (currentIState && !lastIState) {
      isDrawing = !isDrawing;
      updateLEDStatus();
      Serial.println(isDrawing ? "Drawing ON" : "Drawing OFF");
    }
    lastIState = currentIState;

    // Handle clear screen (J button)
    static bool lastJState = false;
    bool currentJState = buttonPressed(BTN_J);
    if (currentJState && !lastJState) {
      tft.fillScreen(BG_COLOR);
      tft.drawRect(0, 0, 160, 128, ST7735_BLUE);
      Serial.println("Screen cleared!");
    }
    lastJState = currentJState;

    // Handle color change (K button)
    static bool lastKState = false;
    bool currentKState = buttonPressed(BTN_K);
    if (currentKState && !lastKState) {
      currentColorIndex = (currentColorIndex + 1) % NUM_COLORS;
      Serial.print("Color: ");
      Serial.println(colorNames[currentColorIndex]);
    }
    lastKState = currentKState;

    // Handle color change (K button)
    static bool lastLState = false;
    bool currentLState = buttonPressed(BTN_L);
    if (currentLState && !lastLState) {
      bigDot = !bigDot;
    }
    lastLState = currentLState;

    // Small delay to prevent excessive loop speed
    delay(10);
  }
}
