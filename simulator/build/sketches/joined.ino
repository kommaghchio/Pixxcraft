#line 1 "C:/Users/kommaghchio/starbie/pixxcraft/pixxcraft/software/sketch_oct27a/sketch_oct27a.ino"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define TOUCH_SDA 5
#define TOUCH_SCL 6
#define TOUCH_RST 44
#define TOUCH_INT 43
#define CST816_ADDR 0x15

#define TOP_H 50
#define BOTTOM_Y 248
#define CANVAS_W 240
#define CANVAS_H (BOTTOM_Y - TOP_H)
#define SLIDER_X 56
#define SLIDER_W 118
#define SLIDER_H 9
#define MAX_FRAMES 4
#define FPS_MIN 1
#define FPS_MAX 12

Adafruit_ST7789 display(2, 3, 4);  // CS, DC, RST

uint16_t selected = ST77XX_RED;
int brushSize = 3;
uint8_t customR = 255;
uint8_t customG = 0;
uint8_t customB = 0;
int previewFps = 4;
bool touchReady = false;
bool wasDown = false;

uint16_t* frames[MAX_FRAMES];
int frameCount = 0;
int currentFrame = 0;

bool isInside(int touchX, int touchY, int bx, int by, int width, int height) {
  return touchX >= bx && touchX < bx + width && touchY >= by && touchY < by + height;
}

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void applyCustomColor() {
  selected = color565(customR, customG, customB);
}

int previewDelayMs() {
  return 1000 / previewFps;
}

size_t frameBytes() {
  return (size_t)CANVAS_W * (size_t)CANVAS_H * sizeof(uint16_t);
}

void clearFrameBuffer(uint16_t* buf) {
  for (int i = 0; i < CANVAS_W * CANVAS_H; i++) {
    buf[i] = ST77XX_BLACK;
  }
}

void blitFrame(int index, int destY) {
  if (index < 0 || index >= frameCount || frames[index] == nullptr) {
    return;
  }
  display.drawRGBBitmap(0, destY, frames[index], CANVAS_W, CANVAS_H);
}

void showCurrentFrame() {
  blitFrame(currentFrame, TOP_H);
}

void paintCircle(int screenX, int screenY, int radius, uint16_t color) {
  display.fillCircle(screenX, screenY, radius, color);

  int canvasY = screenY - TOP_H;
  uint16_t* buf = frames[currentFrame];
  if (buf == nullptr) {
    return;
  }

  for (int dy = -radius; dy <= radius; dy++) {
    for (int dx = -radius; dx <= radius; dx++) {
      if (dx * dx + dy * dy > radius * radius) {
        continue;
      }
      int x = screenX + dx;
      int y = canvasY + dy;
      if (x >= 0 && x < CANVAS_W && y >= 0 && y < CANVAS_H) {
        buf[y * CANVAS_W + x] = color;
      }
    }
  }
}

void clearCurrentDrawing() {
  clearFrameBuffer(frames[currentFrame]);
  display.fillRect(0, TOP_H, CANVAS_W, CANVAS_H, ST77XX_BLACK);
}

void gotoFrame(int index) {
  if (index < 0 || index >= frameCount) {
    return;
  }
  currentFrame = index;
  showCurrentFrame();
  Serial.print("Editing frame ");
  Serial.print(currentFrame + 1);
  Serial.print("/");
  Serial.println(frameCount);
}

bool addFrame() {
  if (frameCount >= MAX_FRAMES) {
    Serial.println("Max frames reached");
    return false;
  }
  uint16_t* next = (uint16_t*)malloc(frameBytes());
  if (next == nullptr) {
    Serial.println("Out of memory for new frame");
    return false;
  }
  memcpy(next, frames[currentFrame], frameBytes());
  frames[frameCount] = next;
  currentFrame = frameCount;
  frameCount++;
  showCurrentFrame();
  Serial.print("Frame ");
  Serial.print(currentFrame + 1);
  Serial.print("/");
  Serial.println(frameCount);
  return true;
}

