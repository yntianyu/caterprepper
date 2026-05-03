#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ----------------------
// UNO PINS
// ----------------------
const int motorPin = 9;     // vibration motor / output device
const int touchPin = 2;     // touch sensor input

// Reed switch inputs (LOW = magnet present, HIGH = no magnet)
const int reedPin  = A3;    // Reed 1
const int reedPin2 = A0;    // Reed 2
const int reedPin3 = A1;    // Reed 3
const int reedPin4 = A2;    // Reed 4

// LED outputs
const int redLED = 5;
const int greenLED = 6;

// Software serial for DFPlayer Mini
SoftwareSerial mySerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// Tracks whether DFPlayer initialized correctly
bool dfReady = false;

// ----------------------
// State machine states
// ----------------------
enum State {
  IDLE,           // waiting for touch press
  START_DELAY,    // startup blinking + delay
  ACTIVE_REED2,   // track 2 repeating while item on reed 2
  WAIT_REED3,     // waiting for item on reed 3
  ACTIVE_REED3,
  WAIT_REED4,
  ACTIVE_REED4,
  WAIT_REED1,
  ACTIVE_REED1,
  DONE_GREEN      // success state
};

State currentState = IDLE;

// Used for touch press edge detection
bool lastTouch = false;

// Used for timing delays
unsigned long startTime = 0;

// ----------------------
// Returns true if touch sensor is currently pressed
// ----------------------
bool touched() {
  return digitalRead(touchPin) == HIGH;
}

// ----------------------
// Detects a NEW touch press only once
// Prevents holding finger from retriggering
// ----------------------
bool newTouchPress() {
  bool now = touched();

  if (now && !lastTouch) {   // rising edge detected
    delay(40);               // debounce
    lastTouch = now;
    return true;
  }

  lastTouch = now;
  return false;
}

// ----------------------
// True if ANY magnet/item is on any reed switch
// ----------------------
bool anyItemPresent() {
  return digitalRead(reedPin)  == LOW ||
         digitalRead(reedPin2) == LOW ||
         digitalRead(reedPin3) == LOW ||
         digitalRead(reedPin4) == LOW;
}

// ----------------------
// True only if ALL magnets removed
// ----------------------
bool noItemsPresent() {
  return digitalRead(reedPin)  == HIGH &&
         digitalRead(reedPin2) == HIGH &&
         digitalRead(reedPin3) == HIGH &&
         digitalRead(reedPin4) == HIGH;
}

// ----------------------
// Stop currently playing audio
// ----------------------
void stopSound() {
  if (dfReady) {
    myDFPlayer.stop();
  }
}

// ----------------------
// Play track from /MP3 folder
// ----------------------
void playTrack(int track) {
  if (dfReady) {
    myDFPlayer.playMp3Folder(track);
  }
}

// ----------------------
// Turn everything OFF
// ----------------------
void allOff() {
  digitalWrite(motorPin, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);
  stopSound();
}

// ----------------------
// Safe green state (no sound)
// ----------------------
void safeGreen() {
  digitalWrite(motorPin, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, HIGH);
}

// ----------------------
// Final success state:
// green LED on + play success sound
// ----------------------
void goGreen() {
  digitalWrite(motorPin, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, HIGH);

  stopSound();
  playTrack(6);              // success sound

  currentState = DONE_GREEN;
}

