#define _GNU_SOURCE
#include "sensor_db.h"

void storagemgr_parse_sensor_data(DBCONN * conn, sbuffer_t * sbuffer)
{
    int db_reconn_counter = 0;
    unsigned long current_time = 0;
    int db_sensor_id = 0;
    double db_sensor_value = 0 ;
    unsigned long db_sensor_time = 0;
    char* log_info = malloc(200 * sizeof(char));

    while  (sbuffer->status->storemgr_status == 0 && sbuffer->status->connmgr_status == 0)
    {
        pthread_mutex_lock(sbuffer->lock);
        if(sbuffer->status->if_storemgr_read == 1)//means storemgr can not read now, because datamgr is still reading
        {
            pthread_cond_wait(&(sbuffer->cond_storemgr),sbuffer->lock); //make this thread block, till datamgr finish reading
        }
        else{}
        int sbuffer_size = sbuffer_get_size(sbuffer);
        if(sbuffer_size == 0)
        {
            pthread_mutex_unlock(sbuffer->lock);
            continue;
        }
        else
        {
            sensor_data_t *data_at_index = sbuffer_get_sensor_data_at_index(sbuffer,0);
            db_sensor_id = data_at_index->id;
            db_sensor_value = data_at_index->value;
            db_sensor_time = data_at_index->ts;
            sbuffer_remove(sbuffer);
            sbuffer->status->if_storemgr_read = 1;//reset this value, prepare for next time ("1" disabled, "0" able)
            sbuffer->status->if_connmgr_write = 0;

            while(1)
         {
            int succeed_flag = 0;
            succeed_flag = insert_sensor(conn, db_sensor_id, db_sensor_value, db_sensor_time);
            if(succeed_flag == 0) //if sql succeed
            {
                    db_reconn_counter = 0; //reset the count of reconnection
                    break;
            }
            else
            {
                if (db_reconn_counter >= 3) //if reconnection does not succeed after 3 attempts
                {
                    sprintf(log_info, "Unable to connect to SQL server.\n");
                    current_time = time(NULL);
                    log_write(log_info, current_time);
                    printf("SQL database will close due to disconnection even after 3 retries\n");
                    sbuffer->status->storemgr_status = 1; //set the status to be 'closed'
                    break;
                }
                else
                {
                    sprintf(log_info, "Connection to SQL server lost.\n");
                    current_time = time(NULL);
                    log_write(log_info, current_time);
                    db_reconn_counter++;
                    printf("SQL database connection lost, start a retry. Retry count = %d\n",db_reconn_counter);
                    sleep(2);//wait a bit before trying again
                    conn = init_connection(0); //try to reconnect
                }
            }
          }
        }
        pthread_cond_signal(&(sbuffer->cond_connmgr)); //send signal to activate connmgr
        pthread_mutex_unlock(sbuffer->lock);
    }
    free(log_info);
    disconnect(conn);
    pthread_cond_signal(&(sbuffer->cond_connmgr)); //send signal to activate connmgr　(when storemgr is closing)
}

DBCONN * init_connection(char clear_up_flag)
{
    int rc;
    DBCONN * database ;
    char * err_msg = 0;
    char * sql = 0;
    unsigned long current_time_db = 0;
    rc = sqlite3_open("Sensor.db", &database);
    char* log_info = malloc(200 * sizeof(char));
    if(rc != SQLITE_OK) 
    {
        fprintf(stdout,"Cannot open this database: %s\n", sqlite3_errmsg(database));
        sprintf(log_info, "Connection to SQL server lost.");
        current_time_db = time(NULL);
        log_write(log_info, current_time_db);

        sqlite3_free(err_msg);   
        sqlite3_close(database); 
        return NULL;         
    }
    else
    {
        sprintf(log_info, "Connection to SQL server established.");
        current_time_db = time(NULL);
        log_write(log_info,current_time_db);

        if(clear_up_flag == 1) //clear up the existing data in the table
        {
            sql = "DROP TABLE IF EXISTS SensorData;"
                  "CREATE TABLE IF NOT EXISTS SensorData(id INTEGER PRIMARY KEY AUTOINCREMENT, sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);";
        }
        else //do not clear
        {
            sql= "CREATE TABLE IF NOT  EXISTS SensorData(id INTEGER PRIMARY KEY AUTOINCREMENT, sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);";
        }

    }
    rc = sqlite3_exec(database,sql,0,0,&err_msg); //execute the sql instruction
    if(rc!=SQLITE_OK)
    {
        printf("Fail to create the table!\n");
    }
    else
    {
        sprintf(log_info, "New table ’SensorData‘ created.");
        current_time_db = time(NULL);
        log_write(log_info,current_time_db);
    }
    free(log_info);
    return database;
}


void disconnect(DBCONN *conn)
{
  sqlite3_close(conn);
}


int insert_sensor(DBCONN * conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
    int rc;
    char * err_msg = 0;
    char sql[200];
    sprintf(sql,"  INSERT INTO SensorData(sensor_id, sensor_value, timestamp) VALUES(%hu,%f,%ld);  ",id,value,ts);
    rc = sqlite3_exec(conn, sql, 0, 0, &err_msg);                               

    if (rc != SQLITE_OK)
    {
       fprintf(stdout, "SQL error when try to insert data: %s\n", err_msg);
       sqlite3_free(err_msg);
       return 1; //return noe-zero if sql fail
    }
    else{return 0;} // return 0 if sql succeed
}




