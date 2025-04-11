#include "Arduino.h"
#include "Car.h"

Car::Car() {
  Car("", "");
}

Car::Car(const Car &car) {
  _driverName = car._driverName;
  _carName = car._carName;
}

Car::Car(String driverName, String carName) {
  _driverName = driverName;
  _carName = carName;
}

String Car::toString() {
  String str;
  str += _driverName; str += "-"; str += _carName;
  return str;
}

void Car::setDriverName(String driverName) {
  _driverName = driverName;
}

String Car::getDriverName() {
  return _driverName;
}

void Car::setCarName(String carName) {
  _carName = carName;
}

String Car::getCarName() {
  return _carName;
}
