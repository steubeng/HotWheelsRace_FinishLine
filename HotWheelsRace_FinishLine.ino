#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>
#include <vector>
#include <SPI.h>
#include <Wire.h>
#include <ezButton.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Car.h>
#include <Constants.h>
#include <RaceEvent.h>
#include <Heat.h>
#include <Schedule.h>

RaceEvent &raceEvent = raceEvent.getInstance();

void addCarsToEvent() {
  raceEvent.addCar(Car("Josiah", ""));
  raceEvent.addCar(Car("Sydney", ""));
  raceEvent.addCar(Car("Luke", ""));
  raceEvent.addCar(Car("Sianna", ""));
  raceEvent.addCar(Car("Nath", ""));
  raceEvent.addCar(Car("Kezia", ""));
  raceEvent.addCar(Car("Colbie", ""));
  raceEvent.addCar(Car("Micah", ""));
  raceEvent.addCar(Car("Abby", ""));
  raceEvent.addCar(Car("Maison", ""));
  raceEvent.addCar(Car("Katie", ""));
  raceEvent.addCar(Car("Ryder", ""));
}

TFT_eSPI tft = TFT_eSPI();

/* Definitions */
#define ESPNOW_WIFI_CHANNEL 6
#define SCL0_Pin 19
#define SDA0_Pin 20

// Touchscreen pins
#define XPT2046_IRQ 48   // T_IRQ
#define XPT2046_MOSI 21  // T_DIN
#define XPT2046_MISO 47  // T_OUT
#define XPT2046_CLK 37   // T_CLK
#define XPT2046_CS 38    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 4
#define REPEATED_TOUCH_TOLERANCE 1000

// lane status types
#define AT_GATE 0
#define RACING 1
#define FINISHED 2

// race status types
#define ALL_AT_GATE 0
#define STILL_RACING 1
#define ALL_FINISHED 2

int potPin = 5;
int breakBeamPin[LANES] = {6, 7, 18, 16};
int finishLineLED[LANES] = {9, 10, 46, 17};
int laneStatus[LANES];
int raceStatus;
int finishedRacerCount;
int readyLED = 11;
int raceActiveLED = 8;
int irThreshold = 1000;
int previousIrThreshold = 0;
bool commEstablished = false;
ezButton resetSwitch(3);
long lastTouchMillis = millis() - REPEATED_TOUCH_TOLERANCE;
long startTimeMillis;
bool scoresReported = false;
bool haveThisHeat = false;
int finishedLaneCount = 0;
Heat thisHeat;

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

// Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
void printTouchToSerial(int touchX, int touchY, int touchZ) {
  Serial.print("X = ");
  Serial.print(touchX);
  Serial.print(" | Y = ");
  Serial.print(touchY);
  Serial.print(" | Pressure = ");
  Serial.print(touchZ);
  Serial.println();
}

// Print Touchscreen info about X, Y and Pressure (Z) on the TFT Display
void printTouchToDisplay(int touchX, int touchY, int touchZ) {
  // Clear TFT screen
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int centerX = SCREEN_WIDTH / 2;
  int textY = 80;
 
  String tempText = "X = " + String(touchX);
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);

  textY += 20;
  tempText = "Y = " + String(touchY);
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);

  textY += 20;
  tempText = "Pressure = " + String(touchZ);
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);
}

// RaceEvent &raceEvent = raceEvent.getInstance();
Schedule sched(min(LANES, NUMBER_OF_CARS), NUMBER_OF_CARS, NUMBER_OF_TIMES); // 4 lanes, 7 cars, 3 times each

void displayHeatNumber() {
  int heatNumber = thisHeat.getHeatNumber();
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  tft.fillRect(140, 70, 180, 20, TFT_BLACK);
  char buffer[32];
  sprintf(buffer, "Heat: %1d", heatNumber);
  tft.drawString(buffer, 5, 70, FONT_SIZE);
  if (heatNumber < regularHeatCount) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Qualifying Heat", 140, 70, FONT_SIZE);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  } else if (heatNumber == regularHeatCount) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Last Qualifier", 140, 70, FONT_SIZE);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  } else if (heatNumber == regularHeatCount + 1) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Finals!!!", 140, 70, FONT_SIZE);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Extra Heat", 140, 70, FONT_SIZE);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  }
}

