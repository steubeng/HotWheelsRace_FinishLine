#ifndef Car_h
#define Car_h

#include "Arduino.h"

class Car {
  private:
    String _driverName;
    String _carName;
  public:
    Car();
    Car(const Car&);
    Car(String driverName, String carName);
    String toString();
    void setDriverName(String driverName);
    String getDriverName();
    void setCarName(String carName);
    String getCarName();
};

#endif