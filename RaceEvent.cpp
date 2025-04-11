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
  for (int i=0 ; i < _carCount ; i++) {
    str += "car "; str += (i+1); str += "  "; str += _car[i].toString(); str += ", ";
    str += _elapsedTimeCount[i]; str += " time records: ";
    for (int j=0 ; j < _elapsedTimeCount[i] ; j++) {
      str += _elapsedTime[i][j]; str += " ";
    }
    str += "--> "; str += _average[i]; str += " (average time)\n";
  }
  return str;
}

void RaceEvent::addCar(Car c) {
  // Serial.print("addCar: _carCount: "); Serial.print(_carCount); Serial.print(", NUMBER_OF_CARS: "); Serial.println(NUMBER_OF_CARS);
  if (_carCount == NUMBER_OF_CARS) {
    Serial.println("ERROR: carCount exceeds NUMBER_OF_RACERS");
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

// void setCar(int carIndex, Car c) {
//   if (index < _carCount) {
//     _car[carIndex] = c;
//   }
// }

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
    // Serial.print("averages: carIndex: "); Serial.print(carIndex);
    // Serial.print(", sum: "); Serial.print("sum");
    // Serial.print(", _elapsedTimeCount["); Serial.print(carIndex); Serial.print("]: "); Serial.print(_elapsedTimeCount[carIndex]);
    // Serial.print(", average: "); Serial.println(_average[carIndex]);
  }
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

String RaceEvent::leaderboardToString() {
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
    str += "  ["; str += (i+1); str += th; str += "] car["; str += _leaderboardCar[i]; str += "]: ";
    // str += (_leaderboardCar[i] + 1);
    str += (getCar(_leaderboardCar[i]).toString());
    str += ", average time: "; str += _leaderboardTime[i]; str += "\n";
  }
  return str;
}

void RaceEvent::createFinalHeat() {
  // Heat finals(regularHeatCount, min(NUMBER_OF_CARS, LANES), HEAT_TYPE_FINALS);
  // return finals;
}

void  RaceEvent::createExtraHeat() {
  // Heat extra(regularHeatCount + 1, LANES, HEAT_TYPE_EXTRA);
  // for (int lane = 0 ; lane < LANES ; lane++) {
  //   extra.setLaneAssignment(lane, Car("Anyone", "Any Car"));
  // }
  // return extra;
}
