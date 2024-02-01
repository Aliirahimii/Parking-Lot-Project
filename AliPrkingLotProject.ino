#include <Wire.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // Adjust the address and columns according to your LCD specifications
SoftwareSerial mySerial(3, 2); // SIM808 Tx & Rx = Arduino #3 & #2
DS3231 rtc;

bool parkingOpen = false;
bool parkingFullMessageSent = false; // Flag to track if parking full message has been sent
int sensorPins[] = {5, 6, 7, 8, 9, 10, 11, 12};
int numSensors = 8;
int lastSensorStates[8] = {1};
unsigned long startTime[8] = {0};
float cost[8] = {0};
const int buttonPin = 13;
const unsigned long costPerMinute = 10;
const float entranceFee = 1.0;

void setup() {
  lcd.begin();
  lcd.clear();
  pinMode(13, INPUT_PULLUP);
  
  for (int i = 0; i < numSensors; i++) {
    pinMode(sensorPins[i], INPUT);
    lastSensorStates[i] = digitalRead(sensorPins[i]);
    attachInterrupt(digitalPinToInterrupt(sensorPins[i]), sensorInterrupt, CHANGE);
  }
  
  Serial.begin(9600);
  Serial.println("Initializing...");
  mySerial.begin(9600);
  delay(1000);
  
  rtc.setYear(2024);
  rtc.setMonth(1);
  rtc.setDate(25);
  rtc.setDoW(SUNDAY);
  rtc.setHour(12);
  rtc.setMinute(0);
  rtc.setSecond(0);
}

void loop() {
  static boolean welcomeDisplayed = false;
  static boolean buttonPressed = false;
  
  memset(cost, 0, sizeof(cost));

 if (!parkingOpen) {
    sendSMS("+989120591557", "Parking is now open!");
    parkingOpen = true;
  }

  if (!welcomeDisplayed) {
    lcd.setCursor(0, 1);
    lcd.print("Welcome to Ali's");
    lcd.setCursor(0, 2);
    lcd.print("Parking ");
    
    delay(3000);
    lcd.clear();
    welcomeDisplayed = true;
    return;
  }

  int freeSlots = 0;

  for (int i = 0; i < numSensors; i++) {
    int sensorValue = digitalRead(sensorPins[i]);
    updateLCD(i, sensorValue);

    if (sensorValue != lastSensorStates[i]) {
      if (sensorValue == HIGH) {
        startTime[i] = millis(); // Record start time when a car parks
      } else {
        // Car left, calculate cost based on entrance fee and duration
        unsigned long duration = (millis() - startTime[i]) / 60000; // Convert milliseconds to minutes
        cost[i] = entranceFee + (duration * costPerMinute);
      }
       if (areAllSlotsOccupied()) {
        if (!parkingFullMessageSent) {
          sendSMS("+989120591557", "Your Parking Is Full");
          parkingFullMessageSent = true; // Set the flag to true to indicate that the message has been sent
        }
      } else {
        parkingFullMessageSent = false; // Reset the flag when at least one slot is available
      }

    }

    freeSlots += (sensorValue == HIGH);
  }

  lcd.setCursor(4, 0);
  lcd.print("Have slots:");
  lcd.print(freeSlots);

  if (digitalRead(buttonPin) == HIGH && !buttonPressed) {
    lcd.clear();
    unsigned long startTime = millis();
    buttonPressed = true;

    while (millis() - startTime < 10000) {
      for (int i = 0; i < numSensors; i++) {
        int row;
        int col;
        if (i < 4) {
          row = i;
          col = 0;
        } else {
          row = i - 4;
          col = 10;
        }
        lcd.setCursor(col, row);
        lcd.print("S" + String(i + 1) + ": $");
        lcd.print(cost[i], 2);
      }
      delay(1000);
    }

    lcd.clear();
  }

  if (digitalRead(buttonPin) == LOW) {
    buttonPressed = false;
  }

  delay(1000);
}

void sensorInterrupt() {
  // This function will be called when any IR sensor state changes
}

void updateLCD(int sensorIndex, int sensorValue) {
  int row;
  int col;
  if (sensorIndex < 4) {
    row = 1;
    col = sensorIndex * 5;
  } else {
    row = 2;
    col = (sensorIndex - 4) * 5;
  }
  lcd.setCursor(col, row);
  lcd.print("S" + String(sensorIndex + 1) + ":" + (sensorValue == HIGH ? "F" : "O")); 
}
void sendSMS(String phoneNumber, String message) {
  mySerial.println("AT+CMGF=1");
  delay(1000);
  mySerial.print("AT+CMGS=\"");
  mySerial.print(phoneNumber);
  mySerial.println("\"");
  delay(1000);
  mySerial.print(message);
  delay(100);
  mySerial.write(26);
  delay(1000);
}
bool areAllSlotsOccupied() {
  for (int i = 0; i < numSensors; i++) {
    if (digitalRead(sensorPins[i]) == HIGH) {
      return false; // At least one slot is free, not all occupied
    }
  }
  return true; // All slots are occupied
}