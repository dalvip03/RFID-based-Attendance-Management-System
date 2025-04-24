function doGet(e) {
    var action = e.parameter.action;
    var response;
    
    if (action == "check_teacher") {
      response = checkTeacher(e.parameter.uid);
    } else if (action == "check_student") {
      response = checkStudent(
        e.parameter.uid,
        e.parameter.teacher_uid,
        e.parameter.semester,
        e.parameter.subject,
        e.parameter.session_time
      );
    }
    
    return ContentService.createTextOutput(JSON.stringify(response))
      .setMimeType(ContentService.MimeType.JSON);
  }
  
  function checkTeacher(uid) {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var teacherSheet = ss.getSheetByName("Teacher Registration");
    var data = teacherSheet.getDataRange().getValues();
    
    for (var i = 1; i < data.length; i++) {
      if (data[i][1] == uid) { // UID is in column B (index 1)
        // Find which semester this teacher is assigned to
        var semesterColumns = ["FYIT sem1", "FYIT sem2", "SYIT sem3", "SYIT sem4", "TYIT sem5", "TYIT sem6"];
        var semester = "";
        var subject = "";
        
        for (var j = 2; j < 8; j++) { // Columns C to H
          if (data[i][j]) {
            semester = semesterColumns[j-2];
            subject = data[i][j];
            break;
          }
        }
        
        return {
          isTeacher: true,
          name: data[i][0],
          semester: semester,
          subject: subject
        };
      }
    }
    
    return { isTeacher: false };
  }
  
  function checkStudent(uid, teacherUid, semester, subject, sessionTime) {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    
    // 1. Verify teacher session is still active
    var teacherData = checkTeacher(teacherUid);
    if (!teacherData.isTeacher) {
      return { error: "INVALID_TEACHER" };
    }
    
    // 2. Get student info
    var studentSheet = ss.getSheetByName("Student Registration");
    var studentData = studentSheet.getDataRange().getValues();
    var studentName = "";
    
    for (var i = 1; i < studentData.length; i++) {
      if (studentData[i][1] == uid) {
        studentName = studentData[i][0];
        break;
      }
    }
    
    if (!studentName) {
      return { error: "STUDENT_NOT_REGISTERED" };
    }
    
    // 3. Determine if this is time-in or time-out
    var attendanceSheet = ss.getSheetByName(semester);
    var attendanceData = attendanceSheet.getDataRange().getValues();
    var today = new Date();
    var dateString = Utilities.formatDate(today, Session.getScriptTimeZone(), "dd/MM/yyyy");
    var timeString = Utilities.formatDate(today, Session.getScriptTimeZone(), "HH:mm:ss");
    
    // Check if student already has a time-in today for this subject
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
      return { result: "ATTENDANCE_RECORDED", action: "TIME_IN" };
    } else {
      // Time-out - check if it's too early
      var sessionTimeNum = parseInt(sessionTime);
      var minAttendanceTime = 15 * 60 * 1000; // 15 minutes in milliseconds
      
      if (sessionTimeNum < minAttendanceTime) {
        // Mark as absent for leaving too early
        attendanceSheet.getRange(existingRecord + 1, 7).setValue("Absent (Left Early)");
        return { result: "EARLY_EXIT" };
      } else {
        // Record time-out
        attendanceSheet.getRange(existingRecord + 1, 6).setValue(timeString);
        return { result: "ATTENDANCE_RECORDED", action: "TIME_OUT" };
      }
    }
  }