//
// Created by Zhenjie Liu on 04.01.20.
//

void* func_connmgr(void * arg_conn);//some thread functions
void* func_datamgr(void * arg_data);
void* func_storemgr(void * arg_store);

/*
 * this is a function used to write log contents
 */
void log_write (char* content, unsigned long time_stamp);