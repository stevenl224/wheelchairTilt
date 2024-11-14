# Exercise Reminder and Tracking System
This Arduino-based project helps users track their exercise routines by detecting tilt angles using a gyroscope and providing visual feedback with LEDs and audio reminders. The system is configurable via rotary encoders, allowing the user to adjust the frequency of notifications and the duration required to complete each exercise. The system also provides feedback for exercise completion and tracks progress throughout the day.

## Features:
- **Tilt Detection**: Tracks user exercise completion by detecting a tilt angle via the MPU6050 gyroscope.
- **Rotary Encoder Control**: Use two rotary encoders to adjust:
  - Interval for exercise reminders (3 levels).
  - Duration required to complete the exercise (3 levels).
- **LED Feedback**: LED board provides visual feedback on exercise completion.
- **Audio Feedback**: A DFPlayer Mini plays random audio tracks to notify the user when exercise is complete.
- **Daily Exercise Count**: Tracks the number of exercises completed each day.
- **Automatic Reset**: Resets daily progress at the end of the day.

## Components:
- **MPU6050**: Gyroscope and accelerometer module for tilt detection.
- **Rotary Encoders**: Two rotary encoders for adjusting exercise reminder interval and duration.
- **LEDs (TLC5947)**: LED driver to provide feedback on progress.
- **DFPlayer Mini**: MP3 player module to play audio files.
- **RTC Module**: Real-time clock to manage daily reset and time-based notifications.

## Pin Setup:
- **LED Control (TLC5947)**:
  - Data: Pin 4
  - Clock: Pin 5
  - Latch: Pin 6

- **Rotary Encoder 1** (for exercise reminder interval):
  - CLK: Pin 3
  - DT: Pin 2
  - SW: Pin 7

- **Rotary Encoder 2** (for exercise duration):
  - CLK: Pin 12
  - DT: Pin 11
  - SW: Pin 10

- **DFPlayer Mini**:
  - RX: Pin 9
  - TX: Pin 8

- **MPU6050**:
  - SDA: Pin A4
  - SCL: Pin A5
  - VCC: 5V
  - GND: GND

## Setup:
1. **Libraries Required**:
   - `Wire`: For I2C communication with the RTC and MPU6050.
   - `RTC`: Real-time clock library.
   - `MPU6050`: Gyroscope library.
   - `Adafruit_TLC5947`: For controlling the LED board.
   - `DFRobotDFPlayerMini`: For audio playback.
   - `SoftwareSerial`: For communicating with the DFPlayer Mini.

2. **Initial Configuration**:
   - Set the current date and time in the code (using `RTC.setTime()`) based on your specific requirements.
   - Connect the hardware according to the pin configuration mentioned above.

3. **Audio Files**: Place audio files in the SD card used with the DFPlayer Mini. Make sure the files are named correctly (e.g., `0001.mp3`, `0002.mp3`, etc.).

## Usage:
1. **Exercise Interval**: Adjust the exercise reminder interval using the first rotary encoder. The interval has three levels:
   - Level 1: 5 seconds
   - Level 2: 10 seconds
   - Level 3: 15 seconds

2. **Exercise Duration**: Adjust the required duration to hold the tilt using the second rotary encoder. The duration has three levels:
   - Level 1: 3 seconds
   - Level 2: 4 seconds
   - Level 3: 5 seconds

3. **Tilt to Exercise**: To complete an exercise, tilt the device (holding the tilt for the specified duration). Once completed, audio will play, and the LED board will light up to indicate progress.

4. **LED Feedback**: LEDs will light up as you complete exercises, with a maximum of 6 LEDs indicating the total exercises completed for the day.

5. **Daily Reset**: At midnight, the system will reset the daily exercise count and LED progress.

## Code Breakdown:

- **`setup()`**:
  - Initializes communication with the MPU6050, DFPlayer Mini, RTC, and LED board.
  - Sets the current time from the RTC.
  - Initializes rotary encoders and other components.

- **`loop()`**:
  - Monitors rotary encoder inputs to adjust exercise interval and duration.
  - Sends notifications based on the configured interval.
  - Tracks tilt angles to detect exercise completion.
  - Provides audio and LED feedback upon completion.
  - Resets the exercise count and LEDs daily.

- **`checkTilt()`**:
  - Reads acceleration values from the MPU6050 and calculates the tilt angle.
  - Starts the exercise when the tilt angle exceeds a threshold.
  - Tracks the time held at the tilt angle and provides feedback when the exercise is completed.

- **`LEDActivityReminder()`**:
  - Flashes an LED to remind the user to perform the exercise.

- **`checkCompletion()`**:
  - Updates the LED board to indicate the number of exercises completed for the day.

## Customization:
- **Exercise Duration & Interval**: Modify the values in the `long level1`, `long level2`, `long level3`, `int duration1`, `int duration2`, and `int duration3` to adjust the exercise interval and duration to your preference.
- **Audio Tracks**: Add more MP3 files to the SD card to customize the audio feedback. Ensure that the files are named in a sequential manner (`0001.mp3`, `0002.mp3`, etc.).
- **LED Feedback**: Customize the LED behavior by adjusting the PWM values in the `LEDActivityReminder()` and `checkCompletion()` functions.

## Troubleshooting:
- **MPU6050 not working**: Make sure the wiring is correct and the sensor is properly initialized in the `setup()` function. The system should print "MPU6050 connection successful" if connected correctly.
- **LEDs not lighting up**: Check the wiring of the TLC5947 LED driver. Ensure that the `tlc.setPWM()` function is correctly setting the PWM values.
- **No audio**: Verify that the DFPlayer Mini is correctly connected, and ensure that audio files are on the SD card in the correct format.

## License:
This project is open-source. Feel free to modify and adapt the code as needed for your personal or educational use.
