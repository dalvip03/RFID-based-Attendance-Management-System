function doGet(e) {
  try {
    if (!e || !e.parameter) {
      return ContentService.createTextOutput(JSON.stringify({
        status: 'Error',
        message: 'No parameters provided or event object missing'
      }));
    }

    Logger.log(JSON.stringify(e));
    var result = { status: 'OK' };

    var spreadsheet = SpreadsheetApp.openById(SHEET_ID);
    var teachersSheet = spreadsheet.getSheetByName(TEACHER_SHEET);
    var studentsSheet = spreadsheet.getSheetByName(STUDENT_SHEET);
    var attendanceSheet = spreadsheet.getSheetByName(ATTENDANCE_SHEET);

    var sts_val = e.parameter.sts; // 'reg' or 'atc'
    var uid_val = e.parameter.uid; // RFID UID

    // Registration mode
    if (sts_val == 'reg') {
      return handleRegistration(e, spreadsheet, teachersSheet, studentsSheet);
    }
    // Attendance mode
    else if (sts_val == 'atc') {
      return handleAttendance(e, spreadsheet, teachersSheet, studentsSheet, attendanceSheet);
    }
    else {
      result.status = 'Error';
      result.message = 'Invalid mode';
      return ContentService.createTextOutput(JSON.stringify(result));
    }
  } catch (error) {
    Logger.log('Error: ' + error.toString());
    return ContentService.createTextOutput(JSON.stringify({
      status: 'Error',
      message: error.toString()
    }));
  }
}
