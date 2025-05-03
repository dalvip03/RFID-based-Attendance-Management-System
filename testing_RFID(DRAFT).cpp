#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include <HTTPClient.h>

// RFID Pins
#define SS_PIN  5  
#define RST_PIN 4

// Button Pin
#define BTN_PIN 15

// WiFi Credentials
const char* ssid = "realme narzo 20";
const char* password = "12345678";

// Google Script URL
String Web_App_URL = "https://script.google.com/macros/s/AKfycbwleRQJLQVwmLMqEBNKWNgla12bZZlrpeY45U5kbK2nX5mog6_GSNdWIYL1S6e5zkbz/exec";

// Variables for registration
String reg_Name = "";
String reg_Subject = "";

// Variables for attendance
String atc_Info = "";
String atc_Name = "";
String atc_Date = "";
String atc_Time_In = "";
String atc_Time_Out = "";
String atc_Subject = "";

// LCD Configuration
int lcdColumns = 16;
int lcdRows = 2;

// RFID Variables
int readsuccess;
char str[32] = "";
String UID_Result = "--------";

// System mode ("atc" = attendance, "reg" = registration)
String modes = "atc";
bool isRegisteringTeacher = false;

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Initialize RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Custom max function to avoid type conflicts
int myMax(int a, int b) {
  return (a > b) ? a : b;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BTN_PIN, INPUT_PULLUP);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  delay(500);

  // Initialize RFID
  SPI.begin();
  mfrc522.PCD_Init();
  delay(500);

  // Show startup message
  displayCenteredText("ESP32 RFID", "Attendance System");
  delay(3000);

  // Connect to WiFi
  connectToWiFi();
}

void loop() {
  // Check button press to switch modes
  checkModeSwitch();

  // Detect RFID card
  readsuccess = getUID();

  // Attendance mode
  if (modes == "atc") {
    handleAttendanceMode();
  }
  // Registration mode
  else if (modes == "reg") {
    handleRegistrationMode();
  }

  delay(10);
}

// Connect to WiFi
void connectToWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  
  WiFi.begin(ssid, password);
  
  int timeout = 20; // 20 seconds timeout
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (timeout-- <= 0) {
      displayCenteredText("WiFi Connect", "Timeout!");
      delay(2000);
      ESP.restart();
    }
  }

  displayCenteredText("WiFi Connected!", IPToString(WiFi.localIP()));
  delay(2000);
  lcd.clear();
}

String IPToString(IPAddress address) {
  return String(address[0]) + "." + String(address[1]) + "." + String(address[2]) + "." + String(address[3]);
}

// Check mode switch button
void checkModeSwitch() {
  if (digitalRead(BTN_PIN) == LOW) {
    delay(200); // Debounce
    
    if (modes == "atc") {
      modes = "reg";
      isRegisteringTeacher = false;
      displayCenteredText("Mode:", "Registration");
    } else {
      modes = "atc";
      displayCenteredText("Mode:", "Attendance");
    }
    delay(1000);
    lcd.clear();
    
    // Wait for button release
    while (digitalRead(BTN_PIN) == LOW) {
      delay(10);
    }
  }
}

// Handle attendance mode
void handleAttendanceMode() {
  lcd.setCursor(0, 0);
  lcd.print("ATTENDANCE MODE");
  lcd.setCursor(0, 1);
  lcd.print("Tap your card");

  if (readsuccess) {
    lcd.clear();
    displayCenteredText("Processing", "Please wait...");
    
    http_Req(modes, UID_Result);
  }
}

// Handle registration mode
void handleRegistrationMode() {
  if (!isRegisteringTeacher) {
    lcd.setCursor(0, 0);
    lcd.print("REGISTER MODE");
    lcd.setCursor(0, 1);
    lcd.print("1.Teacher 2.Student");

    // Check for teacher/student selection
    if (readsuccess) {
      if (UID_Result == "111") { // Teacher registration (using Prof. ABC's UID as selector)
        isRegisteringTeacher = true;
        reg_Name = "";
        reg_Subject = "";
        lcd.clear();
        displayCenteredText("Registering", "TEACHER");
        delay(2000);
        getNameInput();
      } else if (UID_Result == "69") { // Student registration (using John's UID as selector)
        isRegisteringTeacher = false;
        reg_Name = "";
        lcd.clear();
        displayCenteredText("Registering", "STUDENT");
        delay(2000);
        getNameInput();
      }
    }
  } else {
    // Already in registration process
    if (reg_Name == "") {
      getNameInput();
    } else if (isRegisteringTeacher && reg_Subject == "") {
      getSubjectInput();
    } else {
      // All information collected, send to Google Sheets
      lcd.clear();
      displayCenteredText("Registering...", "Please wait");
      
      String url = Web_App_URL + "?sts=reg&uid=" + UID_Result;
      if (isRegisteringTeacher) {
        url += "&name=" + reg_Name + "&subject=" + reg_Subject;
      } else {
        url += "&name=" + reg_Name;
      }
      
      sendHTTPRequest(url);
      
      // Reset for next registration
      isRegisteringTeacher = false;
      reg_Name = "";
      reg_Subject = "";
    }
  }
}

