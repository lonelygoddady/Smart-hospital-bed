#include "queue.h" 

Queue* createQueue(int maxElements)
{
        /* Create a Queue */
        Queue *Q;
        Q = (Queue *)malloc(sizeof(Queue));
        Q->capacity = maxElements;
        
	/* Initialise its properties */
        //Q->elements = (char**)malloc(sizeof(char)*maxElements*64);
        int i;
        for (i = 0; i< maxElements; i++)
          Q->elements[i] = (char*)malloc(sizeof(char)*128);
          
        Q->size = 0;
        Q->front = 0;
        Q->rear = -1;
        /* Return the pointer */
        return Q;
}

void Dequeue(Queue *Q)
{
        if(Q->size!=0)
        {
        	//printf("dequeue starts\n");
		memset(Q->elements[Q->front],0,128); //free the memory
                Q->size--;
                Q->front++;
                /* As we fill elements in circular fashion */
                if(Q->front==Q->capacity)
                {
                        Q->front=0;
                }
		//printf("perform dequeue\n");
		//printf("queue size:%d, queue front:%d, queue rear:%d\n", Q->size,Q->front,Q->rear);
        }
        return;
}

char* front(Queue *Q)
{
        if(Q->size!=0)
        {
                /* Return the element which is at the front*/
                return Q->elements[Q->front];
        }
        return NULL;
}

char* rear(Queue *Q)
{
       if(Q->size!=0)
       {
         return Q->elements[Q->rear];
       } 
        return NULL;
}

void Enqueue(Queue *Q,char *element)
{
        //char *p = (char *) malloc(strlen(element)+1);

        /* If the Queue is full, we cannot push an element into it as there is no space for it.*/
        //printf("Enqueue starts\n");
	if(Q->size == Q->capacity)
        {
	//	printf("Queue is Full\n");
        }
        else
        {
                Q->size++;
                Q->rear = Q->rear + 1;
                /* As we fill the queue in circular fashion */
                if(Q->rear == Q->capacity)
                {
                        Q->rear = 0;
                }
                /* Insert the element in its rear side */ 
                sprintf(Q->elements[Q->rear],"%s", element);
        }
	//printf("perforn enqueue\n");
	//printf("queue size:%d, queue front:%d, queue rear:%d\n",Q->size,Q->front,Q->rear);
        return;
}
