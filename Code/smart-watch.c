// libraries to add - RTClib
//                    Adafruit GFX Library
//                    Adafruit SSD1306

#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RTC_DS3231 rtc;

const int scrollButtonPin = 10;  // Button for scrolling through menu or pausing the game
const int selectButtonPin = 11;  // Button for selecting a menu item or jumping in the game
const int backButtonPin = 12;    // Button for going back to the menu

const char* menuItems[] = {"Show Time", "Show Date", "Stopwatch", "Dino Game"};
const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
int currentMenuIndex = 0;
bool inMenu = true;
bool stopwatchRunning = false;
unsigned long stopwatchStartTime = 0;
unsigned long stopwatchElapsedTime = 0;

// Dino game variables
bool dinoJumping = false;
int dinoY = 48;
int dinoVelocityY = 0;
int gravity = 2;
int jumpStrength = -12;
int groundLevel = 48;
int obstacleX = SCREEN_WIDTH;
int score = 0;
int obstacleSpeed = 4;
bool gamePaused = false;

void setup() {
  pinMode(scrollButtonPin, INPUT_PULLUP);
  pinMode(selectButtonPin, INPUT_PULLUP);
  pinMode(backButtonPin, INPUT_PULLUP);

  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // Comment out below line once the time is set
    rtc.adjust(DateTime(F(_DATE), F(TIME_)));
  }

  displayMenu();
}

void loop() {
  if (inMenu) {
    if (digitalRead(scrollButtonPin) == LOW) {
      // Scroll through menu
      currentMenuIndex = (currentMenuIndex + 1) % menuLength;
      displayMenu();
      delay(200);
    }

    if (digitalRead(selectButtonPin) == LOW) {
      // Select current menu item
      selectMenuItem(currentMenuIndex);
      delay(200);
    }
  } else {
    if (digitalRead(backButtonPin) == LOW) {
      // Go back to the menu
      inMenu = true;
      displayMenu();
      delay(200);
    }

    if (currentMenuIndex == 2 && stopwatchRunning) {
      // If stopwatch is running, update display
      displayStopwatch();
    }

    if (currentMenuIndex == 3) {
      // Dino game logic
      if (digitalRead(scrollButtonPin) == LOW) {
        // Pause or resume the game
        gamePaused = !gamePaused;
        delay(200);
      }

      if (!gamePaused) {
        updateDinoGame();
        delay(50);
      }
    }
  }
}

void displayMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  for (int i = 0; i < menuLength; i++) {
    if (i == currentMenuIndex) {
      display.print("> ");
    } else {
      display.print("  ");
    }
    display.println(menuItems[i]);
  }

  display.display();
}

void selectMenuItem(int index) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  inMenu = false;

  if (strcmp(menuItems[index], "Show Time") == 0) {
    DateTime now = rtc.now();
    display.setCursor(0, 20);
    display.print("Time: ");
    display.setCursor(0, 40);
    display.print(now.hour(), DEC);
    display.print(':');
    display.print(now.minute(), DEC);
    display.print(':');
    display.print(now.second(), DEC);
  } else if (strcmp(menuItems[index], "Show Date") == 0) {
    DateTime now = rtc.now();
    display.setCursor(0, 20);
    display.print("Date: ");
    display.setCursor(0, 40);
    display.print(now.day(), DEC);
    display.print('/');
    display.print(now.month(), DEC);
    display.print('/');
    display.print(now.year(), DEC);
  } else if (strcmp(menuItems[index], "Stopwatch") == 0) {
    displayStopwatchMenu();
    return;  // Do not clear display or set inMenu to true here
  } else if (strcmp(menuItems[index], "Dino Game") == 0) {
    startDinoGame();
    return;  // Do not clear display or set inMenu to true here
  }

  display.display();
}

void displayStopwatchMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (!stopwatchRunning) {
    display.println("Press Select to Start");
  } else {
    display.println("Press Select to Stop");
  }
  display.println("Press Scroll to Reset");
  display.println("Press Back to Menu");

  display.display();

  while (true) {
    if (digitalRead(selectButtonPin) == LOW) {
      // Start/Stop the stopwatch
      if (stopwatchRunning) {
        stopwatchElapsedTime += millis() - stopwatchStartTime;
        stopwatchRunning = false;
      } else {
        stopwatchStartTime = millis();
        stopwatchRunning = true;
      }
      delay(200);
    }

    if (digitalRead(scrollButtonPin) == LOW) {
      // Reset the stopwatch
      stopwatchRunning = false;
      stopwatchElapsedTime = 0;
      displayStopwatch();  // Update the display immediately after reset
      delay(200);
    }

    if (digitalRead(backButtonPin) == LOW) {
      // Go back to menu
      inMenu = true;
      displayMenu();
      delay(200);
      return;
    }

    if (stopwatchRunning) {
      displayStopwatch();
    }
  }
}

void displayStopwatch() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = stopwatchElapsedTime;

  if (stopwatchRunning) {
    elapsedTime += currentTime - stopwatchStartTime;
  }

  unsigned long hours = elapsedTime / 3600000;
  unsigned long minutes = (elapsedTime % 3600000) / 60000;
  unsigned long seconds = (elapsedTime % 60000) / 1000;
  unsigned long milliseconds = elapsedTime % 1000;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((SCREEN_WIDTH - 12 * 6) / 2, (SCREEN_HEIGHT - 16) / 2); // Center the text
  display.print(hours);
  display.print(":");
  display.print(minutes);
  display.print(":");
  display.print(seconds);
  display.print(".");
  display.print(milliseconds);

  display.display();
}

void startDinoGame() {
  dinoY = groundLevel;
  dinoJumping = false;
  obstacleX = SCREEN_WIDTH;
  score = 0;
  gamePaused = false;
  displayDinoGame();
}

void updateDinoGame() {
  if (digitalRead(selectButtonPin) == LOW) {
    // Dino jumps
    if (!dinoJumping && dinoY == groundLevel) {
      dinoJumping = true;
      dinoVelocityY = jumpStrength;
    }
  }

  // Update dino position
  if (dinoJumping) {
    dinoY += dinoVelocityY;
    dinoVelocityY += gravity;

    if (dinoY >= groundLevel) {
      dinoY = groundLevel;
      dinoJumping = false;
      dinoVelocityY = 0;
    }
  }

  // Move the obstacle
  obstacleX -= obstacleSpeed;
  if (obstacleX < 0) {
    obstacleX = SCREEN_WIDTH;
    score++;
  }

  // Check for collision
  if (obstacleX < 18 && obstacleX > 2 && dinoY >= groundLevel - 16) {
    // Collision detected
    inMenu = true;
    displayGameOver();
    delay(2000);
    displayMenu();
    return;
  }

  displayDinoGame();
}

void displayDinoGame() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);

  // Draw dino
  display.fillRect(10, dinoY, 8, 16, SSD1306_WHITE); // Dino is running/jumping

  // Draw obstacle
  display.fillRect(obstacleX, groundLevel - 8, 8, 8, SSD1306_WHITE);

  // Draw ground
  display.drawLine(0, groundLevel + 8, SCREEN_WIDTH, groundLevel + 8, SSD1306_WHITE);

  display.display();
}

void displayGameOver() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.print("Game Over");
  display.setCursor(10, 40);
  display.print("Score: ");
  display.print(score);
  display.display();
}