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
bool scheduledEventOccurred = false;
unsigned long touchStartTime = 0;
const unsigned long touchDuration = 5000;
bool isTouching = false;
bool isFunctionExecuted = false;
unsigned long lastDisplayUpdateTime = 0;
const unsigned long displayUpdateInterval = 1000;
bool isCountingDown = false;

// ตัวแปรสำหรับ Buzzer Alert 30 วินาที
unsigned long buzzerAlertStartTime = 0;
bool isBuzzerAlertActive = false;
unsigned long lastBuzzerToggleTime = 0;
const unsigned int buzzerInterval = 200;

// ตัวแปรสำหรับ Buzzer เมื่อกดปุ่ม
bool isTouchBuzzerActive = false;
unsigned long lastBuzzerPulseTime = 0;
const unsigned int pulseOnTime = 50;
const unsigned int pulseOffTime = 950;
bool buzzerOnState = false;

// ตัวแปรสำหรับฟังก์ชัน Reset
unsigned long lastPressTime = 0;
int pressCount = 0;
const unsigned long rapidPressTimeout = 500;

// ตัวแปรควบคุม Relay แบบ Non-blocking
unsigned long relayControlStartTime = 0;
int controlState = 0; // 0=Idle, 1=ControlAll, 2=ControlRelay1, 3=ControlRelay2, 4=ControlRelay3
int controlStep = 0;

// ประกาศฟังก์ชัน
void handleNonBlockingControl();
void startControlAll();
void startControlRelay1();
void startControlRelay2();
void startControlRelay3();
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
  
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(TOUCH_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  lcd.setCursor(4, 0);
  lcd.print("Welcome!");
  lcd.setCursor(0, 1);
  lcd.print("Smart Control");
  delay(2000);
  lcd.clear();

  if (!rtc.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("RTC Error!");
    while (1);
  }

  // **** ตั้งเวลา RTC เพียงครั้งเดียว! หลังจากนั้นคอมเมนต์บรรทัดนี้ออก ****
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // *************************************************

  Serial.println("ระบบพร้อมใช้งาน");
}

void loop() {
  unsigned long currentMillis = millis();

  // อัปเดตหน้าจอ LCD
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
        startControlRelay1();
        scheduledEventOccurred = true;
      }
    } else if (now.hour() == 11 && now.minute() == 54) {
      if (!scheduledEventOccurred) {
        startControlRelay2();
        scheduledEventOccurred = true;
      }
    } else if (now.hour() == 16 && now.minute() == 0) {
      if (!scheduledEventOccurred) {
        startControlRelay3();
        scheduledEventOccurred = true;
      }
    } else {
      scheduledEventOccurred = false;
    }
  }

  // จัดการการควบคุมแบบ Non-blocking
  handleNonBlockingControl();

  handleTouch();
}

