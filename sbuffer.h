#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#include "config.h"

/*
 * All data that can be stored in the sbuffer should be encapsulated in a
 * structure, this structure can then also hold extra info needed for your implementation
 */
typedef struct sbuffer_data {
    sensor_data_t data;
}sbuffer_data_t;

typedef struct sbuffer_node {
    struct sbuffer_node * next;
    sbuffer_data_t element;
} sbuffer_node_t;

typedef struct sbuffer {
    sbuffer_node_t * head;
    sbuffer_node_t * tail;
    pthread_cond_t cond_connmgr;  //condition variable
    pthread_cond_t cond_datamgr;
    pthread_cond_t cond_storemgr;
    manager_status_t* status;
    pthread_mutex_t*  lock;
}sbuffer_t;

/*
 * Allocates and initializes a new shared buffer
 * Returns SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
 */
int sbuffer_init(sbuffer_t ** buffer);

/*
 * All allocated resources are freed and cleaned up
 * Returns SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
 */
int sbuffer_free(sbuffer_t ** buffer);


/*
 * Removes the first data in 'buffer' (at the 'head') and returns this data as '*data'  
 * 'data' must point to allocated memory because this functions doesn't allocated memory
 * If 'buffer' is empty, the function doesn't block until new data becomes available but returns SBUFFER_NO_DATA
 * Returns SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
 */
int sbuffer_remove(sbuffer_t * buffer);


/* Inserts the data in 'data' at the end of 'buffer' (at the 'tail')
 * Returns SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
 */
int sbuffer_insert(sbuffer_t * buffer, sensor_data_t * data);


/*
 * This function is used to get the node in the sbuffer at a specific position
 */
sensor_data_t* sbuffer_get_sensor_data_at_index(sbuffer_t * buffer, int index);


/*
 * This function is used to get the size of the sbuffer
 */
int sbuffer_get_size(sbuffer_t * buffer);


/*
 * This function is used to write log contents
 */
void log_write (char* content, unsigned long time_stamp);

#endif  //_SBUFFER_H_

