#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <RTClib.h>

#define SS_PIN D4
#define RST_PIN D3
#define GREEN_LED D1
#define RED_LED D2
#define BUZZER D5

const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";
const String scriptURL = "https://script.google.com/macros/s/PASTE_YOURS_HERE/exec";

MFRC522 mfrc522(SS_PIN, RST_PIN);
RTC_DS3231 rtc;

bool sessionActive = false;
DateTime sessionStart;
const int SESSION_DURATION_MIN = 40;

byte teacherUID[4] = {0xDE, 0xAD, 0xBE, 0xEF}; // Change to actual teacher UID

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  rtc.begin();
  WiFi.begin(ssid, password);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected!");
}

bool compareUID(byte *uid1, byte *uid2) {
  for (byte i = 0; i < 4; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

void giveFeedback(bool success) {
  if (success) {
    digitalWrite(GREEN_LED, HIGH);
    tone(BUZZER, 1000, 100);
    delay(100);
    digitalWrite(GREEN_LED, LOW);
  } else {
    digitalWrite(RED_LED, HIGH);
    tone(BUZZER, 400, 300);
    delay(300);
    digitalWrite(RED_LED, LOW);
  }
}

void sendToSheet(String uid, String name, String status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = scriptURL + "?uid=" + uid + "&name=" + name + "&status=" + status;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Google Response: " + payload);
    }
    http.end();
  }
}

void loop() {
  if (sessionActive) {
    DateTime now = rtc.now();
    if ((now - sessionStart).minutes() >= SESSION_DURATION_MIN) {
      sessionActive = false;
      Serial.println("⏱ Session expired.");
      giveFeedback(false);
    }
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  byte scannedUID[4];
  String uidStr = "";
  for (byte i = 0; i < 4; i++) {
    scannedUID[i] = mfrc522.uid.uidByte[i];
    uidStr += String(scannedUID[i], HEX);
  }

  if (!sessionActive) {
    if (compareUID(scannedUID, teacherUID)) {
      sessionStart = rtc.now();
      sessionActive = true;
      Serial.println("✅ Teacher scanned. Session started.");
      giveFeedback(true);
      sendToSheet(uidStr, "Teacher", "Session Start");
    } else {
      Serial.println("❌ Unauthorized. Teacher must scan first.");
      giveFeedback(false);
    }
  } else {
    Serial.println("✅ Student scanned during session.");
    giveFeedback(true);
    sendToSheet(uidStr, "Student", "Present");
  }

  mfrc522.PICC_HaltA();
  delay(2000); // debounce delay
}