// จัดการการแจ้งเตือนด้วย Buzzer เป็นเวลา 30 วินาที
void handleBuzzerAlert() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - buzzerAlertStartTime >= 30000) {
    isBuzzerAlertActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    updateDisplay();
    return;
  }
  
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

    if ((millis() - touchStartTime) >= touchDuration && !isFunctionExecuted) {
      isTouchBuzzerActive = false;
      digitalWrite(BUZZER_PIN, LOW);
      startControlAll();
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
  
  lcd.setCursor(0, 1);
  lcd.print(countdownValue); 
  lcd.print("s ");
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

// ฟังก์ชันเริ่มต้นการควบคุมทั้งหมดแบบ Non-blocking
void startControlAll() {
  Serial.println("Starting all relays non-blocking...");
  relayControlStartTime = millis();
  controlState = 1; // สถานะ Control All
  controlStep = 0;
}

// ฟังก์ชันเริ่มต้นการควบคุม Relay 1
void startControlRelay1() {
  Serial.println("Starting Relay 1 non-blocking...");
  relayControlStartTime = millis();
  controlState = 2; // สถานะ Control Relay 1
  controlStep = 0;
}

// ฟังก์ชันเริ่มต้นการควบคุม Relay 2
void startControlRelay2() {
  Serial.println("Starting Relay 2 non-blocking...");
  relayControlStartTime = millis();
  controlState = 3; // สถานะ Control Relay 2
  controlStep = 0;
}

// ฟังก์ชันเริ่มต้นการควบคุม Relay 3
void startControlRelay3() {
  Serial.println("Starting Relay 3 non-blocking...");
  relayControlStartTime = millis();
  controlState = 4; // สถานะ Control Relay 3
  controlStep = 0;
}

// ฟังก์ชันจัดการการควบคุมแบบ Non-blocking
void handleNonBlockingControl() {
  unsigned long currentMillis = millis();
  
  switch (controlState) {
    case 1: // ControlAll
      switch (controlStep) {
        case 0:
          digitalWrite(BUZZER_PIN, HIGH);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Devices ON");
          relayControlStartTime = currentMillis;
          controlStep = 1;
          break;
        case 1:
          if (currentMillis - relayControlStartTime >= 1000) {
            digitalWrite(BUZZER_PIN, LOW);
            Serial.println("BOX 1 ON!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("BOX 1 ON!");
            digitalWrite(RELAY1_PIN, LOW);
            digitalWrite(LED1_PIN, HIGH);
            relayControlStartTime = currentMillis;
            controlStep = 2;
          }
          break;
        case 2:
          if (currentMillis - relayControlStartTime >= 1000) {
            Serial.println("BOX 2 ON!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("BOX 2 ON!");
            digitalWrite(RELAY2_PIN, LOW);
            digitalWrite(LED2_PIN, HIGH);
            relayControlStartTime = currentMillis;
            controlStep = 3;
          }
          break;
        case 3:
          if (currentMillis - relayControlStartTime >= 1000) {
            Serial.println("BOX 3 ON!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("BOX 3 ON!");
            digitalWrite(RELAY3_PIN, LOW);
            digitalWrite(LED3_PIN, HIGH);
            relayControlStartTime = currentMillis;
            controlStep = 4;
          }
          break;
        case 4:
          if (currentMillis - relayControlStartTime >= 1000) {
            digitalWrite(LED1_PIN, LOW);
            digitalWrite(LED2_PIN, LOW);
            digitalWrite(LED3_PIN, LOW);
            digitalWrite(RELAY1_PIN, HIGH);
            digitalWrite(RELAY2_PIN, HIGH);
            digitalWrite(RELAY3_PIN, HIGH);
            Serial.println("ControlAll finished.");
            controlState = 0;
            updateDisplay();
          }
          break;
      }
      break;

    case 2: // ControlRelay1
      if (controlStep == 0) {
        Serial.println("Scheduled event: BOX 1 ON!");
        digitalWrite(RELAY1_PIN, LOW);
        digitalWrite(LED1_PIN, HIGH);
        shortBuzzerAlert(); // Note: This function still uses delay, but it's short. For a fully non-blocking version, you would also need to handle this with millis().
        relayControlStartTime = currentMillis;
        controlStep = 1;
      }
      if (currentMillis - relayControlStartTime >= 3000) {
        digitalWrite(RELAY1_PIN, HIGH);
        digitalWrite(LED1_PIN, LOW);
        Serial.println("Relay 1 OFF.");
        controlState = 0;
        updateDisplay();
      }
      break;

    case 3: // ControlRelay2
      if (controlStep == 0) {
        Serial.println("Scheduled event: BOX 2 ON!");
        digitalWrite(RELAY2_PIN, LOW);
        digitalWrite(LED2_PIN, HIGH);
        shortBuzzerAlert();
        relayControlStartTime = currentMillis;
        controlStep = 1;
      }
      if (currentMillis - relayControlStartTime >= 3000) {
        digitalWrite(RELAY2_PIN, HIGH);
        digitalWrite(LED2_PIN, LOW);
        Serial.println("Relay 2 OFF.");
        controlState = 0;
        updateDisplay();
      }
      break;

    case 4: // ControlRelay3
      if (controlStep == 0) {
        Serial.println("Scheduled event: BOX 3 ON!");
        digitalWrite(RELAY3_PIN, LOW);
        digitalWrite(LED3_PIN, HIGH);
        shortBuzzerAlert();
        relayControlStartTime = currentMillis;
        controlStep = 1;
      }
      if (currentMillis - relayControlStartTime >= 3000) {
        digitalWrite(RELAY3_PIN, HIGH);
        digitalWrite(LED3_PIN, LOW);
        Serial.println("Relay 3 OFF.");
        controlState = 0;
        updateDisplay();
      }
      break;
  }
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
  
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Reset!");
  lcd.setCursor(0, 1);
  lcd.print("Please wait...");
  delay(2000); // This delay is acceptable here since it's a one-time reset action
  
  scheduledEventOccurred = false;
  isTouching = false;
  isFunctionExecuted = false;
  isCountingDown = false;
  isBuzzerAlertActive = false;
  isTouchBuzzerActive = false;
  controlState = 0;
  controlStep = 0;

  updateDisplay();
}