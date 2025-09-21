#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// กำหนดขา Arduino
#define LED1_PIN 11
#define LED2_PIN 12
#define LED3_PIN 13
#define RELAY1_PIN 5
#define RELAY2_PIN 6
#define RELAY3_PIN 7
#define TOUCH_SENSOR_PIN 8
#define BUZZER_PIN 9

// สร้าง object
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ตัวแปรสำหรับสถานะ
bool scheduledEventOccurred = false; // สถานะสำหรับป้องกันการทำงานซ้ำซ้อนของกำหนดการ
unsigned long touchStartTime = 0;
const unsigned long touchDuration = 5000; // ระยะเวลาในการกดค้างเพื่อเรียกใช้ฟังก์ชันควบคุมทั้งหมด (5 วินาที)
bool isTouching = false;
bool isFunctionExecuted = false; // สถานะเพื่อควบคุมการทำงานของฟังก์ชันหลังจากกดค้าง
unsigned long lastDisplayUpdateTime = 0;
const unsigned long displayUpdateInterval = 1000; // อัปเดตหน้าจอทุก 1 วินาที
bool isCountingDown = false; // สถานะสำหรับการแสดงการนับถอยหลังบนหน้าจอ

// ตัวแปรสำหรับ Buzzer Alert 30 วินาที
unsigned long buzzerAlertStartTime = 0;
bool isBuzzerAlertActive = false;
unsigned long lastBuzzerToggleTime = 0;
const unsigned int buzzerInterval = 200; // สลับสถานะ Buzzer ทุกๆ 200ms

// ตัวแปรสำหรับ Buzzer เมื่อกดปุ่ม (ปรับใหม่)
bool isTouchBuzzerActive = false;
unsigned long lastBuzzerPulseTime = 0;
const unsigned int pulseOnTime = 50; // ระยะเวลาเปิด Buzzer เมื่อกดปุ่ม
const unsigned int pulseOffTime = 950; // ระยะเวลาปิด Buzzer เมื่อกดปุ่ม
bool buzzerOnState = false;

// ตัวแปรสำหรับฟังก์ชัน Reset
unsigned long lastPressTime = 0;
int pressCount = 0;
const unsigned long rapidPressTimeout = 500; // ระยะเวลาสูงสุดระหว่างการกดแต่ละครั้ง (500ms)

// ประกาศฟังก์ชัน
void controlAll();
void controlRelay1();
void controlRelay2();
void controlRelay3();
void controlOff();
void shortBuzzerAlert();
void handleBuzzerAlert();
void print2Digits(int number);
void updateDisplay();
void handleTouch();
void showCountdown();
void handleTouchBuzzer();
void resetSystem();


void setup() {
  Serial.begin(115200);
  
  // กำหนดโหมดของขาพิน
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(TOUCH_SENSOR_PIN, INPUT_PULLUP); // ตั้งค่า Touch Sensor เป็น Input Pull-up
  pinMode(BUZZER_PIN, OUTPUT);

  // ตั้งค่าสถานะเริ่มต้นของอุปกรณ์ทั้งหมดให้เป็น LOW/HIGH
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
  digitalWrite(RELAY1_PIN, HIGH); // RELAY แบบ NC (Normally Closed) ตั้งค่า HIGH เพื่อปิดวงจร
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  // เริ่มต้นใช้งาน LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // แสดงหน้าจอ Welcome ตอนเริ่มต้น
  lcd.setCursor(4, 0);
  lcd.print("Welcome!");
  lcd.setCursor(0, 1);
  lcd.print("Smart Control");
  delay(2000); // แสดงหน้าจอนี้เป็นเวลา 2 วินาที
  lcd.clear();

  // ตรวจสอบการเชื่อมต่อกับโมดูล RTC
  if (!rtc.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("RTC Error!");
    while (1);
  }

  // **** ส่วนสำหรับตั้งเวลา RTC ****
  // **** แก้ไขเวลาในบรรทัดนี้โดยอิงจากเวลาคอมที่ใช้ในการอัปโหลด ****
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
  // *************************************************
  Serial.println("ระบบพร้อมใช้งาน");
}

