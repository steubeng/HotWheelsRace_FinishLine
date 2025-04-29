#ifndef Constants_h
#define Constants_h

#define NUMBER_OF_CARS 4
#define NUMBER_OF_TIMES 2
#define LANES 4

#define HEAT_TYPE_REGULAR 0
#define HEAT_TYPE_FINALS 1
#define HEAT_TYPE_EXTRA 2

const int regularHeatCount = ceil(((NUMBER_OF_CARS * NUMBER_OF_TIMES) / float(min(LANES, NUMBER_OF_CARS))));

#endif