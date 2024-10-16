#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

#include "Arduino.h"
#include <math.h>

#define DELTA 20000

#define TABLE_SIZE 64
#define QUEUE_SIZE 16

#define LIMIT_DISTANCE 1.0  // km
#define EARTH_RADIUS 6371   // km

#define GET_BATTERY(byte) ((byte)&0x7F)
#define GET_FLAG(byte) (((byte)&0x80) != 0)  // update/purge
#define SET_C_BATTERY(battery, command) ((battery & 0x7F) | (command ? 0x80 : 0x00))

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
  float latitude;
  float longitude;
  uint8_t c_battery;  // the highest bit contains the command flag
  uint8_t vehicle_type;
  uint8_t price;
};

class LoRaNode {
  Payload table[TABLE_SIZE];

  Payload queue[QUEUE_SIZE];
  int qhead, qtail, qsize;

  float computeDistance(float latitude, float longitude);
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