void loop() {
  unsigned long currentMillis = millis();

  // อัปเดตหน้าจอ LCD เฉพาะเมื่อไม่ได้อยู่ในโหมดนับถอยหลังหรือแจ้งเตือน
  if (!isCountingDown && !isBuzzerAlertActive && currentMillis - lastDisplayUpdateTime >= displayUpdateInterval) {
    updateDisplay();
    lastDisplayUpdateTime = currentMillis;
  }

  DateTime now = rtc.now();

  // ตรวจสอบเวลาเพื่อเริ่มการแจ้งเตือน 30 วินาที
  if (!isBuzzerAlertActive && ((now.hour() == 7 && now.minute() == 59 && now.second() == 30) ||
      (now.hour() == 11 && now.minute() == 53 && now.second() == 30) ||
      (now.hour() == 15 && now.minute() == 59 && now.second() == 30))) {
    isBuzzerAlertActive = true;
    buzzerAlertStartTime = currentMillis;
    lastBuzzerToggleTime = currentMillis;
  }

  // เรียกใช้ฟังก์ชันจัดการ Buzzer Alert
  if (isBuzzerAlertActive) {
    handleBuzzerAlert();
  }
  
  // เรียกใช้ฟังก์ชันจัดการ Buzzer เมื่อกดปุ่ม
  if(isTouchBuzzerActive) {
    handleTouchBuzzer();
  }

  // การทำงานอัตโนมัติ (จะทำงานเมื่อวินาทีเป็น 0)
  if (now.second() == 0) {
    if (now.hour() == 8 && now.minute() == 0) {
      if (!scheduledEventOccurred) {
        controlRelay1();
        scheduledEventOccurred = true;
      }
    } else if (now.hour() == 11 && now.minute() == 54) {
      if (!scheduledEventOccurred) {
        controlRelay2();
        scheduledEventOccurred = true;
      }
    } else if (now.hour() == 16 && now.minute() == 0) {
      if (!scheduledEventOccurred) {
        controlRelay3();
        scheduledEventOccurred = true;
      }
    } else {
      scheduledEventOccurred = false; // รีเซ็ตสถานะเมื่อผ่านเวลาที่กำหนดแล้ว
    }
  }

  handleTouch();
}

// จัดการการแจ้งเตือนด้วย Buzzer เป็นเวลา 30 วินาที
void handleBuzzerAlert() {
  unsigned long currentMillis = millis();
  
  // ตรวจสอบว่าครบ 30 วินาทีหรือยัง
  if (currentMillis - buzzerAlertStartTime >= 30000) {
    isBuzzerAlertActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    updateDisplay();
    return;
  }
  
  // สลับสถานะ Buzzer ทุกๆ 200ms
  if (currentMillis - lastBuzzerToggleTime >= buzzerInterval) {
    digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));
    lastBuzzerToggleTime = currentMillis;
  }
}

// จัดการการทำงานของ Buzzer เมื่อกดปุ่ม
void handleTouchBuzzer() {
  unsigned long currentMillis = millis();
  if (buzzerOnState) {
    if (currentMillis - lastBuzzerPulseTime >= pulseOnTime) {
      digitalWrite(BUZZER_PIN, LOW);
      buzzerOnState = false;
      lastBuzzerPulseTime = currentMillis;
    }
  } else {
    if (currentMillis - lastBuzzerPulseTime >= pulseOffTime) {
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerOnState = true;
      lastBuzzerPulseTime = currentMillis;
    }
  }
}

// จัดการการตรวจจับการกดปุ่ม
void handleTouch() {
  if (digitalRead(TOUCH_SENSOR_PIN) == LOW) {
    if (!isTouching) {
      // ตรวจจับการกดปุ่มเพื่อรีเซ็ต
      unsigned long currentTime = millis();
      if (currentTime - lastPressTime < rapidPressTimeout) {
        pressCount++;
        Serial.print("Press count: ");
        Serial.println(pressCount);
        if (pressCount >= 3) {
          resetSystem();
          lastPressTime = currentTime; 
          pressCount = 0; 
          return;
        }
      } else {
        pressCount = 1;
      }
      lastPressTime = currentTime;
      
      isTouching = true;
      touchStartTime = millis();
      Serial.println("เริ่มสัมผัส...");
      isCountingDown = true;
      isTouchBuzzerActive = true;
      lastBuzzerPulseTime = millis();
      buzzerOnState = false;
    }
    
    showCountdown();

    // ตรวจสอบการกดค้างเกิน 5 วินาที
    if ((millis() - touchStartTime) >= touchDuration && !isFunctionExecuted) {
      isTouchBuzzerActive = false;
      digitalWrite(BUZZER_PIN, LOW);
      controlAll(); // เรียกใช้ฟังก์ชันควบคุมทั้งหมด
      isFunctionExecuted = true;
      isCountingDown = false;
    }
  } else {
    if (isTouching) {
      Serial.println("หยุดสัมผัส");
      isTouching = false;
      isFunctionExecuted = false;
      isCountingDown = false;
      isTouchBuzzerActive = false;
      digitalWrite(BUZZER_PIN, LOW);
      updateDisplay();
    }
  }
}

