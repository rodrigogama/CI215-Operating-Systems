#include <stdio.h>
#include <stdlib.h>
#include "queue.h"  // provided by the prof. Maziero. All functions MUST follow the prototypes in this header

/** @function queue_append
 *  Append an element to the queue. Return an error message if there's any. Conditions to append:
 *      - the element must exist
 *      - the element must not exist in another queue
 *  @param {queue_t **} q - double pointer to the struct that represents the queue
 *  @param {queue_t *}  e - pointer to the element we want to append
**/
void queue_append(queue_t **q, queue_t *e) {

    // checking whether the element is null or if it's already in another queue
    if (!e) { // empty element
        fprintf(stderr, "Element is null.\n");
        return;
    } 
    else if (e->prev || e->next) { // element has previous or next element, which means that it is already in another queue
        fprintf(stderr, "Element is in another queue.\n");
        return;
    }
    
	// checking whether the queue is null
	if (!(*q)) { // Empty queue
		e->prev = e; // because the queue is empty, 
		e->next = e; // the e->prev and e->next aims
		(*q) = e;      // to the element itself
	}
    else { // Queue isn't empty   
        e->prev = (*q)->prev; // new element receives the last queue element as its previous
        e->next = (*q);       // new element receives the first queue element as its next
        (*q)->prev->next = e; // the old last queue element receives the new element as its next, since it is no longer the last element
        (*q)->prev = e;       // the first queue element receives the new element as its previous
	}
}

/** @function queue_remove
 *  Remove an element from the queue without destroying it. Return an error message if there's any. 
 *  Conditions to remove:
 *      - the element must exist as well as the queue
 *      - the queue must not be empty
 *      - the element must exist in the queue
 *  @param {queue_t **} q - double pointer to the struct that represents the queue
 *  @param {queue_t *}  e - pointer to the element we want to append
**/
queue_t *queue_remove(queue_t **q, queue_t *e) {
    
    if (!e) { // checking if the element is null
        fprintf(stderr, "Element is null.\n");
        return NULL;
    }
    
    if (!(*q)) { // checking if the queue is empty
        fprintf(stderr, "Element is in another queue.\n");
         return NULL;
    }
    
    if (e == (*q)) { // element to be removed is the first in queue
        if (queue_size(*q) > 1) { // queue has more than 1 element
            e->prev->next = e->next; // last element->next is the next element from the first (element removed)
            e->next->prev = e->prev; // next element (now the first in queue) targets to the last one
            *q = e->next; // the second element now is the first on the queue
        }
        else // queue has only one element
            (*q) = NULL; // now that the element is removed, the queue is empty
        
        e->prev = NULL; // remove element's previous and next elements
        e->next = NULL; // that means that now the removed element 
        
        return e; // return the removed element
    } 
    else { // element is not the first in the queue
        queue_t *aux = (*q)->next;
        
        // while the next element isn't equals the first queue element, we get the next until both elements are the same. That means we've finished the queue and returned to the first element
        while (aux != (*q)) {
            if (e == aux) { // we've found the element!
                e->prev->next = e->next; // last element->next is the next element from the first (element removed)
                e->next->prev = e->prev; // next element (now the first in queue) targets to the last one
                e->prev = NULL; // remove element's previous and next elements
                e->next = NULL; // that means that now the removed element
                
                return e; // return the removed element
            }
            
            aux = aux->next; // get the next element
        }
    } 		
    
    return NULL; // the element is not in the queue
}
 
/** @function queue_size
 *  Count how many elements the queue has. Return 0 if the queue is empty. 
 *  @param  {queue_t *} q    - pointer to the struct that represents the queue
 *  @return {int}       size - the queue's size
**/
int queue_size(queue_t *q) {
	int size = 0;
    
    if (!q) // queue is empty
        return size;
	
    size++; // queue has at least one element
    queue_t *aux = q;
    
    // while the next element isn't equals the first queue element, we get the next until both elements are the same. That means we've finished the queue and returned to the first element
	while (aux->next != q) {
		size++; // counting elements
        aux = aux->next; // get the next element
	}
    
	return size; // queue's size
}

/** @function queue_print
 *  Print all the elements in the queue.
 *  @param {char *}    text - Optional text before start printing the queue. Example: 'Starting to print': [<queue_elem_0>, <queue_elem_1>, ...]
 *  @param {queue_t *} q    - pointer to the struct that represents the queue
 *  @param {void}      print_elem(void*) - a function that will actually print the elements
**/
void queue_print(char *text, queue_t *q, void print_elem(void*)) {
	
    if (!q) { // queue is empty
        printf("%s: []\n", text);
        return;
    }
      
    printf("%s: [", text); // starting to print elements
    print_elem((void*)q);  // printing the first element in queue
    
    queue_t *aux = q; // just to keep track of the elements in queue
    
    // while the next element isn't equals the first queue element, we get the next until both elements are the same. That means we've finished the queue and returned to the first element
    while (aux->next != q) {
        aux = aux->next; // get the next element
        printf(" ");     // whitespace between elements
        print_elem((void*)aux); // here the elements are actually printed based on the function passed as parameter
    }
    
    printf("]\n"); // all elements are printed at this point
}