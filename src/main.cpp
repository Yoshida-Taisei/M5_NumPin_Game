#include <M5Core2.h>
#include <vector>
#include <algorithm> // std::shuffle
#include <random>    // std::mt19937, std::random_device

#define LGFX_AUTODETECT
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

static LGFX lcd;
static LGFX_Sprite sprite(&lcd); 

const int screenWidth = 320;
const int screenHeight = 240;
const int radius = 36;
const int textSize = 4;
const int textHeight = 18;

const int gridWidth = 3;
const int gridHeight = 3;
const int cellSize = 80;

int numbers[gridWidth][gridHeight];

int gridTotalWidth = cellSize * gridWidth;
int gridTotalHeight = cellSize * gridHeight;

int startX = (screenWidth - gridTotalWidth);
int startY = cellSize / 2;

int lastRemoved = 0;
unsigned long startTime;
unsigned long finalTime = 0;

void displayRandomNumbers()
{
  sprite.fillScreen(TFT_BLACK);

  // 1～20の数字を用意
  std::vector<int> candidates;
  candidates.reserve(20);
  for (int n = 1; n <= 20; n++)
  {
    candidates.push_back(n);
  }

  uint32_t seedVal = esp_random();
  std::mt19937 g(seedVal);
  std::shuffle(candidates.begin(), candidates.end(), g);

  // 先頭9個を 3x3 に配置
  int idx = 0;
  for (int i = 0; i < gridWidth; i++)
  {
    for (int j = 0; j < gridHeight; j++)
    {
      numbers[i][j] = candidates[idx];
      idx++;

      int x = startX + i * cellSize;
      int y = startY + j * cellSize;

      sprite.drawCircle(x, y, radius, TFT_WHITE);

      String numStr = String(numbers[i][j]);

      sprite.setTextSize(textSize);
      sprite.setTextColor(TFT_WHITE);
      sprite.setFont(&fonts::efontJA_10);

      int16_t txtWidth = sprite.textWidth(numStr);
      int16_t txtHeight = sprite.fontHeight();

      int txtX = x - (txtWidth / 2);
      int txtY = y - (txtHeight / 2);

      sprite.setCursor(txtX, txtY);
      sprite.print(numStr);
    }
  }

  sprite.pushSprite(0, 0);
}

// -----------------------------
// 枠線を描画 (赤/青枠)
// -----------------------------
void drawBorder(int color)
{

  sprite.fillRect(0, 0, screenWidth, 10, color);                 // 上端
  sprite.fillRect(0, 0, 10, screenHeight, color);                // 左端
  sprite.fillRect(screenWidth - 10, 0, 10, screenHeight, color); // 右端
  sprite.fillRect(0, screenHeight - 10, screenWidth, 10, color); // 下端

  sprite.pushSprite(0, 0);
}

// -----------------------------
// 最小の数字を調べる
// -----------------------------
int findSmallestNumber()
{
  int smallestNumber = 21;
  for (int i = 0; i < gridWidth; i++)
  {
    for (int j = 0; j < gridHeight; j++)
    {
      if (numbers[i][j] != 0 && numbers[i][j] < smallestNumber)
      {
        smallestNumber = numbers[i][j];
      }
    }
  }
  return smallestNumber;
}

// -----------------------------
// タッチされた数字を消す
// -----------------------------
void removeNumberAtTouch()
{
  int smallestNumber = findSmallestNumber();

  for (int i = 0; i < gridWidth; i++)
  {
    for (int j = 0; j < gridHeight; j++)
    {
      int x = startX + (i * cellSize);
      int y = startY + (j * cellSize);

      TouchPoint_t tp = M5.Touch.getPressPoint();
      if (abs(tp.x - x) <= radius && abs(tp.y - y) <= radius)
      {
        int touchedNumber = numbers[i][j];
        if (touchedNumber == smallestNumber)
        {
          numbers[i][j] = 0;
          sprite.fillCircle(x, y, radius, TFT_BLACK);

          lastRemoved = touchedNumber;
          drawBorder(TFT_BLUE);
        }
        else
        {
          drawBorder(TFT_RED);
        }
      }
    }
  }
}

// -----------------------------
// ゲームクリア画面を表示 (スプライト)
// -----------------------------
void displayClearTime()
{
  if (finalTime == 0)
  {
    finalTime = millis() - startTime;
  }

  float seconds = finalTime / 1000.0f;

  sprite.fillScreen(TFT_BLACK);

  sprite.setCursor(80, 60);
  sprite.setTextSize(3);
  sprite.setTextColor(TFT_YELLOW);
  sprite.print("Excellent!!");

  sprite.setCursor(80, 100);
  sprite.setTextSize(3);
  sprite.setTextColor(TFT_WHITE);
  sprite.printf("Time: %.1f s", seconds);

  // リトライボタン描画 --------------------------
  int rectX = 80;
  int rectY = 155;
  int rectW = 160;
  int rectH = 40;
  sprite.drawRect(rectX, rectY, rectW, rectH, TFT_GREEN);

  sprite.setTextSize(3);
  sprite.setTextColor(TFT_GREEN);
  String label = "RETRY";
  int16_t labelWidth = sprite.textWidth(label);
  int16_t labelHeight = sprite.fontHeight();
  int labelX = rectX + (rectW - labelWidth) / 2;
  int labelY = rectY + (rectH - labelHeight) / 2;
  sprite.setCursor(labelX, labelY);
  sprite.print(label);

  sprite.pushSprite(0, 0);

  // リセット処理 ------------------------------
  TouchPoint_t tp = M5.Touch.getPressPoint();
  if (tp.x >= rectX && tp.x <= rectX + rectW &&
      tp.y >= rectY && tp.y <= rectY + rectH)
  {
    finalTime = 0;
    lastRemoved = 0;
    displayRandomNumbers();
    startTime = millis();
  }
}

// -----------------------------
// ゲームの進行状況を監視
// -----------------------------
void checkGameStatus()
{
  bool gameCleared = true;

  for (int i = 0; i < gridWidth; i++)
  {
    for (int j = 0; j < gridHeight; j++)
    {
      if (numbers[i][j] != 0)
      {
        gameCleared = false;
        break;
      }
    }
  }

  if (gameCleared)
  {
    displayClearTime();
  }
}

// -----------------------------
// 初期化
// -----------------------------
void setup()
{
  M5.begin();
  lcd.init();
  randomSeed(micros());

  sprite.setColorDepth(8);
  sprite.createSprite(lcd.width(), lcd.height());

  sprite.fillScreen(TFT_BLACK);
  sprite.pushSprite(0, 0);

  startTime = millis();

  // 数字の初期描画
  displayRandomNumbers();
}

// -----------------------------
// ループ
// -----------------------------
void loop()
{
  removeNumberAtTouch();
  checkGameStatus();
}
