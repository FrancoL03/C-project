#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

dplist_t * dpl_create
(
        void * (*element_copy)(void * src_element), //callback functions
        void (*element_free)(void ** element),
        int (*element_compare)(void * x, void * y)
)
{
    dplist_t * list;  
    list = malloc(sizeof(struct dplist)); 
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
    list->head = NULL; 
    list->element_copy = element_copy; 
    list->element_free = element_free;
    list->element_compare = element_compare; 
    return list;
}

void dpl_free(dplist_t ** list, bool free_element)
{
    int list_size = dpl_size(*list);
    int index ;
    for (index = 0; index < list_size ; index++)
    {
        dpl_remove_at_index(*list, 0,free_element); //remove the first node in the list
    }
    free(*list);
    *list = NULL;
}

dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy)
{
    dplist_node_t * ref_at_index, * list_node; 
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    list_node = malloc(sizeof(dplist_node_t)); 
    DPLIST_ERR_HANDLER(list_node==NULL,DPLIST_MEMORY_ERROR);
    list_node->element =(void *) malloc(sizeof(void *));
    assert(list_node->element != NULL);

    if(insert_copy)
    {
        free(list_node->element);
        (list_node->element) = (list->element_copy(element));
    }
    else 
    {
        free(list_node->element);
        (list_node->element) = element;
    }

    if (list->head == NULL)
    {                                                           // case 1, an empty list. For the first member in the list, its pre will point to NULL
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
    } else if (index <= 0)                                      // if 'index' is 0 or negative, the node is inserted at the start of 'list'
    {                                                           // case 2, the list already has members, and we want to insert the element at the beginning of the list
        list_node->prev = NULL;
        list_node->next = list->head; 
        list->head->prev = list_node;
        list->head = list_node;
    } else
    {
        ref_at_index = dpl_get_reference_at_index(list, index); //ref_at_index is a pointer, it points to the start address of the node(the indexed one)
        assert( ref_at_index != NULL);
        if (index < dpl_size(list))
        {
            list_node->prev = ref_at_index->prev;               // case 3, 'index' is smaller than the size of the list, the node is inserted at the indexed position
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;

        } else
        {
            assert(ref_at_index->next == NULL);                 // case 4, 'index' is bigger than the size of the list, the node is inserted at the end of the list
            list_node->next = NULL; 
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
        }
    }
    return list;
}

dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element)
{
    dplist_node_t * ref_at_index;
    dplist_node_t * deleted_node;
    if (list->head == NULL)                                        // case 1, an empty list, free the list
    {
        return list;
    }
    else if (list->head->prev == NULL && list->head->next == NULL) // case 2, only one node in the list
    {
        deleted_node = list->head;
        if(free_element == true) 
        {
            (*(list->element_free)) (&(deleted_node->element));
            free(deleted_node);
            list->head = NULL;
        }
        else                    
        {
            free(list->head);
            list->head = NULL;
        }
        return list;
    }
    else if(index <= 0)                                            // case 3, the list already has members, if 'index' is 0 or negative, the first node is removed
    {
        deleted_node = list->head;
        list->head = list->head->next; 
        list->head->prev = NULL;
        deleted_node->prev = NULL; 
        deleted_node->next = NULL;
    }
    else                                                           // case 4, index > 0
    {
        ref_at_index = dpl_get_reference_at_index(list, index); 
        assert( ref_at_index != NULL);
        if (index < dpl_size(list)-1)                              // if index < list size -1 ,then the node at indexed position is deleted
        {
            deleted_node = ref_at_index;
            ref_at_index->prev->next = ref_at_index->next; 
            ref_at_index->next->prev = ref_at_index->prev;
            deleted_node->prev = NULL; 
            deleted_node->next = NULL;
        }
        else                                                       // otherwise, delete the last node
        {
            deleted_node = ref_at_index;
            assert(ref_at_index->next == NULL);
            ref_at_index->prev->next = NULL;
            deleted_node->prev = NULL; 
            deleted_node->next = NULL;
        }
    }
    if(free_element == true)                                       // if free_element == true: call element_free() on the element of the list node
    {
        (*(list->element_free))(&(deleted_node->element)); 
        free(deleted_node); 
    }
    else
    {
        free(deleted_node); 
    }
    return list;
}

int dpl_size( dplist_t * list)
{
    int count = 0;
    dplist_node_t  *dummy;                                                         // set a dummy, which is a pointer that points to the starting address of a node
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if(list->head == NULL) return 0;
    for(dummy = list->head, count = 1; dummy ->next != NULL ; dummy = dummy->next)
    {
        count++;
    }
    return count;
}

void * dpl_get_element_at_index( dplist_t * list, int index )
{
    int counter ;
    dplist_node_t *dummy;
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if(list->head == NULL) return (void*)0;                                        // the list is empty, (void *)0 is returned
    else if (index <= 0)   return list->head->element;                             // if 'index' is 0 or negative, the element of the first node is returned
    for(dummy = list->head, counter = 0; dummy->next != NULL ; dummy = dummy->next, counter++)
    {
        if (counter >= index) return dummy->element;
    }
    return dummy->element;                                                         // if 'index' is bigger than the size of the list, the element of the last list node is returned
}

dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index )
{
    int count;
    dplist_node_t * dummy;
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if (list->head == NULL ) return NULL;                                          // if the list is empty, then return NULL
    for ( dummy = list->head, count = 0; dummy->next != NULL  ; dummy = dummy->next, count++)
    {
        if (count >= index) return dummy;
    }
    return dummy;
}




