#include "Arduino.h"
#include "Constants.h"
#include "Heat.h"

Heat::Heat() {
  Heat(-1, LANES, HEAT_TYPE_REGULAR);
}

Heat::Heat(int heatNumber, int laneUsageCount, int heatType) {
  _heatNumber = heatNumber;
  _laneUsageCount = laneUsageCount;
  _heatType = heatType;
}

String Heat::toString() {
  String str;
  str += "    _heatNumber: "; str += _heatNumber; str += "\n";
  str += "    _laneUsageCount: "; str += _laneUsageCount; str += "\n";
  str += "    _heatType: "; str += getHeatType(_heatType); str += "\n";
  str += "    _laneAssignment:\n";
  // for (int laneIndex = 0 ; laneIndex < _laneUsageCount ; laneIndex++) {
  //   str += "      _laneAssignment["; str += laneIndex; str += "]: \""; str += _laneAssignment[laneIndex]; str += "\" (carIndex)\n";
  // }
  // for (int laneIndex = 0 ; laneIndex < _laneUsageCount ; laneIndex++) {
  //   str += "      Lane "; str += (laneIndex + 1); str += ": \"";
  //   str += (raceEvent.getCar(_laneAssignment[laneIndex])).toString();
  //   str += "\" (car "; str += (_laneAssignment[laneIndex] + 1); str += ")\n";
  // }
  for (int laneIndex = 0 ; laneIndex < _laneUsageCount ; laneIndex++) {
    str += "      Lane "; str += (laneIndex + 1); str += ": ";
    // str += (raceEvent.getCar(_laneAssignment[laneIndex])).toString();
    str += " car "; str += (_laneAssignment[laneIndex] + 1); str += "\n";
  }
  return str;
}

String Heat::getHeatType(int type) {
  if (type == HEAT_TYPE_REGULAR) {
    return "Regular Heat";
  } else if (type == HEAT_TYPE_FINALS) {
    return "Finals!!!";
  } else if (type == HEAT_TYPE_EXTRA) {
    return "Extra Heat";
  } else {
    return "<unknown>";
  }
}

void Heat::setHeatNumber(int heatNumber) {
  _heatNumber = heatNumber;
}

int Heat::getHeatNumber() {
  return _heatNumber;
}

void Heat::setLaneUsageCount(int laneUsageCount) {
  _laneUsageCount = laneUsageCount;
}

int Heat::getLaneUsageCount() {
  return _laneUsageCount;
}

void Heat::setHeatType(int heatType) {
  _heatType = heatType;
}

int Heat::getHeatType() {
  return _heatType;
}

void Heat::setLaneAssignment(int laneIndex, int carIndex) {
  _laneAssignment[laneIndex] = carIndex;
}

int Heat::getLaneAssignment(int laneIndex) {
  return _laneAssignment[laneIndex];
}

