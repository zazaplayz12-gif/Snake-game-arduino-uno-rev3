#include "LedControl.h"

// --- PIN CONFIGURATIONS ---
// MAX7219 Dot Matrix Pins
// pin 12 -> DataIn, pin 11 -> LOAD(CS), pin 10 -> CLK
LedControl lc = LedControl(12, 10, 11, 1);

// Joystick Pins (Matched exactly to your Elegoo Joystick script)
const int SW_pin = 2; // Digital pin connected to switch output
const int X_pin = 0;  // Analog pin A0 connected to X output
const int Y_pin = 1;  // Analog pin A1 connected to Y output

// --- GAME CONFIGURATIONS ---
const int MATRIX_SIZE = 8;
const int JOYSTICK_THRESHOLD_LOW = 300;
const int JOYSTICK_THRESHOLD_HIGH = 700;

struct Point {
  int x;
  int y;
};

// Snake State
Point snake[64];
int snakeLength = 1;   // Modified: Changed default starting length to 1
int currentDir = 1;    // 0=Up, 1=Right, 2=Down, 3=Left

// Food State
Point food;

// Engine Timing & Mechanics
unsigned long lastUpdateTime = 0;
unsigned long gameSpeed = 250; // Game tick speed in milliseconds
bool gameOver = false;
bool isPaused = false;

// Button Debounce Variables
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50; 

void setup() {
  // Initialize Joystick Switch (Using internal pull-up as per your code)
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);

  // Initialize Dot Matrix Display
  lc.shutdown(0, false);       
  lc.setIntensity(0, 4);       
  lc.clearDisplay(0);          

  randomSeed(analogRead(5)); // Seed with an unused analog pin (A5)
  resetGame();
}

void loop() {
  // 1. Constantly watch for Pause button clicks and directional shifts
  handlePauseButton();
  
  if (isPaused) {
    return; 
  }

  if (gameOver) {
    showGameOverAnimation();
    resetGame();
    return;
  }

  readJoystick();

  // 2. Timed Game Tick Execution
  if (millis() - lastUpdateTime >= gameSpeed) {
    moveSnake();
    checkCollision();
    
    if (!gameOver) {
      drawGame();
    }
    lastUpdateTime = millis();
  }
}

// Resets game variables back to a default starting state
void resetGame() {
  snakeLength = 1; // Modified: Ensures game always restarts at length 1
  currentDir = 1;  // Start by sliding right
  gameOver = false;
  isPaused = false;
  
  // Modified: Only assign the very first dot (the head) at the start
  snake[0].x = 3;
  snake[0].y = 4;
  
  spawnFood();
  drawGame();
}

// Checks if the joystick center button has been clicked to toggle pause
void handlePauseButton() {
  int reading = digitalRead(SW_pin);

  // Simple hardware debounce check
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading == LOW && lastButtonState == HIGH) {
      isPaused = !isPaused;
      if (isPaused) {
        showPauseScreen();
      } else {
        drawGame(); 
      }
    }
  }
  lastButtonState = reading;
}

// Standard direction check (matching your explicit X/Y analog targets)
void readJoystick() {
  int xVal = analogRead(X_pin);
  int yVal = analogRead(Y_pin);

  // X-Axis Mapping (Left/Right)
  if (xVal < JOYSTICK_THRESHOLD_LOW && currentDir != 1) {
    currentDir = 3; // Turn Left
  } else if (xVal > JOYSTICK_THRESHOLD_HIGH && currentDir != 3) {
    currentDir = 1; // Turn Right
  }
  
  // Y-Axis Mapping (Up/Down)
  if (yVal < JOYSTICK_THRESHOLD_LOW && currentDir != 2) {
    currentDir = 0; // Turn Up
  } else if (yVal > JOYSTICK_THRESHOLD_HIGH && currentDir != 0) {
    currentDir = 2; // Turn Down
  }
}

void moveSnake() {
  for (int i = snakeLength - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }

  switch (currentDir) {
    case 0: snake[0].y--; break; 
    case 1: snake[0].x++; break; 
    case 2: snake[0].y++; break; 
    case 3: snake[0].x--; break; 
  }
}

void checkCollision() {
  // Out-of-bounds crosscheck
  if (snake[0].x < 0 || snake[0].x >= MATRIX_SIZE || snake[0].y < 0 || snake[0].y >= MATRIX_SIZE) {
    gameOver = true;
    return;
  }

  // Self-biting crosscheck
  for (int i = 1; i < snakeLength; i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      gameOver = true;
      return;
    }
  }

  // Safe ingestion crosscheck
  if (snake[0].x == food.x && snake[0].y == food.y) {
    if (snakeLength < 64) {
      snakeLength++;
    }
    spawnFood();
  }
}

void spawnFood() {
  bool validLocation = false;
  while (!validLocation) {
    food.x = random(0, MATRIX_SIZE);
    food.y = random(0, MATRIX_SIZE);
    
    validLocation = true;
    for (int i = 0; i < snakeLength; i++) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        validLocation = false;
        break;
      }
    }
  }
}

void drawGame() {
  lc.clearDisplay(0);
  lc.setLed(0, food.y, food.x, true); // Render Food point
  for (int i = 0; i < snakeLength; i++) {
    lc.setLed(0, snake[i].y, snake[i].x, true); // Render entire Snake chain
  }
}

// Displays a simple double-bar "||" Pause graphic symbol
void showPauseScreen() {
  lc.clearDisplay(0);
  lc.setRow(0, 2, B01100110);
  lc.setRow(0, 3, B01100110);
  lc.setRow(0, 4, B01100110);
  lc.setRow(0, 5, B01100110);
}

void showGameOverAnimation() {
  for (int blink = 0; blink < 3; blink++) {
    for (int r = 0; r < MATRIX_SIZE; r++) {
      lc.setRow(0, r, B11111111);
    }
    delay(150);
    lc.clearDisplay(0);
    delay(150);
  }
}
