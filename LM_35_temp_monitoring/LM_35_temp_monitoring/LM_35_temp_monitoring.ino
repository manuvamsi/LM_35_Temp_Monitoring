// Define Blynk template information BEFORE including Blynk libraries
#define BLYNK_TEMPLATE_ID "TMPL3MCE-W-US"
#define BLYNK_TEMPLATE_NAME "TempControl"
#define BLYNK_AUTH_TOKEN "rpbJiYaLbjx23ra69y84uos5Xge_dRlS"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "pitches.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Your WiFi credentials
// Set password to "" for open networks
char ssid[] = "vivo V29";
char pass[] = "        ";

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions
const int lm35Pin = 34;     // LM35 temperature sensor
const int relayPin = 5;     // Relay
const int buttonPin = 13;   // Push button
const int buzzerPin = 18;   // Buzzer
const int ledRelayOnPin = 14;  // LED for relay ON (LOW)
const int ledRelayOffPin = 12; // LED for relay OFF (HIGH)

// Constants
const float referenceVoltage = 3.3;
const float temperatureThreshold = 30.0;
const unsigned long updateInterval = 2000;
const unsigned long debounceDelay = 50;
const unsigned long alarmInterval = 1000;
const unsigned long blynkUpdateInterval = 1000; // Blynk update interval

// Variables
float temperature;
float voltage;
bool systemActive = true;
bool alarmTriggered = false;
bool buttonPressed = false;
bool relayLocked = false;
bool previousOverheatState = false; // Track previous overheat state
unsigned long lastUpdateTime = 0;
unsigned long lastButtonTime = 0;
unsigned long lastAlarmTime = 0;
unsigned long lastBlynkUpdateTime = 0; // Last Blynk update timestamp
int buttonState = HIGH;  // Initialize buttonState
int lastButtonState = HIGH;
bool blynkConnected = false;

// Buzzer melody for alarm - INCREASED DURATION
int alarmMelody[] = {NOTE_C6, NOTE_C6};
int alarmDurations[] = {500, 500};  // Increased from 200 to 500
int alarmNotes = 2;
int currentNote = 0;

// Overheat buzzer melody - NEW
int overheatMelody[] = {NOTE_A6, NOTE_G6, NOTE_A6};
int overheatDurations[] = {300, 300, 500};
int overheatNotes = 3;

// Blynk virtual pins
#define VPIN_TEMPERATURE V0
#define VPIN_RELAY_STATUS V1
#define VPIN_SYSTEM_STATUS V2
#define VPIN_VOLTAGE V3
#define VPIN_OVERHEAT_STATUS V4
#define VPIN_SYSTEM_TOGGLE V5

// Function prototypes
void toggleSystem();
void updateBlynk();
void playOverheatAlarm();
void playRecoverySound();
void updateDisplay(int adcValue, float voltage, float temperature);
void displaySystemStopped();

// Handle Blynk system toggle button
BLYNK_WRITE(VPIN_SYSTEM_TOGGLE) {
  int value = param.asInt();
  
  if ((value == 1 && !systemActive) || (value == 0 && systemActive)) {
    // When Blynk button state changes, simulate a physical button press
    toggleSystem();
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledRelayOnPin, OUTPUT);
  pinMode(ledRelayOffPin, OUTPUT);
  
  // REVERSED: Relay starts ON (LOW)
  digitalWrite(relayPin, LOW);
  digitalWrite(ledRelayOnPin, HIGH);
  digitalWrite(ledRelayOffPin, LOW);
  
  analogReadResolution(12);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Temperature"));
  display.println(F("Monitoring"));
  display.println(F("System"));
  display.display();
  delay(2000);
  
  // Connect to Blynk
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Connecting to WiFi"));
  display.println(ssid);
  display.display();
  
  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);
  
  // Start Blynk connection
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  // Check if connected to Blynk
  if (Blynk.connected()) {
    blynkConnected = true;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Connected to Blynk!"));
    display.display();
    delay(1000);
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Blynk connection failed"));
    display.println(F("Continuing offline"));
    display.display();
    delay(2000);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Temperature Monitor"));
  display.println(F("Starting..."));
  display.println(F("Threshold: "));
  display.print(temperatureThreshold);
  display.println(F(" C"));
  display.display();
  
  // Increased duration for startup sounds
  tone(buzzerPin, NOTE_C5, 400);
  delay(450);
  tone(buzzerPin, NOTE_E5, 400);
  delay(450);
  tone(buzzerPin, NOTE_G5, 400);
  delay(500);
  noTone(buzzerPin);
  
  // Update Blynk with initial values
  if (blynkConnected) {
    Blynk.virtualWrite(VPIN_SYSTEM_STATUS, systemActive ? 1 : 0);
    Blynk.virtualWrite(VPIN_RELAY_STATUS, digitalRead(relayPin) == LOW ? "ON" : "OFF");
    Blynk.virtualWrite(VPIN_SYSTEM_TOGGLE, systemActive ? 1 : 0);
  }
  
  delay(1000);
}

