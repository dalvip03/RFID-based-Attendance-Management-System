//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 03_ESP32_RFID_Google_Spreadsheet_Attendance
//----------------------------------------Including the libraries.
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include <HTTPClient.h>
//----------------------------------------

// Defines SS/SDA PIN and Reset PIN for RFID-RC522.
#define SS_PIN  5  
#define RST_PIN 4

// Defines the button PIN.
#define BTN_PIN 15

//----------------------------------------SSID and PASSWORD of your WiFi network.
const char* ssid = "Juwale 5G";  //--> Your wifi name
const char* password = "mayuresh@2011"; //--> Your wifi password
//----------------------------------------

// Google script Web_App_URL.
String Web_App_URL = "https://script.google.com/macros/s/AKfycbzOm7dw6Tx7xohNX8MWT3BAyibOMLRZK0qQ0zkkvgU76HtfxDy1DvTHUheh1NaNAAw/exec";

String reg_Info = "";

String atc_Info = "";
String atc_Name = "";
String atc_Date = "";
String atc_Time_In = "";
String atc_Time_Out = "";

// Variables for the number of columns and rows on the LCD.
int lcdColumns = 16; // Changed to 16 for 16x2 LCD
int lcdRows = 2;     // Changed to 2 for 16x2 LCD

// Variable to read data from RFID-RC522.
int readsuccess;
char str[32] = "";
String UID_Result = "--------";

String modes = "atc";

// Create LiquidCrystal_I2C object as "lcd" and set the LCD I2C address to 0x27 and set the LCD configuration to 16x2.
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  // (lcd_address, lcd_Columns, lcd_Rows)

// Create MFRC522 object as "mfrc522" and set SS/SDA PIN and Reset PIN.
MFRC522 mfrc522(SS_PIN, RST_PIN);  //--> Create MFRC522 instance.

//________________________________________________________________________________http_Req()
// Subroutine for sending HTTP requests to Google Sheets.
void http_Req(String str_modes, String str_uid) {
  if (WiFi.status() == WL_CONNECTED) {
    String http_req_url = "";

    //----------------------------------------Create links to make HTTP requests to Google Sheets.
    if (str_modes == "atc") {
      http_req_url  = Web_App_URL + "?sts=atc";
      http_req_url += "&uid=" + str_uid;
    }
    if (str_modes == "reg") {
      http_req_url = Web_App_URL + "?sts=reg";
      http_req_url += "&uid=" + str_uid;
    }
    //----------------------------------------

    //----------------------------------------Sending HTTP requests to Google Sheets.
    Serial.println();
    Serial.println("-------------");
    Serial.println("Sending request to Google Sheets...");
    Serial.print("URL : ");
    Serial.println(http_req_url);
    
    // Create an HTTPClient object as "http".
    HTTPClient http;

    // HTTP GET Request.
    http.begin(http_req_url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Gets the HTTP status code.
    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);

    // Getting response from google sheet.
    String payload;
    if (httpCode > 0) {
      payload = http.getString();
      Serial.println("Payload : " + payload);  
    }
    
    Serial.println("-------------");
    http.end();
    //----------------------------------------

    String sts_Res = getValue(payload, ',', 0);

    //----------------------------------------Conditions that are executed based on the payload response.
    if (sts_Res == "OK") {
      if (str_modes == "atc") {
        atc_Info = getValue(payload, ',', 1);
        
        if (atc_Info == "TI_Successful") {
          atc_Name = getValue(payload, ',', 2);
          atc_Date = getValue(payload, ',', 3);
          atc_Time_In = getValue(payload, ',', 4);

          displaySplitText(atc_Name, "IN: " + atc_Time_In, atc_Date);
        }

        if (atc_Info == "TO_Successful") {
          atc_Name = getValue(payload, ',', 2);
          atc_Date = getValue(payload, ',', 3);
          atc_Time_In = getValue(payload, ',', 4);
          atc_Time_Out = getValue(payload, ',', 5);

          displaySplitText(atc_Name, "OUT: " + atc_Time_Out, atc_Date);
        }

        if (atc_Info == "atcInf01") {
          displayCenteredText("Attendance", "record complete");
        }

        if (atc_Info == "atcErr01") {
          displayCenteredText("Error!", "Card not reg");
        }

        atc_Info = "";
        atc_Name = "";
        atc_Date = "";
        atc_Time_In = "";
        atc_Time_Out = "";
      }

      if (str_modes == "reg") {
        reg_Info = getValue(payload, ',', 1);
        
        if (reg_Info == "R_Successful") {
          displayCenteredText("UID uploaded", "successfully");
        }

        if (reg_Info == "regErr01") {
          displayCenteredText("Error!", "Card already reg");
        }

        reg_Info = "";
      }
    }
    //----------------------------------------
    else {
      displayCenteredText("Connection" , "Error!");
    }
  }
}
//________________________________________________________________________________

//________________________________________________________________________________getValue()
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
//________________________________________________________________________________ 

