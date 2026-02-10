/**
 * Etch-a-Sketch for Orpheus Pico with Screen Buffer
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
#define CURSOR_COLOR ST7735_RED
#define MOVE_SPEED 2 // Pixels to move per button press
#define NUM_COLORS 8
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 128
#define CURSOR_SIZE 2 // 2x2 cursor

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, DC, MOSI, SCK, RST);
CRGB pixel[1];

// Screen buffer - stores what we've drawn
uint16_t screenBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

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
bool bigDot = false;   // false = thin line, true = thick line

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
void drawToBuffer(int x, int y, uint16_t color);
void drawLineToBuffer(int x0, int y0, int x1, int y1, uint16_t color);
void clearBuffer();
void refreshScreen();
void drawCursor();
void eraseCursor();

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

  // Initialize buffer and screen
  clearBuffer();
  refreshScreen();

  // Set LED to show drawing mode
  updateLEDStatus();

  Serial.println(F("Ready to draw!"));
}

void clearBuffer() {
  // Fill buffer with background color
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      // Draw border in buffer
      if (x == 0 || x == SCREEN_WIDTH - 1 || y == 0 || y == SCREEN_HEIGHT - 1) {
        screenBuffer[y][x] = ST7735_BLUE;
      } else {
        screenBuffer[y][x] = BG_COLOR;
      }
    }
  }
}

void drawToBuffer(int x, int y, uint16_t color) {
  // Draw pixel to buffer (bounds check)
  if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
    screenBuffer[y][x] = color;
  }
}

void drawLineToBuffer(int x0, int y0, int x1, int y1, uint16_t color) {
  // Bresenham's line algorithm
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    drawToBuffer(x0, y0, color);

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void refreshScreen() {
  // Draw entire buffer to screen using fast bulk transfer
  // Faster as it's one transaction compared to the prev one transaction per
  // pixel
  tft.startWrite();                                     // Begin SPI transaction
  tft.setAddrWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // Set drawing area

  // Send all pixels at once
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      tft.pushColor(screenBuffer[y][x]);
    }
  }

  tft.endWrite(); // End SPI transaction
}
void drawCursor() {
  // Draw cursor on screen (not in buffer)
  if (bigDot) {
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
      for (int dx = 0; dx < CURSOR_SIZE; dx++) {
        int x = cursorX - CURSOR_SIZE / 2 + dx;
        int y = cursorY - CURSOR_SIZE / 2 + dy;
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
          tft.drawPixel(x, y, drawColors[currentColorIndex]);
        }
      }
    }
  } else {
    tft.drawPixel(cursorX, cursorY, drawColors[currentColorIndex]);
  }
}

void eraseCursor() {
  // Restore cursor area from buffer
  if (bigDot) {
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
      for (int dx = 0; dx < CURSOR_SIZE; dx++) {
        int x = oldX - CURSOR_SIZE / 2 + dx;
        int y = oldY - CURSOR_SIZE / 2 + dy;
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
          tft.drawPixel(x, y, screenBuffer[y][x]);
        }
      }
    }
  } else {
    tft.drawPixel(oldX, oldY, screenBuffer[oldY][oldX]);
  }
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
        // Draw line to buffer
        drawLineToBuffer(oldX, oldY, cursorX, cursorY,
                         drawColors[currentColorIndex]);

        if (bigDot) {
          // Draw thick line
          drawLineToBuffer(oldX + 1, oldY, cursorX + 1, cursorY,
                           drawColors[currentColorIndex]);
          drawLineToBuffer(oldX, oldY + 1, cursorX, cursorY + 1,
                           drawColors[currentColorIndex]);
          drawLineToBuffer(oldX + 1, oldY + 1, cursorX + 1, cursorY + 1,
                           drawColors[currentColorIndex]);
        }

        // Draw the line to screen
        tft.drawLine(oldX, oldY, cursorX, cursorY,
                     drawColors[currentColorIndex]);
        if (bigDot) {
          tft.drawLine(oldX + 1, oldY, cursorX + 1, cursorY,
                       drawColors[currentColorIndex]);
          tft.drawLine(oldX, oldY + 1, cursorX, cursorY + 1,
                       drawColors[currentColorIndex]);
          tft.drawLine(oldX + 1, oldY + 1, cursorX + 1, cursorY + 1,
                       drawColors[currentColorIndex]);
        }
      } else {
        // Not drawing - erase old cursor and draw new one
        eraseCursor();
        oldX = cursorX;
        oldY = cursorY;
        drawCursor();
      }

      // Update old position for next line segment
      if (isDrawing) {
        oldX = cursorX;
        oldY = cursorY;
      }
    }
  }

  // Handle toggle drawing mode (I button)
  static bool lastIState = false;
  bool currentIState = buttonPressed(BTN_I);
  if (currentIState && !lastIState) {
    isDrawing = !isDrawing;
    updateLEDStatus();

    if (isDrawing) {
      eraseCursor(); // Remove cursor when starting to draw
    } else {
      drawCursor(); // Show cursor when stopping drawing
    }

    Serial.println(isDrawing ? "Drawing ON" : "Drawing OFF");
  }
  lastIState = currentIState;

  // Handle clear screen (J button)
  static bool lastJState = false;
  bool currentJState = buttonPressed(BTN_J);
  if (currentJState && !lastJState) {
    clearBuffer();
    refreshScreen();

    if (!isDrawing) {
      drawCursor(); // Redraw cursor after clear if not drawing
    }

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

  // Handle thick/thin toggle (L button)
  static bool lastLState = false;
  bool currentLState = buttonPressed(BTN_L);
  if (currentLState && !lastLState) {
    if (!isDrawing)
      eraseCursor();
    bigDot = !bigDot;
    drawCursor();
    Serial.println(bigDot ? "Thick line" : "Thin line");
  }
  lastLState = currentLState;

  if (!isDrawing) {
    drawCursor(); // For Color/Size Changes
  }

  // Small delay to prevent excessive loop speed
  delay(10);
}
