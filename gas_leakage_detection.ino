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
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-up successful");
    signupOK = true;
  } else {
    Serial.printf("Firebase Sign-Up Error: %s\n", config.signer.signupError.message.c_str());
  }

  

  if (signupOK) {
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
  }
}

void loop() {
  int mq2Value = analogRead(mq2Pin);
  Serial.print("MQ-2 Value: ");
  Serial.println(mq2Value);

  if (mq2Value < lowThreshold) {
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(redLedPin, LOW);
    buzzerInterval = 0;
    if (!Firebase.RTDB.setInt(&fbdo, "/status", 0)) {
      Serial.println("Error writing data to Firebase");
      Serial.println(fbdo.errorReason());
    }
  } else if (mq2Value >= lowThreshold && mq2Value < highThreshold) {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
    buzzerInterval = 3000;
    if (!Firebase.RTDB.setInt(&fbdo, "/status", 1)) {
      Serial.println("Error writing data to Firebase");
      Serial.println(fbdo.errorReason());
    }
  } else {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
    buzzerInterval = 1000;
    if (!Firebase.RTDB.setInt(&fbdo, "/status", 2)) {
      Serial.println("Error writing data to Firebase");
      Serial.println(fbdo.errorReason());
    }
  }

  if (buzzerInterval > 0 && millis() - lastBuzzerTime >= buzzerInterval) {
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    lastBuzzerTime = millis();
  }

  delay(100);
}
