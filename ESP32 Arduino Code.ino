#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 5
#define RST_PIN 4
#define BTN_PIN 15

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
String Web_App_URL = "PASTE_YOUR_WEBAPP_URL_HERE"; // From Google Apps Script deployment

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

String UID_Result = "";
String mode = "atc"; // Can be toggled with button

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(BTN_PIN, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected!");
  delay(1000);
  lcd.clear();
}

void loop() {
  if (digitalRead(BTN_PIN) == LOW) {
    mode = (mode == "atc") ? "reg" : "atc";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mode: " + mode);
    delay(1000);
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  UID_Result = getUID(mfrc522.uid.uidByte, mfrc522.uid.size);
  sendToWebApp(UID_Result, mode);
  delay(3000);
  lcd.clear();
}

String getUID(byte *buffer, byte bufferSize) {
  String uid = "";
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] < 0x10) uid += "0";
    uid += String(buffer[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

void sendToWebApp(String uid, String mode) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = Web_App_URL + "?uid=" + uid + "&mode=" + mode;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);
      lcd.setCursor(0, 0);
      lcd.print(payload.substring(0, 16));
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Failed to send");
    }
    http.end();
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Error");
  }
}