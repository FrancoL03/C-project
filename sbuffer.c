
#include "sbuffer.h"

int sbuffer_init(sbuffer_t ** buffer)
{
  *buffer = malloc(sizeof(sbuffer_t));
  if (*buffer == NULL) return SBUFFER_FAILURE;
  (*buffer)->head = NULL;
  (*buffer)->tail = NULL;
  return SBUFFER_SUCCESS; 
}


int sbuffer_free(sbuffer_t ** buffer)
{
  if ((buffer==NULL) || (*buffer==NULL)) 
  {
    return SBUFFER_FAILURE;
  } 
  while ( (*buffer)->head )
  {
    sbuffer_node_t * dummy = (*buffer)->head;
    (*buffer)->head = (*buffer)->head->next;
    free(dummy);
  }
  free(*buffer);
  *buffer = NULL;
  return SBUFFER_SUCCESS;		
}


int sbuffer_remove(sbuffer_t * buffer)
{
  sbuffer_node_t * dummy;
  if (buffer == NULL) return SBUFFER_FAILURE;       //case 1, the buffer is NULL
  if (buffer->head == NULL) return SBUFFER_NO_DATA; //case 2, the buffer does not have any node inside, it is empty

  dummy = buffer->head;                             //get the head(node), remove and free it later
  if (buffer->head == buffer->tail)                 //the buffer has only one node
  {
    buffer->head = buffer->tail = NULL; 
  }
  else                                              //case 3, buffer has more than one node
  {
    buffer->head = buffer->head->next;              //delete the first node
  }
  free(dummy);
  return SBUFFER_SUCCESS;
}


int sbuffer_insert(sbuffer_t * buffer, sensor_data_t * data)
{
  sbuffer_node_t * dummy;
  if (buffer == NULL) return SBUFFER_FAILURE;
  dummy = malloc(sizeof(sbuffer_node_t));
  if (dummy == NULL) return SBUFFER_FAILURE;
  dummy->element.data = *data;
  dummy->next = NULL;
  if (buffer->tail == NULL)                          //the buffer is empty
  {
    buffer->head = buffer->tail = dummy;
  } 
  else                                               //the buffer is not empty
  {
    buffer->tail->next = dummy;
    buffer->tail = buffer->tail->next; 
  }
  return SBUFFER_SUCCESS;
}

sensor_data_t* sbuffer_get_sensor_data_at_index(sbuffer_t * buffer, int index)
{
    int counter ;
    sbuffer_node_t *dummy;
    if(buffer->head == NULL) return (sensor_data_t*)0;           // the list is empty, (void *)0 is returned.
    else if (index <= 0) return &(buffer->head->element.data);   // if 'index' is 0 or negative, the element of the first list node is returned.

    for(dummy = buffer->head, counter = 0; dummy->next != NULL ; dummy = dummy->next, counter++)
    {
        if (counter >= index) return &(dummy->element.data);
    }
    return &(dummy->element.data);                               // if 'index' exceeds the size of the list, the element of the last list node is returned.
}

int sbuffer_get_size(sbuffer_t * buffer)
{
    int count = 0;
    sbuffer_node_t *dummy;
    if(buffer->head == NULL) return 0;
    for(dummy = buffer->head, count = 1; dummy->next != NULL ; dummy = dummy->next)
    {
        count++;
    }
    return count;
}





