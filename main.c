
//
// Created by Zhenjie liu on 28.12.19.
//
#define _GNU_SOURCE
#include "main.h"
#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <signal.h>

typedef struct
{
    int    port_num;
    sbuffer_t *sbuffer;
}arg_connmgr_t;                //this is an arguments for creating a thread

typedef struct
{
    FILE * fp_sensor_map;
    sbuffer_t* sbuffer;
}arg_datamgr_t;

typedef struct
{
    sbuffer_t* sbuffer;
}arg_storemgr_t;

void run_child (pthread_mutex_t fifoLock);
pthread_mutex_t fifo_lock = PTHREAD_MUTEX_INITIALIZER; //create a mutex lock for fifo and initialize it

int main()
{
    FILE * fp_gateway_log = fopen("gateway.log", "w+");//used to delete the old log data
    fclose(fp_gateway_log);

    printf("/********** Test server is started **********/\n");
    pid_t child_pid;
    child_pid = fork();

    if(child_pid < 0)
    {
        perror("fork fail");
        exit(EXIT_FAILURE);
    }
    else if(child_pid > 0)   //a parent process
    {
        arg_connmgr_t arg_c; //arguments of the thread functions
        arg_datamgr_t arg_d;
        arg_storemgr_t arg_s;
        mkfifo(FIFO_NAME, 0666); //create a FIFO for log transfer, enable block sate
        FILE * fp_sensor_map;

        pthread_mutex_t main_lock;
        if (pthread_mutex_init(&main_lock, NULL) != 0) {
            printf("\n mutex init has failed\n");
            return 1;
        }
        else{}

        manager_status_t *mgr_status;
        mgr_status = (manager_status_t* )malloc(sizeof(manager_status_t));
        mgr_status->connmgr_status = 0; //initialization, "0" means open
        mgr_status->datamgr_status = 0;
        mgr_status->storemgr_status = 0;
        mgr_status->if_connmgr_write = 0; //first make connmgr write data into the sbuffer
        mgr_status->if_datamgr_read = 1;
        mgr_status->if_storemgr_read = 1;

        sbuffer_t *sbuffer;
        sbuffer_init(&sbuffer); //initialize the shared buffer
        pthread_cond_t cond_connmgr = PTHREAD_COND_INITIALIZER;
        pthread_cond_t cond_datamgr = PTHREAD_COND_INITIALIZER;
        pthread_cond_t cond_storemgr = PTHREAD_COND_INITIALIZER;
        sbuffer->cond_connmgr = cond_connmgr;
        sbuffer->cond_datamgr = cond_datamgr;
        sbuffer->cond_storemgr = cond_storemgr;
        sbuffer->status = mgr_status;
        sbuffer->lock = &main_lock;

        pthread_t thread_ID_connmgr;
        pthread_t thread_ID_datamgr;
        pthread_t thread_ID_storemgr;

        arg_c.port_num = 1234;
        arg_c.sbuffer = sbuffer;
        pthread_create(&thread_ID_connmgr, NULL, func_connmgr, (void *) &arg_c);

        fp_sensor_map = fopen("room_sensor.map", "r");
        arg_d.fp_sensor_map = fp_sensor_map;
        arg_d.sbuffer = sbuffer;
        pthread_create(&thread_ID_datamgr, NULL, func_datamgr, (void *) &arg_d);

        arg_s.sbuffer = sbuffer;
        pthread_create(&thread_ID_storemgr, NULL, func_storemgr, (void*)&arg_s );

        pthread_join(thread_ID_connmgr, NULL);
        pthread_join(thread_ID_datamgr, NULL);
        pthread_join(thread_ID_storemgr, NULL);

        pthread_mutex_destroy(&main_lock); //destroy the lock
        sbuffer_free(&sbuffer);
        free(mgr_status);
        fclose(fp_sensor_map);
        fprintf(stdout, "Parent Process terminated!\n\n");

        exit(0);
    }
    else //if this is a child process
    {
        run_child(fifo_lock);
        pthread_mutex_destroy(&fifo_lock);
    }
}

void* func_connmgr(void * arg_conn)
{
    printf("func_connmgr has been started!\n");
    connmgr_listen(((arg_connmgr_t*)arg_conn)->port_num, ((arg_connmgr_t*)arg_conn)->sbuffer);
    fprintf(stdout,"func_connmgr has been terminated!\n");
    return 0;
}

void* func_datamgr(void * arg_data)
{
    printf("func_datamgr has been started!\n");
    datamgr_parse_sensor_data(((arg_datamgr_t*)arg_data)->fp_sensor_map, ((arg_datamgr_t*)arg_data)->sbuffer);
    fprintf(stdout,"func_datamgr has been terminated!\n");
    return 0;
}

void* func_storemgr(void * arg_store)
{
    printf("func_storemgr has been started!\n");
    DBCONN *conn;
    conn = init_connection(1);
    storagemgr_parse_sensor_data(conn, ((arg_storemgr_t*)arg_store)->sbuffer);
    fprintf(stdout,"func_storemgr has been terminated!\n");
    return 0;
}

void run_child (pthread_mutex_t fifoLock)
{
    char recv_buf[200];
    int sequence_num =1;

    do
    {
        prctl(PR_SET_PDEATHSIG,SIGKILL);
        FILE * fp_gateway_log = fopen("gateway.log", "at+");
        pthread_mutex_lock(&fifoLock);
        FILE *fp_child = fopen(FIFO_NAME, "r");
        char *str_result = fgets(recv_buf, 200, fp_child);
        fclose(fp_child);
        pthread_mutex_unlock(&fifoLock);

        if ( str_result != NULL )
        {
            fprintf(fp_gateway_log, "%d  %s \n", sequence_num, recv_buf);
            sequence_num++;
        }
        else{}
        fclose(fp_gateway_log);
    } while (1);
}

void log_write (char* content, unsigned long time_stamp) //used to write log content
{
    FILE * fp_fifo;
    char *send_buf;

    pthread_mutex_lock(&fifo_lock);
    fp_fifo = fopen(FIFO_NAME, "w");
    send_buf = malloc(200* sizeof(char));
    sprintf(send_buf, "%lu  %s\n", time_stamp, content);
    fputs( send_buf, fp_fifo );
    fclose(fp_fifo);
    pthread_mutex_unlock(&fifo_lock);

    free(send_buf);
}