void displayLaneAssignments() {
  char buffer[64];
  for (int lane=0 ; lane < thisHeat.getLaneUsageCount() ; lane++) {
    int carIndex = thisHeat.getLaneAssignment(lane);
    String carName = (raceEvent.getCar(carIndex)).toString();
    tft.fillRect(5, 70 + ((lane+1) * 30), 315, 210, TFT_BLACK);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    // sprintf(buffer, "%s", carName);
    // tft.drawString(buffer, 5, 70 + ((lane+1) * 30), FONT_SIZE);
    if (thisHeat.getHeatType() == HEAT_TYPE_EXTRA) {
      carName = "<open>";
    }
    tft.drawString(carName, 5, 70 + ((lane+1) * 30), FONT_SIZE);
    float average = raceEvent.getAverage(carIndex);
    int elapsedTimeCount = raceEvent.getElapsedTimeCount(carIndex);
    if ((elapsedTimeCount == 0) || (thisHeat.getHeatType() == HEAT_TYPE_EXTRA)) {
      sprintf(buffer, "[ --- ]");
    }
    else if (average >= 0) {
      sprintf(buffer, "[%1.3fs]", average);
    } 
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawString(buffer, 175, 70 + ((lane+1) * 30), FONT_SIZE);
  }
}

void displayReady() {
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("Ready... ", 45, 30, FONT_SIZE);
  tft.fillRect(150, 30, 170, 20, TFT_BLACK);
}

void displaySet() {
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.fillRect(150, 30, 170, 20, TFT_BLACK);
  tft.drawString("Set...", 150, 30, FONT_SIZE);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  displayHeatNumber();
  displayLaneAssignments();
}

void displayGo() {
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Go!", 225, 30, FONT_SIZE);
}

void displayLaneTime(int lane, float time) {
  String place;
  tft.setTextColor(TFT_SILVER, TFT_BLACK);
  if (finishedRacerCount == 1) {
    place = " 1st    ";
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  } else if (finishedRacerCount == 2) {
    place = " 2nd    ";
  } else if (finishedRacerCount == 3) {
    place = " 3rd    ";
  } else {
    place = " 4th    ";
  }
  char buffer[32];
  sprintf(buffer, "%s", place);
  tft.drawString(buffer, 175, 70 + ((lane+1) * 30), FONT_SIZE);
  sprintf(buffer, "%7.3fs", time);
  tft.drawString(buffer, 225, 70 + ((lane+1) * 30), FONT_SIZE);
}

