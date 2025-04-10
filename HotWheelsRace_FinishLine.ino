#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>
#include <vector>
#include <SPI.h>
#include <Wire.h>
#include <ezButton.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

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

#define LANES 4
#define NUMBER_OF_CARS 7
#define NUMBER_OF_TIMES 3
const int regularHeatCount = ceil(((NUMBER_OF_CARS * NUMBER_OF_TIMES) / float(min(LANES, NUMBER_OF_CARS))));

// heat types
#define HEAT_TYPE_REGULAR 0
#define HEAT_TYPE_FINALS 1
#define HEAT_TYPE_EXTRA 2

// lane status types
#define AT_GATE 0
#define RACING 1
#define FINISHED 2

int breakBeamPin[LANES] = {6, 7, 18, 16};
int finishLineLED[LANES] = {9, 10, 46, 17};
int readyLED = 11;
int raceActiveLED = 8;
bool commEstablished = false;
ezButton resetSwitch(3);
long lastTouchMillis = millis() - REPEATED_TOUCH_TOLERANCE;

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

class Car {
  private:
    String _driverName;
    String _carName;
  public:
    Car() {
      Car("", "");
    }

    Car(String driverName, String carName) {
      _driverName = driverName;
      _carName = carName;
    }

    String toString() {
      String str;
      str += _driverName; str += ":"; str += _carName;
      return str;
    }

    void setDriverName(String driverName) {
      _driverName = driverName;
    }

    String getDriverName() {
      return _driverName;
    }

    void setCarName(String carName) {
      _carName = carName;
    }

    String getCarName() {
      return _carName;
    }
};

class RaceData {
  private:
    Car _car[NUMBER_OF_CARS];
    int _leaderboardCar[NUMBER_OF_CARS];
    float _leaderboardTime[NUMBER_OF_CARS];
    float _elapsedTime[NUMBER_OF_CARS][NUMBER_OF_TIMES]; // seconds
    float _average[NUMBER_OF_CARS]; // seconds
    int _carCount;
    int _elapsedTimeCount[NUMBER_OF_CARS];

  public:
    RaceData() {
      _carCount = 0;
      for (int i=0 ; i < NUMBER_OF_CARS ; i++) {
        _elapsedTimeCount[i] = 0;
        _average[i] = 0;
      }
    }

    String toString() {
      String str;
      for (int i=0 ; i < _carCount ; i++) {
        str += "_car["; str += i; str += "]: "; str += _car[i].toString(); str += "\n";
        str += "  "; str += _elapsedTimeCount[i]; str += " time records: ";
        for (int j=0 ; j < _elapsedTimeCount[i] ; j++) {
          str += _elapsedTime[i][j]; str += " ";
        }
        str += "--> "; str += _average[i]; str += " (average time)\n";
      }
      return str;
    }

    void addCar(Car c) {
      // Serial.print("addCar: _carCount: "); Serial.print(_carCount); Serial.print(", NUMBER_OF_CARS: "); Serial.println(NUMBER_OF_CARS);
      if (_carCount == NUMBER_OF_CARS) {
        Serial.println("ERROR: carCount exceeds NUMBER_OF_RACERS");
        return;
      }
      _car[_carCount] = c;
      _carCount++;
      // Serial.print("_carCount incremented to: "); Serial.println(_carCount);
    }

    // void setCar(int carIndex, Car c) {
    //   if (index < _carCount) {
    //     _car[carIndex] = c;
    //   }
    // }

    Car getCar(int index) {
      if (index < NUMBER_OF_CARS) {
        return _car[index];
      }
    }

    int getCarCount() {
      return _carCount;
    }

    void addElapsedTime(int carIndex, float seconds) {
      _elapsedTime[carIndex][_elapsedTimeCount[carIndex]] = seconds;
      _elapsedTimeCount[carIndex]++;
    }

    float getElapsedTime(int carIndex, int elapsedTimeIndex) {
      if ((carIndex < NUMBER_OF_CARS) && (elapsedTimeIndex < _elapsedTimeCount[carIndex])) {
        return _elapsedTime[carIndex][elapsedTimeIndex];
      } else {
        Serial.println("ERROR: second index to _elapsedTime exceeded expected value");
        return 0;
      }
    }

