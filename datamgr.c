
#define _GNU_SOURCE
#include "datamgr.h"


void datamgr_parse_sensor_data(FILE * fp_sensor_map, sbuffer_t* sbuffer)
{
    uint16_t scan_room_ID;     
    uint16_t scan_sensor_ID;
    unsigned long current_time;
    char* log_info = malloc(200 * sizeof(char));
    dplist_t * datamgr_list = dpl_create(d_element_copy, d_element_free, d_element_compare);
    sensor_node *sensorNode;
    sensorNode = (sensor_node *) malloc(sizeof(sensor_node));
    assert(sensorNode != 0);

    while (fscanf(fp_sensor_map, "%hu %hu", &scan_room_ID, &scan_sensor_ID) != -1)
    {
        if (check_sensor_duplication(scan_sensor_ID, datamgr_list) == false)//check whether there is already the same sensor id inside the list
        {
            sensorNode->room_ID = scan_room_ID;
            sensorNode->sensor_ID = scan_sensor_ID;
            sensorNode->last_modified_timestamp = 0; //initialization
            sensorNode->running_avg = 0;

            for (int i = 0; i < RUN_AVG_LENGTH; i++)       //initialize the Array
            { sensorNode->tempArray[i] = 0;}
            dpl_insert_at_index(datamgr_list, sensorNode, 0, true); //insert the sensor node into the list
        }
        else {} //if duplication occurs, do not insert
    }
    sensor_data_t each_sensorData;
    while(sbuffer->status->connmgr_status == 0 && sbuffer->status->storemgr_status == 0 ) // check whether the other managers are open, '0' means open
    {
        pthread_mutex_lock(sbuffer->lock);
        if(sbuffer->status->if_datamgr_read == 1) //"1" means datamgr can not read now, because connmgr is still writing
        {pthread_cond_wait(   &(sbuffer->cond_datamgr),sbuffer->lock);} //block this thread, till connmgr finish writing
        else{}

        int sbuffer_size = sbuffer_get_size(sbuffer);
        if(sbuffer_size ==0)
        {
            pthread_mutex_unlock(sbuffer->lock);
            continue;
        }
        else
        {
            sensor_data_t* sbuffer_data_at_index = sbuffer_get_sensor_data_at_index(sbuffer, 0); //retrieve the first node in the sbuffer (the new data)
            each_sensorData.id = sbuffer_data_at_index->id;
            each_sensorData.value = sbuffer_data_at_index->value;
            each_sensorData.ts = sbuffer_data_at_index->ts;
            sbuffer->status->if_datamgr_read = 1; //reset this value, prepare for next time ("1" disabled, "0" able)
            sbuffer->status->if_storemgr_read = 0;
        }
        int list_index = 0;  
        double temp_sum = 0; 
        double temp_avg = 0;
        bool   if_valid_sensor = false;

        while (list_index <= dpl_size(datamgr_list))
        {
            sensor_node *sensorPointer = dpl_get_element_at_index(datamgr_list,list_index);
            if (sensorPointer->sensor_ID ==each_sensorData.id)
            {
                if_valid_sensor = true;
                for (int i = 1; i < RUN_AVG_LENGTH; i++)//renew the array
                {
                    sensorPointer->tempArray[RUN_AVG_LENGTH - i] = sensorPointer->tempArray[RUN_AVG_LENGTH - i - 1];
                }
                sensorPointer->tempArray[0] = each_sensorData.value;
                sensorPointer->last_modified_timestamp = each_sensorData.ts;

                for (int i = 0; i < RUN_AVG_LENGTH; i++)//caculate the average temperatue value using the array
                {
                    temp_sum = temp_sum + sensorPointer->tempArray[i];
                }
                temp_avg = temp_sum / RUN_AVG_LENGTH;
                sensorPointer->running_avg = temp_avg; 

                if(sensorPointer->tempArray[RUN_AVG_LENGTH-1] != 0) //check whether there are enough values inside the array
                {
                    if(sensorPointer->running_avg > SET_MAX_TEMP)
                    {
                        fprintf(stdout,"The running average temperature of the sensor is too high (%f °C), Sensor id = %d, timestamp is %ld\n", sensorPointer->running_avg, sensorPointer->sensor_ID, sensorPointer->last_modified_timestamp);
                        fflush(stdout);
                        sprintf(log_info, "The sensor node with id = %d reports it’s too hot (running avg temperature = %f)", sensorPointer->sensor_ID, sensorPointer->running_avg );
                        current_time = time(NULL);
                        log_write(log_info, current_time);
                    }
                    else if(sensorPointer->running_avg < SET_MIN_TEMP)
                    {
                        fprintf(stdout,"The running average temperature of the sensor is too low (%f °C), Sensor id = %d, timestamp is %ld\n", sensorPointer->running_avg,sensorPointer->sensor_ID, sensorPointer->last_modified_timestamp);
                        fflush(stdout);
                        sprintf(log_info, "The sensor node with id = %d reports it’s too cold (running avg temperature = %f)", sensorPointer->sensor_ID, sensorPointer->running_avg);
                        current_time = time(NULL);
                        log_write(log_info, current_time);
                    }
                    else{}
                }
                else{}
                break;//break the while loop
            }
            else{}
            list_index++;
        }

        if(if_valid_sensor==false)
        {

            fprintf(stdout, "Received sensor data with invalid sensor node ID %d\n", each_sensorData.id);
            fflush(stdout);
            sprintf(log_info, "Received sensor data with invalid sensor node ID %d", each_sensorData.id);
            current_time = time(NULL);
            log_write(log_info, current_time);
        }
        else{}
        pthread_cond_signal(&(sbuffer->cond_storemgr)); //send signal to activate storagemgr
        pthread_mutex_unlock(sbuffer->lock);
    }
    free(log_info);
    free(sensorNode);
    datamgr_free(datamgr_list);
    pthread_cond_signal(&(sbuffer->cond_storemgr)); //send signal to activate storagemgr (when datamgr is closing)
}

