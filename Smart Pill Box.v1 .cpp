#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>

// ---------- RTC ----------
RTC_DS3231 rtc;

// ---------- LCD Shield ----------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ---------- Pin Setup ----------
const int ledPins[3]      = {2, 3, 11};     // LED แยกช่อง
const int solenoidPins[3] = {12, 13, A1};   // ผ่านโมดูล Delay
const int buzzerPin       = 10;             // Buzzer
const int buttonPin       = A0;             // ปุ่มบน LCD Shield

const int DOOR_PIN        = A3;             // สวิตช์ประตู: INPUT_PULLUP (LOW = เปิดประตู)

// ---------- เวลาแจกยา ----------
int pillHours[3]   = {8, 12, 16};  // 08:00, 12:00, 16:00
int pillMinutes[3] = {0, 0, 0};    // xx:00

// ---------- ตัวแปรสถานะ ----------
bool hasDispensed[3]   = {false, false, false};  // กันจ่ายซ้ำในวันเดียวกัน
bool awaitingPickup[3] = {false, false, false};  // ไฟค้างรอเปิดประตู

// ---------- Utilities ----------
void beep(unsigned freq, unsigned ms) {
  tone(buzzerPin, freq);
  delay(ms);
  noTone(buzzerPin);
}

void showTime(int h, int m, int s) {
  lcd.setCursor(0, 0);
  lcd.print("ระบบจ่ายยา      "); // เคลียร์คร่าว ๆ
  lcd.setCursor(0, 1);
  if (h < 10) lcd.print('0'); lcd.print(h); lcd.print(':');
  if (m < 10) lcd.print('0'); lcd.print(m); lcd.print(':');
  if (s < 10) lcd.print('0'); lcd.print(s);
  lcd.print("   ");
}

// ---------- ฟังก์ชันจ่ายยา ----------
void dispensePill(int channel) {
  Serial.print(">> จ่ายยาช่องที่ "); Serial.println(channel + 1);

  lcd.clear();
  lcd.print("ปลดล็อค ช่อง ");
  lcd.print(channel + 1);

  // ปลดล็อก + เตือน
  digitalWrite(solenoidPins[channel], HIGH);
  beep(1200, 300);
  delay(4700); // รวม ~5s

  digitalWrite(solenoidPins[channel], LOW);

  // เข้าสถานะ "รอรับยา" → ไฟติดค้างจนกว่าจะเปิดประตู
  digitalWrite(ledPins[channel], HIGH);
  awaitingPickup[channel] = true;

  lcd.clear();
  lcd.print("รอเปิดประตูรับยา");
}

// ---------- ปลดล็อคทุกช่องด้วยปุ่มค้าง ----------
void unlockAll() {
  Serial.println(">> ปลดล็อคทุกช่อง (กดค้าง 10 วิ)");
  lcd.clear(); lcd.print("ปลดล็อคทุกช่อง");

  for (int i = 0; i < 3; i++) digitalWrite(solenoidPins[i], HIGH);
  tone(buzzerPin, 1500);
  delay(5000);
  for (int i = 0; i < 3; i++) digitalWrite(solenoidPins[i], LOW);
  noTone(buzzerPin);

  lcd.clear(); lcd.print("ระบบทำงานต่อ");
}

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);
  delay(2000);

  if (!rtc.begin()) {
    Serial.println("ไม่พบ RTC!");
    while (1);
  }
  // ใช้ครั้งแรกค่อยปลดคอมเมนต์บรรทัดล่างเพื่อ set เวลา
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(solenoidPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
    digitalWrite(solenoidPins[i], LOW);
  }
  pinMode(buzzerPin, OUTPUT);
  pinMode(DOOR_PIN, INPUT_PULLUP);  // HIGH = ปิดประตู, LOW = เปิดประตู

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("ระบบจ่ายยา");

  Serial.println("เริ่มระบบเรียบร้อย!");
}

// ---------- Loop ----------
void loop() {
  DateTime now = rtc.now();
  int hourTH = (now.hour() + 7) % 24; // เวลาไทย
  int minute = now.minute();
  int second = now.second();

  showTime(hourTH, minute, second);

  // ----- Debug -----
  Serial.print("เวลาปัจจุบัน: ");
  Serial.print(hourTH); Serial.print(':');
  Serial.print(minute); Serial.print(':');
  Serial.println(second);

  // ----- ถึงเวลาแจกยา? -----
  for (int i = 0; i < 3; i++) {
    if (hourTH == pillHours[i] && minute == pillMinutes[i] && !hasDispensed[i]) {
      dispensePill(i);
      hasDispensed[i] = true;
    }
  }

  // ----- ตรวจจับการเปิดประตูเพื่อ "รับยา" -----
  bool doorOpen = (digitalRead(DOOR_PIN) == LOW); // LOW = เปิด
  if (doorOpen) {
    // เมื่อเปิดประตู ให้ดับไฟทุกช่องที่กำลังรอรับ
    for (int i = 0; i < 3; i++) {
      if (awaitingPickup[i]) {
        digitalWrite(ledPins[i], LOW);
        awaitingPickup[i] = false;
        Serial.print(">> รับยาช่อง "); Serial.print(i + 1); Serial.println(" แล้ว (ดับไฟ)");
      }
    }
  }

  // ----- รีเซ็ตสถานะทุกวันตอน 00:00:00 -----
  if (hourTH == 0 && minute == 0 && second == 0) {
    for (int i = 0; i < 3; i++) {
      hasDispensed[i]   = false;
      awaitingPickup[i] = false;
      digitalWrite(ledPins[i], LOW);
    }
    Serial.println(">> รีเซ็ตสถานะประจำวัน");
  }

  // ----- ปุ่มค้าง 10 วิ เพื่อปลดล็อคทุกช่อง -----
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