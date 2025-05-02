/******************************************************************************
 *  Arduino Coffee / Milk / Tea Dispenser
 *  ───────────────────────────────────────
 *  • RFID (MFRC522)  or  4 × 4 Keypad authentication
 *  • 16 × 2 I²C LCD user interface
 *  • Three relays → peristaltic pumps: Café (13) ‑ Lait (12) ‑ Thé (11)
 *  • SG90 servo → sugar gate  (pin 45)
 *  • HC‑SR04 ultrasonic sensor → cup presence (TRIG 44, ECHO 43)
 *  • Piezo buzzer for feedback (46)
 *
 *  MIT License – 2025 Aymen El Achhab
 *  Use at your own risk; mains voltage is involved!
 ******************************************************************************/

#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Ultrasonic.h>          // “Ultrasonic by Erick Simões” library

/* ─── Pin map ─────────────────────────────────────────────────────────── */
#define SS_PIN        53
#define RST_PIN       48

#define ROW_NUM       4
#define COLUMN_NUM    4

#define TRIG_PIN      44
#define ECHO_PIN      43
#define BUZZER_PIN    46

const int relayCafe = 13;
const int relayLait = 12;
const int relayThe  = 11;

const int sugarServoPin = 45;

/* ─── Global objects ──────────────────────────────────────────────────── */
MFRC522 rfid(SS_PIN, RST_PIN);

char keys[ROW_NUM][COLUMN_NUM] = {
  {'D', 'C', 'B', 'A'},
  {'#', '9', '6', '3'},
  {'0', '8', '5', '2'},
  {'*', '7', '4', '1'}
};
byte pin_rows[ROW_NUM]      = {2, 3, 4, 5};
byte pin_column[COLUMN_NUM] = {6, 7, 8, 9};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column,
                       ROW_NUM, COLUMN_NUM);

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo            sugarServo;
Ultrasonic       ultrasonic(TRIG_PIN, ECHO_PIN);   // cm mode by default

/* ─── Access control data ─────────────────────────────────────────────── */
String password        = "1234";
String enteredPassword = "";
bool   accessGranted   = false;
byte   allowedUID[]    = {0x33, 0xBA, 0xDD, 0xA9};   // change to your tag

/* ─────────────────────────────────────────────────────────────────────── */
void displayPrompt();

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  sugarServo.attach(sugarServoPin);
  sugarServo.write(90);          // gate closed

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(relayCafe, OUTPUT);
  pinMode(relayLait, OUTPUT);
  pinMode(relayThe , OUTPUT);

  lcd.init();
  lcd.backlight();
  displayPrompt();
}

void loop() {
  if (!accessGranted) {
    checkAccess();
  } else {
    showMenu();
  }
}

/* ─── UI helpers ─────────────────────────────────────────────────────── */
void displayPrompt() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID or");
  lcd.setCursor(0, 1);
  lcd.print("Enter Password");
}

/* ─── Authentication ─────────────────────────────────────────────────── */
void checkAccess() {
  /* 1️⃣  RFID path */
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("RFID UID:");
    bool authorized = true;
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(rfid.uid.uidByte[i], HEX);
      if (rfid.uid.uidByte[i] != allowedUID[i]) authorized = false;
    }
    Serial.println();

    if (authorized) {
      accessGranted = true;
      lcd.clear(); lcd.print("Access Granted");
      delay(2000); lcd.clear();
    } else {
      lcd.clear(); lcd.print("Access Denied");
      tone(BUZZER_PIN, 2000); delay(1000);
      noTone(BUZZER_PIN);    delay(1400);
      displayPrompt();
    }
    rfid.PICC_HaltA();     // halt card
    return;
  }

  /* 2️⃣  Keypad path */
  checkPassword();
}