// แสดงการนับถอยหลังบนหน้าจอ LCD
void showCountdown() {
  unsigned long timePassed = millis() - touchStartTime;
  int countdownValue = 5 - (timePassed / 1000);

  lcd.clear(); 
  lcd.setCursor(0, 0);
  lcd.print("Countdown: "); 
  
  // แสดงผลบนบรรทัดที่ 2
  lcd.setCursor(0, 1);
  lcd.print(countdownValue); 
  lcd.print("s "); // เพิ่ม space ด้านหลังเพื่อลบตัวอักษรเก่า
}

// ส่งสัญญาณ Buzzer สั้นๆ 3 ครั้ง
void shortBuzzerAlert() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

// ควบคุมการทำงานของ Relay และ LED ทั้งหมดเมื่อกดค้าง 5 วินาที
void controlAll() {
  Serial.println("สัมผัสครบ 5 วินาที");
  digitalWrite(BUZZER_PIN, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Devices ON");
  delay(1000); 
  digitalWrite(BUZZER_PIN, LOW);

  // ควบคุม Relay 1 และ LED 1
  Serial.println("BOX 1 ON!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BOX 1 ON!");
  digitalWrite(RELAY1_PIN, LOW);
  delay(100);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(LED1_PIN, HIGH);
  delay(1000);
  
  // ควบคุม Relay 2 และ LED 2
  Serial.println("BOX 2 ON!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BOX 2 ON!");
  digitalWrite(RELAY2_PIN, LOW);
  delay(100);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);
  delay(1000);
  
  // ควบคุม Relay 3 และ LED 3
  Serial.println("BOX 3 ON!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BOX 3 ON!");
  digitalWrite(RELAY3_PIN, LOW);
  delay(100);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(LED3_PIN, HIGH);
  delay(1000);
  
  // สั่งให้ LED ทั้งหมดดับลง
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
  updateDisplay();
}

// ควบคุม Relay 1 ตามกำหนดการ
void controlRelay1() {
  Serial.println("Scheduled event: BOX 1 ON!");
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(LED1_PIN, HIGH);

  shortBuzzerAlert();
  delay(3000);

  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(LED1_PIN, LOW);

  updateDisplay();
}

// ควบคุม Relay 2 ตามกำหนดการ
void controlRelay2() {
  Serial.println("Scheduled event: BOX 2 ON!");
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(LED2_PIN, HIGH);

  shortBuzzerAlert();
  delay(3000);

  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(LED2_PIN, LOW);

  updateDisplay();
}

// ควบคุม Relay 3 ตามกำหนดการ
void controlRelay3() {
  Serial.println("Scheduled event: BOX 3 ON!");
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(LED3_PIN, HIGH);

  shortBuzzerAlert();
  delay(3000);

  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(LED3_PIN, LOW);

  updateDisplay();
}

// ฟังก์ชันสำหรับแสดงตัวเลขที่มี 2 หลักบน LCD
void print2Digits(int number) {
  if (number < 10) {
    lcd.print("0");
  }
  lcd.print(number);
}

// ฟังก์ชันสำหรับอัปเดตข้อมูลเวลาและวันที่บนหน้าจอ LCD
void updateDisplay() {
  DateTime now = rtc.now();

  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  print2Digits(now.hour());
  lcd.print(":");
  print2Digits(now.minute());
  lcd.print(":");
  print2Digits(now.second());

  lcd.setCursor(0, 1);
  lcd.print("Date: ");
  lcd.print(now.day(), DEC);
  lcd.print("/");
  lcd.print(now.month(), DEC);
  lcd.print("/");
  lcd.print(now.year(), DEC);
}

// ฟังก์ชันสำหรับรีเซ็ตระบบ
void resetSystem() {
  Serial.println("System Reset Initiated!");
  
  // ปิดการทำงานทั้งหมด
  digitalWrite(RELAY1_PIN, HIGH); // ปิดการทำงานของรีเลย์
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(LED1_PIN, LOW); // ปิด LED
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW); // ปิด Buzzer
  
  // แสดงข้อความรีเซ็ต
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Reset!");
  lcd.setCursor(0, 1);
  lcd.print("Please wait...");
  delay(2000);
  
  // รีเซ็ตตัวแปรสถานะทั้งหมด
  scheduledEventOccurred = false;
  isTouching = false;
  isFunctionExecuted = false;
  isCountingDown = false;
  isBuzzerAlertActive = false;
  isTouchBuzzerActive = false;

  updateDisplay(); // กลับไปแสดงเวลาบนหน้าจอ
}