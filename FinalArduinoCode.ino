#include <LiquidCrystal.h>
#include <IRremote.h>
#include <Servo.h>

// ------------------- LCD Setup -------------------
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// ------------------- IR Setup -------------------
const int RECV_PIN = 7;

// ------------------- Servo Setup -------------------
Servo lockServo;
const int SERVO_PIN = 10;
const int LOCKED_POS = 90;   // locked
const int UNLOCKED_POS = 0;  // unlocked

// ------------------- Buzzer Setup (NEW) -------------------
const int BUZZER_PIN = 8; // Connect your buzzer to digital pin 8

// ------------------- Locker & PIN Database -------------------
const int NUM_LOCKERS = 10;
int lockers[NUM_LOCKERS][2] = {
  {10, 1234}, {23, 4321}, {31, 1111}, {4, 2222}, {57, 3355},
  {6, 4646}, {7, 5555}, {8, 6789}, {19, 2468}, {70, 9876}
};

// ------------------- Input Buffers -------------------
char inputBuffer[5]; // up to 4 digits + null
int inputCount = 0;

// ------------------- State Machine -------------------
enum State { WAITING, ENTER_LOCKER, ENTER_PIN };
State currentState = WAITING;

int enteredLocker = -1; // store locker number

// ------------------- RGB LED Setup -------------------
const int RED_PIN = A3;    // PWM pin
const int GREEN_PIN = A4;  // PWM pin
const int BLUE_PIN = A5;   // PWM pin

void setLED(int r, int g, int b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

// ------------------- Sound Functions (NEW) -------------------
// Manual tone generation to avoid timer conflicts with the IRremote library.
// This function is blocking but prevents the program from hanging.
void manualTone(int pin, unsigned int frequency, unsigned long duration) {
  if (frequency == 0) return; // Avoid division by zero
  long halfPeriod = 1000000L / frequency / 2;
  unsigned long cycles = (unsigned long)frequency * duration / 1000;
  for (unsigned long i = 0; i < cycles; i++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(halfPeriod);
    digitalWrite(pin, LOW);
    delayMicroseconds(halfPeriod);
  }
}

void playKeyPressSound() {
  manualTone(BUZZER_PIN, 1500, 50); // High pitch, short duration for key press
}

void playSuccessSound() {
  manualTone(BUZZER_PIN, 523, 150); // C5
  delay(160);
  manualTone(BUZZER_PIN, 659, 150); // E5
  delay(160);
  manualTone(BUZZER_PIN, 784, 150); // G5
}

void playErrorSound() {
  manualTone(BUZZER_PIN, 200, 500); // Low pitch, long duration for error
}

// ------------------- IR Remote Keymap -------------------
int mapIRToKey(unsigned long value) {
  switch (value) {
    case 0xE619FF00: return -2; // Enter
    case 0xE916FF00: return 0;
    case 0xF30CFF00: return 1;
    case 0xE718FF00: return 2;
    case 0xA15EFF00: return 3;
    case 0xF708FF00: return 4;
    case 0xE31CFF00: return 5;
    case 0xA55AFF00: return 6;
    case 0xBD42FF00: return 7;
    case 0xAD52FF00: return 8;
    case 0xB54AFF00: return 9;
    default: return -1; // ignore noise
  }
}

// ------------------- Helpers -------------------
void resetInput() {
  inputCount = 0;
  inputBuffer[0] = '\0';
}

int bufferToInt() {
  int val = 0;
  for (int i = 0; i < inputCount; i++) {
    val = val * 10 + (inputBuffer[i] - '0');
  }
  return val;
}

bool lockerExists(int lockerNum) {
  for (int i = 0; i < NUM_LOCKERS; i++) {
    if (lockers[i][0] == lockerNum) return true;
  }
  return false;
}

bool checkPinMatch(int locker, int pin) {
  for (int i = 0; i < NUM_LOCKERS; i++) {
    if (lockers[i][0] == locker && lockers[i][1] == pin) {
      return true;
    }
  }
  return false;
}

// show a temporary message on LCD for ms milliseconds
void showMessage(const char *line1, const char *line2, unsigned long ms) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  delay(ms);
}

