#ifndef Schedule_h
#define Schedule_h

#include "Arduino.h"
#include "Constants.h"
#include "Heat.h"

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
    Schedule();
    Schedule(int laneUsageCount, int numberOfCars, int numberOfTimes);
    String toString();
    Heat nextHeat();
    void setNumberOfCars(int numberOfCars);
    int getNumberOfCars();
    void setNumberOfTimes(int numberOfTimes);
    int getNumberOfTimes() ;
    void setLaneUsageCount(int laneUsageCount);
    void createRegularHeats();
    void setFinals(Heat finals);
    void setExtra(Heat extra);
    void incrementCurrentHeatNumber();
    int getCurrentHeatNumber();
};

#endif