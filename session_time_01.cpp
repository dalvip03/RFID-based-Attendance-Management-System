#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <RTClib.h>

// Pin setup
#define RST_PIN 9
#define SS_PIN 10
#define GREEN_LED 6
#define RED_LED 5
#define BUZZER 4

MFRC522 mfrc522(SS_PIN, RST_PIN);
RTC_DS3231 rtc;

const int SESSION_DURATION_MIN = 40;
bool sessionActive = false;
DateTime sessionStart;

// Replace with actual teacher RFID UID (4 bytes)
byte teacherUID[4] = {0xDE, 0xAD, 0xBE, 0xEF};

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  rtc.begin();

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  Serial.println("RFID Attendance - Fixed Session Time Mode Initialized");
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
    tone(BUZZER, 1000, 200);
    delay(200);
    digitalWrite(GREEN_LED, LOW);
  } else {
    digitalWrite(RED_LED, HIGH);
    tone(BUZZER, 500, 400);
    delay(400);
    digitalWrite(RED_LED, LOW);
  }
}

void loop() {
  // Check if session is ongoing and expired
  if (sessionActive) {
    DateTime now = rtc.now();
    TimeSpan elapsed = now - sessionStart;
    if (elapsed.minutes() >= SESSION_DURATION_MIN) {
      sessionActive = false;
      Serial.println("Session time ended.");
      giveFeedback(false); // Red LED + Buzz
    }
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  byte scannedUID[4];
  for (byte i = 0; i < 4; i++) {
    scannedUID[i] = mfrc522.uid.uidByte[i];
  }

  if (!sessionActive) {
    if (compareUID(scannedUID, teacherUID)) {
      sessionStart = rtc.now();
      sessionActive = true;
      Serial.println("✅ Teacher verified. Session started for 40 minutes.");
      giveFeedback(true); // Green LED + buzz
    } else {
      Serial.println("❌ Unauthorized user. Teacher must scan first.");
      giveFeedback(false); // Red LED + buzz
    }
  } else {
    Serial.println("✅ Session is active. Attendance scan accepted.");
    giveFeedback(true); // Green LED + buzz
  }

  mfrc522.PICC_HaltA(); // Halt reader
}
 