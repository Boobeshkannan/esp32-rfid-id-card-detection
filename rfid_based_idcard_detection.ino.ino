
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <time.h>

// ---------- Wi-Fi: optional ----------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// India Standard Time (UTC+5:30)
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 19800;
const int DAYLIGHT_OFFSET_SEC = 0;

// ---------- RC522 pins ----------
#define RFID_SS_PIN   5
#define RFID_RST_PIN  22

// ESP32 VSPI defaults:
// SCK=18, MISO=19, MOSI=23
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// ---------- Direction sensors ----------
#define IR1_PIN 32
#define IR2_PIN 33

// Set true if the sensor output goes LOW when blocked.
// Change to false if your sensors output HIGH when blocked.
const bool IR_ACTIVE_LOW = true;

// ---------- Timing ----------
const unsigned long CROSS_TIMEOUT_MS = 2500;
const unsigned long CARD_TIMEOUT_MS  = 5000;
const unsigned long DEBOUNCE_MS      = 40;

// Maximum students in this demo's RAM table
const int MAX_STUDENTS = 100;

// ---------- Student record ----------
struct Student {
  String uid;
  unsigned long entries = 0;
  unsigned long exits = 0;
};

Student students[MAX_STUDENTS];
int studentCount = 0;

// Last card read waiting for a crossing
String pendingUID = "";
unsigned long cardReadAt = 0;

// Direction sequence state
// 0 = idle, 1 = IR1 first, 2 = IR2 first
int directionState = 0;
unsigned long directionStartedAt = 0;

// Debounce
unsigned long lastIR1Change = 0;
unsigned long lastIR2Change = 0;
bool previousIR1 = false;
bool previousIR2 = false;

// ---------- Time helpers ----------
String getTimestamp() {
  struct tm timeInfo;

  if (getLocalTime(&timeInfo, 50)) {
    char buffer[24];
    strftime(buffer, sizeof(buffer),
             "%Y-%m-%d %H:%M:%S", &timeInfo);
    return String(buffer);
  }

  return "UPTIME_" + String(millis() / 1000) + "s";
}

void setupWiFiTime() {
  if (String(WIFI_SSID) == "YOUR_WIFI_NAME") {
    Serial.println("Wi-Fi not configured; using uptime.");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 10000) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC,
               NTP_SERVER);
    Serial.println("Wi-Fi connected; syncing clock.");
  } else {
    Serial.println("Wi-Fi unavailable; using uptime.");
  }
}

// ---------- Student table ----------
int findStudent(const String& uid) {
  for (int i = 0; i < studentCount; i++) {
    if (students[i].uid == uid) {
      return i;
    }
  }
  return -1;
}

int getOrCreateStudent(const String& uid) {
  int index = findStudent(uid);
  if (index >= 0) return index;

  if (studentCount >= MAX_STUDENTS) {
    Serial.println("ERROR: Student table full!");
    return -1;
  }

  index = studentCount++;
  students[index].uid = uid;
  students[index].entries = 0;
  students[index].exits = 0;

  return index;
}

// ---------- Sensor helpers ----------
bool beamBlocked(int pin) {
  int value = digitalRead(pin);

  if (IR_ACTIVE_LOW) {
    return value == LOW;
  }

  return value == HIGH;
}

void printSummary(int index) {
  Serial.println("----- STUDENT SUMMARY -----");
  Serial.print("UID: ");
  Serial.println(students[index].uid);

  Serial.print("Total entries: ");
  Serial.println(students[index].entries);

  Serial.print("Total exits: ");
  Serial.println(students[index].exits);

  Serial.println("---------------------------");
}

