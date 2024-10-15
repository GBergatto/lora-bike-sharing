#include "application.h"

void LoRaNode::handleMessage(const Payload &message) {
  // TODO: use the highest bit of vehicle type instead
  if (message.command == UPDATE) {
    if (checkSequence(message.id, message.seq_num)) {
      update(message);
    }
  } else {  // purge
    purge(message.id);
  }
}

/**
 * Compute distance from this node to the source data's node
 * return: distance in (km)
 */
float LoRaNode::computeDistance(float longitude, float latitude) {
  // Using Haversine formular
  float dlat = latitude - data.latitude;
  float dlon = longitude - data.longitude;

  float a = pow(sin(dlat / 2), 2) + cos(data.latitude) * cos(latitude) * pow(sin(dlon / 2), 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));

  return EARTH_RADIUS * c;
}

/**
 * Check the target data is new or outdated
 * return: true if the target data is new, false otherwise
 */
bool LoRaNode::checkSequence(uint16_t id, uint8_t seq_num) {
  for (int i = 0; i < TABLE_SIZE; i++) {
    if (table[i].id == id) {
      return (table[i].seq_num < seq_num);
    }
  }
  return true;  // no matching ID in the table
}

/**
 * Get the index of a record in the table given its ID
 * return: -1 if the record isn't found
 */
int LoRaNode::getIndex(uint16_t id) {
  for (int i = 0; i < TABLE_SIZE; i++) {
    if (table[i].id == id) {
      return i;
    }
  }
  return -1;
}

/**
 * Add new record to the table or update and existing one
 */
void LoRaNode::update(const Payload &newRecord) {
  int i = getIndex(newRecord.id);
  if (i != -1) {  // update existing record
    table[i] = newRecord;
  } else {  // new record
    if (tail < TABLE_SIZE) {
      table[tail++] = newRecord;
    } else {
      // TODO: replace record of the furthest away bike with new_data
    }
  }
}

/**
 * Purge record from table given
 */
void LoRaNode::purge(uint16_t id) {
  if (getIndex(id) != -1) {
    for (int i = id; i < tail; i++) {
      table[i] = table[i + 1];
    }
  }
  tail--;
}