void loop() {
  // Run Blynk process
  if (blynkConnected) {
    Blynk.run();
  }
  
  checkButton();
  
  if (systemActive) {
    if (millis() - lastUpdateTime >= updateInterval) {
      lastUpdateTime = millis();
      
      int adcValue = analogRead(lm35Pin);
      voltage = (adcValue / 4095.0) * referenceVoltage;
      
      // Fixed temperature calculation
      temperature = voltage * 100.0; // Removed the +10 offset
      
      Serial.print("ADC Value: ");
      Serial.print(adcValue);
      Serial.print(" | Voltage: ");
      Serial.print(voltage, 3);
      Serial.print("V | Temperature: ");
      Serial.print(temperature, 2);
      Serial.print("°C | System: ");
      Serial.println(systemActive ? "ACTIVE" : "STOPPED");
      Serial.print("Relay Locked: ");
      Serial.println(relayLocked ? "YES" : "NO");
      
      updateDisplay(adcValue, voltage, temperature);
      
      // REVERSED RELAY OPERATION: Check temperature against threshold and control relay
      bool currentOverheatState = (temperature > temperatureThreshold);
      
      if (currentOverheatState && !relayLocked) {
        // REVERSED: For overheat, turn OFF relay (HIGH)
        digitalWrite(relayPin, HIGH);
        
        // For overheat, turn on first LED only
        digitalWrite(ledRelayOnPin, LOW);
        digitalWrite(ledRelayOffPin, HIGH);
        
        alarmTriggered = true;
        
        // Play overheat buzzer melody when temperature first exceeds threshold
        if (!previousOverheatState) {
          playOverheatAlarm();
        }
        
        Serial.println("RELAY DEACTIVATED - HIGH TEMPERATURE");
      } else if (!relayLocked) {
        // REVERSED: For normal operation, turn ON relay (LOW)
        digitalWrite(relayPin, LOW);
        
        // For normal operation, turn on second LED only
        digitalWrite(ledRelayOnPin, HIGH);
        digitalWrite(ledRelayOffPin, LOW);
        
        // Play recovery sound if transitioning from overheat to normal
        if (previousOverheatState && !currentOverheatState) {
          playRecoverySound();
        }
        
        alarmTriggered = false;
        noTone(buzzerPin);
        currentNote = 0;
      }
      
      previousOverheatState = currentOverheatState;
    }
    
    // Continue sounding alarm until temperature returns to normal
    if (alarmTriggered && (millis() - lastAlarmTime > alarmDurations[currentNote])) {
      lastAlarmTime = millis();
      if (currentNote % 2 == 0) {
        // Increased buzzer sound duration
        tone(buzzerPin, alarmMelody[currentNote / 2 % alarmNotes], alarmDurations[currentNote]);
      } else {
        noTone(buzzerPin);
      }
      currentNote = (currentNote + 1) % (alarmNotes * 2);
    }
  } else {
    if (millis() - lastUpdateTime >= updateInterval) {
      lastUpdateTime = millis();
      
      // When system is stopped, relay is OFF (HIGH)
      digitalWrite(relayPin, HIGH);
      digitalWrite(ledRelayOnPin, LOW);
      digitalWrite(ledRelayOffPin, HIGH);
      
      noTone(buzzerPin);
      
      displaySystemStopped();
    }
  }
  
  // Update Blynk with current values
  if (blynkConnected && (millis() - lastBlynkUpdateTime >= blynkUpdateInterval)) {
    lastBlynkUpdateTime = millis();
    updateBlynk();
  }
}

void updateBlynk() {
  // Update temperature value
  Blynk.virtualWrite(VPIN_TEMPERATURE, temperature);
  
  // Update voltage value
  Blynk.virtualWrite(VPIN_VOLTAGE, voltage);
  
  // Update relay status
  Blynk.virtualWrite(VPIN_RELAY_STATUS, digitalRead(relayPin) == LOW ? "ON" : "OFF");
  
  // Update system status
  Blynk.virtualWrite(VPIN_SYSTEM_STATUS, systemActive ? "ACTIVE" : "STOPPED");
  
  // Update overheat status
  Blynk.virtualWrite(VPIN_OVERHEAT_STATUS, temperature > temperatureThreshold ? "OVERHEAT" : "NORMAL");
  
  // Make sure the Blynk app button state matches actual system state
  Blynk.virtualWrite(VPIN_SYSTEM_TOGGLE, systemActive ? 1 : 0);
}

