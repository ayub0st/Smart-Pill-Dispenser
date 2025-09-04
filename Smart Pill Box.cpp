#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>

// ---------- RTC ----------
RTC_DS3231 rtc;

// ---------- LCD Shield ----------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ---------- Pin Setup ----------
const int ledPins[3] = {2, 3, 4};        // LED แต่ละช่องยา
const int solenoidPins[3] = {5, 6, 7};   // ควบคุมผ่านโมดูล Delay
const int buzzerPin = 10;                // Buzzer
const int buttonPin = A0;                // ปุ่มจาก LCD Shield

// ---------- เวลาแจกยา ----------
int pillHours[3] = {8, 12, 16};          // 08:00, 12:00, 16:00
int pillMinutes[3] = {0, 0, 0};          // นาที = 00 ทั้งหมด

// ---------- ตัวแปร ----------
bool hasDispensed[3] = {false, false, false};  // กันการปลดล็อคซ้ำ

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);
  delay(2000);

  if (!rtc.begin()) {
    Serial.println("ไม่พบ RTC!");
    while (1);
  }

  // ตั้งเวลาอัตโนมัติจากคอม (ครั้งแรก)
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  lcd.begin(16, 2);
  lcd.print("ระบบจ่ายยา");

  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(solenoidPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
    digitalWrite(solenoidPins[i], LOW);
  }
  pinMode(buzzerPin, OUTPUT);

  Serial.println("เริ่มระบบเรียบร้อย!");
}

// ---------- Loop ----------
void loop() {
  DateTime now = rtc.now();
  int hourTH = (now.hour() + 7) % 24; // เวลาไทย
  int minute = now.minute();
  int second = now.second();

  // ----- แสดงเวลาใน LCD -----
  lcd.setCursor(0, 1);
  if (hourTH < 10) lcd.print('0');
  lcd.print(hourTH);
  lcd.print(':');
  if (minute < 10) lcd.print('0');
  lcd.print(minute);
  lcd.print(':');
  if (second < 10) lcd.print('0');
  lcd.print(second);

  // ----- Debug Serial -----
  Serial.print("เวลาปัจจุบัน: ");
  Serial.print(hourTH);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);

  // ----- ตรวจสอบเวลาแจกยา -----
  for (int i = 0; i < 3; i++) {
    if (hourTH == pillHours[i] && minute == pillMinutes[i] && !hasDispensed[i]) {
      dispensePill(i);
      hasDispensed[i] = true;
    }

    // รีเซ็ต flag ทุกวันเที่ยงคืน
    if (hourTH == 0 && minute == 0 && second == 0) {
      hasDispensed[i] = false;
    }
  }

  // ----- ตรวจสอบปุ่ม (กดค้าง 10 วิ เพื่อปลดล็อคทุกช่อง) -----
  if (analogRead(buttonPin) < 100) {  // ปุ่ม SELECT
    unsigned long pressTime = millis();
    while (analogRead(buttonPin) < 100) {
      if (millis() - pressTime > 10000) {
        unlockAll();
        break;
      }
    }
  }

  delay(500);
}

// ---------- ฟังก์ชันจ่ายยา ----------
void dispensePill(int channel) {
  Serial.print(">> จ่ายยาช่องที่ ");
  Serial.println(channel + 1);

  lcd.clear();
  lcd.print("ปลดล็อค ช่อง ");
  lcd.print(channel + 1);

  digitalWrite(ledPins[channel], HIGH);
  digitalWrite(solenoidPins[channel], HIGH);
  tone(buzzerPin, 1000);  // เสียงเตือน

  delay(5000);  // เปิด 5 วิ

  digitalWrite(ledPins[channel], LOW);
  digitalWrite(solenoidPins[channel], LOW);
  noTone(buzzerPin);

  lcd.clear();
  lcd.print("รอรอบถัดไป");
}

// ---------- ปลดล็อคทุกช่อง ----------
void unlockAll() {
  Serial.println(">> กดปุ่ม 10 วิ -> ปลดล็อคทุกช่อง!!");

  lcd.clear();
  lcd.print("ปลดล็อคทุกช่อง");

  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], HIGH);
    digitalWrite(solenoidPins[i], HIGH);
  }
  tone(buzzerPin, 1500);
  delay(5000);

  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], LOW);
    digitalWrite(solenoidPins[i], LOW);
  }
  noTone(buzzerPin);

  lcd.clear();
  lcd.print("ระบบทำงานต่อ");
}
