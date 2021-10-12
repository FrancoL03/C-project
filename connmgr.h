//
// Created by Zhenjie liu on 24.12.19.
//
#include "sbuffer.h"
#include "config.h"
#include "./lib/tcpsock.h"
#include "./lib/dplist.h"

struct conn_socket
{
    tcpsock_t* tcpsock_pointer;     // the specific TCP socket
    int tcpsock_id;                 // the TCP socket id
    unsigned long tcpsock_last_ts;  // the 'last active' timestamp of every socket connection
    bool tcpsock_state;             // 'true' means the connection is open, 'false' means close
    int sensor_id;
};

typedef struct conn_socket conn_socket_t;

void * c_element_copy(void * src_element);//callback functions
void c_element_free(void ** element);
int c_element_compare(void * x, void * y);

/*
 * This method starts listening on the given port and when when a sensor node connects it 
 * stores the sensor data in the shared buffer.
 */
void connmgr_listen(int port_number, sbuffer_t *shared_buffer);

/*
 * This method should be called to clean up the connmgr, and to free all used memory. 
 * After this no new connections will be accepted
 */
void connmgr_free();


