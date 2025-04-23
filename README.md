<h1>#📚 RFID-Based Attendance Management System Using ESP32 and Google Sheets</h1>

This project automates attendance tracking in educational institutions using an **ESP32 microcontroller**, an **RFID reader**, and **Google Sheets** for cloud-based data storage.

The RFID-Based Attendance Management System using ESP32 and Google Sheets is an IoT-based solution designed to automate and secure the attendance process in educational institutions. This system utilizes an ESP32 microcontroller and an RFID reader (MFRC522) to identify students and teachers through unique RFID cards. Attendance data is captured in real time and seamlessly stored in Google Sheets using the Google Apps Script API. A key feature of the system is the teacher-first logic, where students can only mark their attendance after the assigned teacher has scanned their RFID and opened the session. To prevent proxy attendance, the system uses multiple hardcoded methods including scan time restrictions, batch locking, and unique UID validation. It also supports substitute sessions, allowing a different teacher to take over a cancelled class while recording the actual subject taught. The system is lightweight, cost-effective, and scalable, and offers a cloud-connected, paperless solution for managing attendance records efficiently and securely.

## 🔧 Features

- 📲 **RFID-based student identification**
- 🌐 **ESP32 connects to Wi-Fi and logs data online**
- 📋 **Attendance data is stored in real-time to Google Sheets**
- 👨‍🏫 **Teacher-first validation**: Students can only mark attendance after the teacher
- 🔒 **Proxy prevention techniques using hardcoded logic**
- 🔁 **Handles substitute teachers and session overrides**
- 📈 **Supports multi-semester subject-teacher mapping**

## 🛠️ Hardware Requirements

- ESP32 Dev Board  
- MFRC522 RFID Reader  
- RFID Tags/Cards  
- 16x2 I2C LCD Display  
- Push Button (for mode switching)
