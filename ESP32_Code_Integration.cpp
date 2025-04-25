#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// Pins
#define RST_PIN 22
#define SS_PIN 21
#define LED_GREEN 19
#define LED_RED 23
#define MODE_BUTTON 15

// LCD Configuration
#define LCD_COLS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS); // I2C address 0x27, 16x2 display

MFRC522 mfrc522(SS_PIN, RST_PIN);

// WiFi credentials
const char* ssid = "Juwale 5G";
const char* password = "mayuresh@2011";

// Google Apps Script URL
const char* GAS_URL = "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec";

// Session variables
bool sessionActive = false;
unsigned long sessionStartTime = 0;
const unsigned long sessionDuration = 40 * 60 * 1000; // 40 minutes
const unsigned long minAttendanceTime = 15 * 60 * 1000; // 15 minutes
String currentTeacherUID = "";
String currentSubject = "";
String currentSemester = "";

// Mode variables
enum SystemMode { ATTENDANCE_MODE, REGISTRATION_MODE, TEACHER_MODE };
SystemMode currentMode = ATTENDANCE_MODE;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Initialize pins
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  displayMessage("System Initializing", "Connecting WiFi...");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print(".");
  }
  
  displayMessage("WiFi Connected", "IP: " + WiFi.localIP().toString());
  digitalWrite(LED_GREEN, HIGH);
  delay(2000);
  digitalWrite(LED_GREEN, LOW);
  
  showCurrentMode();
}

void loop() {
  // Check mode button
  if (digitalRead(MODE_BUTTON) == LOW) {
    changeMode();
    delay(500); // Debounce
    while(digitalRead(MODE_BUTTON) == LOW); // Wait for button release
  }
  
  // Check session timeout
  if (sessionActive && (millis() - sessionStartTime >= sessionDuration)) {
    endSession();
  }

  // Process RFID card if present
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = getUID();
    Serial.println("Card UID: " + uid);
    
    // Visual feedback
    digitalWrite(LED_GREEN, HIGH);
    displayMessage("Processing Card", "Please wait...");
    
    processCard(uid);
    
    digitalWrite(LED_GREEN, LOW);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    delay(1000); // Show message for 1 second before clearing
    showCurrentMode();
  }
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

void changeMode() {
  switch(currentMode) {
    case ATTENDANCE_MODE:
      currentMode = REGISTRATION_MODE;
      break;
    case REGISTRATION_MODE:
      currentMode = TEACHER_MODE;
      break;
    case TEACHER_MODE:
      currentMode = ATTENDANCE_MODE;
      break;
  }
  
  // Visual feedback
  digitalWrite(LED_GREEN, HIGH);
  delay(200);
  digitalWrite(LED_GREEN, LOW);
  delay(200);
  digitalWrite(LED_GREEN, HIGH);
  delay(200);
  digitalWrite(LED_GREEN, LOW);
  
  showCurrentMode();
}

void showCurrentMode() {
  lcd.clear();
  switch(currentMode) {
    case ATTENDANCE_MODE:
      displayMessage("Attendance Mode", "Scan student card");
      break;
    case REGISTRATION_MODE:
      displayMessage("Registration Mode", "Scan new card");
      break;
    case TEACHER_MODE:
      displayMessage("Teacher Mode", "Scan teacher card");
      break;
  }
}

void displayMessage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void processCard(String uid) {
  switch(currentMode) {
    case TEACHER_MODE:
      processTeacherCard(uid);
      break;
    case ATTENDANCE_MODE:
      processStudentCard(uid);
      break;
    case REGISTRATION_MODE:
      registerStudent(uid);
      break;
  }
}

void processTeacherCard(String uid) {
  if (WiFi.status() != WL_CONNECTED) {
    displayMessage("Error:", "WiFi not connected");
    digitalWrite(LED_RED, HIGH);
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }
  
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
      
      String message1 = "Session Started:";
      String message2 = currentSubject + " " + currentSemester;
      displayMessage(message1, message2);
      
      digitalWrite(LED_GREEN, HIGH);
      delay(2000);
      digitalWrite(LED_GREEN, LOW);
    } else {
      displayMessage("Error:", "Not a teacher card");
      digitalWrite(LED_RED, HIGH);
      delay(2000);
      digitalWrite(LED_RED, LOW);
    }
  } else {
    displayMessage("Error:", "Failed to verify");
    digitalWrite(LED_RED, HIGH);
    delay(2000);
    digitalWrite(LED_RED, LOW);
  }
  http.end();
}