// Creating a new class that inherits from the ESP_NOW_Peer class is required.
class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:
  // Constructor of the class
  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}

  // Destructor of the class
  ~ESP_NOW_Peer_Class() {}

  // Function to register the master peer
  bool add_peer() {
    if (!add()) {
      log_e("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }

  // Function to print the received messages from the master
  void onReceive(const uint8_t *data, size_t len, bool broadcast) {
    Serial.printf("Received a message from master " MACSTR " (%s)\n", MAC2STR(addr()), broadcast ? "broadcast" : "unicast");
    Serial.printf("  Message: %s\n", (char *)data);

    char *buffer = (char*)data;

    if (strcmp(buffer, "START_GATE_OPENED") == 0) {
      digitalWrite(readyLED, LOW);
      digitalWrite(raceActiveLED, HIGH);
      scoresReported = false;
      finishedRacerCount = 0;
      for (int i=0 ; i < LANES ; i++) {
        laneStatus[i] = RACING;
      }
      Serial.println("RACE STARTED!!!");
      displayGo();
      Serial.println("Watching Finish Line:");
      startTimeMillis = millis();
      raceStatus = STILL_RACING;
    } else if (strcmp(buffer, "START_GATE_CLOSED") == 0) {
      raceStatus = ALL_AT_GATE;
      finishedLaneCount = 0;
      digitalWrite(readyLED, HIGH);
      digitalWrite(raceActiveLED, LOW);
      for (int i=0 ; i < LANES ; i++) {
        digitalWrite(finishLineLED[i], LOW);
      }
      displayReady();
      displaySet();
      Serial.println("############################################"); Serial.println(thisHeat.toString());
    }
  }
};

// List of all the masters. It will be populated when a new master is registered
std::vector<ESP_NOW_Peer_Class> masters;

/* Callbacks */

// Callback called when an unknown peer sends a message
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {
  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {
    Serial.printf("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
    Serial.println("Registering the peer as a master");
    commEstablished = true;
    Serial.print("Break beam sensor threshold: ");
    Serial.println(irThreshold);
    tft.fillScreen(TFT_BLACK);
    displayReady();
    Serial.println("Ready...");
    ESP_NOW_Peer_Class new_master(info->src_addr, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);

    masters.push_back(new_master);
    if (!masters.back().add_peer()) {
      Serial.println("Failed to register the new master");
      return;
    }
  } else {
    // The slave will only receive broadcast messages
    log_v("Received a unicast message from " MACSTR, MAC2STR(info->src_addr));
    log_v("Igorning the message");
  }
}

String getDefaultMacAddress() {
  String mac = "";
  unsigned char mac_base[6] = {0};
  if (esp_efuse_mac_get_default(mac_base) == ESP_OK) {
    char buffer[18];  // 6*2 characters for hex + 5 characters for colons + 1 character for null terminator
    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac_base[0], mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
    mac = buffer;
  }
  return mac;
}

void displayToggleGate() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Toggle start gate to establish connection.", 160, 210, 2);
}

void displayIrThreshold() {
  char buffer[16];
  int beamReadingL1 = analogRead(breakBeamPin[0]);
  int beamReadingL2 = analogRead(breakBeamPin[1]);
  int beamReadingL3 = analogRead(breakBeamPin[2]);
  int beamReadingL4 = analogRead(breakBeamPin[3]);
  // Serial.print("beamReading: "); Serial.print(beamReading); Serial.print(", irThreshold: "); Serial.println(irThreshold);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Calibrate Break Beam Sensor Threshold.", 30, 30, 2);
  tft.drawString("Break-Beam              Current", 60, 60, 2);
  tft.drawString(" Reading               Threshold", 60, 80, 2);
  tft.drawString("L1:", 40, 100, 4);
  tft.drawString("L2:", 40, 124, 4);
  tft.drawString("L3:", 40, 148, 4);
  tft.drawString("L4:", 40, 172, 4);

  tft.fillRect(80, 100, 240, 92, TFT_BLACK);

  sprintf(buffer, "%4d", beamReadingL1);
  if (beamReadingL1 < irThreshold) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  tft.drawString(buffer, 80, 100, 4);

  sprintf(buffer, "%4d", beamReadingL2);
  if (beamReadingL2 < irThreshold) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  tft.drawString(buffer, 80, 124, 4);

  sprintf(buffer, "%4d", beamReadingL3);
  if (beamReadingL3 < irThreshold) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  tft.drawString(buffer, 80, 148, 4);

  sprintf(buffer, "%4d", beamReadingL4);
  if (beamReadingL4 < irThreshold) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  tft.drawString(buffer, 80, 172, 4);

  sprintf(buffer, "%4d", irThreshold);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawString(buffer, 180, 100, 6);
}


void resetRace() {
  finishedRacerCount = 0;
  resetHeat();
}

void resetHeat() {
  raceStatus = ALL_AT_GATE;
  for (int i=0 ; i < LANES ; i++) {
    digitalWrite(finishLineLED[i], LOW);
    laneStatus[i] = AT_GATE;
  }
  digitalWrite(raceActiveLED, LOW);
  if (commEstablished) {
    digitalWrite(readyLED, HIGH);
    displayReady();
    displaySet();
  }
}

/* Main */
void setup() {
  Serial.begin(115200);
  sched.createRegularHeats();
  addCarsToEvent();
  // raceEvent.addCar(Car("Happy", "Elated"));      // 0
  // raceEvent.addCar(Car("Sleepy", "Tired"));      // 1
  // raceEvent.addCar(Car("Sneezy", "Achew"));      // 2
  // raceEvent.addCar(Car("Dopey", "Duh"));         // 3
  // raceEvent.addCar(Car("Doc", "PhD"));           // 4
  // raceEvent.addCar(Car("Grumpy", "Old Man"));    // 5
  // raceEvent.addCar(Car("Bashful", "Shy"));       // 6

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;

  // Print wire/configutation debugging information
  // Serial.print("TFT_MISO: "); Serial.println(TFT_MISO);
  // Serial.print("TFT_MOSI: "); Serial.println(TFT_MOSI);
  // Serial.print("TFT_SCLK: "); Serial.println(TFT_SCLK);
  // Serial.print("TFT_CS: "); Serial.println(TFT_CS);
  // Serial.print("TFT_DC: "); Serial.println(TFT_DC);
  // Serial.print("TFT_RST: "); Serial.println(TFT_RST);
  // Serial.print("TFT_BL: "); Serial.println(TFT_BL);

  // Serial.print("XPT2046_IRQ: "); Serial.println(XPT2046_IRQ);
  // Serial.print("XPT2046_MOSI: "); Serial.println(XPT2046_MOSI);
  // Serial.print("XPT2046_MISO: "); Serial.println(XPT2046_MISO);
  // Serial.print("XPT2046_CLK: "); Serial.println(XPT2046_CLK);
  // Serial.print("XPT2046_CS: "); Serial.println(XPT2046_CS);

  Wire.begin(SDA0_Pin, SCL0_Pin);
  Serial.println(F("SSD1306 allocation OK"));
  while (!Serial) {
    delay(10);
  }

  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }
  
  char buffer[256];
  String text = getDefaultMacAddress();
  text.toCharArray(buffer, text.length()+1);
  Serial.println(buffer);
  delay(2000);

  Serial.println("ESP-NOW Example - Broadcast Slave");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Initialize the ESP-NOW protocol
  if (!ESP_NOW.begin()) {
    Serial.println("Failed to initialize ESP-NOW");
    Serial.println("Reeboting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  // Register the new peer callback
  ESP_NOW.onNewPeer(register_new_master, NULL);

  Serial.println("Setup complete. Waiting for a master to broadcast a message...");

  for (int i=0 ; i < LANES ; i++) {
    pinMode(finishLineLED[i], OUTPUT);
  }
  pinMode(readyLED, OUTPUT);
  pinMode(raceActiveLED, OUTPUT);
  resetSwitch.setDebounceTime(50);
  resetRace();
  if (commEstablished == false) {
    displayToggleGate();
  }
}

void loop() {
  if ((raceStatus == ALL_AT_GATE) && (commEstablished == false)) {
    irThreshold = map(analogRead(potPin), 650, 4096, 0, 4096);
    irThreshold = analogRead(potPin);
    if (irThreshold != previousIrThreshold) {
      displayIrThreshold();
      previousIrThreshold = irThreshold;
    }
    for (int i=0 ; i < LANES ; i++) {
      if (analogRead(breakBeamPin[i]) < irThreshold) {
        digitalWrite(finishLineLED[i], HIGH);
      } else {
        digitalWrite(finishLineLED[i], LOW);
      }
    }
    delay(200);
  }
  if ((raceStatus == ALL_AT_GATE) && (haveThisHeat == false) && (commEstablished)) {
    Heat nextHeat = sched.nextHeat();
    thisHeat.setHeatNumber(nextHeat.getHeatNumber());
    thisHeat.setLaneUsageCount(nextHeat.getLaneUsageCount());
    thisHeat.setHeatType(nextHeat.getHeatType());
    for (int i=0 ; i < LANES ; i++) {
      thisHeat.setLaneAssignment(i, nextHeat.getLaneAssignment(i));
    }
    haveThisHeat = true;
  } else if (raceStatus == STILL_RACING) {
    for (int i=0 ; i < thisHeat.getLaneUsageCount() ; i++) {
      if ((analogRead(breakBeamPin[i]) < irThreshold) && (laneStatus[i] == RACING)) { // someone crossed the finish line
        laneStatus[i] = FINISHED;
        finishedLaneCount++;
        finishedRacerCount++;
        float elapsedTime = (millis() - startTimeMillis) / 1000.0;
        displayLaneTime(i, elapsedTime);
        if (thisHeat.getHeatType() == HEAT_TYPE_REGULAR) {
          raceEvent.addElapsedTime(thisHeat.getLaneAssignment(i), elapsedTime);
        }
        Serial.print("  Lane "); Serial.print(i+1); Serial.print(": ");
        if ((thisHeat.getHeatType() == HEAT_TYPE_REGULAR) || (thisHeat.getHeatType() == HEAT_TYPE_FINALS)) {
          Serial.print(raceEvent.getCar(thisHeat.getLaneAssignment(i)).toString());
        } else {
          Serial.print("<open>");
        }
        char buffer[16];
        sprintf(buffer, "%1.3f", elapsedTime);
        Serial.print(", time: "); Serial.println(buffer);
        digitalWrite(finishLineLED[i], HIGH);
      }
    }
    if (finishedLaneCount == thisHeat.getLaneUsageCount()) {
      raceStatus = ALL_FINISHED;
      Serial.println("RACE FINISHED!!!");
    }
  } else if ((raceStatus == ALL_FINISHED) && (scoresReported == false)) {
    if (thisHeat.getHeatType() == HEAT_TYPE_REGULAR) {
      raceEvent.calculateAverages();
      Serial.println();
      Serial.println("Running Average of Elapsed Times from Regular Heats:");
      Serial.println(raceEvent.toString());
      raceEvent.generateLeaderboard();
    }

    // TODO: set the finals heat here.
    if (sched.getCurrentHeatNumber() == regularHeatCount - 1) { // last regular heat just ended, build finals
      Heat finals;
      finals.setHeatNumber(regularHeatCount+1);
      finals.setLaneUsageCount(min(LANES, NUMBER_OF_CARS));
      finals.setHeatType(HEAT_TYPE_FINALS);
      for (int i=0 ; i < min(LANES, NUMBER_OF_CARS) ; i++) {
        finals.setLaneAssignment(i, raceEvent.getLeaderboardCar(i));
      }
      sched.setFinals(finals);

      Heat extra;
      extra.setHeatNumber(regularHeatCount+2);
      extra.setLaneUsageCount(min(LANES, NUMBER_OF_CARS));
      extra.setHeatType(HEAT_TYPE_EXTRA);
      for (int i=0 ; i < LANES ; i++) {
        extra.setLaneAssignment(i, -1);
      }
      sched.setExtra(extra);
    }

    if (thisHeat.getHeatType() == HEAT_TYPE_REGULAR) {
      Serial.println(raceEvent.leaderboardToString());
    }
    raceStatus = ALL_AT_GATE;
    for (int i=0 ; i < thisHeat.getLaneUsageCount() ; i++) {
      laneStatus[i] = AT_GATE;
    }
    haveThisHeat = false;  // invalidate thisHeat since it's over; next time through we'll get a new one
    sched.incrementCurrentHeatNumber();
    scoresReported = true;
  }

  resetSwitch.loop();
  if ((resetSwitch.isPressed()) && (commEstablished)) {
    resetHeat();
  }

  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z) info on the TFT display and Serial Monitor
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;
    long now = millis();
    if (now - lastTouchMillis > REPEATED_TOUCH_TOLERANCE) {
      // what to do if the screen is touched?
      lastTouchMillis = now;
      Serial.println(raceEvent.leaderboardToString());
    }

    // printTouchToSerial(x, y, z);
    // printTouchToDisplay(x, y, z);
    // delay(100);
  }
}
