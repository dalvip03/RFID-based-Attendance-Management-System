// Google Sheet Structure: 
// Sheets: Students, Teachers, FYIT SEM I

function doGet(e) {
    const sheetID = 'PASTE_YOUR_SPREADSHEET_ID'; // Replace with your Sheet ID
    const ss = SpreadsheetApp.openById(sheetID);
    const teacherSheet = ss.getSheetByName("Teachers");
    const studentSheet = ss.getSheetByName("Students");
    const attendSheet = ss.getSheetByName("FYIT SEM I"); // Adjust as needed
  
    const uid = e.parameter.uid;
    const mode = e.parameter.mode;
  
    if (!uid || !mode) {
      return ContentService.createTextOutput("ERROR,Missing Parameters");
    }
  
    const teachersUIDs = teacherSheet.getRange(2, 2, teacherSheet.getLastRow()).getValues().flat();
    const studentsUIDs = studentSheet.getRange(2, 2, studentSheet.getLastRow()).getValues().flat();
    const now = new Date();
    const timeNow = Utilities.formatDate(now, "Asia/Kolkata", "HH:mm:ss");
    const dateNow = Utilities.formatDate(now, "Asia/Kolkata", "dd/MM/yyyy");
  
    // Helper for time diff
    function getTimeInMinutes(timeStr) {
      const parts = timeStr.split(":");
      return parseInt(parts[0]) * 60 + parseInt(parts[1]);
    }
  
    // TEACHER SCANS CARD TO START SESSION
    if (teachersUIDs.includes(uid)) {
      const teacherName = teacherSheet.getRange(teachersUIDs.indexOf(uid) + 2, 1).getValue();
      CacheService.getScriptCache().put("teacherUID", uid, 3600); // Store for 1hr
      CacheService.getScriptCache().put("sessionStartTime", timeNow, 3600);
      CacheService.getScriptCache().put("sessionStartDate", dateNow, 3600);
  
      return ContentService.createTextOutput(`OK,SessionStarted,${teacherName}`);
    }
  
    // STUDENT SCANS - ONLY IF TEACHER HAS STARTED SESSION
    const sessionTeacher = CacheService.getScriptCache().get("teacherUID");
    const sessionStartTime = CacheService.getScriptCache().get("sessionStartTime");
    const sessionDate = CacheService.getScriptCache().get("sessionStartDate");
  
    if (!sessionTeacher || sessionDate !== dateNow) {
      return ContentService.createTextOutput("OK,NoActiveSession");
    }
  
    // Is registered student?
    if (!studentsUIDs.includes(uid)) {
      return ContentService.createTextOutput("OK,UnknownStudent");
    }
  
    const studentName = studentSheet.getRange(studentsUIDs.indexOf(uid) + 2, 1).getValue();
  
    // Check if student already has "Time In"
    const data = attendSheet.getDataRange().getValues();
    let foundRow = -1;
    for (let i = 1; i < data.length; i++) {
      if (data[i][1] === uid && data[i][2] === dateNow) {
        foundRow = i + 1;
        break;
      }
    }
  
    if (foundRow === -1) {
      // Time In
      attendSheet.appendRow([studentName, uid, dateNow, "", timeNow, "", ""]);
      return ContentService.createTextOutput(`OK,TimeIn,${studentName},${timeNow}`);
    } else {
      // Time Out
      const timeIn = data[foundRow - 1][4];
      const timeDiff = getTimeInMinutes(timeNow) - getTimeInMinutes(timeIn);
      let status = (timeDiff < 20) ? "Absent" : "Present";
  
      attendSheet.getRange(foundRow, 6).setValue(timeNow);
      attendSheet.getRange(foundRow, 7).setValue(status);
  
      return ContentService.createTextOutput(`OK,TimeOut,${studentName},${timeNow},${status}`);
    }
  }  