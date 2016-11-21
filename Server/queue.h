#include<stdio.h>
#include<stdlib.h>
#include<string.h>
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


