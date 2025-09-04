# -Smart Pill Dispenser (Automatic 3-Channel Medicine Box)
⸻
💊 Smart Pill Dispenser (Automatic 3-Channel Medicine Box)

This project is an Automatic Pill Dispenser designed to help users take medicine on time.
It is built using Arduino UNO R3, RTC DS3231, and an LCD 16x2 Keypad Shield.
The system dispenses pills 3 times per day (08:00, 12:00, 16:00).

✅ Features:
	•	Real-Time Clock (RTC DS3231) for accurate timing
	•	3 Solenoid locks (one for each pill compartment)
	•	LED indicators for each channel
	•	Buzzer alarm notification
	•	LCD display for real-time clock & status
	•	Manual unlock (press SELECT button for 10 seconds)

⸻

🛠 Hardware Components
	•	Arduino UNO R3
	•	RTC DS3231 (I2C)
	•	LCD 16x2 Keypad Shield (Blue Screen)
	•	3 × Solenoid Locks + Delay Module 5V
	•	3 × LEDs (Red, Yellow, Green) + 220Ω Resistors
	•	1 × Active Buzzer 5V
	•	Breadboard + Jumper Wires (M-M, M-F, F-F)
	•	5V Adapter or Power Bank

⸻

⚡ Circuit Connections

RTC DS3231
	•	VCC → 5V
	•	GND → GND
	•	SDA → A4
	•	SCL → A5

LEDs
	•	LED1 → Pin 2 (via 220Ω resistor → GND)
	•	LED2 → Pin 3 (via 220Ω resistor → GND)
	•	LED3 → Pin 4 (via 220Ω resistor → GND)

Solenoid Locks (via Delay Module 5V)
	•	IN1 → Pin 5
	•	IN2 → Pin 6
	•	IN3 → Pin 7
	•	VCC → 5V
	•	GND → GND
	•	OUT → Solenoid Lock

Buzzer
	•	+ → Pin 10
	•	- → GND

LCD 16x2 Keypad Shield
	•	Plug directly into Arduino UNO

⸻

💻 Software Setup
	1.	Install Arduino IDE
	2.	Install RTClib Library (Adafruit)
	•	Open Arduino IDE → Tools → Manage Libraries → Search RTClib → Install
	3.	Open the code file SmartPillDispenser.ino

⸻

🔄 Uploading the Code
	1.	Connect Arduino UNO via USB
	2.	Select Board: Arduino UNO
	3.	Select the correct Port (e.g. COM3 / /dev/ttyUSB0)
	4.	Click Upload
	5.	Open Serial Monitor (9600 baud) → Monitor debug messages

⸻

🔔 How It Works
	•	Dispenses pills automatically at: 08:00, 12:00, 16:00
	•	When dispensing:
	•	Corresponding LED turns ON
	•	Solenoid unlocks pill box
	•	Buzzer alarm sounds
	•	LCD shows "Unlock Channel X"
	•	Hold SELECT button for 10 seconds → Unlocks all channels (for refilling)
	•	After dispensing, system resets and waits for the next cycle

⸻

📌 Notes
	•	If Solenoid consumes high current, use 12V Adapter + Relay Module instead of Delay Module
	•	Ensure all grounds (GND) are properly connected
	•	To sync RTC time with your PC (first upload only), uncomment this line in code:

rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

Then re-upload

⸻

🎯 Project Workflow
	1.	RTC keeps track of real-time
	2.	Arduino checks schedule (08:00, 12:00, 16:00)
	3.	At scheduled time → Activate LED + Solenoid + Buzzer
	4.	LCD + Serial Monitor show real-time status
	5.	Manual refill → Hold button 10 sec → Unlock all channels

⸻

👨‍💻 Author
	•	Ayub Saheebatu
Computer Engineering 

⸻
