#include "Arduino.h"
#include "Constants.h"
#include "Heat.h"
#include "Schedule.h"

Schedule::Schedule() {
  Serial.println("Schedule, default constructor called");
}

Schedule::Schedule(int laneUsageCount, int numberOfCars, int numberOfTimes) {
  _numberOfCars = numberOfCars;
  _numberOfTimes = numberOfTimes;
  _laneUsageCount = laneUsageCount;
  _currentHeatNumber = 0;
  // Serial.println("Schedule constructor:");
  // Serial.print("  _laneUsageCount: "); Serial.print(_laneUsageCount);
  // Serial.print(", _numberOfCars: "); Serial.print(_numberOfCars);
  // Serial.print(", numberOfTimes: "); Serial.println(_numberOfTimes);
}

String Schedule::toString() {
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

Heat Schedule::nextHeat() {
  if (_currentHeatNumber < regularHeatCount) {
    return _heat[_currentHeatNumber];
  } else if (_currentHeatNumber == regularHeatCount) {
    return _finals;
  } else {
    return _extra;
  }
}

void Schedule::setNumberOfCars(int numberOfCars) {
  _numberOfCars = numberOfCars;
}

int Schedule::getNumberOfCars() {
  return _numberOfCars;
}

void Schedule::setNumberOfTimes(int numberOfTimes) {
  _numberOfTimes = numberOfTimes;
}

int Schedule::getNumberOfTimes() {
  return _numberOfTimes;
}

void Schedule::setLaneUsageCount(int laneUsageCount) {
  _laneUsageCount = laneUsageCount;
}

void Schedule::createRegularHeats() {
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
      // Serial.print("Last regular heat, setting thisHeatLaneCount: "); Serial.println(thisHeatLaneCount);
    }
    // Serial.print("thisHeatLaneCount: "); Serial.println(thisHeatLaneCount);
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

void Schedule::setFinals(Heat finals) {
  // _finals = finals;
}

void Schedule::setExtra(Heat extra) {
  // _extra = extra;
}
