#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

#include <map>
#include <math.h>

#define DEFAULT_LIMIT_DISTANCE 2.0 // km
#define EARTH_RADIUS 6371          // km

using namespace std;

struct Payload
{
    enum COMMAND
    {
        UPDATE = 1,
        PURGE = 2,
    };

    enum VEHICLE_TYPE
    {
        BICYCLE = 0,
        ELECTRIC_BICYCLE = 1,
        CARGO_BICYCLE = 2,
        MOTORCYCLE = 3,
    };

    uint16_t id;
    uint8_t seq_num;
    uint16_t timer;
    float longitude;
    float latidute;
    uint8_t battery;
    uint8_t vehicle_type;
    uint8_t price;
    uint8_t command;
};

class NodeManager
{
public:
    Payload data;
    map<uint16_t, Payload> table;

    NodeManager() {}
    ~NodeManager() {}

    /**
     * Compute distance from this node to the source data's node
     * return: distance in (km)
     */
    float computeDistance(const Payload &target_data)
    {
        // Using Haversine formular
        float dlat = target_data.latidute - data.latidute;
        float dlon = target_data.longitude - data.longitude;

        float a = pow(sin(dlat / 2), 2) + cos(data.latidute) * cos(target_data.latidute) * pow(sin(dlon / 2), 2);
        float c = 2 * atan2(sqrt(a), sqrt(1 - a));

        return EARTH_RADIUS * c;
    }

    /**
     * Check distance from this node to source data's node
     * return: true if the distance is under limitation, false in other case
     */
    bool checkDistance(const Payload &target_data)
    {
        if (computeDistance(target_data) > DEFAULT_LIMIT_DISTANCE)
            return false;

        return true;
    }

    /**
     * Check record existance
     * return false if no record found, true in other case
     */
    bool checkRecord(uint16_t id)
    {
        auto it = table.find(id);
        if (it == table.end())
        {
            return false;
        }

        return true;
    }

    /**
     * Check the target data is new or outdated
     * return: true if the target data is new, false in other cases
     */
    bool checkSequence(const Payload &target_data)
    {
        auto it = table.find(target_data.id);
        if (it == table.end())
        {
            // Case: No record found in table => the target data is new
            return true;
        }

        if (it->second.seq_num < target_data.seq_num)
        {
            // Case: The target data is new
            return true;
        }

        return false;
    }

    /**
     * Update new data into the table, please checkSeq and distance before do this
     */
    void update(const Payload &target_data)
    {
        Payload new_data = target_data;
        table[new_data.id] = new_data;
    }

    /**
     * Purge data from table, please checkSeq and before do this
     */
    void purge(uint16_t id)
    {
        table.erase(id);
    }
};

#endif