// ----------------------
// Setup
// ----------------------
void setup() {

  // Configure outputs
  pinMode(motorPin, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  // Configure inputs
  pinMode(touchPin, INPUT);
  pinMode(reedPin, INPUT_PULLUP);
  pinMode(reedPin2, INPUT_PULLUP);
  pinMode(reedPin3, INPUT_PULLUP);
  pinMode(reedPin4, INPUT_PULLUP);

  // Start with outputs off
  digitalWrite(motorPin, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);

  // Serial monitor
  Serial.begin(9600);

  // DFPlayer serial
  mySerial.begin(9600);

  Serial.println("Starting...");

  // Try to initialize DFPlayer
  if (myDFPlayer.begin(mySerial)) {
    dfReady = true;
    myDFPlayer.volume(30);   // max volume
    Serial.println("DFPlayer ready");
  } else {
    dfReady = false;
    Serial.println("DFPlayer NOT found, continuing without sound");
  }

  // Startup blink to show code is running
  digitalWrite(greenLED, HIGH);
  delay(300);
  digitalWrite(greenLED, LOW);
}

// ----------------------
// Main loop
// ----------------------
void loop() {

  // Print reed switch states for debugging
  Serial.print("Reed1: "); Serial.print(digitalRead(reedPin));
  Serial.print(" Reed2: "); Serial.print(digitalRead(reedPin2));
  Serial.print(" Reed3: "); Serial.print(digitalRead(reedPin3));
  Serial.print(" Reed4: "); Serial.println(digitalRead(reedPin4));

  delay(200);

  switch (currentState) {

    // ----------------------
    // IDLE STATE
    // Waiting for touch press
    // ----------------------
    case IDLE:

      digitalWrite(motorPin, LOW);
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, LOW);

      if (newTouchPress()) {

        if (anyItemPresent()) {
          playTrack(1);             // start prompt sound
          startTime = millis();     // begin timer
          currentState = START_DELAY;
        } else {
          goGreen();                // already complete
        }
      }
      break;

    // ----------------------
    // Startup blinking state
    // ----------------------
    case START_DELAY: {

      unsigned long elapsed = millis() - startTime;

      // Blink green LED twice
      if ((elapsed > 100 && elapsed < 300) ||
          (elapsed > 500 && elapsed < 700)) {
        digitalWrite(greenLED, HIGH);
      } else {
        digitalWrite(greenLED, LOW);
      }

      // After 3 seconds continue
      if (elapsed > 3000) {

        if (anyItemPresent()) {

          digitalWrite(greenLED, LOW);
          digitalWrite(redLED, HIGH);
          digitalWrite(motorPin, HIGH);

          // If reed2 occupied, start there first
          if (digitalRead(reedPin2) == LOW) {
            playTrack(2);
            currentState = ACTIVE_REED2;
          } else {
            currentState = WAIT_REED3;
          }

        } else {
          goGreen();
        }
      }
      break;
    }

    // ----------------------
    // REED 2 ACTIVE
    // ----------------------
    case ACTIVE_REED2:

      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      // Replay track when finished
      if (dfReady && myDFPlayer.available()) {
        if (myDFPlayer.readType() == DFPlayerPlayFinished) {
          if (digitalRead(reedPin2) == LOW) {
            playTrack(2);
          }
        }
      }

      // If item removed, advance
      if (digitalRead(reedPin2) == HIGH) {
        stopSound();
        currentState = WAIT_REED3;
      }

      if (newTouchPress()) goGreen();

      break;

    // ----------------------
    // WAIT FOR REED 3
    // ----------------------
    case WAIT_REED3:

      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (noItemsPresent()) {
        goGreen();
      }
      else if (digitalRead(reedPin3) == LOW) {
        playTrack(3);
        currentState = ACTIVE_REED3;
      }

      if (newTouchPress()) goGreen();

      break;

    // ----------------------
    // REED 3 ACTIVE
    // ----------------------
    case ACTIVE_REED3:

      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (dfReady && myDFPlayer.available()) {
        if (myDFPlayer.readType() == DFPlayerPlayFinished) {
          if (digitalRead(reedPin3) == LOW) {
            playTrack(3);
          }
        }
      }

      if (digitalRead(reedPin3) == HIGH) {
        stopSound();
        currentState = WAIT_REED4;
      }

      if (newTouchPress()) goGreen();

      break;

    // ----------------------
    // WAIT FOR REED 4
    // ----------------------
    case WAIT_REED4:

      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (noItemsPresent()) {
        goGreen();
      }
      else if (digitalRead(reedPin4) == LOW) {
        playTrack(4);
        currentState = ACTIVE_REED4;
      }

      if (newTouchPress()) goGreen();

      break;

    // ----------------------
    // REED 4 ACTIVE
    // ----------------------
    case ACTIVE_REED4:

      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (dfReady && myDFPlayer.available()) {
        if (myDFPlayer.readType() == DFPlayerPlayFinished) {
          if (digitalRead(reedPin4) == LOW) {
            playTrack(4);
          }
        }
      }

      if (digitalRead(reedPin4) == HIGH) {
        stopSound();
        currentState = WAIT_REED1;
      }

      if (newTouchPress()) goGreen();

      break;

    // ----------------------
    // WAIT FOR REED 1
    // ----------------------
    case WAIT_REED1:

      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (noItemsPresent()) {
        goGreen();
      }
      else if (digitalRead(reedPin) == LOW) {
        playTrack(5);
        currentState = ACTIVE_REED1;
      }

      if (newTouchPress()) goGreen();

      break;

    // ----------------------
    // REED 1 ACTIVE
    // ----------------------
    case ACTIVE_REED1:

      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (dfReady && myDFPlayer.available()) {
        if (myDFPlayer.readType() == DFPlayerPlayFinished) {
          if (digitalRead(reedPin) == LOW) {
            playTrack(5);
          }
        }
      }

      if (digitalRead(reedPin) == HIGH) {
        stopSound();

        if (noItemsPresent()) {
          goGreen();
        }
      }

      if (newTouchPress()) goGreen();

      break;

    // ----------------------
    // SUCCESS STATE
    // Touch again resets to IDLE
    // ----------------------
    case DONE_GREEN:

      safeGreen();

      if (newTouchPress()) {
        allOff();
        currentState = IDLE;
      }

      break;
  }
}
