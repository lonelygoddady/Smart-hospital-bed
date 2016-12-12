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

Queue_NN* createQueue_NN(int maxElements)
{
        /* Create a Queue */
        Queue_NN *Q;
        Q = (Queue_NN *)malloc(sizeof(Queue_NN));
        Q->capacity = maxElements;
        
	/* Initialise its properties */
        //Q->elements = (char**)malloc(sizeof(char)*maxElements*64);
        int i;
        for (i = 0; i< maxElements; i++){
          Q->elements[i] = 0;
	}
          
        Q->size = 0;
        Q->front = 0;
        Q->rear = -1;
        /* Return the pointer */
        return Q;
}

Queue_T* createQueue_T(int maxElements)
{
        /* Create a Queue */
        Queue_T *Q;
        Q = (Queue_T *)malloc(sizeof(Queue_T));
        Q->capacity = maxElements;
        
	/* Initialise its properties */
        int i;
        for (i = 0; i< maxElements; i++)
          Q->elements[i] = (char*)malloc(sizeof(char)*32);
          
        Q->size = 0;
        Q->front = 0;
        Q->rear = -1;
        /* Return the pointer */
        return Q;
}

Queue_INT* createQueue_INT(int maxElements)
{
        /* Create a Queue */
        Queue_INT *Q;
        Q = (Queue_INT *)malloc(sizeof(Queue_INT));
        Q->capacity = maxElements;
        
	/* Initialise its properties */
        int i;
        for (i = 0; i< maxElements; i++)
          Q->elements[i] = 0;
          
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
        }
        return;
}

void Dequeue_NN(Queue_NN *Q)
{
        if(Q->size!=0)
        {
        	//printf("dequeue starts\n");
		//memset(Q->elements[Q->front],0,128); //free the memory
                Q->elements[Q->front] = 0;
		Q->size--;
                Q->front++;
                /* As we fill elements in circular fashion */
                if(Q->front==Q->capacity)
                {
                        Q->front=0;
                }
        }
        return;
}

void Dequeue_T(Queue_T *Q)
{
        if(Q->size!=0)
        {
               	memset(Q->elements[Q->front],0,32); //free the memory
                Q->size--;
                Q->front++;
                /* As we fill elements in circular fashion */
                if(Q->front==Q->capacity)
                {
                        Q->front=0;
                }
        }
        return;
}

void Dequeue_INT(Queue_INT *Q)
{
        if(Q->size!=0)
        {
        	//printf("dequeue starts\n");
		//memset(Q->elements[Q->front],0,128); //free the memory
                Q->elements[Q->front] = 0;
		Q->size--;
                Q->front++;
                /* As we fill elements in circular fashion */
                if(Q->front==Q->capacity)
                {
                        Q->front=0;
                }
        }
        return;
}

int front_INT(Queue_INT *Q)
{
        if(Q->size!=0)
        {
                return Q->elements[Q->front];
        }
        return 0;
}

int rear_INT(Queue_INT *Q)
{
       if(Q->size!=0)
       {
         return Q->elements[Q->rear];
       } 
        return 0;
}

char* front(Queue *Q)
{
        if(Q->size!=0)
        {
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

void Enqueue_INT(Queue_INT *Q,int element)
{
        //char *p = (char *) malloc(strlen(element)+1);

        /* If the Queue is full, we cannot push an element into it as there is no space for it.*/
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
                Q->elements[Q->rear] =  element;
        }
	//printf("queue size:%d, queue front:%d, queue rear:%d\n",Q->size,Q->front,Q->rear);
        return;
}


void Enqueue(Queue *Q,char *element)
{
        //char *p = (char *) malloc(strlen(element)+1);

        /* If the Queue is full, we cannot push an element into it as there is no space for it.*/
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
	//printf("queue size:%d, queue front:%d, queue rear:%d\n",Q->size,Q->front,Q->rear);
        return;
}

fann_type front_NN(Queue_NN *Q)
{
        if(Q->size!=0)
        {
                return Q->elements[Q->front];
        }
        return 0;
}

fann_type rear_NN(Queue_NN *Q)
{
       if(Q->size!=0)
       {
         return Q->elements[Q->rear];
       } 
        return 0;
}

char* front_T(Queue_T *Q)
{
        if(Q->size!=0)
        {
                return Q->elements[Q->front];
        }
        return NULL;
}

char* rear_T(Queue_T *Q)
{
       if(Q->size!=0)
       {
         return Q->elements[Q->rear];
       } 
        return NULL;
}

void Enqueue_NN(Queue_NN *Q,fann_type element)
{
        /* If the Queue is full, we cannot push an element into it as there is no space for it.*/
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
                Q->elements[Q->rear] =  element;
        }
        return;
}

void Enqueue_T(Queue_T *Q,char* element)
{
        /* If the Queue is full, we cannot push an element into it as there is no space for it.*/
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
       	return;
}
