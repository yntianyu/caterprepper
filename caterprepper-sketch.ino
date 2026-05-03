#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// UNO PINS
const int motorPin = 9;
const int touchPin = 2;
const int reedPin = A3;
const int reedPin2 = A0;
const int reedPin3 = A1;
const int reedPin4 = A2;

const int redLED = 5;
const int greenLED = 6;

SoftwareSerial mySerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

bool dfReady = false;

enum State {
  IDLE,
  START_DELAY,
  ACTIVE_REED2,
  WAIT_REED3,
  ACTIVE_REED3,
  WAIT_REED4,
  ACTIVE_REED4,
  WAIT_REED1,
  ACTIVE_REED1,
  DONE_GREEN
};

State currentState = IDLE;

bool lastTouch = false;
unsigned long startTime = 0;

bool touched() {
  return digitalRead(touchPin) == HIGH;
}

bool newTouchPress() {
  bool now = touched();

  if (now && !lastTouch) {
    delay(40);
    lastTouch = now;
    return true;
  }

  lastTouch = now;
  return false;
}

// Any magnet on any reed = true
bool anyItemPresent() {
  return digitalRead(reedPin)  == LOW ||
         digitalRead(reedPin2) == LOW ||
         digitalRead(reedPin3) == LOW ||
         digitalRead(reedPin4) == LOW;
}

// All magnets removed from all reeds = true
bool noItemsPresent() {
  return digitalRead(reedPin)  == HIGH &&
         digitalRead(reedPin2) == HIGH &&
         digitalRead(reedPin3) == HIGH &&
         digitalRead(reedPin4) == HIGH;
}

void stopSound() {
  if (dfReady) {
    myDFPlayer.stop();
  }
}

void playTrack(int track) {
  if (dfReady) {
    myDFPlayer.playMp3Folder(track);
  }
}

void allOff() {
  digitalWrite(motorPin, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);
  stopSound();
}

void safeGreen() {
  digitalWrite(motorPin, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, HIGH);
}

void goGreen() {
  digitalWrite(motorPin, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, HIGH);
  stopSound();
  playTrack(6);
  currentState = DONE_GREEN;
}

void setup() {
  pinMode(motorPin, OUTPUT);
  pinMode(touchPin, INPUT);
  pinMode(reedPin, INPUT_PULLUP);
  pinMode(reedPin2, INPUT_PULLUP);
  pinMode(reedPin3, INPUT_PULLUP);
  pinMode(reedPin4, INPUT_PULLUP);

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  digitalWrite(motorPin, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);

  Serial.begin(9600);
  mySerial.begin(9600);

  Serial.println("Starting...");

  if (myDFPlayer.begin(mySerial)) {
    dfReady = true;
    myDFPlayer.volume(30);
    Serial.println("DFPlayer ready");
  } else {
    dfReady = false;
    Serial.println("DFPlayer NOT found, continuing without sound");
  }

  // quick startup blink so you know code is running
  digitalWrite(greenLED, HIGH);
  delay(300);
  digitalWrite(greenLED, LOW);
}

void loop() {
  Serial.print("Reed1: "); Serial.print(digitalRead(reedPin));
  Serial.print(" Reed2: "); Serial.print(digitalRead(reedPin2));
  Serial.print(" Reed3: "); Serial.print(digitalRead(reedPin3));
  Serial.print(" Reed4: "); Serial.println(digitalRead(reedPin4));
  delay(200);

  switch (currentState) {

    case IDLE:
      digitalWrite(motorPin, LOW);
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, LOW);

      if (newTouchPress()) {
        if (anyItemPresent()) {
          playTrack(1);
          startTime = millis();
          currentState = START_DELAY;
        } else {
          goGreen();
        }
      }
      break;

    case START_DELAY: {
      unsigned long elapsed = millis() - startTime;

      if ((elapsed > 100 && elapsed < 300) ||
          (elapsed > 500 && elapsed < 700)) {
        digitalWrite(greenLED, HIGH);
      } else {
        digitalWrite(greenLED, LOW);
      }

      if (elapsed > 3000) {
        if (anyItemPresent()) {
          digitalWrite(greenLED, LOW);
          digitalWrite(redLED, HIGH);
          digitalWrite(motorPin, HIGH);

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

    // ---- REED 2: playing track 2 while magnet is on ----
    case ACTIVE_REED2:
      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (dfReady && myDFPlayer.available()) {
        if (myDFPlayer.readType() == DFPlayerPlayFinished) {
          if (digitalRead(reedPin2) == LOW) {
            playTrack(2);
          }
        }
      }

      if (digitalRead(reedPin2) == HIGH) {
        stopSound();
        currentState = WAIT_REED3;
      }

      if (newTouchPress()) {
        goGreen();
      }
      break;

    // ---- WAIT FOR REED 3 CHECK ----
    case WAIT_REED3:
      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (noItemsPresent()) {
        goGreen();
      } else if (digitalRead(reedPin3) == LOW) {
        playTrack(3);
        currentState = ACTIVE_REED3;
      }

      if (newTouchPress()) {
        goGreen();
      }
      break;

    // ---- REED 3: playing track 3 while magnet is on ----
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

      if (newTouchPress()) {
        goGreen();
      }
      break;

    // ---- WAIT FOR REED 4 CHECK ----
    case WAIT_REED4:
      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (noItemsPresent()) {
        goGreen();
      } else if (digitalRead(reedPin4) == LOW) {
        playTrack(4);
        currentState = ACTIVE_REED4;
      }

      if (newTouchPress()) {
        goGreen();
      }
      break;

    // ---- REED 4: playing track 4 while magnet is on ----
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

      if (newTouchPress()) {
        goGreen();
      }
      break;

    // ---- WAIT FOR REED 1 CHECK ----
    case WAIT_REED1:
      digitalWrite(redLED, HIGH);
      digitalWrite(motorPin, HIGH);

      if (noItemsPresent()) {
        goGreen();
      } else if (digitalRead(reedPin) == LOW) {
        playTrack(5);
        currentState = ACTIVE_REED1;
      }

      if (newTouchPress()) {
        goGreen();
      }
      break;

    // ---- REED 1: playing track 5 while magnet is on ----
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

      if (newTouchPress()) {
        goGreen();
      }
      break;

    // ---- DONE ----
    case DONE_GREEN:
      safeGreen();

      if (newTouchPress()) {
        allOff();
        currentState = IDLE;
      }
      break;
  }
}