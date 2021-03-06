#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include "server.h"
#include "queue.h"

#define PORTNO 5000
#define cli_num 4
#define queue_cap 2
#define sync_time 20 //20*0.25 = 5 second 

static volatile int run_flag = 1;
struct thread_argu{
	CONNECTION *Client;
	int client_n;
	struct Queue *thread_Q;
};

struct thread_argu_struc{
	struct thread_argu *Client_thread[cli_num];
};

void do_when_interrupted()
{
	run_flag = 0;
	printf("\n Threads exiting. Please wait for cleanup operations to complete...\n");
}

double timestamp()
{
	// calculate and return the timestamp
	struct timeval tv;
	double sec_since_epoch;

	gettimeofday(&tv, NULL);
	sec_since_epoch = (double) tv.tv_sec + (double) tv.tv_usec/1000000.0;

	return sec_since_epoch;
}

void* handle_client(void *arg)
{
	CONNECTION *client;
	int n, client_socket_fd;
	char buffer[256], tmp[256];
	struct thread_argu* data_thread = (struct thread_argu*) arg;
	FILE *fp;
	
	client = data_thread->Client;
	client_socket_fd = client->sockfd;

	memset(buffer, 0, 256);
	sprintf(buffer, "output_file_IP_%s.csv", client->ip_addr_str);
	printf("filename: %s", buffer);

	fp = fopen(buffer, "w+b");

	while (run_flag) {
		// clear the buffer
		memset(buffer, 0, 256);

		// read what the client sent to the server and store it in "buffer"
		n = read(client_socket_fd, buffer, 255);	
		// an error has occurred
		if (n < 0) {
			server_error("ERROR reading from socket");
			return NULL;
		}
		// no data was sent, assume the connection was terminated
		if (n == 0) { 
			printf("%s has terminated the connection.\n", client->ip_addr_str);
			return NULL;
		}
		fprintf(fp,"%s,%s\n",client->ip_addr_str, buffer);
		
		Enqueue(data_thread->thread_Q, buffer);
		
	}

	fclose(fp);
	close(client_socket_fd);
	return NULL;
}

void* Synchronize_data(void *arg)
{
	struct thread_argu_struc *Sync_argu = (struct thread_argu_struc*)arg;
	int thread_data_num[cli_num];
        int i,sig;
	char buffer[1024];
	char *tmp;
	sig = 1;
	FILE *file;
	Queue *thread_Q[cli_num];
	int time = 0;
	int cli_con;
	
	for (i = 0; i < cli_num; i++)
		thread_Q[i] = Sync_argu->Client_thread[i]->thread_Q;
	
	//csv file to collect client data
	file = fopen("./client_data_value.csv","wb");
	if (file == NULL)
		printf("the error number:%d\n",errno);
	
	while(run_flag){
		cli_con = 0;

		for(i = 0; i< cli_num; i++){
			if (thread_Q[i]->size == thread_Q[i]->capacity)
				cli_con ++; //check whetehr all thread data has reaches its capacity
		}

		if (cli_con == cli_num)
			sig = 0;
		else 
			sig = 1;

		memset(buffer, 0, sizeof(buffer));
		
		if (sig == 0){
			for (i=0; i< cli_num; i++){
				if(i == 0)
					strcpy(buffer,front(thread_Q[i]));
				else {
			                //tmp = (char*)malloc(256);
					tmp = front(thread_Q[i]);
					strcpy(tmp, tmp + 14);
					//strcat(buffer,front(thread_Q[i]));	
					strcat(buffer,tmp);
					strcpy(tmp, "");
				}
				Dequeue(thread_Q[i]);	
			}
			if (time == sync_time){	
				printf("%s\n",buffer);
				fprintf(file,"%s\n",buffer);
			}
			else{ 
				time ++;
				printf("please wait %f seconds for Synchronization to accomplish\n", (0.25*(sync_time-time))); 
			}
		}	
	}
	if (file != NULL)
		fclose(file);
	
}


void* manage_server(void *arg)
{
	int max_connections;
	int i,n;
	max_connections = cli_num;
	struct thread_argu_struc *server_thread_argu; 
	struct thread_argu *Client_thread[cli_num];
	CONNECTION *server;
	
	server_thread_argu = (struct thread_argu_struc*)arg;
	for (i = 0; i< cli_num; i++)
		Client_thread[i] = server_thread_argu->Client_thread[i];

	pthread_t tids[cli_num];

	server = (CONNECTION *) server_init(PORTNO, 10);
	if ((int) server == -1){
		run_flag = 0;
	}

	i = 0; //-1

	while(i < max_connections && run_flag) {	
		Client_thread[i]->Client = (CONNECTION*) server_accept_connection(server->sockfd);
		if ((int) Client_thread[i]->Client == -1) {
			printf("Latest child process is waiting for an incoming client connection.\n");
			printf("Waiting for all client to be connected. %d client left\n",max_connections - i);
		}
		else {
			i++;
			if (i == max_connections) {
				printf("All clients connected, wait user to press enter to continue.\n");
				getchar();
			
				for (n = 0; n < max_connections; n++){
					pthread_create(&tids[n], NULL, handle_client, (void *)Client_thread[n]);		
					printf("creating a thread\n");
				}

			}
		}
	}

	if (i > max_connections) {
		printf("Max number of connections reached. No longer accepting connections. Continuing to service old connections.\n");
	}

	for (n = 0; n < i ; n++)
		pthread_join(tids[n],NULL);
	
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t manage_server_tid, Synchronize_data_tid;
	int rc;

	signal(SIGINT, do_when_interrupted);
	struct thread_argu_struc *data_thread;
	
	data_thread = (struct thread_argu_struc*)malloc(sizeof(struct thread_argu_struc));

	int i;
	for (i = 0; i < cli_num; i++){
		data_thread->Client_thread[i] = (struct thread_argu*)malloc(sizeof(struct thread_argu)); 
		data_thread->Client_thread[i]->Client = (CONNECTION*)malloc(sizeof(CONNECTION*));
		data_thread->Client_thread[i]->client_n = i;
		data_thread->Client_thread[i]->thread_Q = createQueue(queue_cap);
	}	
	
	rc = pthread_create(&manage_server_tid, NULL, manage_server, (void*)data_thread);
	if (rc != 0) {
		fprintf(stderr, "Failed to create manage_server thread. Exiting program.\n");
		exit(0);
	}

	rc = pthread_create(&Synchronize_data_tid, NULL, Synchronize_data, (void*)data_thread);
	if (rc != 0) {
		fprintf(stderr, "Failed to create Synchronize_data thread. Exiting program.\n");
		exit(0);
	}

	pthread_join(manage_server_tid, NULL);
	pthread_join(Synchronize_data_tid, NULL);
	
	FILE* f_p;
	printf("\n...cleanup operations complete. Exiting main.\n");

	return 0;
}