void drawSlider(int y, uint8_t value, uint16_t fillColor) {
  display.fillRect(SLIDER_X, y, SLIDER_W, SLIDER_H, 0xC618);
  display.drawRect(SLIDER_X, y, SLIDER_W, SLIDER_H, ST77XX_BLACK);
  int filled = (int)value * (SLIDER_W - 2) / 255;
  if (filled > 0) {
    display.fillRect(SLIDER_X + 1, y + 1, filled, SLIDER_H - 2, fillColor);
  }
  int knobX = SLIDER_X + (int)value * (SLIDER_W - 5) / 255;
  display.fillRect(knobX, y - 1, 5, SLIDER_H + 2, ST77XX_BLACK);
}

void drawTinyBtn(int x, int y, int w, int h, const char* label) {
  display.fillRect(x, y, w, h, ST77XX_WHITE);
  display.drawRect(x, y, w, h, ST77XX_BLACK);
  display.setTextColor(ST77XX_BLACK);
  display.setTextSize(1);
  display.setCursor(x + 3, y + (h - 8) / 2);
  display.print(label);
}

void drawTopBar() {
  display.fillRect(0, 0, 240, TOP_H, ST77XX_WHITE);

  // brush sizes (compact)
  display.fillRect(4, 2, 14, 18, 0xC618);
  display.fillCircle(11, 11, 1, ST77XX_RED);
  display.fillRect(20, 2, 14, 18, 0xC618);
  display.fillCircle(27, 11, 2, ST77XX_RED);
  display.fillRect(36, 2, 14, 18, 0xC618);
  display.fillCircle(43, 11, 3, ST77XX_RED);

  // RGB sliders
  display.setTextSize(1);
  display.setTextColor(ST77XX_RED);
  display.setCursor(52, 2);
  display.print("R");
  display.setTextColor(ST77XX_GREEN);
  display.setCursor(52, 14);
  display.print("G");
  display.setTextColor(ST77XX_BLUE);
  display.setCursor(52, 26);
  display.print("B");

  drawSlider(2, customR, ST77XX_RED);
  drawSlider(14, customG, ST77XX_GREEN);
  drawSlider(26, customB, ST77XX_BLUE);

  // color preview
  display.fillRect(180, 2, 22, 32, color565(customR, customG, customB));
  display.drawRect(180, 2, 22, 32, ST77XX_BLACK);

  // frame / play / fps row
  drawTinyBtn(4, 34, 16, 14, "<");
  drawTinyBtn(22, 34, 28, 14, "FR+");
  drawTinyBtn(52, 34, 16, 14, ">");
  drawTinyBtn(72, 34, 34, 14, "PLAY");

  drawTinyBtn(110, 34, 16, 14, "-");
  display.setTextColor(ST77XX_BLACK);
  display.setCursor(128, 37);
  display.print(previewFps);
  display.print("fps");
  drawTinyBtn(158, 34, 16, 14, "+");

  display.setCursor(180, 37);
  display.print("F");
  display.print(currentFrame + 1);
  display.print("/");
  display.print(frameCount);
}

void drawBottomBar() {
  display.fillRect(0, BOTTOM_Y, 240, 280 - BOTTOM_Y, ST77XX_WHITE);
  // palette + eraser + clear (shorter)
  display.fillRect(6, 252, 16, 22, ST77XX_RED);
  display.fillRect(26, 252, 16, 22, ST77XX_ORANGE);
  display.fillRect(46, 252, 16, 22, ST77XX_YELLOW);
  display.fillRect(66, 252, 16, 22, ST77XX_GREEN);
  display.fillRect(86, 252, 16, 22, ST77XX_BLUE);
  display.fillRect(106, 252, 16, 22, ST77XX_MAGENTA);
  display.fillRect(126, 252, 16, 22, 0xC618);
  display.drawRect(126, 252, 16, 22, ST77XX_BLACK);
  drawTinyBtn(150, 252, 28, 22, "CLR");
}

