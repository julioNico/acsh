#ifndef _QUEUE_H_
#define _QUEUE_H_

typedef struct queue Queue;


Queue* Queue_init(int);
void Queue_del(Queue*);

void enqueue(Queue*, int);
int dequeue(Queue*);

int Queue_empty(Queue*);
void Queue_print(Queue*);


#endif