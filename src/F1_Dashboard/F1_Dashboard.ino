#include <LiquidCrystal_I2C.h>

// HALL Sensor
#define HALL_PIN 2

// Buzzer
#define BUZZER_PIN 9

// Buttons
#define START_STOP_BUTTON 6
#define INFO_BUTTON 7

// Function declarations
void hallSensorISR();
void handleButtons();
void displayWelcomeMessage();
void displayInfoMode();
void startRace();
void stopRace();
void calculateSpeed();
void updateRaceDisplay();
void checkSpeedLimit();
void checkRaceCompletion();
void playWelcomeTone();
void playStartTone();
void playCountDownTone();
void playSpeedLimitTone();
void playFinishTone();

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

const float wheelCircumference = 0.065; // meters - whee; circumference for speed calculation
const int speedLimit = 2; // km/h - maximum allowed speed before warning
const int totalLaps = 5;  // number of laps to complete the race
const int speedBoost = 10;  // multiplier to amplify speed reading for better visibility
const unsigned long minLapTime = 1000; // Minimum time between laps in milliseconds (1 second)

volatile unsigned long lastPulseTime = 0;
volatile unsigned long timeBetweenLaps = 0;

unsigned long raceStartTime = 0;
unsigned long currentTime = 0;
float currentSpeed = 0;

float lapSpeedSum = 0;
int lapsCompleted = 0;

bool raceActive = false;
bool speedLimitExceeded = false;
bool infoModeActive = false;
volatile bool newLapDetected = false;

bool lastInfoButtonState = HIGH;
bool lastStartButtonState = HIGH;
unsigned long lastInfoButtonPress = 0;
unsigned long lastStartButtonPress = 0;
const unsigned long debounceDelay = 200; // Debounce delay in milliseconds

void setup() {
  Serial.begin(9600); // Initialize serial communication

  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight

  pinMode(HALL_PIN, INPUT_PULLUP); // Set hall sensor pin as input with pull-up resistor
  pinMode(BUZZER_PIN, OUTPUT); // Set buzzer pin as output
  pinMode(START_STOP_BUTTON, INPUT); // Set start/stop button pin as input with pull-up extern resistor
  pinMode(INFO_BUTTON, INPUT); // Set info button pin as input with pull-up extern resistor

  lastInfoButtonState = digitalRead(INFO_BUTTON);
  lastStartButtonState = digitalRead(START_STOP_BUTTON);

  attachInterrupt(digitalPinToInterrupt(HALL_PIN), hallSensorISR, FALLING);
  displayWelcomeMessage();

  Serial.println("F1 Speed Monitor Initialized");
}

void loop() {
  currentTime = millis(); // Get the current time in milliseconds

  // Handle button presses
  handleButtons();

  if (raceActive) {
    calculateSpeed();
    updateRaceDisplay();
    checkSpeedLimit();
    checkRaceCompletion(); // Check if race should end due to lap completion
  }

  delay(100); // Delay to avoid overwhelming the loop
}


void hallSensorISR() {
  unsigned long now = millis();
  
  // Only register a new lap if enough time has passed since the last one
  if (lastPulseTime == 0 || (now - lastPulseTime) >= minLapTime) {
    if (lastPulseTime > 0) {
      timeBetweenLaps = now - lastPulseTime;
      newLapDetected = true;
    }
    lastPulseTime = now;
  }
}

// Calculate speed based on lap time and update counters
void calculateSpeed() {
  if (newLapDetected && timeBetweenLaps > 0) {
    // Convert milliseconds to seconds
    float lapTimeSec = timeBetweenLaps / 1000.0;
    // Speed = distance/time, converted to km/h, with boost multiplier
    float lapSpeed = (wheelCircumference / lapTimeSec) * speedBoost * 3.6;
    currentSpeed = lapSpeed;

    // Update running totals for average calculation
    lapSpeedSum += lapSpeed;
    lapsCompleted++;

    Serial.print("Lap ");
    Serial.print(lapsCompleted);
    Serial.print(" Speed: ");
    Serial.print(lapSpeed, 2);
    Serial.println(" km/h");

    newLapDetected = false;
  }
}

void handleButtons() {
  // Read the current state of the buttons
  bool currentInfoButtonState = digitalRead(INFO_BUTTON);
  bool currentStartButtonState = digitalRead(START_STOP_BUTTON);

  // Handle INFO button press
  if (currentInfoButtonState == HIGH && lastInfoButtonState == LOW &&
      (currentTime - lastInfoButtonPress) > debounceDelay) {
    if (!raceActive) {
      displayInfoMode();
      lastInfoButtonPress = currentTime;
    }
  }

  // Handle START/STOP button press
  if (currentStartButtonState == HIGH && lastStartButtonState == LOW &&
      (currentTime - lastStartButtonPress) > debounceDelay) {
    if (!raceActive) {
      startRace();
    } else {
      stopRace();
    } 
    lastStartButtonPress = currentTime;
  }

  // Update button states
  lastInfoButtonState = currentInfoButtonState; 
  lastStartButtonState = currentStartButtonState;
}

