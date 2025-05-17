#ifndef RaceEvent_h
#define RaceEvent_h

#include "Arduino.h"
#include "Constants.h"
#include "Car.h"

class RaceEvent {
  private:
    Car _car[NUMBER_OF_CARS];
    int _leaderboardCar[NUMBER_OF_CARS];
    float _leaderboardTime[NUMBER_OF_CARS];
    float _elapsedTime[NUMBER_OF_CARS][NUMBER_OF_TIMES]; // seconds
    float _average[NUMBER_OF_CARS]; // seconds
    int _carCount;
    int _elapsedTimeCount[NUMBER_OF_CARS];
    RaceEvent();
  public:
    // static RaceEvent &getInstance();
    RaceEvent &getInstance();
    RaceEvent(const RaceEvent &) = delete; // no copying
    RaceEvent &operator=(const RaceEvent &) = delete;
    String toString();
    void addCar(Car c);
    void addCars(Car car[NUMBER_OF_CARS]);
    Car getCar(int index);
    int getCarCount();
    void addElapsedTime(int carIndex, float seconds);
    void removeMostRecentElapsedTime(int carIndex);
    float getElapsedTime(int carIndex, int elapsedTimeIndex);
    int getElapsedTimeCount(int carIndex);
    void calculateAverages();
    float getAverage(int index);
    void generateLeaderboard();
    float getLeaderboardTime(int index);
    int getLeaderboardCar(int index);
    String leaderboardToString();
};

#endif