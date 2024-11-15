#include <Wire.h>
#include <RTC.h>
#include <MPU6050.h>
#include <Adafruit_TLC5947.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

/*
This Arduino code manages an exercise reminder and tracking system using tilt detection, LEDs, and sound. 
It tracks exercise completion based on tilt angle (detected via a gyroscope) and uses rotary encoders to adjust reminder intervals and exercise duration. 
The system provides LED feedback for progress and plays audio after each completed exercise, resetting daily.
*/


// --------------- Pin Setup  ---------------
// Represents number of LED boards used
#define NUM_TLC5947 1

// Defining pins for LEDs
#define data   4
#define clock   5
#define latch   6

// Encoder 1 Pins       all wires that arent below are going to ground for the encoder
const int clkPin = 3; // brown wire
const int dtPin = 2;  // silver wire
const int swPin = 7;  // black wire

// Encoder 2 Pins       all wires that arent below are going to ground for the encoder
const int clkPin2 = 12; // pink wire
const int dtPin2 = 11;  // blue wire
const int swPin2 = 10;  // black wire



// --------------- Configurable Variables  ---------------

// Interval controls how often it notifes them, it has 3 levels which are toggled by the rotary encoder
long level1 = 5000;
long level2 = 10000;
long level3 = 15000;
long interval = level1; // initialize it to level 1

// How long they need to hold the tilt to get a succesful completion of exercise,  it also has 3 levels controlled by the other rotary encoder
int duration1 = 3000;
int duration2 = 4000;
int duration3 = 5000;
int exerciseDuration = duration1; // initialze it to level 1

const long exerciseWindow = 8000; // The window of time they have to do the exercise
const unsigned long minTiltAngle = 15; // Minimum angle they need to tilt to in order to begin exercise
const int dayOfMonth = 14; // Configures day of the month for rtc (not super important but you should update when uploading to project)

// --------------- Gyroscope SETUP ---------------

/* For the gyroscope the wiring is as follows:
VCC: 5V
GND: GND
SCL: A5
SDA: A4
*/

MPU6050 mpu;


// --------------- Timing Varibles SETUP ---------------

// Timer variables
unsigned long previousMillis = 0; 
unsigned long exerciseStartTime = 0; 
unsigned long waitingStartTime = 0;

// Status and tracking variables
bool active = false; 
bool exerciseStarted = false;
bool waitingForExercise = false;
int dailyExerciseCount = 0;
int prevExerciseCount = -1;

// Purely for debugging purposes and result readability (youll notice all print statements print condtionally based on these variables)
unsigned long lastPrintTime = 0; 
const unsigned long printInterval = 500; // You can change this in order to increase or decrease the rate at which serial monitor prints out stuff to read


// --------------- Encoder SETUP ---------------

// Encoder Constants
int clkState, dtState;
int lastClkState;
int stepCounter = 0;
float currentAngle = 0.0;
const int stepsPerRevolution = 20;
float anglePerStep = 360.0 / stepsPerRevolution;

// Second encoder for exercise duration adjustment
int clkState2, dtState2;
int lastClkState2;
int stepCounter2 = 0;
float currentAngle2 = 0.0;

// Debounce timing variables
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 0.1;

// Button hold timing variables
unsigned long buttonPressStart1 = 0;
unsigned long buttonpressStart2 = 0;
const unsigned long holdTimeRequired = 3000;


// --------------- LED SETUP ---------------
Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, clock, data, latch);

// --------------- SPEAKER SETUP ---------------
// Define pins 8 and 9 to communicate with DFPlayer Mini
static const uint8_t PIN_MP3_TX = 8; // Connects to the module's RX
static const uint8_t PIN_MP3_RX = 9; // Connects to the module's TX
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX);

// Create the DF player object
DFRobotDFPlayerMini player; 

long numOfTracks = 0;
int lastTrackPlayed = 0;
long volumeLevel = 30;