void datamgr_free(dplist_t* datamgr_list)
{
    dpl_free(&datamgr_list, true);
}

bool check_sensor_duplication (uint16_t sensorID, dplist_t* datamgr_list) //used to check whether there are same sensors (id) when reading from the 'room_sensor.map' file
{
    int list_index = 0;
    if(dpl_size(datamgr_list) == 0) {return false;}//if there is not any nodes in the list, then return 'false'
    while (list_index < dpl_size(datamgr_list))
    {
        if (((sensor_node*)(dpl_get_element_at_index(datamgr_list,list_index)))->sensor_ID == sensorID) // if the same id is already in the list, then duplication occurs
        {
            return true;  
        }
        else{list_index++;}
    }
    return false;//no duplication
}

void * d_element_copy(void * src_element)
{
    sensor_node* copy = malloc(sizeof(sensor_node)); 
    copy->room_ID = ((sensor_node*)(src_element))->room_ID;
    copy->sensor_ID = ((sensor_node*)(src_element))->sensor_ID;
    copy->running_avg = ((sensor_node*)(src_element))->running_avg;
    copy->last_modified_timestamp = ((sensor_node*)(src_element))->last_modified_timestamp;

    for(int i=0;i< RUN_AVG_LENGTH; i++)
    {copy->tempArray[i] = ((sensor_node*)(src_element))->tempArray[i];}
    return copy;
}

void d_element_free(void ** element)
{
    free(*element);
}

int d_element_compare(void * x, void * y) //	Compare two element elements; returns -1 if x<y, 0 if x==y, or 1 if x>y
{
    if((sensor_node*)(x) < (sensor_node*)(y)) {return -1;}
    else if((sensor_node*)(x) == (sensor_node*)(y)) {return 0;}
    else{return 1;}
}