    int getElapsedTimeCount(int carIndex) {
      return _elapsedTimeCount[carIndex];
    }

    void calculateAverages() {
      for (int carIndex = 0 ; carIndex < NUMBER_OF_CARS ; carIndex++) {
        float sum = 0;
        for (int elapsedTimeIndex = 0 ; elapsedTimeIndex < _elapsedTimeCount[carIndex] ; elapsedTimeIndex++) {
          sum += _elapsedTime[carIndex][elapsedTimeIndex];
        }
        _average[carIndex] = sum / _elapsedTimeCount[carIndex];
        // Serial.print("averages: carIndex: "); Serial.print(carIndex);
        // Serial.print(", sum: "); Serial.print("sum");
        // Serial.print(", _elapsedTimeCount["); Serial.print(carIndex); Serial.print("]: "); Serial.print(_elapsedTimeCount[carIndex]);
        // Serial.print(", average: "); Serial.println(_average[carIndex]);
      }
    }

    void generateLeaderboard() {
      for (int i=0 ; i < NUMBER_OF_CARS ; i++) {
        _leaderboardCar[i] = i;
        _leaderboardTime[i] = _average[i];
      }
      for (int i=0 ; i < NUMBER_OF_CARS - 1 ; i++) {
        for (int j=0 ; j < NUMBER_OF_CARS - i - 1 ; j++) {
          if (_leaderboardTime[j] > _leaderboardTime[j+1]) {
            float tempTime = _leaderboardTime[j];
            _leaderboardTime[j] = _leaderboardTime[j+1];
            _leaderboardTime[j+1] = tempTime;
            int tempIndex = _leaderboardCar[j];
            _leaderboardCar[j] = _leaderboardCar[j+1];
            _leaderboardCar[j+1] = tempIndex;
          }
        }
      }
    }

    String leaderboardToString() {
      String str = "Leaderboard:\n";
      for (int i=0 ; i < NUMBER_OF_CARS ; i++) {
        String th;
        if (((i+1) % 10 == 1) && (i != 10)) {
          th = "st";
        } else if (((i+1) % 10 == 2) && (i!= 11)) {
          th = "nd";
        } else if (((i+1) % 10 == 3) && (i!= 12)) {
          th = "rd";
        } else {
          th = "th";
        }
        str += "  ["; str += (i+1); str += th; str += "] car: "; str += (_leaderboardCar[i] + 1);
        str += ", average time: "; str += _leaderboardTime[i]; str += "\n";
      }
      return str;
    }

    void createFinalHeat() {
      // Heat finals(regularHeatCount, min(NUMBER_OF_CARS, LANES), HEAT_TYPE_FINALS);
      // return finals;
    }

    void  createExtraHeat() {
      // Heat extra(regularHeatCount + 1, LANES, HEAT_TYPE_EXTRA);
      // for (int lane = 0 ; lane < LANES ; lane++) {
      //   extra.setLaneAssignment(lane, Car("Anyone", "Any Car"));
      // }
      // return extra;
    }

};

class Heat {
  private:
    int _heatNumber;
    int _laneUsageCount;
    int _heatType;
    int _laneAssignment[LANES];


  public:
    Heat() {
      Heat(-1, LANES, HEAT_TYPE_REGULAR);
    }

    Heat(int heatNumber, int laneUsageCount, int heatType) {
      _heatNumber = heatNumber;
      _laneUsageCount = laneUsageCount;
      _heatType = heatType;
    }

    String toString() {
      String str;
      str += "    _heatNumber: "; str += _heatNumber; str += "\n";
      str += "    _laneUsageCount: "; str += _laneUsageCount; str += "\n";
      str += "    _heatType: "; str += _heatType; str += "\n";
      str += "    _laneAssignment:\n";
      for (int laneIndex = 0 ; laneIndex < _laneUsageCount ; laneIndex++) {
        str += "      _laneAssignment["; str += laneIndex; str += "]: \""; str += _laneAssignment[laneIndex]; str += "\" (carIndex)\n";
      }
      return str;
    }

    void setHeatNumber(int heatNumber) {
      _heatNumber = heatNumber;
    }

