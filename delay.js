function onEdit(e) {
    const sheet = e.source.getActiveSheet();
    const range = e.range;
    const row = range.getRow();
    const col = range.getColumn();
  
    // Check if edit is in "Action" column (assuming column D)
    if (col === 4 && sheet.getName() === "Attendance") { 
      const action = e.value;
      const studentId = sheet.getRange(row, 2).getValue(); // Column B: Student ID
      const timestamp = new Date(sheet.getRange(row, 1).getValue()); // Column A: Timestamp
  
      if (action === "Time Out") {
        checkEarlyExit(sheet, row, studentId, timestamp);
      } else if (action === "Time In") {
        markPresent(sheet, row);
      }
    }
  }
  
  function checkEarlyExit(sheet, row, studentId, timeOut) {
    const data = sheet.getDataRange().getValues();
    let lastTimeIn = null;
  
    // Find the last "Time In" for the same student
    for (let i = row - 1; i >= 1; i--) {
      if (data[i][1] === studentId && data[i][3] === "Time In") {
        lastTimeIn = new Date(data[i][0]);
        break;
      }
    }
  
    if (lastTimeIn) {
      const timeDiffMinutes = (timeOut - lastTimeIn) / (1000 * 60); // Convert ms to minutes
  
      // If Time Out is within 15 minutes of Time In, mark as Absent
      if (timeDiffMinutes < 15) {
        sheet.getRange(row, 5).setValue("Absent (Early Exit)");
      } else {
        sheet.getRange(row, 5).setValue("Present");
      }
    }
  }
  
  function markPresent(sheet, row) {
    sheet.getRange(row, 5).setValue("Present"); // Column E: Status
  }