void checkPassword() {
  char key = keypad.getKey();
  if (!key) return;

  if (key == '#') {                                 // submit
    if (enteredPassword == password) {
      accessGranted = true;
      lcd.clear(); lcd.print("Access Granted");
      enteredPassword = "";
      delay(2000); lcd.clear();
    } else {
      lcd.clear(); lcd.print("Access Denied");
      enteredPassword = "";
      tone(BUZZER_PIN, 2000); delay(1000);
      noTone(BUZZER_PIN);    delay(1400);
      displayPrompt();
    }
  }
  else if (key == '*') {                            // clear
    enteredPassword = "";
    displayPrompt();
  }
  else {                                            // collect digit
    enteredPassword += key;
    lcd.clear();
    lcd.setCursor(6, 1);
    for (unsigned int i = 0; i < enteredPassword.length(); i++) lcd.print('*');
  }
}

/* ─── Menu ───────────────────────────────────────────────────────────── */
void showMenu() {
  lcd.clear();
  lcd.print("1:Cafe 2:C-Lait");
  lcd.setCursor(0, 1);
  lcd.print("3:The  4:T-Lait");

  char key = keypad.waitForKey();
  switch (key) {
    case '1': pourCafe();        break;
    case '2': pourCafeAuLait();  break;
    case '3': pourThe();         break;
    case '4': pourTheAuLait();   break;
    default:
      lcd.clear(); lcd.print("Invalid choice");
      delay(2000); showMenu();
  }
}

/* ─── Peripherals ─────────────────────────────────────────────────────── */
bool isGlassPresent() {
  int distance = ultrasonic.Ranging(CM);
  delay(100);
  return (distance < 3);         // <3 cm → cup detected
}

void addSugar() {
  lcd.clear(); lcd.print("Sugar Qty (0-3):");
  int sugarQty = 0;
  for (int i = 0; i < 3; i++) {
    char key = keypad.waitForKey();
    if (key >= '0' && key <= '3') { sugarQty = key - '0'; break; }
  }
  for (int i = 0; i < sugarQty; i++) {
    sugarServo.write(180);  delay(500);
    sugarServo.write(0);    delay(500);
  }
}

/* ─── Drink routines ─────────────────────────────────────────────────── */
void pourCafe() {
  do { lcd.clear(); lcd.print("Place a glass"); }
  while (!isGlassPresent());

  lcd.clear(); lcd.print("Adding Sugar..."); addSugar();
  lcd.clear(); lcd.print("Pouring Cafe...");
  digitalWrite(relayCafe, HIGH); delay(2000); digitalWrite(relayCafe, LOW);

  accessGranted = false; displayPrompt();
}

void pourCafeAuLait() {
  do { lcd.clear(); lcd.print("Place a glass"); }
  while (!isGlassPresent());

  lcd.clear(); lcd.print("Adding Sugar..."); addSugar();
  lcd.clear(); lcd.print("Pouring Cafe...");
  digitalWrite(relayCafe, HIGH); delay(2000); digitalWrite(relayCafe, LOW);

  lcd.clear(); lcd.print("Pouring Lait...");
  digitalWrite(relayLait, HIGH); delay(2000); digitalWrite(relayLait, LOW);

  accessGranted = false; displayPrompt();
}

void pourThe() {
  do { lcd.clear(); lcd.print("Place a glass"); }
  while (!isGlassPresent());

  lcd.clear(); lcd.print("Adding Sugar..."); addSugar();
  lcd.clear(); lcd.print("Pouring The...");
  digitalWrite(relayThe, HIGH); delay(2000); digitalWrite(relayThe, LOW);

  accessGranted = false; displayPrompt();
}

void pourTheAuLait() {
  do { lcd.clear(); lcd.print("Place a glass"); }
  while (!isGlassPresent());

  lcd.clear(); lcd.print("Adding Sugar..."); addSugar();
  lcd.clear(); lcd.print("Pouring The...");
  digitalWrite(relayThe, HIGH); delay(2000); digitalWrite(relayThe, LOW);

  lcd.clear(); lcd.print("Pouring Lait...");
  digitalWrite(relayLait, HIGH); delay(2000); digitalWrite(relayLait, LOW);

  accessGranted = false; displayPrompt();
}
