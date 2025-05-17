#include "Arduino.h"
#include "Constants.h"
#include "RaceEvent.h"
#include "Car.h"

RaceEvent::RaceEvent() {
  _carCount = 0;
  for (int i=0 ; i < NUMBER_OF_CARS ; i++) {
    _elapsedTimeCount[i] = 0;
    _average[i] = 0;
  }
}

// static 
RaceEvent &RaceEvent::getInstance() {
  static RaceEvent instance;
  return instance;
}

// RaceEvent::RaceEvent(const RaceEvent &) = delete; // no copying
// RaceEvent::RaceEvent &operator=(const RaceEvent &) = delete;

String RaceEvent::toString() {
  String str;
  char buffer[16];
  for (int i=0 ; i < _carCount ; i++) {
    str += "  Car "; str += (i+1); str += "  "; str += _car[i].toString(); str += ", ";
    str += _elapsedTimeCount[i]; str += " time records";

    if (_elapsedTimeCount[i] > 0) {
      str += ": ";
      for (int j=0 ; j < _elapsedTimeCount[i] ; j++) {
        sprintf(buffer, "%1.3f", _elapsedTime[i][j]);
        str += buffer; str += " ";
      }
      sprintf(buffer, "%1.3f", _average[i]);
      str += "--> "; str += buffer; str += " (average time)\n";
    } else {
      str += ".\n";
    }

  }
  return str;
}

void RaceEvent::addCar(Car c) {
  // Serial.print("addCar: _carCount: "); Serial.print(_carCount); Serial.print(", NUMBER_OF_CARS: "); Serial.println(NUMBER_OF_CARS);
  if (_carCount == NUMBER_OF_CARS) {
    Serial.println("WARNING: carCount exceeds NUMBER_OF_RACERS");
    return;
  }
  _car[_carCount] = c;
  _carCount++;
  // Serial.print("_carCount incremented to: "); Serial.println(_carCount);
}

void RaceEvent::addCars(Car car[NUMBER_OF_CARS]) {
  for (int i=0 ; i < NUMBER_OF_CARS ; i++) {
    _car[i] = car[i];
  }
}

Car RaceEvent::getCar(int index) {
  if (index < NUMBER_OF_CARS) {
    return _car[index];
  }
}

int RaceEvent::getCarCount() {
  return _carCount;
}

void RaceEvent::addElapsedTime(int carIndex, float seconds) {
  _elapsedTime[carIndex][_elapsedTimeCount[carIndex]] = seconds;
  _elapsedTimeCount[carIndex]++;
}

void RaceEvent::removeMostRecentElapsedTime(int carIndex) {
  _elapsedTimeCount[carIndex]--;
  _elapsedTime[carIndex][_elapsedTimeCount[carIndex]] = 0; // this is probably not necessary
}

float RaceEvent::getElapsedTime(int carIndex, int elapsedTimeIndex) {
  if ((carIndex < NUMBER_OF_CARS) && (elapsedTimeIndex < _elapsedTimeCount[carIndex])) {
    return _elapsedTime[carIndex][elapsedTimeIndex];
  } else {
    Serial.println("ERROR: second index to _elapsedTime exceeded expected value");
    return 0;
  }
}

int RaceEvent::getElapsedTimeCount(int carIndex) {
  return _elapsedTimeCount[carIndex];
}

void RaceEvent::calculateAverages() {
  for (int carIndex = 0 ; carIndex < NUMBER_OF_CARS ; carIndex++) {
    float sum = 0;
    for (int elapsedTimeIndex = 0 ; elapsedTimeIndex < _elapsedTimeCount[carIndex] ; elapsedTimeIndex++) {
      sum += _elapsedTime[carIndex][elapsedTimeIndex];
    }
    _average[carIndex] = sum / _elapsedTimeCount[carIndex];
  }
}

float RaceEvent::getAverage(int index) {
  return _average[index];
}

void RaceEvent::generateLeaderboard() {
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

float RaceEvent::getLeaderboardTime(int index) {
  return _leaderboardTime[index];
}

int RaceEvent::getLeaderboardCar(int index) {
  return _leaderboardCar[index];
}

String RaceEvent::leaderboardToString() {
  String str = "Leaderboard:\n";
  char buffer[64];
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
    str += "  ["; str += (i+1); str += th; str += "]: ";
    if (_leaderboardTime[i] >= 0) {
      sprintf(buffer, "%6.3f", _leaderboardTime[i]);
    } else {
      sprintf(buffer, " -----");
    }
    str += buffer;
    str += " "; str += getCar(_leaderboardCar[i]).toString(); str += "\n";
  }
  return str;
}
