#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "floatfann.h"
/*Queue has five properties. capacity stands for the maximum number of elements Queue can hold.
  Size stands for the current size of the Queue and elements is the array of elements. front is the
 index of first element (the index at which we remove the element) and rear is the index of last element
 (the index at which we insert the element) */
 
typedef struct Queue
{
        int capacity;
        int size;
        int front;
        int rear;
        char *elements[20];
}Queue;

Queue* createQueue(int maxElements);
void Dequeue(Queue *Q);
void Enqueue(Queue *Q, char *element);
char* front(Queue *Q);
char* rear(Queue *Q);

 
typedef struct Queue_NN
{
        int capacity;
        int size;
        int front;
        int rear;
        fann_type elements[50];
}Queue_NN;

Queue_NN* createQueue_NN(int maxElements);
void Dequeue_NN(Queue_NN *Q);
void Enqueue_NN(Queue_NN *Q, fann_type element);
fann_type front_NN(Queue_NN *Q);
fann_type rear_NN(Queue_NN *Q);

typedef struct Queue_T
{
        int capacity;
        int size;
        int front;
        int rear;
        char *elements[8];
}Queue_T;

Queue_T* createQueue_T(int maxElements);
void Dequeue_T(Queue_T *Q);
void Enqueue_T(Queue_T *Q, char* element);
char* front_T(Queue_T *Q);
char* rear_T(Queue_T *Q);

typedef struct Queue_INT
{
        int capacity;
        int size;
        int front;
        int rear;
        int elements[8];
}Queue_INT;

Queue_INT* createQueue_INT(int maxElements);
void Dequeue_INT(Queue_INT *Q);
void Enqueue_INT(Queue_INT *Q, int element);
int front_INT(Queue_INT *Q);
int rear_INT(Queue_INT *Q);


