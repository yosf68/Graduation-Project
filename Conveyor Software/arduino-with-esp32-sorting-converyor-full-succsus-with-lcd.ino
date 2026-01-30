#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// إنشاء كائن شاشة LCD (تأكد من عنوان الـ I2C الصحيح: 0x27 أو 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// تعاريف المداخل
#define IR_SENSOR 2       // حساس IR
#define COLOR_YELLOW 3     // من ESP32-CAM
#define COLOR_BLUE 4
#define COLOR_GREEN 5

// تعريف المحرك (عبر L298N)
#define MOTOR_IN1 8
#define MOTOR_IN2 12
#define MOTOR_ENA 13   // للتحكم بالسرعة (PWM)

// السيرفوهات
Servo servoYellow;
Servo servoBlue;
Servo servoGreen;

// متغيرات زمن التأخير (قابلة للتعديل)
unsigned long delayYellow = 6000; // 6 ثواني
unsigned long delayBlue = 2200;   // 2.2 ثواني
unsigned long delayGreen = 8200;  // 8.2 ثواني

// متغير لتتبع حالة بدء تشغيل السير
bool conveyorStarted = false;

void setup() {
  Serial.begin(9600);

  // إعداد شاشة LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Color Sort System");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");

  pinMode(IR_SENSOR, INPUT);
  pinMode(COLOR_YELLOW, INPUT);
  pinMode(COLOR_BLUE, INPUT);
  pinMode(COLOR_GREEN, INPUT);

  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);

  servoYellow.attach(6);
  servoBlue.attach(7);
  servoGreen.attach(9);

  stopMotor();
  servoYellow.write(0);
  servoBlue.write(180);
  servoGreen.write(180);
}

void loop() {
  int irState = digitalRead(IR_SENSOR);

  // تشغيل السير مرة واحدة فقط عند أول اكتشاف لجسم
  if (!conveyorStarted && irState == LOW) {
    runMotor();
    conveyorStarted = true;
    Serial.println("Conveyor started for the first time and will not stop.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conveyor Running");
  }

  // تحقق من اللون القادم من ESP32
  if (digitalRead(COLOR_YELLOW) == HIGH) {
    handleGate(servoYellow, delayYellow, "YELLOW");
  }
  else if (digitalRead(COLOR_BLUE) == HIGH) {
    handleGate(servoBlue, delayBlue, "BLUE");
  }
  else if (digitalRead(COLOR_GREEN) == HIGH) {
    handleGate(servoGreen, delayGreen, "GREEN");
  }
}

void runMotor() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_ENA, 255); // السرعة (0–255)
}

void stopMotor() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
}

void handleGate(Servo &servo, unsigned long delayTime, String colorName) {
  Serial.print("Opening gate for color: ");
  Serial.println(colorName);

  // عرض اللون الجاري على الشاشة
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sorting: ");
  lcd.print(colorName);

  delay(delayTime);  // الانتظار حتى يصل المنتج أمام البوابة

  // السيرفو الخاص باللون الأصفر يعمل بالعكس
  if (colorName == "YELLOW") {
    for (int angle = 0; angle <= 70; angle += 2) {
      servo.write(angle);
      delay(15);
    }
    delay(2000);
    for (int angle = 70; angle >= 0; angle -= 2) {
      servo.write(angle);
      delay(15);
    }
  } 
  else {
    for (int angle = 180; angle >= 120; angle -= 2) {
      servo.write(angle);
      delay(15);
    }
    delay(2000);
    for (int angle = 120; angle <= 180; angle += 2) {
      servo.write(angle);
      delay(15);
    }
  }

  Serial.println("Gate closed.");

  // عرض على الشاشة عند الإغلاق
  lcd.setCursor(0, 1);
  lcd.print("Gate Closed");
  delay(1500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conveyor Running");
}