// --------------- Begining of SETUP CODE ---------------
void setup() {
  Serial.begin(230400);
  softwareSerial.begin(9600); 
  delay(1000);
  Wire.begin();
  RTC.begin();
  tlc.begin();

  // Encoder setup for exercise interval
  pinMode(clkPin, INPUT_PULLUP);
  pinMode(dtPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  lastClkState = digitalRead(clkPin);

  // Encoder 2 setup for exercise duration
  pinMode(clkPin2, INPUT_PULLUP);
  pinMode(dtPin2, INPUT_PULLUP);
  pinMode(swPin2, INPUT_PULLUP);
  lastClkState2 = digitalRead(clkPin2);

  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
  }

  if (player.begin(softwareSerial)) {
    Serial.println("Serial connection with DFPlayer established.");
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }

  String testtime = __TIME__;
  int testHour = testtime.substring(0, 2).toInt();
  int testMinute = testtime.substring(3, 5).toInt();
  int testSecond = testtime.substring(6, 8).toInt();
  RTCTime startTime(dayOfMonth, Month::NOVEMBER, 2024, testHour, testMinute, testSecond, DayOfWeek::WEDNESDAY, SaveLight::SAVING_TIME_ACTIVE);
  RTC.setTime(startTime);
  randomSeed(millis());  // Generate a random seed to help randomly choose track from sd card
}


// Function to update interval based on angle
void updateIntervalFromAngle(float angle) {
    if (angle >= 0 && angle < 120) {
        interval = level1;
    } else if (angle >= 120 && angle < 240) {
        interval = level2;
    } else if (angle >= 240 && angle <= 360) {
        interval = level3;
    }
}

// Function to update exercise duration based on angle from second encoder
void updateExerciseDurationFromAngle(float angle) {
    if (angle >= 0 && angle < 120) {
        exerciseDuration = duration1;
    } else if (angle >= 120 && angle < 240) {
        exerciseDuration = duration2;
    } else if (angle >= 240 && angle <= 360) {
        exerciseDuration = duration3;
    }
}




// Updated Encoder 1 angle function with improved reset handling
void updateEncoderAngle() {
    unsigned long currentTime = millis();
    clkState = digitalRead(clkPin);

    // Check if encoder state changed with debounce
    if ((clkState != lastClkState) && (currentTime - lastDebounceTime > debounceDelay)) {
        lastDebounceTime = currentTime;
        dtState = digitalRead(dtPin);
        if (clkState == HIGH && dtState == LOW) {
            stepCounter++;
        } else if (clkState == HIGH && dtState == HIGH) {
            stepCounter--;
        }

        currentAngle = stepCounter * anglePerStep;
        currentAngle = fmod(currentAngle, 360.0);
        if (currentAngle < 0.0) {
            currentAngle += 360.0;
        }

        updateIntervalFromAngle(currentAngle);

        Serial.print("Steps: ");
        Serial.print(stepCounter);
        Serial.print(" | Current Angle: ");
        Serial.print(currentAngle);
        Serial.println("°");
    }

    // Button press reset handling with dedicated timing
    if (digitalRead(swPin) == LOW) {
        if (buttonPressStart1 == 0) {
            buttonPressStart1 = currentTime;  // Record start time if button is pressed
        }
        if (currentTime - buttonPressStart1 >= holdTimeRequired) {
            stepCounter = 0;
            currentAngle = 0.0;
            Serial.println("Angle reset to 0° after holding button for 3 seconds");
            buttonPressStart1 = 0;  // Reset the press start time after reset action
        }
    } else {
        buttonPressStart1 = 0;  // Reset timing if button is not held
    }

    lastClkState = clkState;
}

// Function to update angle and exercise duration based on second encoder
void updateEncoderAngle2() {
    unsigned long currentTime = millis();
    clkState2 = digitalRead(clkPin2);
    if ((clkState2 != lastClkState2) && (currentTime - lastDebounceTime > debounceDelay)) {
        lastDebounceTime = currentTime;
        dtState2 = digitalRead(dtPin2);
        if (clkState2 == HIGH && dtState2 == LOW) {
            stepCounter2++;
        } else if (clkState2 == HIGH && dtState2 == HIGH) {
            stepCounter2--;
        }

        currentAngle2 = stepCounter2 * anglePerStep;
        currentAngle2 = fmod(currentAngle2, 360.0);
        if (currentAngle2 < 0.0) {
            currentAngle2 += 360.0;
        }

        updateExerciseDurationFromAngle(currentAngle2);

        Serial.print("Encoder 2 - Current Angle: ");
        Serial.print(currentAngle2);
        Serial.println("°");
    }

    if (digitalRead(swPin2) == LOW) {
        if (buttonpressStart2 == 0) {
            buttonpressStart2 = currentTime;
        }
        if (currentTime - buttonpressStart2 >= holdTimeRequired) {
            stepCounter2 = 0;
            currentAngle2 = 0.0;
            Serial.println("Encoder 2 - Angle reset to 0° after holding button for 5 seconds");
            buttonpressStart2 = 0;
        }
    } else {
        buttonpressStart2 = 0;
    }

    lastClkState2 = clkState2;
}