void drawUI() {
  drawTopBar();
  drawBottomBar();
  showCurrentFrame();
}

bool readTouch(int* fingerx, int* fingery, uint8_t* fingers) {
  Wire.beginTransmission(CST816_ADDR);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(CST816_ADDR, (uint8_t)5) != 5) {
    return false;
  }

  uint8_t data[5];
  for (int i = 0; i < 5; i++) {
    data[i] = Wire.read();
  }

  *fingers = data[0] & 0x0F;
  *fingerx = ((data[1] & 0x0F) << 8) | data[2];
  *fingery = ((data[3] & 0x0F) << 8) | data[4];
  return true;
}

bool touchDown(int* fingerx, int* fingery) {
  uint8_t fingers = 0;
  if (!readTouch(fingerx, fingery, &fingers)) {
    return false;
  }
  return fingers > 0 && digitalRead(TOUCH_INT) == LOW;
}

uint8_t sliderValueFromX(int touchX) {
  int x = touchX - SLIDER_X;
  if (x < 0) {
    x = 0;
  }
  if (x > SLIDER_W - 1) {
    x = SLIDER_W - 1;
  }
  return (uint8_t)(x * 255 / (SLIDER_W - 1));
}

bool handleSliderTouch(int fingerx, int fingery) {
  if (isInside(fingerx, fingery, SLIDER_X - 2, 1, SLIDER_W + 4, 11)) {
    customR = sliderValueFromX(fingerx);
    applyCustomColor();
    drawTopBar();
    return true;
  }
  if (isInside(fingerx, fingery, SLIDER_X - 2, 13, SLIDER_W + 4, 11)) {
    customG = sliderValueFromX(fingerx);
    applyCustomColor();
    drawTopBar();
    return true;
  }
  if (isInside(fingerx, fingery, SLIDER_X - 2, 25, SLIDER_W + 4, 11)) {
    customB = sliderValueFromX(fingerx);
    applyCustomColor();
    drawTopBar();
    return true;
  }
  if (isInside(fingerx, fingery, 180, 2, 22, 32)) {
    applyCustomColor();
    return true;
  }
  return false;
}

void waitForFingerUp() {
  int x, y;
  while (touchDown(&x, &y)) {
    delay(20);
  }
}

void playPreview() {
  Serial.print("Preview at ");
  Serial.print(previewFps);
  Serial.println(" fps - tap to exit");
  display.fillScreen(ST77XX_BLACK);
  int destY = (280 - CANVAS_H) / 2;
  waitForFingerUp();

  int delayMs = previewDelayMs();
  bool running = true;
  while (running) {
    for (int i = 0; i < frameCount && running; i++) {
      blitFrame(i, destY);

      unsigned long start = millis();
      while (millis() - start < (unsigned long)delayMs) {
        int x, y;
        if (touchDown(&x, &y)) {
          running = false;
          break;
        }
        delay(20);
      }
    }
  }

  waitForFingerUp();
  display.fillScreen(ST77XX_BLACK);
  drawUI();
  Serial.println("Preview ended");
}

void setup() {
  Serial.begin(115200);

  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);  // backlight
  pinMode(TOUCH_INT, INPUT);

  for (int i = 0; i < MAX_FRAMES; i++) {
    frames[i] = nullptr;
  }
  frames[0] = (uint16_t*)malloc(frameBytes());
  if (frames[0] == nullptr) {
    Serial.println("Failed to allocate frame 0");
  } else {
    clearFrameBuffer(frames[0]);
    frameCount = 1;
    currentFrame = 0;
  }

  display.init(240, 280);
  // Adafruit's default rotation (0) sets MADCTL MX|MY, which flips this panel.
  display.setRotation(2);
  display.fillScreen(ST77XX_BLACK);
  applyCustomColor();
  drawUI();

  Wire.begin(TOUCH_SDA, TOUCH_SCL);

  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(20);
  digitalWrite(TOUCH_RST, HIGH);
  delay(100);

  int x, y;
  uint8_t fingers;
  for (int i = 0; i < 10; i++) {
    if (readTouch(&x, &y, &fingers) && fingers == 0) {
      break;
    }
    delay(10);
  }
  touchReady = false;
  wasDown = false;

  Serial.println("Touch ready!");
}

