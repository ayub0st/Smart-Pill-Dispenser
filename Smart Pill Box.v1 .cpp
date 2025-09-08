#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>

// ====================================================================
// 🟢 การต่อวงจร
// --------------------------------------------------------------------
// LCD Keypad Shield : ใช้พอร์ต D8, D9, D4, D5, D6, D7 บนบอร์ด
// RTC DS3231        : SDA -> A4, SCL -> A5, VCC -> 5V, GND -> GND
//
// LED ช่องยา (3 ดวง): 
//   - LED1 (ยาเช้า) -> D2 ผ่าน R220Ω -> LED -> GND
//   - LED2 (ยากลางวัน) -> D3 ผ่าน R220Ω -> LED -> GND
//   - LED3 (ยาเย็น) -> D11 ผ่าน R220Ω -> LED -> GND
//
// โซลินอยด์ (3 ช่อง, ผ่านโมดูล Delay Relay/Transistor):
//   - Solenoid1 -> D12 -> IN โมดูล -> OUT -> Solenoid1+ (Solenoid1- -> GND)
//   - Solenoid2 -> D13 -> IN โมดูล -> OUT -> Solenoid2+ (Solenoid2- -> GND)
//   - Solenoid3 -> A1  -> IN โมดูล -> OUT -> Solenoid3+ (Solenoid3- -> GND)
//
// Buzzer Active 3 ขา (มี VCC, GND, SIG):
//   - VCC -> 5V
//   - GND -> GND
//   - SIG -> D10
//
// ปุ่ม SELECT จาก LCD Shield ใช้ A0
// ====================================================================

// ---------- RTC ----------
RTC_DS3231 rtc;

// ---------- LCD Shield ----------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ---------- Pin Setup ----------
const int ledPins[3]      = {2, 3, 11};     // LED ช่องยา
const int solenoidPins[3] = {12, 13, A1};   // Solenoid Lock
const int buzzerPin       = 10;             // Buzzer Active 3 ขา
const int buttonPin       = A0;             // ปุ่ม SELECT

// ---------- เวลาแจกยา ----------
int pillHours[3]   = {8, 12, 16};  // เวลา 08:00, 12:00, 16:00
int pillMinutes[3] = {0, 0, 0};    // นาที = 00

// ---------- ตัวแปรสถานะ ----------
bool hasDispensed[3] = {false, false, false};  // กันจ่ายซ้ำภายในวัน

// ====================================================================
// 🟢 ฟังก์ชัน Buzzer Active
// --------------------------------------------------------------------
// Active buzzer แค่จ่าย HIGH ก็มีเสียง
// ใช้ beepActive(ms) สำหรับบี๊บตามเวลาที่กำหนด
// ====================================================================
void beepActive(unsigned ms) {
  digitalWrite(buzzerPin, HIGH); // จ่ายไฟ -> มีเสียง
  delay(ms);                     // ระยะเวลาบี๊บ
  digitalWrite(buzzerPin, LOW);  // ตัดไฟ -> เงียบ
}

/*
----------------------------------------
📌 วิธีปรับแต่งเสียง:

1) บี๊บ 2 จังหวะ:
   beepActive(200);
   delay(100);
   beepActive(200);

2) บี๊บยาวขึ้น:
   beepActive(500);  // 0.5 วินาที

3) บี๊บสั้น:
   beepActive(100);  // 0.1 วินาที

4) หลายจังหวะ:
   beepActive(100);
   delay(50);
   beepActive(100);
   delay(50);
   beepActive(100);
----------------------------------------
*/

// ====================================================================
// 🟢 ฟังก์ชันแสดงเวลาใน LCD
// ====================================================================
void showTime(int h, int m, int s) {
  lcd.setCursor(0, 0);
  lcd.print("ระบบจ่ายยา      "); 
  lcd.setCursor(0, 1);
  if (h < 10) lcd.print('0'); lcd.print(h); lcd.print(':');
  if (m < 10) lcd.print('0'); lcd.print(m); lcd.print(':');
  if (s < 10) lcd.print('0'); lcd.print(s);
  lcd.print("   ");
}

