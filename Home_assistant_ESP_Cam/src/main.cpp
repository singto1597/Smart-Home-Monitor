#include <Arduino.h>

// กำหนดขา Serial2 ของ ESP32-CAM
#define RX_PIN 3   // U0R (GPIO3) ใช้อ่านจาก TX ของ ESP32
#define TX_PIN 1   // U0T (GPIO1) ใช้ส่งกลับไปหา ESP32 (ถ้าต้องการตอบ)

void setup() {
  Serial.begin(115200);         // Debug Monitor
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Serial2 สำหรับรับจาก ESP32
  Serial.println("ESP32-CAM ready to receive commands...");
}

void loop() {
  if (Serial2.available()) {
    String command = Serial2.readStringUntil('\n');  // อ่านทีละบรรทัด
    command.trim(); // ตัดช่องว่าง/เว้นบรรทัดออก

    if (command.length() > 0) {
      Serial.print("Received command: ");
      Serial.println(command);

      // ตัวอย่าง: ตรวจสอบคำสั่ง
      if (command == "TAKE_PHOTO") {
        Serial.println("📷 Received request: TAKE_PHOTO");
        // ยังไม่ถ่ายภาพ แค่รับเฉย ๆ
      } 
      else if (command == "TAKE_PHOTO_AND_SEND") {
        Serial.println("📷 Received request: TAKE_PHOTO_AND_SEND");
        // ยังไม่ถ่ายภาพ
      }
      else {
        Serial.println("❓ Unknown command");
      }
    }
  }
}
