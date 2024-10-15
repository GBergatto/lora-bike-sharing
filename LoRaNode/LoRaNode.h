#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

#include "Arduino.h"
#include <math.h>

#define DELTA 20000

#define TABLE_SIZE 64

#define LIMIT_DISTANCE 2.0  // km
#define EARTH_RADIUS 6371   // km

enum COMMAND {
  UPDATE = 1,
  PURGE = 2,
};

enum VEHICLE_TYPE {
  BICYCLE = 0,
  ELECTRIC_BICYCLE = 1,
  CARGO_BICYCLE = 2,
  MOTORCYCLE = 3,
};

struct Payload {
  uint16_t id;
  uint8_t seq_num;
  uint16_t timer;
  float longitude;
  float latitude;
  uint8_t battery;
  uint8_t vehicle_type;
  uint8_t price;
  uint8_t command;
};

class LoRaNode {
  Payload table[TABLE_SIZE];
  int tail = 0;

  int getIndex(uint16_t id);
  void update(const Payload &new_data);
  void purge(uint16_t id);

public:
  Payload data;

  Node();
  void handleMessage(const Payload &message);
  bool checkSequence(uint16_t id, uint8_t seq_num);
  float computeDistance(float longitude, float latitude);
};

#endif