    int getHeatNumber() {
      return _heatNumber;
    }

    void setLaneUsageCount(int laneUsageCount) {
      _laneUsageCount = laneUsageCount;
    }

    int getLaneUsageCount() {
      return _laneUsageCount;
    }

    void setHeatType(int heatType) {
      _heatType = heatType;
    }

    int getHeatType() {
      return _heatType;
    }

    void setLaneAssignment(int laneIndex, int carIndex) {
      _laneAssignment[laneIndex] = carIndex;
    }

    int getLaneAssignment(int laneIndex) {
      return _laneAssignment[laneIndex];
    }
};

class Schedule {
  private:
    Heat _heat[regularHeatCount];
    Heat _finals;
    Heat _extra;
    int _currentHeatNumber;
    int _laneUsageCount;
    int _numberOfCars;
    int _numberOfTimes;

  public:
    Schedule() {
      Serial.println("Schedule, default constructor called");
    }

    Schedule(int laneUsageCount, int numberOfCars, int numberOfTimes) {
      _numberOfCars = numberOfCars;
      _numberOfTimes = numberOfTimes;
      _laneUsageCount = laneUsageCount;
      _currentHeatNumber = 0;
      Serial.println("Schedule constructor:");
      Serial.print("  _laneUsageCount: "); Serial.print(_laneUsageCount);
      Serial.print(", _numberOfCars: "); Serial.print(_numberOfCars);
      Serial.print(", numberOfTimes: "); Serial.println(_numberOfTimes);
    }

    String toString() {
      String str;
      str += "Schedule:\n";
      str += "  _currentHeatNumber: "; str += (_currentHeatNumber + 1); str += "\n";
      str += "  _numberOfCars: "; str += _numberOfCars; str += "\n";
      str += "  _numberOfTimes: "; str += _numberOfTimes; str += "\n";
      str += "  _laneUsageCount: "; str += _laneUsageCount; str += "\n";
      str += "  _heat array:\n";
      for (int heatIndex = 0 ; heatIndex < regularHeatCount ; heatIndex++) {
        str += _heat[heatIndex].toString();
      }
      return str;
    }

    Heat nextHeat() {
      if (_currentHeatNumber < regularHeatCount) {
        return _heat[_currentHeatNumber];
      } else if (_currentHeatNumber == regularHeatCount) {
        return _finals;
      } else {
        return _extra;
      }
    }

    void setNumberOfCars(int numberOfCars) {
      _numberOfCars = numberOfCars;
    }

    int getNumberOfCars() {
      return _numberOfCars;
    }

    void setNumberOfTimes(int numberOfTimes) {
      _numberOfTimes = numberOfTimes;
    }

    int getNumberOfTimes() {
      return _numberOfTimes;
    }

    void setLaneUsageCount(int laneUsageCount) {
      _laneUsageCount = laneUsageCount;
    }

    void createRegularHeats() {
      int carIndex = 0;
      for (int regularHeatIndex = 0 ; regularHeatIndex < regularHeatCount ; regularHeatIndex++) {
        _heat[regularHeatIndex].setHeatNumber(regularHeatIndex + 1);
        _heat[regularHeatIndex].setHeatType(HEAT_TYPE_REGULAR);
        int thisHeatLaneCount = _laneUsageCount;
        if (regularHeatIndex == regularHeatCount - 1) { // this is the last REGULAR heat - there could be less used lanes than usual
          thisHeatLaneCount = (NUMBER_OF_CARS * NUMBER_OF_TIMES) % _laneUsageCount;
          if (thisHeatLaneCount == 0) { // then we actually need all the lanes, not 0 of them
            thisHeatLaneCount = _laneUsageCount;
          }
          Serial.print("Last regular heat, setting thisHeatLaneCount: "); Serial.println(thisHeatLaneCount);
        }
        Serial.print("thisHeatLaneCount: "); Serial.println(thisHeatLaneCount);
        _heat[regularHeatIndex].setLaneUsageCount(thisHeatLaneCount);
        for (int laneIndex = 0 ; laneIndex < thisHeatLaneCount ; laneIndex++) {
          _heat[regularHeatIndex].setLaneAssignment(laneIndex, carIndex);
          carIndex++;
          if (carIndex == _numberOfCars) {
            carIndex = 0;
          }
        }
      }
    }