//________________________________________________________________________________getUID()
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
//________________________________________________________________________________

//________________________________________________________________________________byteArray_to_string()
void byteArray_to_string(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len*2] = '\0';
}
//________________________________________________________________________________

//________________________________________________________________________________displayCenteredText()
void displayCenteredText(String line1, String line2) {
  lcd.clear();
  delay(500);

  // Display first line
  int line1Length = line1.length();
  if (line1Length > 16) {
    line1 = line1.substring(0, 16); // Truncate if too long
  }
  int line1Pos = (16 - line1Length) / 2;
  lcd.setCursor(line1Pos, 0);
  lcd.print(line1);

  // Display second line
  int line2Length = line2.length();
  if (line2Length > 16) {
    line2 = line2.substring(0, 16); // Truncate if too long
  }
  int line2Pos = (16 - line2Length) / 2;
  lcd.setCursor(line2Pos, 1);
  lcd.print(line2);

  delay(5000);
  lcd.clear();
  delay(500);
}
//________________________________________________________________________________

//________________________________________________________________________________displaySplitText()
void displaySplitText(String line1, String line2, String line3) {
  lcd.clear();
  delay(500);

  // Display first line (Name)
  int line1Length = line1.length();
  if (line1Length > 16) {
    line1 = line1.substring(0, 16); // Truncate if too long
  }
  int line1Pos = (16 - line1Length) / 2;
  lcd.setCursor(line1Pos, 0);
  lcd.print(line1);

  // Display second line (Time)
  int line2Length = line2.length();
  if (line2Length > 16) {
    line2 = line2.substring(0, 16); // Truncate if too long
  }
  int line2Pos = (16 - line2Length) / 2;
  lcd.setCursor(line2Pos, 1);
  lcd.print(line2);

  delay(3000); // Display for 3 seconds

  // Display third line (Date)
  lcd.clear();
  delay(500);
  int line3Length = line3.length();
  if (line3Length > 16) {
    line3 = line3.substring(0, 16); // Truncate if too long
  }
  int line3Pos = (16 - line3Length) / 2;
  lcd.setCursor(line3Pos, 0);
  lcd.print(line3);

  delay(3000); // Display for 3 seconds
  lcd.clear();
  delay(500);
}
//________________________________________________________________________________

//________________________________________________________________________________VOID SETUP()
void setup(){
  Serial.begin(115200);
  Serial.println();
  delay(1000);

  pinMode(BTN_PIN, INPUT_PULLUP);
  
  // Initialize LCD.
  lcd.init();
  lcd.backlight();
  lcd.clear();
  delay(500);

  // Init SPI bus.
  SPI.begin();      
  // Init MFRC522.
  mfrc522.PCD_Init(); 
  delay(500);

  lcd.setCursor(0, 0);
  lcd.print("ESP32 RFID");
  lcd.setCursor(0, 1);
  lcd.print("Google Sheets");
  delay(3000);
  lcd.clear();

  // Connect to Wi-Fi
  Serial.println();
  Serial.println("------------");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int connecting_process_timed_out = 20; // 20 seconds timeout
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    lcd.setCursor(0, 0);
    lcd.print("Connecting...");
    delay(250);
    lcd.clear();
    delay(250);
    
    if (connecting_process_timed_out > 0) connecting_process_timed_out--;
    if (connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("------------");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connected");
  delay(2000);
  lcd.clear();
  delay(500);
}
//________________________________________________________________________________

//________________________________________________________________________________VOID LOOP()
void loop(){
  // Switch modes when the button is pressed.
  int BTN_State = digitalRead(BTN_PIN);

  if (BTN_State == LOW) {
    lcd.clear();
    
    if (modes == "atc") {
      modes = "reg";
    } else if (modes == "reg") {
      modes = "atc";
    }
    
    delay(500);
  }

  // Detect if reading the UID from the card or keychain was successful.
  readsuccess = getUID();

  // Attendance mode
  if (modes == "atc") {
    lcd.setCursor(0, 0);
    lcd.print("ATTENDANCE");
    lcd.setCursor(0, 1);
    lcd.print("Tap your card");

    if (readsuccess){
      lcd.clear();
      delay(500);
      lcd.setCursor(0, 0);
      lcd.print("Getting UID...");
      lcd.setCursor(0, 1);
      lcd.print("Please wait...");
      delay(1000);

      http_Req(modes, UID_Result);
    }
  }

  // Registration mode
  if (modes == "reg") {
    lcd.setCursor(0, 0);
    lcd.print("REGISTRATION");
    lcd.setCursor(0, 1);
    lcd.print("Tap your card ");

    if (readsuccess){
      lcd.clear();
      delay(500);
      lcd.setCursor(0, 0);
      lcd.print("Getting UID...");
      lcd.setCursor(0, 1);
      lcd.print("Please wait...");
      delay(1000);

      http_Req(modes, UID_Result);
    }
  }

  delay(10);
}
//________________________________________________________________________________
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<