void displayWelcomeMessage() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("WELCOME TO");
  lcd.setCursor(1, 1);
  lcd.print("MONACO GP 2025");
  playWelcomeTone();
}

void displayInfoMode() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MAX SPEED:");
  lcd.print(speedLimit);
  lcd.print("KM/H");
  lcd.setCursor(0, 1);
  lcd.print("LAPS: ");
  lcd.print(totalLaps);

  // After 3 seconds, return to welcome display
  delay(3000);
  displayWelcomeMessage();
}

void startRace() {
  playStartTone();

  // Countdown until race
  for (int i = 3; i > 0; i--) {
    lcd.clear();
    lcd.setCursor(7, 0);
    lcd.print(i);
    delay(250);
    lcd.print(".");
    delay(250);
    lcd.print(".");

    playCountDownTone();
    delay(1000);
  }

  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("RACE!");
  delay(1000);

  raceStartTime = millis();
  lastPulseTime = 0;
  timeBetweenLaps = 0;
  newLapDetected = false;
  currentSpeed = 0;
  lapSpeedSum = 0;
  lapsCompleted = 0;
  raceActive = true;
  speedLimitExceeded = false;

  Serial.println("Race Started!");
}

void stopRace() {
  raceActive = false;
  float averageSpeed = (lapsCompleted > 0) ? (lapSpeedSum / lapsCompleted) : 0;

  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Race Finished!");
  lcd.setCursor(3, 1);
  lcd.print("CONGRATS!");

  playFinishTone();
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Laps: ");
  lcd.print(lapsCompleted);
  lcd.setCursor(0, 1);
  lcd.print("Avg: ");
  lcd.print(averageSpeed, 2);
  lcd.print(" km/h");

  delay(2000);
  Serial.println("Race Finished!");
  delay(1000);
  displayWelcomeMessage();
}

void updateRaceDisplay() {
  if (!speedLimitExceeded) {
    unsigned long elapsedTime = (currentTime - raceStartTime) / 1000; // Convert to seconds
    int minutes = elapsedTime / 60;
    int seconds = elapsedTime % 60;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("L:");
    lcd.print(lapsCompleted);
    lcd.print("/");
    lcd.print(totalLaps);
    lcd.print(" ");
    lcd.print(currentSpeed, 1);
    lcd.print("KM/H");
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    if (minutes < 10) {
      lcd.print("0");
    }
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) {
      lcd.print("0");
    }
    lcd.print(seconds);
  }
}

void checkSpeedLimit() {
  if (currentSpeed > speedLimit && !speedLimitExceeded) {
    speedLimitExceeded = true;
  
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("SLOW DOWN!");

    float excessSpeed = currentSpeed - speedLimit;
    lcd.setCursor(0, 1);
    lcd.print("Reduce ");
    lcd.print(excessSpeed, 1);
    lcd.print(" km/h");

    playSpeedLimitTone();

    Serial.print("Reduce with: ");
    Serial.print(excessSpeed, 1);
    Serial.println(" km/h");
  } else if (currentSpeed <= speedLimit && speedLimitExceeded) {
    // Speed limit is no longer exceeded
    speedLimitExceeded = false;
    lcd.clear(); // Clear warning message
    Serial.println("Speed limit back to normal.");
  }
}

void checkRaceCompletion() {
  if (lapsCompleted >= totalLaps) {
    Serial.print("Race completed! ");
    Serial.print(totalLaps);
    Serial.println(" laps finished.");
    stopRace();
  }
}

// Welcome sound
void playWelcomeTone() {
  tone(BUZZER_PIN, 523, 200); // C5
  delay(250);
  tone(BUZZER_PIN, 659, 200); // E5
  delay(250);
  tone(BUZZER_PIN, 784, 300); // G5
  delay(350);
}

// Race start sound
void playStartTone() {
  for (int freq = 200; freq <= 1000; freq += 100) {
    tone(BUZZER_PIN, freq, 100);
    delay(120);
  }
}

// Race countdown sound
void playCountDownTone() {
  tone(BUZZER_PIN, 700, 400);
}

// Speed limit exceeded sound
void playSpeedLimitTone() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1200, 200);
    delay(250);
    tone(BUZZER_PIN, 500, 200);
    delay(250);
  }
}

// Race finish sound
void playFinishTone() {
  int melody[] = {
    784, 784, 880, 784, 988, 1047,  // Intro: rising
    1047, 988, 880, 784, 880, 988   // Outro: descending bounce
  };
  int durations[] = {
    150, 150, 150, 150, 250, 300,   // Intro rhythm
    200, 150, 150, 150, 200, 300    // Outro rhythm
  };

  for (int i = 0; i < 12; i++) {
    tone(BUZZER_PIN, melody[i], durations[i]);
    delay(durations[i] + 50);
  }
}
