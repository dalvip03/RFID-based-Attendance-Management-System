//Required Libraries
#include <SPI.h>
#include <MFRC522.h>

//Define Pins and Variables
#define RST_PIN 9
#define SS_PIN 10
MFRC522 mfrc522(SS_PIN, RST_PIN);

bool sessionStarted = false;

// Replace with your actual teacher RFID UID (4 bytes)
byte teacherUID[4] = {0xDE, 0xAD, 0xBE, 0xEF};


//Setup Function
void setup() {
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();
  
    Serial.println("Teacher-First RFID Attendance System Initialized");
  }


//Helper Function to Compare UIDs
bool compareUID(byte *uid1, byte *uid2) {
    for (byte i = 0; i < 4; i++) {
      if (uid1[i] != uid2[i]) return false;
    }
    return true;
  }

  
// Main Loop
void loop() {
    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;
  
    byte readUID[4];
    for (byte i = 0; i < 4; i++) {
      readUID[i] = mfrc522.uid.uidByte[i];
    }
  
    if (!sessionStarted) {
      if (compareUID(readUID, teacherUID)) {
        sessionStarted = true;
        Serial.println("Teacher verified. Session started.");
      } else {
        Serial.println("Only teacher can start the session.");
      }
    } else {
      Serial.print("Student scanned: ");
      for (byte i = 0; i < 4; i++) {
        Serial.print(readUID[i], HEX);
        if (i < 3) Serial.print(":");
      }
      Serial.println(" | Attendance recorded.");
    }
  
    mfrc522.PICC_HaltA(); // Stop reading card
  }
  