// ---------- Event registration ----------
void registerCrossing(const String& direction) {
  if (pendingUID.length() == 0) {
    Serial.println(
      "UNMATCHED CROSSING: No recently scanned card."
    );
    Serial.println(
      "This event cannot be assigned to a student."
    );
    return;
  }

  if (millis() - cardReadAt > CARD_TIMEOUT_MS) {
    Serial.println("Card scan expired; event not assigned.");
    pendingUID = "";
    return;
  }

  int index = getOrCreateStudent(pendingUID);

  if (index < 0) {
    pendingUID = "";
    return;
  }

  if (direction == "EXIT") {
    students[index].exits++;
  } else {
    students[index].entries++;
  }

  Serial.println();
  Serial.println("===== CROSSING REGISTERED =====");

  Serial.print("Student UID: ");
  Serial.println(pendingUID);

  Serial.print("Direction: ");
  Serial.println(direction);

  Serial.print("Timestamp: ");
  Serial.println(getTimestamp());

  Serial.print("Entry count: ");
  Serial.println(students[index].entries);

  Serial.print("Exit count: ");
  Serial.println(students[index].exits);

  Serial.println("===============================");
  Serial.println();

  printSummary(index);

  pendingUID = "";
}

// ---------- RFID reading ----------
void readRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    uid += String(rfid.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();

  // Repeated reads of the same card while held near
  // the reader must not continuously replace the ID.
  if (uid != pendingUID ||
      millis() - cardReadAt > CARD_TIMEOUT_MS) {
    pendingUID = uid;
    cardReadAt = millis();

    Serial.println();
    Serial.print("Card detected: ");
    Serial.println(pendingUID);

    Serial.println(
      "Now cross the two sensors within 5 seconds."
    );
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ---------- Direction detection ----------
void detectDirection() {
  unsigned long now = millis();

  bool ir1 = beamBlocked(IR1_PIN);
  bool ir2 = beamBlocked(IR2_PIN);

  // Debounced sensor rising events: unblocked -> blocked
  bool ir1Event = false;
  bool ir2Event = false;

  if (ir1 != previousIR1 &&
      now - lastIR1Change >= DEBOUNCE_MS) {
    lastIR1Change = now;
    previousIR1 = ir1;
    if (ir1) ir1Event = true;
  }

  if (ir2 != previousIR2 &&
      now - lastIR2Change >= DEBOUNCE_MS) {
    lastIR2Change = now;
    previousIR2 = ir2;
    if (ir2) ir2Event = true;
  }

  // Sequence is meaningful only if the beams are
  // physically separated along the walking direction.
  if (directionState == 0) {
    if (ir1Event) {
      directionState = 1;
      directionStartedAt = now;
    } else if (ir2Event) {
      directionState = 2;
      directionStartedAt = now;
    }
  } else {
    if (now - directionStartedAt > CROSS_TIMEOUT_MS) {
      directionState = 0;
    } else if (directionState == 1 && ir2Event) {
      // IR1 -> IR2
      registerCrossing("EXIT");
      directionState = 0;
    } else if (directionState == 2 && ir1Event) {
      // IR2 -> IR1
      registerCrossing("ENTRY");
      directionState = 0;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(IR1_PIN, INPUT_PULLUP);
  pinMode(IR2_PIN, INPUT_PULLUP);

  SPI.begin(18, 19, 23, RFID_SS_PIN);
  rfid.PCD_Init();

  Serial.println();
  Serial.println("Smart Gate Prototype Starting...");

  byte version = rfid.PCD_ReadRegister(
    rfid.VersionReg
  );

  if (version == 0x00 || version == 0xFF) {
    Serial.println(
      "WARNING: RC522 not detected. Check wiring."
    );
  } else {
    Serial.println("RC522 initialized.");
  }

  setupWiFiTime();

  previousIR1 = beamBlocked(IR1_PIN);
  previousIR2 = beamBlocked(IR2_PIN);

  Serial.println("System ready.");
  Serial.println(
    "Scan one card, then cross the sensor pair."
  );
  Serial.println(
    "IR1->IR2 = EXIT; IR2->IR1 = ENTRY."
  );
}

void loop() {
  readRFID();
  detectDirection();

  // Discard a card that was scanned but not used
  // within the allowed matching interval.
  if (pendingUID.length() > 0 &&
      millis() - cardReadAt > CARD_TIMEOUT_MS) {
    Serial.print("Expired unmatched card: ");
    Serial.println(pendingUID);
    pendingUID = "";
  }
}