// ------------------- LCD UI -------------------
void showWelcome() {
  // Set LED to blue during welcome
  setLED(0, 0, 255);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome!!");
  lcd.setCursor(0, 1);
  lcd.print("Red Raider");
  delay(5000);   // LED stays blue during this

  lcd.clear();
  lcd.print("Enter Locker No");
  setLED(0, 0, 255); // Keep blue while entering locker number
}

void showInputOnLCD(const char *label) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(label);
  lcd.setCursor(0, 1);
  for (int i = 0; i < inputCount; i++) {
    lcd.print(inputBuffer[i]); // show as digits
  }
}

// ------------------- Setup -------------------
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK);

  // Servo setup
  lockServo.attach(SERVO_PIN);
  lockServo.write(LOCKED_POS); // start locked

  // LED setup
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  // Buzzer setup (NEW)
  pinMode(BUZZER_PIN, OUTPUT);

  setLED(0, 0, 255); // Blue = waiting/in progress

  resetInput();
  showWelcome();
  currentState = ENTER_LOCKER;
}

// ------------------- Main Loop -------------------
void loop() {
  if (IrReceiver.decode()) {
    unsigned long value = IrReceiver.decodedIRData.decodedRawData;
    int key = mapIRToKey(value);

    if (key >= 0 && key <= 9) {
      playKeyPressSound(); // Sound for each number press
      setLED(0, 0, 255); // Blue while entering input
      if (currentState == ENTER_LOCKER) {
        if (inputCount < 2) {
          inputBuffer[inputCount++] = '0' + key;
          inputBuffer[inputCount] = '\0';
        }
        showInputOnLCD("Locker No:");
      } else if (currentState == ENTER_PIN) {
        if (inputCount < 4) {
          inputBuffer[inputCount++] = '0' + key;
          inputBuffer[inputCount] = '\0';
        }
        showInputOnLCD("PIN:");
      }
    }

    else if (key == -2) { // ENTER
      playKeyPressSound(); // Sound for enter press
      if (currentState == ENTER_LOCKER) {
        int lockerNum = bufferToInt();

        if (lockerNum < 1 || lockerNum > 99 || !lockerExists(lockerNum)) {
          setLED(255, 0, 0); // Red instantly for unavailable
          playErrorSound();
          showMessage("Locker", "Unavailable", 2000);
          resetInput();
          showWelcome();
          currentState = ENTER_LOCKER;
          setLED(0, 0, 255); // Back to blue
        } else {
          enteredLocker = lockerNum;
          resetInput();
          currentState = ENTER_PIN;
          lcd.clear();
          lcd.print("Enter PIN:");
          setLED(0, 0, 255); // Blue while entering PIN
        }
      }

      else if (currentState == ENTER_PIN) {
        if (inputCount == 4) {
          int enteredPin = bufferToInt();

          if (checkPinMatch(enteredLocker, enteredPin)) {
            setLED(0, 255, 0); // Green instantly for unlocked
            playSuccessSound();
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Locker Unlocked!");
            lockServo.write(UNLOCKED_POS);
            delay(5000);
            lockServo.write(LOCKED_POS);
            delay(500);
          } else {
            setLED(255, 0, 0); // Red instantly for wrong PIN
            playErrorSound();
            showMessage("Wrong PIN!", "Access Denied", 2000);
            lockServo.write(LOCKED_POS);
          }

          resetInput();
          showWelcome();
          currentState = ENTER_LOCKER;
          setLED(0, 0, 255); // Back to blue instantly
        } else {
          playErrorSound();
          showMessage("Need 4 digits", "", 1500);
          resetInput();
          lcd.clear();
          lcd.print("Enter PIN:");
          setLED(0, 0, 255); // Stay blue while retrying
        }
      }
    }

    IrReceiver.resume();
  }
}