void processStudentCard(String uid) {
  if (!sessionActive) {
    displayMessage("Error:", "No active session");
    displayMessage("Teacher must", "scan first");
    digitalWrite(LED_RED, HIGH);
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    displayMessage("Error:", "WiFi not connected");
    digitalWrite(LED_RED, HIGH);
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }
  
  unsigned long timeInSession = millis() - sessionStartTime;
  
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
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    if (doc["status"] == "OK") {
      String action = doc["action"].as<String>();
      
      if (action == "TIME_IN") {
        String name = doc["name"].as<String>();
        String time = doc["time_in"].as<String>();
        displayMessage("Time In Recorded", name);
        delay(1000);
        displayMessage("Time:", time);
        digitalWrite(LED_GREEN, HIGH);
        delay(2000);
        digitalWrite(LED_GREEN, LOW);
      } 
      else if (action == "TIME_OUT") {
        String name = doc["name"].as<String>();
        String time = doc["time_out"].as<String>();
        displayMessage("Time Out Recorded", name);
        delay(1000);
        displayMessage("Time:", time);
        digitalWrite(LED_GREEN, HIGH);
        delay(2000);
        digitalWrite(LED_GREEN, LOW);
      }
      else if (action == "EARLY_EXIT") {
        displayMessage("Warning:", "Left too early");
        displayMessage("Marked as", "Absent");
        digitalWrite(LED_RED, HIGH);
        delay(2000);
        digitalWrite(LED_RED, LOW);
      }
      else if (action == "ALREADY_COMPLETE") {
        displayMessage("Info:", "Attendance");
        displayMessage("Already completed", "");
        delay(2000);
      }
    } else {
      displayMessage("Error:", doc["message"].as<String>());
      digitalWrite(LED_RED, HIGH);
      delay(2000);
      digitalWrite(LED_RED, LOW);
    }
  } else {
    displayMessage("Error:", "Server error");
    digitalWrite(LED_RED, HIGH);
    delay(2000);
    digitalWrite(LED_RED, LOW);
  }
  http.end();
}

void registerStudent(String uid) {
  if (WiFi.status() != WL_CONNECTED) {
    displayMessage("Error:", "WiFi not connected");
    digitalWrite(LED_RED, HIGH);
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }
  
  HTTPClient http;
  String url = String(GAS_URL) + "?action=reg&uid=" + uid;
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    if (doc["status"] == "OK") {
      displayMessage("Success:", "Card registered");
      digitalWrite(LED_GREEN, HIGH);
      delay(2000);
      digitalWrite(LED_GREEN, LOW);
    } else {
      displayMessage("Error:", doc["message"].as<String>());
      digitalWrite(LED_RED, HIGH);
      delay(2000);
      digitalWrite(LED_RED, LOW);
    }
  } else {
    displayMessage("Error:", "Registration failed");
    digitalWrite(LED_RED, HIGH);
    delay(2000);
    digitalWrite(LED_RED, LOW);
  }
  http.end();
}

void startSession() {
  sessionActive = true;
  sessionStartTime = millis();
  
  String message1 = "Session Started:";
  String message2 = currentSubject + " " + currentSemester;
  displayMessage(message1, message2);
  
  digitalWrite(LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  delay(500);
  digitalWrite(LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
}

void endSession() {
  sessionActive = false;
  
  displayMessage("Session Ended", currentSubject);
  delay(1000);
  displayMessage("Duration:", "40 minutes");
  
  digitalWrite(LED_RED, HIGH);
  delay(1000);
  digitalWrite(LED_RED, LOW);
  delay(500);
  digitalWrite(LED_RED, HIGH);
  delay(1000);
  digitalWrite(LED_RED, LOW);
  
  currentTeacherUID = "";
  currentSubject = "";
  currentSemester = "";
  
  delay(2000);
  showCurrentMode();
}