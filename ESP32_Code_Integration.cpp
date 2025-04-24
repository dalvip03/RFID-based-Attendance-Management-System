#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_PIN          21         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Google Apps Script URL
const char* GAS_URL = "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec";

// Session variables
bool sessionActive = false;
unsigned long sessionStartTime = 0;
const unsigned long sessionDuration = 40 * 60 * 1000; // 40 minutes in milliseconds
const unsigned long minAttendanceTime = 15 * 60 * 1000; // 15 minutes minimum
String currentTeacherUID = "";
String currentSubject = "";
String currentSemester = "";

void setup() {
  Serial.begin(115200);   // Initialize serial communications with the PC
  SPI.begin();           // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522 card
  delay(4);              // Optional delay
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println(F("RFID Attendance System Ready!"));
}

void loop() {
  // Check if session is active and time has expired
  if (sessionActive && (millis() - sessionStartTime >= sessionDuration)) {
    endSession();
  }

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Get UID
  String uid = getUID();
  Serial.print("UID tag: ");
  Serial.println(uid);

  // Process the card
  processRFID(uid);

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

String getUID() {
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidString.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    uidString.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  uidString.toUpperCase();
  return uidString;
}

void processRFID(String uid) {
  if (!sessionActive) {
    // Check if this is a teacher card
    checkTeacher(uid);
  } else {
    // Check student attendance
    checkStudent(uid);
  }
}

void checkTeacher(String uid) {
  // Send request to Google Apps Script to verify teacher
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(GAS_URL) + "?action=check_teacher&uid=" + uid;
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      
      if (doc["isTeacher"]) {
        currentTeacherUID = uid;
        currentSubject = doc["subject"].as<String>();
        currentSemester = doc["semester"].as<String>();
        startSession();
        
        Serial.println("Teacher verified. Session started for:");
        Serial.println("Subject: " + currentSubject);
        Serial.println("Semester: " + currentSemester);
      } else {
        Serial.println("Not a registered teacher card");
      }
    } else {
      Serial.println("Error verifying teacher");
    }
    http.end();
  }
}

void checkStudent(String uid) {
  unsigned long currentTime = millis();
  unsigned long timeInSession = currentTime - sessionStartTime;
  
  // Check if this is a time-in or time-out
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(GAS_URL) + "?action=check_student&uid=" + uid + 
                 "&teacher_uid=" + currentTeacherUID + 
                 "&semester=" + currentSemester + 
                 "&subject=" + currentSubject + 
                 "&session_time=" + String(timeInSession);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
      
      // Check if this was an early exit
      if (payload.indexOf("EARLY_EXIT") != -1) {
        Serial.println("Student left too early - marked absent");
      } else if (payload.indexOf("ATTENDANCE_RECORDED") != -1) {
        Serial.println("Attendance recorded successfully");
      }
    } else {
      Serial.println("Error recording attendance");
    }
    http.end();
  }
}

void startSession() {
  sessionActive = true;
  sessionStartTime = millis();
  Serial.println("Attendance session started");
}

void endSession() {
  sessionActive = false;
  currentTeacherUID = "";
  currentSubject = "";
  currentSemester = "";
  Serial.println("Attendance session ended");
}