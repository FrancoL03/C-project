
//
// Created by Zhenjie liu on 24.12.19.
//
#define _GNU_SOURCE
#include <poll.h>
#include <inttypes.h>
#include "connmgr.h"

void connmgr_listen(int port_number,sbuffer_t *sbuffer)
{
#ifdef DEBUG        //if you turn on the debug mode, then a .txt file will be generated to store the measurement data
    FILE* debug_txt;
    debug_txt = fopen("Debug_outout.txt", "w");
#endif
    conn_socket_t * sever_node;
    conn_socket_t * conn_manager_node ;
    conn_socket_t * node_at_index;

    tcpsock_t * server, * client;  
    sensor_data_t data; 
    int bytes, result;   
    int fds_counter = 0;
    unsigned long current_ts;
    struct pollfd  *fds;
    bool time_flag = false;
    unsigned long clear_time = 0;//the moment all the socket connections are terminated
    unsigned long terminated_time = 0;//the moment one socket is terminated
    char* log_info = malloc(200 * sizeof(char));
    dplist_t * connmgr_list  = dpl_create(c_element_copy, c_element_free, c_element_compare);//create a list to store sockets.

    if (tcp_passive_open(&server,port_number)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
    conn_manager_node=(conn_socket_t*)malloc(sizeof(conn_socket_t));
    sever_node =(conn_socket_t*)malloc(sizeof(conn_socket_t));
    fds=(struct pollfd*)malloc(sizeof(struct pollfd)); //we are going to use this as a dynamic array
    fds[0].fd=server->sd;
    fds[0].events=POLLIN;

    sever_node->tcpsock_pointer=server; //initialization
    sever_node->tcpsock_id = 0; 
    sever_node->tcpsock_state = true;
    sever_node->tcpsock_last_ts = time(NULL);
    data.id = 0;
    data.value = 0;
    data.ts = 0;

    while(sbuffer->status->connmgr_status == 0 && sbuffer->status->storemgr_status == 0 ) // "0" means open
    {
        printf("Now getting into the poll() function \n"); //for debug, please delete it
        fflush(stdout);
        poll(fds, fds_counter + 1, 1000); // wait 1 second, if no data coming in, go on
        if(fds[0].revents == POLLIN)  //if new TCP sockets coming
        {
            if (tcp_wait_for_connection(server,&client)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
            printf("Incoming  client connection  %d\n", fds_counter);
            fflush(stdout);
            conn_manager_node->tcpsock_pointer=client;//new TCP connection comes, add it into the conn_manager_node
            conn_manager_node->tcpsock_id=fds_counter;
            conn_manager_node->tcpsock_state = true;
            conn_manager_node->tcpsock_last_ts = time(NULL);
            conn_manager_node->sensor_id = 0; //initialization

            bytes = sizeof(data.id);
            tcp_receive(conn_manager_node->tcpsock_pointer, (void *)&data.id, &bytes);

            bytes = sizeof(data.value);
            tcp_receive(conn_manager_node->tcpsock_pointer, (void *)&data.value, &bytes);

            bytes = sizeof(data.ts);
            tcp_receive(conn_manager_node->tcpsock_pointer, (void *)&data.ts, &bytes);
            printf("/***********\nNew sensor node connection added! Sensor id = %d""\n/************\n", data.id);
            fflush(stdout);
            printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value, (long int)data.ts);
            fflush(stdout);

            pthread_mutex_lock(sbuffer->lock);
            if(sbuffer->status->if_connmgr_write == 1) //means connmgr can not wirte, because storemgr is still reading
            {pthread_cond_wait(   &(sbuffer->cond_connmgr), sbuffer->lock);}//block the thread, till storemgr finish reading
            else{}

            sbuffer_insert(sbuffer, &data);
            sbuffer->status->if_connmgr_write = 1;//reset this value, prepare for next time ("1" disabled, "0" able)
            sbuffer->status->if_datamgr_read = 0;
            pthread_cond_signal(&(sbuffer->cond_datamgr)); //send signal to activate datamgr
            pthread_mutex_unlock(sbuffer->lock);

            sprintf(log_info, "A sensor node with ID = %d has opened a new connection", data.id);
            log_write(log_info, data.ts);

            conn_manager_node->sensor_id = data.id;
            dpl_insert_at_index(connmgr_list, conn_manager_node, fds_counter, true); // we use the dplist to store the new TCP sockets
            fds_counter++;

            struct pollfd  *new_fds=(struct pollfd*)realloc(fds, (1 + fds_counter) * sizeof(struct pollfd));
            if (new_fds == NULL)
            {}
            else
            {fds = new_fds;} //realloc succeed
            fds[fds_counter].fd= conn_manager_node->tcpsock_pointer->sd;
            fds[fds_counter].events=POLLIN;
            fds[fds_counter].revents = 0;  //initialization
        }
        else{}
        for(int i=0;i<dpl_size(connmgr_list);i++) //loop through the dplist, print the data of each node
        {
            node_at_index = (conn_socket_t *) (dpl_get_element_at_index(connmgr_list, i));
            int this_sensor_id = node_at_index->sensor_id;
            current_ts = time(NULL);
            if ((current_ts - (node_at_index->tcpsock_last_ts)) > TIMEOUT )
            {
                terminated_time = time(NULL);
                printf("A sensor node with ID = %d was terminated due to timeout, timestamp = %ld.\n", node_at_index->sensor_id, terminated_time);

                sprintf(log_info, "A sensor node with ID = %d has closed the connection.", node_at_index->sensor_id);
                log_write(log_info, terminated_time);

                tcp_close(&(node_at_index->tcpsock_pointer));
                dpl_remove_at_index(connmgr_list, i,true);
                i--; //because a node is removed from the list

                if(dpl_size(connmgr_list)==0)//if any sockets exist
                {
                    time_flag = true; //this flag is used to avoid terminating the serve at the beginning (when the list is empty)
                    clear_time = time(NULL);
                }
                else{}
                continue;
            }
            else{}
            if(node_at_index->tcpsock_state == false) //if this socket is disconnected, we skip it
            {
                continue;
            }
            else
            {
                int fds_index = 1;
                for(int j =1; j < fds_counter + 1; j++) //try to find the correct socket
                {
                    if(fds[j].fd == node_at_index->tcpsock_pointer->sd)
                    {fds_index = j;
                     break;
                    }
                    else{};
                }

                if(fds[fds_index].revents == POLLIN) //if there are data available to read
                {
                        bytes = sizeof(data.id);
                        tcp_receive(node_at_index->tcpsock_pointer, (void *) &data.id, &bytes);
                        bytes = sizeof(data.value);
                        tcp_receive(node_at_index->tcpsock_pointer, (void *)&data.value, &bytes);
                        bytes = sizeof(data.ts);
                        result = tcp_receive(node_at_index->tcpsock_pointer, (void *)&data.ts, &bytes);

                    if ((result==TCP_NO_ERROR) && bytes)
                    {
                        pthread_mutex_lock(sbuffer->lock);
                        if(sbuffer->status->if_connmgr_write == 1) //means connmgr can not wirte, because storemgr is still reading
                        {pthread_cond_wait(   &(sbuffer->cond_connmgr), sbuffer->lock);}//block the thread, till storemgr finish reading

                        sbuffer_insert(sbuffer, &data);
                        sbuffer->status->if_connmgr_write = 1;
                        sbuffer->status->if_datamgr_read = 0;
                        node_at_index->tcpsock_last_ts = time(NULL);
                        pthread_cond_signal(&(sbuffer->cond_datamgr));
                        pthread_mutex_unlock(sbuffer->lock);

                        printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value, (long int)data.ts);
                        fflush(stdout);
#ifdef DEBUG
                            fprintf(debug_txt, "%d  %f  %lu \n", data.id, data.value, (unsigned long int)data.ts);
#endif
                    }
                    else if (result==TCP_CONNECTION_CLOSED)
                    {
                        fprintf(stdout,"/**********\nPeer has closed connection, sensor id = %d \n /**********\n", this_sensor_id);
                        fflush(stdout);
                        unsigned  long time_closed = time(NULL);
                        printf("closed time is: %lu \n", time_closed);
                        fflush(stdout);

                        fds[fds_index].fd = -1; //disable this fd
                        node_at_index->tcpsock_state = false; //change the state of this connection.
                    }
                    else{}
                }
                else{}
            }
        }
        int current_time_v2 = time(NULL);//finally, if no sockets exist, then terminate the server after TIMEOUT
        if(dpl_size(connmgr_list)==0 && (current_time_v2 - clear_time) > TIMEOUT && time_flag==true)
        {
            printf("/************\n Server closed, due to empty list of sockets\n ************/ \n");
            fflush(stdout);
            break; 
        }
        else{}
    }
    free(server);//free all the variables and buffers
    free(sever_node);
    dpl_free(&connmgr_list,true);
    free(conn_manager_node);
    free(fds);
    free(log_info);
#ifdef DEBUG
    fclose(debug_txt);
#endif
    sbuffer->status->connmgr_status = 1; //disable it
    pthread_cond_signal(&(sbuffer->cond_datamgr));//send signal to activate datamgr　(when connmgr is closing)
}

