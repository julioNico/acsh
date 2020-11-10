#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

struct queue {
    int first;
    int N, NMAX;
    int *data;
};


Queue* Queue_init(int sz) {
    Queue *q = (Queue*) malloc(sizeof(Queue));
    if(!q) {
        printf("Problems during malloc.\n");
        return NULL;
    }
    q->data = (int*) malloc(sz*sizeof(int));
    if(!q->data) {
        printf("Problems during malloc.\n");
        return NULL;
    }

    q->NMAX = sz;
    q->first = q->N = 0;
    return q;
}

void Queue_del(Queue *q) {
    free(q->data);
    free(q);
}

void enqueue(Queue *q, int data) {
    if(q->N == q->NMAX)
        return;

    q->data[(q->first + q->N) % q->NMAX] = data;
    q->N++;
}

int dequeue(Queue *q) {
    if(Queue_empty(q))
        return -1;
    
    q->N--;
    q->first = (q->first + 1) % q->NMAX;
    return q->data[(q->first + q->NMAX - 1) % q->NMAX];
}

int Queue_empty(Queue *q) {
    return q->N == 0;
}

void Queue_print(Queue *q) {
    printf("\n############\n");
    printf("Queue");
    printf("\n############\n");
    while(!Queue_empty(q))
        printf("%d\n", dequeue(q));
    printf("############\n\n");
}