// Get name input using RFID cards
void getNameInput() {
  displayCenteredText("Enter Name:", "Use letter cards");
  
  // In a real implementation, you would have a way to input names
  // For this example, we'll simulate with a delay
  delay(3000);
  reg_Name = "Demo Name"; // Replace with actual input method
}

// Get subject input using RFID cards
void getSubjectInput() {
  displayCenteredText("Enter Subject:", "Use subject cards");
  
  // In a real implementation, you would have a way to input subjects
  // For this example, we'll simulate with a delay
  delay(3000);
  reg_Subject = "Demo Subject"; // Replace with actual input method
}

// Send HTTP request
void sendHTTPRequest(String url) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    String payload;
    if (httpCode > 0) {
      payload = http.getString();
      processResponse(payload);
    } else {
      displayCenteredText("HTTP Error", String(httpCode));
    }
    http.end();
  } else {
    displayCenteredText("WiFi", "Disconnected!");
    connectToWiFi();
  }
}

// Process HTTP response
void processResponse(String payload) {
  String sts_Res = getValue(payload, ',', 0);

  if (sts_Res == "OK") {
    if (modes == "atc") {
      atc_Info = getValue(payload, ',', 1);
      
      if (atc_Info == "TI_Successful") {
        atc_Name = getValue(payload, ',', 2);
        atc_Date = getValue(payload, ',', 3);
        atc_Time_In = getValue(payload, ',', 4);
        displaySplitText(atc_Name, "IN: " + atc_Time_In, atc_Date);
      }
      else if (atc_Info == "TO_Successful") {
        atc_Name = getValue(payload, ',', 2);
        atc_Date = getValue(payload, ',', 3);
        atc_Time_In = getValue(payload, ',', 4);
        atc_Time_Out = getValue(payload, ',', 5);
        displaySplitText(atc_Name, "OUT: " + atc_Time_Out, atc_Date);
      }
      else if (atc_Info == "T_Subject") {
        atc_Subject = getValue(payload, ',', 2);
        displaySplitText("Subject:", atc_Subject, "Ready for students");
      }
      else if (atc_Info == "atcInf01") {
        displayCenteredText("Attendance", "record complete");
      }
      else if (atc_Info == "atcInf02") {
        displayCenteredText("Attendance", "already marked");
      }
      else if (atc_Info == "atcErr01") {
        displayCenteredText("Error!", "Card not reg");
      }
      else if (atc_Info == "atcErr02") {
        displayCenteredText("Error!", "No teacher present");
      }
      else if (atc_Info == "atcErr03") {
        displayCenteredText("Error!", "Teacher not out");
      }
    }
    else if (modes == "reg") {
      String reg_Info = getValue(payload, ',', 1);
      
      if (reg_Info == "R_Successful") {
        displayCenteredText("Registered", "successfully");
      }
      else if (reg_Info == "T_Registered") {
        String subject = getValue(payload, ',', 2);
        displaySplitText("Teacher reg.", subject, "successfully");
      }
      else if (reg_Info == "S_Registered") {
        displayCenteredText("Student", "registered");
      }
      else if (reg_Info == "regErr01") {
        displayCenteredText("Error!", "Card already reg");
      }
    }
  } else {
    displayCenteredText("Server", "Error!");
  }
}

// HTTP Request function
void http_Req(String str_modes, String str_uid) {
  String url = Web_App_URL + "?sts=" + str_modes + "&uid=" + str_uid;
  sendHTTPRequest(url);
}

// Get UID from RFID card
int getUID() {  
  if(!mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }
  if(!mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }
  
  byteArray_to_string(mfrc522.uid.uidByte, mfrc522.uid.size, str);
  UID_Result = str;
  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  return 1;
}

// Convert byte array to string
void byteArray_to_string(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len*2] = '\0';
}

// Parse string value
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// Display centered text
void displayCenteredText(String line1, String line2) {
  lcd.clear();
  
  // Display first line
  int line1Pos = myMax(0, (int)((16 - line1.length()) / 2));
  lcd.setCursor(line1Pos, 0);
  lcd.print(line1);

  // Display second line
  int line2Pos = myMax(0, (int)((16 - line2.length()) / 2));
  lcd.setCursor(line2Pos, 1);
  lcd.print(line2);
}

// Display split text (name, time, date)
void displaySplitText(String line1, String line2, String line3) {
  lcd.clear();
  
  // Display name
  int line1Pos = myMax(0, (int)((16 - line1.length()) / 2));
  lcd.setCursor(line1Pos, 0);
  lcd.print(line1);

  // Display time
  int line2Pos = myMax(0, (int)((16 - line2.length()) / 2));
  lcd.setCursor(line2Pos, 1);
  lcd.print(line2);

  delay(3000);
  
  // Display date
  lcd.clear();
  int line3Pos = myMax(0, (int)((16 - line3.length()) / 2));
  lcd.setCursor(line3Pos, 0);
  lcd.print(line3);

  delay(3000);
  lcd.clear();
}