    void setFinals(Heat finals) {
      _finals = finals;
    }

    void setExtra(Heat extra) {
      _extra = extra;
    }
};

class Environment {
  private:
    int _laneStatus[LANES];
  public:
    Environment() {
      for (int i=0 ; i < LANES ; i++) {
        _laneStatus[i] = AT_GATE;
      }
    }

    void setLaneStatus(int lane, int status) {
      if (lane < LANES) {
        _laneStatus[lane] = status;
      }
    }

    int getLaneStatus(int lane) {
      if (lane < LANES) {
        return _laneStatus[lane];
      }
      return -1;
    }
};

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

    // if ((strcmp(buffer, "START_GATE_OPENED") == 0) && (allFinished == false)) {
    if (strcmp(buffer, "START_GATE_OPENED") == 0) {
      digitalWrite(readyLED, LOW);
    } else if (strcmp(buffer, "START_GATE_CLOSED") == 0) {
      digitalWrite(readyLED, HIGH);
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
    tft.fillScreen(TFT_BLACK);
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
  tft.drawCentreString("Toggle start gate to establish connection. ", 160, 20, 2);
}


RaceData raceData;
Schedule sched(min(LANES, NUMBER_OF_CARS), NUMBER_OF_CARS, NUMBER_OF_TIMES); // 4 lanes, 7 cars, 3 times each
// Environment env();

/* Main */
void setup() {
  Serial.begin(115200);

  sched.createRegularHeats();
  Serial.println(sched.toString());

  raceData.addCar(Car("Happy", "Elated"));      // 0
  raceData.addCar(Car("Sleepy", "Tired"));      // 1
  raceData.addCar(Car("Sneezy", "Gesundheit")); // 2
  raceData.addCar(Car("Dopey", "Duh"));         // 3
  raceData.addCar(Car("Doc", "PhD"));           // 4
  raceData.addCar(Car("Grumpy", "Old Man"));    // 5
  raceData.addCar(Car("Bashful", "Shy"));       // 6

  raceData.addElapsedTime(0, 2.0); raceData.addElapsedTime(0, 3.0); raceData.addElapsedTime(0, 4.0);
  raceData.addElapsedTime(5, 5.0); raceData.addElapsedTime(5, 6.0); raceData.addElapsedTime(5, 7.0);
  raceData.addElapsedTime(2, 8.0); raceData.addElapsedTime(2, 9.0); raceData.addElapsedTime(2, 10.0);
  raceData.addElapsedTime(3, 11.0); raceData.addElapsedTime(3, 12.0); raceData.addElapsedTime(3, 13.0);
  raceData.addElapsedTime(4, 14.0); raceData.addElapsedTime(4, 15.0); raceData.addElapsedTime(4, 16.0);
  raceData.addElapsedTime(1, 17.0); raceData.addElapsedTime(1, 18.0); raceData.addElapsedTime(1, 19.0);
  raceData.addElapsedTime(6, 20.0); raceData.addElapsedTime(6, 21.0); raceData.addElapsedTime(6, 22.0);
  raceData.calculateAverages();
  Serial.println(raceData.toString());
  raceData.generateLeaderboard();
  Serial.println(raceData.leaderboardToString());


  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;

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

  // initialize the race
  for (int i=0 ; i < LANES ; i++) {
    pinMode(finishLineLED[i], OUTPUT);
    digitalWrite(finishLineLED[i], LOW);
  }
  pinMode(readyLED, OUTPUT);
  pinMode(raceActiveLED, OUTPUT);
  digitalWrite(raceActiveLED, LOW);
  resetSwitch.setDebounceTime(50);
  if (commEstablished == false) {
    displayToggleGate();
  }
}

void loop() {
  resetSwitch.loop();
  if ((resetSwitch.isPressed()) && (commEstablished)) {
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
      Serial.println(raceData.leaderboardToString());
    }

    // printTouchToSerial(x, y, z);
    // printTouchToDisplay(x, y, z);
    // delay(100);
  }
}
