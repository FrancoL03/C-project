#ifndef DATAMGR_H
#define DATAMGR_H

#include "config.h"
#include "sbuffer.h"
#include "./lib/dplist.h"

typedef struct
{
    uint16_t room_ID;
    uint16_t sensor_ID;
    time_t   last_modified_timestamp ;  //time_t is a long int type
    double   running_avg;
    double   tempArray[RUN_AVG_LENGTH]; //the array is used for storing the temperature value
}sensor_node;

void * d_element_copy(void * src_element);//callback functions
void d_element_free(void ** element);
int  d_element_compare(void * x, void * y);

/*
 * Reads continiously all data from the shared buffer data structure, parse the room_id's
 * and calculate the running avarage for all sensor ids
 * When *buffer becomes NULL the method finishes. This method will NOT automatically free all used memory
 */
void datamgr_parse_sensor_data(FILE * fp_sensor_map, sbuffer_t* sbuffer); //'read_index' means the index of the node of the sbuffer

/*
 * This method should be called to clean up the datamgr, and to free all used memory.
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
 */
void datamgr_free(dplist_t* datamgr_list);

/*
 * Gets the room ID for a certain sensor ID
 */
uint16_t datamgr_get_room_id(sensor_id_t sensor_id);

/*
 * Gets the running AVG of a certain senor ID 
 */
sensor_value_t datamgr_get_avg(sensor_id_t sensor_id);

/*
 * Returns the time of the last reading for a certain sensor ID
 */
time_t datamgr_get_last_modified(sensor_id_t sensor_id);

/*
 * Return the total amount of unique sensor ID's recorded by the datamgr
 */
int datamgr_get_total_sensors();

/*
 * this function is used to check whether there are same sensors (id) when reading from the 'room_sensor' file
 */
bool check_sensor_duplication (uint16_t sensorID, dplist_t* datamgr_list);

#endif /* DATAMGR_H */

