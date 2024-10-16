#include "LoRaNode.h"

LoRaNode::LoRaNode() {
  tableTail = 0;
  qhead = 0;
  qtail = -1;
  qsize = 0;
}

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
  // Convert to rad
  longitude *= M_PI / 180.0f;
  latitude *= M_PI / 180.0f;
  
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
      return (table[i].seq_num < seq_num || table[i].seq_num == 0xFF);
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
    if (tableTail < TABLE_SIZE) {
      table[tableTail++] = newRecord;
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
    for (int i = id; i < tableTail; i++) {
      table[i] = table[i + 1];
    }
    tableTail--;
  }
}

/**
* Add record to the queue
* Return: -1 if operation failed, -2 if distance limit was exceeded,
*         otherwise number of records in the queue
*/
int LoRaNode::enqueue(const Payload &message) {
  if (qsize == QUEUE_SIZE) {
    return -1;
  } else {
    float d = computeDistance(message.longitude, message.latitude)
    Serial.print("Distance to source node: ");
    Serial.println(d);
    if (d > LIMIT_DISTANCE) {
      return -2;  // too far away to relay information
    }

    // TODO: remove outdated messages (same id and smaller seq_number) from the queue

    qtail = (qtail + 1) % QUEUE_SIZE;
    queue[qtail] = message;

    return ++qsize;
  }
}

/**
* Add record to the queue
* Return: -1 if queue was empty, otherwise number of records in the queue
*/
int LoRaNode::dequeue(Payload *message) {
  if (qsize == 0) {
    return -1;
  }

  *message = queue[qhead];
  qhead = (qhead + 1) % QUEUE_SIZE;

  return --qsize;
}