void playTrack() {
  long track = random(1, numOfTracks + 1);

  while (track == lastTrackPlayed) {
    track = random(1, numOfTracks + 1);
  }

  player.volume(volumeLevel); // controls volume level
  player.playMp3Folder(track);
}

void loop() {
    unsigned long currentTime = millis();

    updateEncoderAngle();
    updateEncoderAngle2();

    // Only print if the defined interval has passed
    if (currentTime - lastPrintTime >= printInterval) {
        updateEncoderAngle();
        Serial.print("Current Notification Duration: ");
        Serial.println(interval);

        RTCTime timeNow;
        RTC.getTime(timeNow);

        if (timeNow.getHour() >= 9 && timeNow.getHour() <= 21) {
            Serial.println("Board Active");
            active = true;
        } else {
            Serial.println("Board Inactive");
            active = false;
            dailyExerciseCount = 0;
            resetLEDs();
        }

        if (active) {
            if (millis() - previousMillis >= interval) {
                Serial.println("It's time to do your exercise!!");
                LEDActivityReminder();
                waitingForExercise = true;
                waitingStartTime = millis();
            }

            if (waitingForExercise) {
                if (millis() - waitingStartTime >= exerciseWindow) {
                    Serial.println("Failed to perform exercise, will notify next exercise time.");
                    waitingForExercise = false;
                    previousMillis = millis();
                } else {
                    checkTilt();
                    previousMillis = millis();
                }
            }
        }

        if (dailyExerciseCount != prevExerciseCount) {
            checkCompletion();
            prevExerciseCount = dailyExerciseCount;
        }

        lastPrintTime = currentTime; // Reset the timer for the next print cycle
    }
}
void checkCompletion() {
  int numLEDsToLight = min(dailyExerciseCount, 6);
  
  for (int j = 0; j < 6; j++) {
    tlc.setPWM(j, 0);
  }

  for (int i = 0; i < numLEDsToLight; i++) {
    tlc.setPWM(i, 2400);
  }

  tlc.write();
}

void resetLEDs() {
  Serial.println("New day. All completed tilts are reset.");
  for (int i = 0; i < 6; i++) {
    tlc.setPWM(i, 0);
  }
  tlc.write();
}

void LEDActivityReminder() {
  unsigned long flickerDuration = 10000;
  unsigned long flickerInterval = 500;
  unsigned long startTime = millis();

  Serial.println("Reminder: It's time to tilt again!");

  while (millis() - startTime < flickerDuration) {
    tlc.setPWM(6, 2400);
    tlc.write();
    delay(flickerInterval);

    tlc.setPWM(6, 0);
    tlc.write();
    delay(flickerInterval);
  }
}

void checkTilt() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float angle = atan2(ay, az) * 180 / PI;

  if (abs(angle) > minTiltAngle) {
    if (!exerciseStarted) {
      exerciseStartTime = millis();
      Serial.println("Exercise started!");
      exerciseStarted = true;
    } else {
      Serial.print("Tilt Angle: ");
      Serial.println(angle);

      unsigned long elapsedTime = millis() - exerciseStartTime;
      unsigned long remainingTime = exerciseDuration - elapsedTime;

      Serial.print("Time remaining: ");
      Serial.print(remainingTime / 1000);
      Serial.println(" seconds");

      if (elapsedTime >= exerciseDuration) {
        dailyExerciseCount++;
        Serial.print("Good job! You did your exercise this time. Total Exercises Today: ");
        Serial.println(dailyExerciseCount);
        playTrack();
        
        exerciseStarted = false;
        waitingForExercise = false;
      }
    }
  } else {
    Serial.print("Tilt Angle: ");
    Serial.println(angle);
    if (exerciseStarted) {
      Serial.println("Went below angle threshold.");
      exerciseStarted = false;
    }
  }
}
