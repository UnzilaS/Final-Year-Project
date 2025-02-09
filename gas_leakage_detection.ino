#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define DATABASE_URL "https://gas-detection-a032a-default-rtdb.firebaseio.com"
#define API_KEY "AIzaSyARisU5Zx2fOB-aSk13-auv9sbl3kzPl6Y"

#define WIFI_SSID "ESP-32"
#define WIFI_PASSWORD "ESP1222"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

int mq2Pin = 34;
int buzzerPin = 13;

int greenLedPin = 27;
int yellowLedPin = 26;
int redLedPin = 25;

int lowThreshold = 600;
int highThreshold = 800;

unsigned long lastBuzzerTime = 0;
int buzzerInterval = 3000;

bool signupOK = false;

void setup() {
  Serial.begin(115200);  // Initialize serial communication for debugging

  pinMode(buzzerPin, OUTPUT);  // Set buzzer pin as output
  pinMode(greenLedPin, OUTPUT);  // Set green LED pin as output
  pinMode(yellowLedPin, OUTPUT);  // Set yellow LED pin as output
  pinMode(redLedPin, OUTPUT);  // Set red LED pin as output

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // Connect to Wi-Fi network
  Serial.print("Connecting Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {  // Wait for Wi-Fi connection
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());  // Print local IP address once connected

  config.api_key = API_KEY;  // Set Firebase API key
  config.database_url = DATABASE_URL;  // Set Firebase database URL
  config.token_status_callback = tokenStatusCallback;  // Set token status callback function

  if (Firebase.signUp(&config, &auth, "", "")) {  // Attempt Firebase sign-up
    Serial.println("Firebase sign-up successful");
    signupOK = true;  // Set signupOK flag to true if successful
  } else {
    Serial.printf("Firebase Sign-Up Error: %s\n", config.signer.signupError.message.c_str());  // Print error message if sign-up fails
  }

  if (signupOK) {
    Firebase.begin(&config, &auth);  // Initialize Firebase connection
    Firebase.reconnectWiFi(true);  // Reconnect Wi-Fi if connection is lost
  }
}

void loop() {
  int mq2Value = analogRead(mq2Pin);  // Read analog value from MQ-2 sensor
  Serial.print("MQ-2 Value: ");
  Serial.println(mq2Value);  // Print sensor value to serial monitor

  if (mq2Value < lowThreshold) {  // If sensor value is below low threshold
    digitalWrite(greenLedPin, HIGH);  // Turn on green LED
    digitalWrite(yellowLedPin, LOW);  // Turn off yellow LED
    digitalWrite(redLedPin, LOW);  // Turn off red LED
    buzzerInterval = 0;  // Set buzzer interval to 0 (no sound)
    if (!Firebase.RTDB.setInt(&fbdo, "/status", 0)) {  // Update Firebase with status 0
      Serial.println("Error writing data to Firebase");
      Serial.println(fbdo.errorReason());  // Print error reason if update fails
    }
  } else if (mq2Value >= lowThreshold && mq2Value < highThreshold) {  // If sensor value is between low and high thresholds
    digitalWrite(greenLedPin, LOW);  // Turn off green LED
    digitalWrite(yellowLedPin, HIGH);  // Turn on yellow LED
    digitalWrite(redLedPin, LOW);  // Turn off red LED
    buzzerInterval = 3000;  // Set buzzer interval to 3000ms (3 seconds)
    if (!Firebase.RTDB.setInt(&fbdo, "/status", 1)) {  // Update Firebase with status 1
      Serial.println("Error writing data to Firebase");
      Serial.println(fbdo.errorReason());  // Print error reason if update fails
    }
  } else {  // If sensor value is above high threshold
    digitalWrite(greenLedPin, LOW);  // Turn off green LED
    digitalWrite(yellowLedPin, LOW);  // Turn off yellow LED
    digitalWrite(redLedPin, HIGH);  // Turn on red LED
    buzzerInterval = 1000;  // Set buzzer interval to 1000ms (1 second)
    if (!Firebase.RTDB.setInt(&fbdo, "/status", 2)) {  // Update Firebase with status 2
      Serial.println("Error writing data to Firebase");
      Serial.println(fbdo.errorReason());  // Print error reason if update fails
    }
  }

  if (buzzerInterval > 0 && millis() - lastBuzzerTime >= buzzerInterval) {  // If buzzer interval is greater than 0 and enough time has passed
    digitalWrite(buzzerPin, HIGH);  // Turn on buzzer
    delay(500);  // Keep buzzer on for 500ms (0.5 seconds)
    digitalWrite(buzzerPin, LOW);  // Turn off buzzer
    lastBuzzerTime = millis();  // Update last buzzer activation time
  }

  delay(100);  // Small delay to avoid rapid looping and to allow other tasks to run
}