void * c_element_copy(void * src_element)
{
    conn_socket_t* copy = malloc(sizeof(conn_socket_t)); //create a copy
    copy->tcpsock_pointer = ((conn_socket_t*)(src_element))->tcpsock_pointer;
    copy->tcpsock_id = ((conn_socket_t*)(src_element))->tcpsock_id;
    copy->tcpsock_last_ts= ((conn_socket_t*)(src_element))->tcpsock_last_ts;
    copy->tcpsock_state = ((conn_socket_t*)(src_element))->tcpsock_state;
    copy->sensor_id     = ((conn_socket_t*)(src_element))->sensor_id;

    copy->tcpsock_pointer->sd = ((conn_socket_t*)(src_element))->tcpsock_pointer->sd;
    copy->tcpsock_pointer->cookie = ((conn_socket_t*)(src_element))->tcpsock_pointer->cookie;
    copy->tcpsock_pointer->ip_addr = ((conn_socket_t*)(src_element))->tcpsock_pointer->ip_addr;
    copy->tcpsock_pointer->port = ((conn_socket_t*)(src_element))->tcpsock_pointer->port;
    return copy;
}

void c_element_free(void ** element)
{
    free(*element);
}

int c_element_compare(void * x, void * y) //	Compare two element elements; returns -1 if x<y, 0 if x==y, or 1 if x>y
{
    if ((conn_socket_t *) (x) < (conn_socket_t *) (y)) { return -1; }
    else if ((conn_socket_t *) (x) == (conn_socket_t *) (y)) { return 0; }
    else { return 1; }
}

