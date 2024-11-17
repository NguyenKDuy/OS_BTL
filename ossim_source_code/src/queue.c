#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        if (q->size < MAX_QUEUE_SIZE) {
                q->proc[q->size] = proc;
                q->size++;
        }
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
        * in the queue [q] and remember to remove it from q
        * */
        if (empty(q))
                return NULL;
        else {
                
                int index_prio = 0;
                int size = q->size;
                for (int i = 1; i < size; i++) {
                        if (q->proc[i]->priority < q->proc[index_prio]->priority)
                                index_prio = i;
                }
        
                struct pcb_t * store = q->proc[index_prio];
                for (int i = index_prio; i < size-1; i++) {
                        q->proc[i] = q->proc[i+1];
                }
                q->size--;
                return store;
                }
}