void toggleSystem() {
  // When button is pressed, turn off relay (HIGH)
  digitalWrite(relayPin, HIGH);
        
  // When button is pressed, turn on BOTH LEDs
  digitalWrite(ledRelayOnPin, HIGH);  
  digitalWrite(ledRelayOffPin, HIGH);
        
  noTone(buzzerPin);
        
  systemActive = !systemActive;
        
  if (systemActive) {
    relayLocked = false;  // Unlock relay when system is active
          
    // Increased buzzer duration
    tone(buzzerPin, NOTE_E5, 400);
    delay(450);
    tone(buzzerPin, NOTE_G5, 400);
          
    // After button operation, return to appropriate LED state
    if (temperature > temperatureThreshold) {
      // Overheat condition - relay OFF (HIGH)
      digitalWrite(relayPin, HIGH);
      digitalWrite(ledRelayOnPin, LOW);
      digitalWrite(ledRelayOffPin, HIGH);
      alarmTriggered = true;
      playOverheatAlarm();
    } else {
      // Normal condition - relay ON (LOW)
      digitalWrite(relayPin, LOW);
      digitalWrite(ledRelayOnPin, HIGH);
      digitalWrite(ledRelayOffPin, LOW);
    }
  } else {
    relayLocked = true;  // Lock relay when system stopped
          
    // Increased buzzer duration
    tone(buzzerPin, NOTE_G5, 400);
    delay(450);
    tone(buzzerPin, NOTE_E5, 400);
          
    // System stopped, relay OFF (HIGH)
    digitalWrite(relayPin, HIGH);
    digitalWrite(ledRelayOnPin, LOW);
    digitalWrite(ledRelayOffPin, HIGH);
  }
  delay(450);
  noTone(buzzerPin);
        
  // Update Blynk immediately after system state change
  if (blynkConnected) {
    updateBlynk();
  }
}

void checkButton() {
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastButtonTime = millis();
  }

  if ((millis() - lastButtonTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        toggleSystem();
      }
    }
  }
  lastButtonState = reading;
}

// Function to play the overheat alarm melody
void playOverheatAlarm() {
  for (int i = 0; i < overheatNotes; i++) {
    tone(buzzerPin, overheatMelody[i], overheatDurations[i]);
    delay(overheatDurations[i] + 50); // Add a small delay between notes
  }
  noTone(buzzerPin);
}

// Function to play recovery sound when temperature returns to normal
void playRecoverySound() {
  tone(buzzerPin, NOTE_C6, 300);
  delay(350);
  tone(buzzerPin, NOTE_E6, 300);
  delay(350);
  tone(buzzerPin, NOTE_G6, 500);
  delay(550);
  noTone(buzzerPin);
}

void updateDisplay(int adcValue, float voltage, float temperature) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Temperature Monitor"));
  display.drawLine(0, 8, display.width(), 8, SSD1306_WHITE);
  
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print(temperature, 1);
  display.print(F(" C"));
  
  display.drawRect(100, 10, 4, 12, SSD1306_WHITE);
  display.fillRect(100, 22-(int)((temperature < 40.0 ? temperature : 40.0)/40.0*12), 4, (int)((temperature < 40.0 ? temperature : 40.0)/40.0*12), SSD1306_WHITE);
  display.fillRect(98, 22, 8, 3, SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(72, 16);
  // REVERSED: Display logic for relay status
  if (temperature > temperatureThreshold && systemActive && !relayLocked) {
    display.print(F("RELAY: OFF"));
    display.setCursor(72, 24);
    display.print(F("ALARM!"));
  } else if (systemActive && !relayLocked) {
    display.print(F("RELAY: ON"));
    display.setCursor(72, 24);
    display.print(F("Normal"));
  } else {
    display.print(F("RELAY: OFF"));
    display.setCursor(72, 24);
    display.print(F("Locked"));
  }
  
  // Show Blynk status
  if (blynkConnected) {
    display.setCursor(0, 24);
    display.print(F("B")); // Indicator for Blynk connection
  }
  
  display.display();
}

void displaySystemStopped() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Temperature Monitor"));
  display.drawLine(0, 8, display.width(), 8, SSD1306_WHITE);
  display.setCursor(10, 16);
  display.println(F("SYSTEM STOPPED"));
  display.setCursor(0, 24);
  display.println(F("Press button to restart"));
  
  // Show Blynk status
  if (blynkConnected) {
    display.setCursor(120, 24);
    display.print(F("B")); // Indicator for Blynk connection
  }
  
  display.display();
}
