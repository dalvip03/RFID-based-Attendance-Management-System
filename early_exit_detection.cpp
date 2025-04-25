//Constants (Add at the top)
const int SESSION_DURATION_MIN = 40;       // Full session time
const int MINIMUM_ATTENDANCE_MIN = 20;     // Minimum time required to avoid being marked absent

//Data Structures
struct AttendanceLog {
    String uid;
    DateTime timeIn;
    bool hasScannedIn;
    bool hasScannedOut;
  };
  
  AttendanceLog studentRecords[50];
  int studentCount = 0;


//Helper Functions
int findStudentIndex(String uid) {
    for (int i = 0; i < studentCount; i++) {
      if (studentRecords[i].uid == uid) return i;
    }
    return -1;
  }
  
  void addStudent(String uid, DateTime now) {
    studentRecords[studentCount].uid = uid;
    studentRecords[studentCount].timeIn = now;
    studentRecords[studentCount].hasScannedIn = true;
    studentRecords[studentCount].hasScannedOut = false;
    studentCount++;
  }

  
//Inside loop() After Checking That Session is Active
DateTime now = rtc.now();
String uidStr = "";  // Construct this from RFID bytes as in your full code

for (byte i = 0; i < 4; i++) {
  uidStr += String(mfrc522.uid.uidByte[i], HEX);
}

int index = findStudentIndex(uidStr);

if (index == -1) {
  // First scan - Time In
  addStudent(uidStr, now);
  Serial.println("✅ Time In recorded.");
  sendToSheet(uidStr, "Student", "Time In");
  giveFeedback(true);
} else if (!studentRecords[index].hasScannedOut) {
  // Second scan - Time Out
  TimeSpan attendanceDuration = now - studentRecords[index].timeIn;
  studentRecords[index].hasScannedOut = true;

  if (attendanceDuration.minutes() < MINIMUM_ATTENDANCE_MIN) {
    Serial.println("❌ Left Early - Marked Absent");
    sendToSheet(uidStr, "Student", "Absent (Left Early)");
    giveFeedback(false);
  } else {
    Serial.println("✅ Present - Full Duration Met");
    sendToSheet(uidStr, "Student", "Present");
    giveFeedback(true);
  }
} else {
  Serial.println("⚠️ Already scanned out.");
  giveFeedback(false);
}

//sendToSheet Function Reminder:
void sendToSheet(String uid, String name, String status) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = scriptURL + "?uid=" + uid + "&name=" + name + "&status=" + status;
      http.begin(url);
      int httpCode = http.GET();
      http.end();
    }
  }
  


