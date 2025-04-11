#ifndef Heat_h
#define Heat_h

#include "Arduino.h"
#include "Constants.h"
#include "RaceEvent.h"

class Heat {
  private:
    RaceEvent &raceEvent = raceEvent.getInstance();
    int _heatNumber;
    int _laneUsageCount;
    int _heatType;
    int _laneAssignment[LANES];
  public:
    Heat();
    Heat(int heatNumber, int laneUsageCount, int heatType);
    String toString();
    String getHeatType(int type);
    void setHeatNumber(int heatNumber);
    int getHeatNumber();
    void setLaneUsageCount(int laneUsageCount);
    int getLaneUsageCount();
    void setHeatType(int heatType);
    int getHeatType();
    void setLaneAssignment(int laneIndex, int carIndex);
    int getLaneAssignment(int laneIndex);
};

#endif
