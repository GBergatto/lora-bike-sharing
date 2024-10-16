#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

#include "Arduino.h"
#include <math.h>

#define DELTA 20000

#define TABLE_SIZE 64
#define QUEUE_SIZE 16

#define LIMIT_DISTANCE 1.0  // km
#define EARTH_RADIUS 6371   // km

enum COMMAND {
  UPDATE,
  PURGE,
};

enum VEHICLE_TYPE {
  BICYCLE,
  ELECTRIC_BICYCLE,
  CARGO_BICYCLE,
  MOTORCYCLE,
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

  Payload queue[QUEUE_SIZE];
  int qhead, qtail, qsize;

  float computeDistance(float longitude, float latitude);
  int getIndex(uint16_t id);
  void update(const Payload &new_data);
  void purge(uint16_t id);
  bool isQueueFull();

public:
  int tableTail;
  Payload data;

  LoRaNode();
  bool checkSequence(uint16_t id, uint8_t seq_num);
  void handleMessage(const Payload &message);
  int enqueue(const Payload &message);
  int dequeue(Payload *message);
};

#endif
