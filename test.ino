#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5
#define RST_PIN 4

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
String UID_Result = "";
String Name_Result = "";

void setup() {
  Serial.begin(115200);
  SPI.begin();      
  mfrc522.PCD_Init(); 
  Serial.println("Tap your card...");
  
  // Default key for authentication (factory default is all 0xFF)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Get UID
  UID_Result = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    UID_Result += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    UID_Result += String(mfrc522.uid.uidByte[i], HEX);
  }
  UID_Result.toUpperCase();

  // Authenticate and read from block 4 (you must have written the name to this block beforehand)
  byte block = 4;
  byte buffer[18];
  byte size = sizeof(buffer);
  Name_Result = "";

  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status == MFRC522::STATUS_OK) {
    status = mfrc522.MIFARE_Read(block, buffer, &size);
    if (status == MFRC522::STATUS_OK) {
      for (int i = 0; i < 16; i++) {
        if (buffer[i] != 0x00) Name_Result += (char)buffer[i];
      }
    } else {
      Serial.print("Read failed: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  } else {
    Serial.println("Authentication failed.");
  }

  // Print UID and Name
  Serial.print("UID  : ");
  Serial.println(UID_Result);
  Serial.print("Name : ");
  Serial.println(Name_Result);

  delay(1500);
}