// ====================================================================
// 🟢 ฟังก์ชันจ่ายยา
// ====================================================================
void dispensePill(int channel) {
  Serial.print(">> จ่ายยาช่องที่ "); Serial.println(channel + 1);

  lcd.clear();
  lcd.print("ปลดล็อค ช่อง ");
  lcd.print(channel + 1);

  // เปิดโซลินอยด์ 5 วินาที + บี๊บเตือน
  digitalWrite(solenoidPins[channel], HIGH);
  beepActive(300);    // บี๊บสั้น 0.3 วิ
  delay(4700);        // รวมเป็น 5 วิ
  digitalWrite(solenoidPins[channel], LOW);

  // ถึงเวลาแล้ว → ดับไฟช่องนั้น
  digitalWrite(ledPins[channel], LOW);

  lcd.clear();
  lcd.print("ถึงเวลา: ดับไฟ");
  lcd.setCursor(0, 1);
  lcd.print("ช่อง ");
  lcd.print(channel + 1);
}

// ====================================================================
// 🟢 ฟังก์ชันปลดล็อคทุกช่อง (กดปุ่มค้าง 10 วิ)
// ====================================================================
void unlockAll() {
  Serial.println(">> ปลดล็อคทุกช่อง (กดค้าง 10 วิ)");
  lcd.clear(); lcd.print("ปลดล็อคทุกช่อง");

  for (int i = 0; i < 3; i++) digitalWrite(solenoidPins[i], HIGH);
  digitalWrite(buzzerPin, HIGH);  // เปิดเสียงค้าง
  delay(5000);
  digitalWrite(buzzerPin, LOW);
  for (int i = 0; i < 3; i++) digitalWrite(solenoidPins[i], LOW);

  lcd.clear(); lcd.print("ระบบทำงานต่อ");
}

// ====================================================================
// 🟢 Setup
// ====================================================================
void setup() {
  Serial.begin(9600);
  delay(2000);

  if (!rtc.begin()) {
    Serial.println("ไม่พบ RTC!");
    while (1);
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // ใช้ครั้งแรกเท่านั้น

  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(solenoidPins[i], OUTPUT);
    digitalWrite(solenoidPins[i], LOW);
    digitalWrite(ledPins[i], HIGH);   // ไฟติดเริ่มต้น
  }
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("ระบบจ่ายยา");

  Serial.println("เริ่มระบบเรียบร้อย!");
}

// ====================================================================
// 🟢 Loop
// ====================================================================
void loop() {
  DateTime now = rtc.now();
  int hourTH = (now.hour() + 7) % 24; // เวลาไทย
  int minute = now.minute();
  int second = now.second();

  showTime(hourTH, minute, second);

  // Debug Serial
  Serial.print("เวลาปัจจุบัน: ");
  Serial.print(hourTH); Serial.print(':');
  Serial.print(minute); Serial.print(':');
  Serial.println(second);

  // ตรวจสอบเวลาแจกยา
  for (int i = 0; i < 3; i++) {
    if (hourTH == pillHours[i] && minute == pillMinutes[i] && !hasDispensed[i]) {
      dispensePill(i);
      hasDispensed[i] = true;
    }
  }

  // รีเซ็ตทุกวันตอน 00:00:00
  if (hourTH == 0 && minute == 0 && second == 0) {
    for (int i = 0; i < 3; i++) {
      hasDispensed[i] = false;
      digitalWrite(ledPins[i], HIGH); // เปิดไฟกลับมาติด
    }
    Serial.println(">> รีเซ็ตสถานะประจำวัน + เปิดไฟทุกช่อง");
  }

  // ปุ่มค้าง 10 วิ เพื่อปลดล็อคทุกช่อง
  if (analogRead(buttonPin) < 100) {  // SELECT
    unsigned long pressTime = millis();
    while (analogRead(buttonPin) < 100) {
      if (millis() - pressTime > 10000) {
        unlockAll();
        break;
      }
    }
  }

  delay(200);
}