#define _GNU_SOURCE
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdint.h>    // int16_t, uint32_t, int64_t, used to replace short, int, long types
#include <assert.h>    // used for error inspection
#include <memory.h>    // used for malloc
#include <time.h>      // used for time_t type
#include <pthread.h>   // uesd for pthread and mutex locks
#include <sqlite3.h>   // used for sqlite3 databse

//The #define below are used in the main.c
#define FIFO_NAME  "logFifo"

//The #define below are used in the connmgr.c
#ifndef TIMEOUT
#define TIMEOUT 5
#endif

//The #define below are used in the datamgr.c
#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

#ifndef SET_MAX_TEMP
#define SET_MAX_TEMP 17.00
#endif

#ifndef SET_MIN_TEMP
#define SET_MIN_TEMP 15.00
#endif

//The #define below are used in the sensor_db.c
#ifndef DB_NAME
#define DB_NAME Sensor.db
#endif

#ifndef TABLE_NAME
#define TABLE_NAME SensorData
#endif

#define DBCONN sqlite3

//The #define below are used in the sbuffer.c
#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS  0
#define SBUFFER_NO_DATA  1


//definition of error codes
#define DPLIST_NO_ERROR      0
#define DPLIST_MEMORY_ERROR  1      // error due to memory alloc failure
#define DPLIST_INVALID_ERROR 2      // error due to a list operation applied on a NULL list

//Below are some variables and structs being used
typedef uint16_t sensor_id_t;       // sensor ID
typedef double   sensor_value_t;    // temperature values
typedef time_t   sensor_ts_t;       // timestamps, UTC timestamp as returned by time()

typedef struct
{
    sensor_id_t    id;
    sensor_value_t value;
    sensor_ts_t    ts;
} sensor_data_t;                    //used to store each measurement.


typedef struct
{
    int connmgr_status;
    int datamgr_status;
    int storemgr_status;

    int if_connmgr_write;           //these three variables are used for synchronization
    int if_datamgr_read;
    int if_storemgr_read;
} manager_status_t;                 //used to store the status of the three managers

#endif /* _CONFIG_H_ */