void loop() {
  int fingerx = 0;
  int fingery = 0;

  if (!touchDown(&fingerx, &fingery)) {
    touchReady = true;
    wasDown = false;
    delay(20);
    return;
  }

  if (!touchReady) {
    delay(20);
    return;
  }

  bool picked = false;
  if (handleSliderTouch(fingerx, fingery)) {
    picked = true;
  } else if (isInside(fingerx, fingery, 4, 34, 16, 14)) {
    // previous frame
    if (!wasDown && currentFrame > 0) {
      gotoFrame(currentFrame - 1);
      drawTopBar();
    }
    picked = true;
  } else if (isInside(fingerx, fingery, 22, 34, 28, 14)) {
    // new frame
    if (!wasDown) {
      if (addFrame()) {
        drawTopBar();
      }
    }
    picked = true;
  } else if (isInside(fingerx, fingery, 52, 34, 16, 14)) {
    // next frame
    if (!wasDown && currentFrame < frameCount - 1) {
      gotoFrame(currentFrame + 1);
      drawTopBar();
    }
    picked = true;
  } else if (isInside(fingerx, fingery, 72, 34, 34, 14)) {
    if (!wasDown) {
      playPreview();
      wasDown = false;
      return;
    }
    picked = true;
  } else if (isInside(fingerx, fingery, 110, 34, 16, 14)) {
    if (!wasDown && previewFps > FPS_MIN) {
      previewFps--;
      drawTopBar();
      Serial.print("FPS ");
      Serial.println(previewFps);
    }
    picked = true;
  } else if (isInside(fingerx, fingery, 158, 34, 16, 14)) {
    if (!wasDown && previewFps < FPS_MAX) {
      previewFps++;
      drawTopBar();
      Serial.print("FPS ");
      Serial.println(previewFps);
    }
    picked = true;
  } else if (isInside(fingerx, fingery, 6, 252, 16, 22)) {
    selected = ST77XX_RED;
    picked = true;
  } else if (isInside(fingerx, fingery, 26, 252, 16, 22)) {
    selected = ST77XX_ORANGE;
    picked = true;
  } else if (isInside(fingerx, fingery, 46, 252, 16, 22)) {
    selected = ST77XX_YELLOW;
    picked = true;
  } else if (isInside(fingerx, fingery, 66, 252, 16, 22)) {
    selected = ST77XX_GREEN;
    picked = true;
  } else if (isInside(fingerx, fingery, 86, 252, 16, 22)) {
    selected = ST77XX_BLUE;
    picked = true;
  } else if (isInside(fingerx, fingery, 106, 252, 16, 22)) {
    selected = ST77XX_MAGENTA;
    picked = true;
  } else if (isInside(fingerx, fingery, 126, 252, 16, 22)) {
    selected = ST77XX_BLACK;
    picked = true;
  } else if (isInside(fingerx, fingery, 150, 252, 28, 22)) {
    if (!wasDown) {
      clearCurrentDrawing();
      Serial.println("Screen cleared!");
    }
    picked = true;
  } else if (isInside(fingerx, fingery, 4, 2, 14, 18)) {
    brushSize = 1;
    picked = true;
  } else if (isInside(fingerx, fingery, 20, 2, 14, 18)) {
    brushSize = 3;
    picked = true;
  } else if (isInside(fingerx, fingery, 36, 2, 14, 18)) {
    brushSize = 5;
    picked = true;
  }

  if (picked) {
    if (!wasDown) {
      Serial.println("Button pressed!");
    }
  } else if (fingery > TOP_H && fingery < BOTTOM_Y) {
    paintCircle(fingerx, fingery, brushSize, selected);
  }

  wasDown = true;
  delay(20);
}
