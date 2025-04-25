function doGet(e) {
  var result = {status: 'ERROR', message: 'No parameters'};
  
  try {
    if (e.parameter) {
      var action = e.parameter.action;
      var spreadsheetId = 'YOUR_SPREADSHEET_ID'; // Replace with your spreadsheet ID
      var ss = SpreadsheetApp.openById(spreadsheetId);
      
      switch(action) {
        case 'check_teacher':
          result = checkTeacher(e.parameter.uid, ss);
          break;
        case 'check_student':
          result = checkStudent(
            e.parameter.uid,
            e.parameter.teacher_uid,
            e.parameter.semester,
            e.parameter.subject,
            parseFloat(e.parameter.session_time),
            ss
          );
          break;
        case 'register_student':
          result = registerStudent(e.parameter.uid, ss);
          break;
        default:
          result = {status: 'ERROR', message: 'Invalid action'};
      }
    }
  } catch (error) {
    result = {status: 'ERROR', message: error.toString()};
  }
  
  return ContentService.createTextOutput(JSON.stringify(result))
    .setMimeType(ContentService.MimeType.JSON);
}

function checkTeacher(uid, ss) {
  try {
    var teacherSheet = ss.getSheetByName("Teacher Registration");
    if (!teacherSheet) {
      return {status: 'ERROR', message: 'Teacher sheet not found'};
    }
    
    var data = teacherSheet.getDataRange().getValues();
    var semesterColumns = ["FYIT sem1", "FYIT sem2", "SYIT sem3", "SYIT sem4", "TYIT sem5", "TYIT sem6"];
    
    for (var i = 1; i < data.length; i++) {
      if (data[i][1] == uid) { // UID is in column B
        for (var j = 2; j < 8; j++) { // Columns C to H
          if (data[i][j]) {
            return {
              status: 'OK',
              isTeacher: true,
              name: data[i][0],
              semester: semesterColumns[j-2],
              subject: data[i][j]
            };
          }
        }
      }
    }
    
    return {status: 'OK', isTeacher: false};
  } catch (error) {
    return {status: 'ERROR', message: error.toString()};
  }
}

function checkStudent(uid, teacherUid, semester, subject, sessionTime, ss) {
  try {
    // 1. Verify teacher session is still valid
    var teacherData = checkTeacher(teacherUid, ss);
    if (!teacherData.isTeacher) {
      return {status: 'ERROR', message: 'Invalid teacher session'};
    }
    
    // 2. Get student info
    var studentSheet = ss.getSheetByName("Student Registration");
    if (!studentSheet) {
      return {status: 'ERROR', message: 'Student sheet not found'};
    }
    
    var studentData = studentSheet.getDataRange().getValues();
    var studentName = "";
    
    for (var i = 1; i < studentData.length; i++) {
      if (studentData[i][1] == uid) { // UID is in column B
        studentName = studentData[i][0]; // Name is in column A
        break;
      }
    }
    
    if (!studentName) {
      return {status: 'ERROR', message: 'Student not registered'};
    }
    
    // 3. Process attendance
    var attendanceSheet = ss.getSheetByName(semester);
    if (!attendanceSheet) {
      return {status: 'ERROR', message: 'Semester sheet not found'};
    }
    
    var today = new Date();
    var dateString = Utilities.formatDate(today, Session.getScriptTimeZone(), "dd/MM/yyyy");
    var timeString = Utilities.formatDate(today, Session.getScriptTimeZone(), "HH:mm:ss");
    
    // Check existing records
    var attendanceData = attendanceSheet.getDataRange().getValues();
    var existingRecord = null;
    
    for (var i = 1; i < attendanceData.length; i++) {
      if (attendanceData[i][1] == uid && 
          attendanceData[i][2] == dateString && 
          attendanceData[i][3] == subject) {
        existingRecord = i;
        break;
      }
    }
    
    if (existingRecord === null) {
      // Time-in
      attendanceSheet.appendRow([
        studentName,
        uid,
        dateString,
        subject,
        timeString,
        "", // Time-out empty
        "Present" // Default status
      ]);
      
      return {
        status: 'OK',
        action: 'TIME_IN',
        name: studentName,
        date: dateString,
        time_in: timeString,
        message: 'Time In recorded'
      };
    } else {
      // Check if already has time-out
      if (attendanceData[existingRecord][5] != "") {
        return {
          status: 'OK',
          action: 'ALREADY_COMPLETE',
          message: 'Attendance already completed'
        };
      }
      
      // Check for early exit (less than 15 minutes)
      var minAttendanceTime = 15 * 60 * 1000; // 15 minutes in milliseconds
      if (sessionTime < minAttendanceTime) {
        // Mark as absent for leaving early
        attendanceSheet.getRange(existingRecord + 1, 7).setValue("Absent (Left Early)");
        attendanceSheet.getRange(existingRecord + 1, 6).setValue(timeString);
        
        return {
          status: 'OK',
          action: 'EARLY_EXIT',
          name: studentName,
          date: dateString,
          time_in: attendanceData[existingRecord][4],
          time_out: timeString,
          message: 'Left too early - marked absent'
        };
      } else {
        // Normal time-out
        attendanceSheet.getRange(existingRecord + 1, 6).setValue(timeString);
        
        return {
          status: 'OK',
          action: 'TIME_OUT',
          name: studentName,
          date: dateString,
          time_in: attendanceData[existingRecord][4],
          time_out: timeString,
          message: 'Time Out recorded'
        };
      }
    }
  } catch (error) {
    return {status: 'ERROR', message: error.toString()};
  }
}

function registerStudent(uid, ss) {
  try {
    var studentSheet = ss.getSheetByName("Student Registration");
    if (!studentSheet) {
      return {status: 'ERROR', message: 'Student sheet not found'};
    }
    
    // Check if UID already exists
    var data = studentSheet.getDataRange().getValues();
    for (var i = 1; i < data.length; i++) {
      if (data[i][1] == uid) {
        return {status: 'ERROR', message: 'UID already registered'};
      }
    }
    
    // Register new student (UID only, name can be added manually later)
    var lastRow = studentSheet.getLastRow();
    studentSheet.getRange(lastRow + 1, 2).setValue(uid); // Column B for UID
    studentSheet.getRange(lastRow + 1, 1).setValue("New Student"); // Default name
    
    return {
      status: 'OK',
      message: 'Student registered successfully',
      uid: uid
    };
  } catch (error) {
    return {status: 'ERROR', message: error.toString()};
  }
}