#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

#include "server.h"
#include "9DOF.h"
#include "LSM9DS0.h"
#include "dirent.h" 
#include "ctype.h"
#include "queue.h"

#define PORTNO 5000
#define cli_num 2
#define queue_cap 4

#include <errno.h>
//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 

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

void* manage_9dof(void *arg) 
{
	NINEDOF *ninedof;
	double sec_since_epoch;

	mraa_init();

	FILE *fp;
	fp = fopen("./server_test_data.csv", "w");

	// write header to file "test_data.csv"
	fprintf(fp, "time (epoch), accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, mag_x, mag_y, mag_z, temperature");
	fclose(fp);

	// 9DOF sensor initialization
	ninedof = ninedof_init(A_SCALE_4G, G_SCALE_245DPS, M_SCALE_2GS);

	while(run_flag) {
		// timestamp right before reading 9DOF data
		sec_since_epoch = timestamp();
		ninedof_read(ninedof);

		// append 9DOF data with timestamp to file "server_test_data.csv"
		fp = fopen("./server_test_data.csv", "a");
		fprintf(fp, "%10.10f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", sec_since_epoch,
		ninedof->accel_data.x, ninedof->accel_data.y, ninedof->accel_data.z,
		ninedof->gyro_data.x - ninedof->gyro_offset.x, 
  		ninedof->gyro_data.y - ninedof->gyro_offset.y, 
  		ninedof->gyro_data.z - ninedof->gyro_offset.z,
  		ninedof->mag_data.x, ninedof->mag_data.y, ninedof->mag_data.z,
  		ninedof->temperature);
		fclose(fp); 
	
		// print server 9DOF data
	//	ninedof_print(ninedof);
		usleep(10000);
	}

	return NULL;
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
	sprintf(buffer, "output_file_IP_%s_TIMESTAMP_%u.csv", client->ip_addr_str, (unsigned)time(NULL));
//	sprintf(buffer, "Client_data_file.csv");
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
		
		// send an acknowledgement back to the client saying that we received the message
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "%s sent the server: %s", client->ip_addr_str, buffer);
		n = write(client_socket_fd, tmp, strlen(tmp));
		if (n < 0) {
			server_error("ERROR writing to socket");
			return NULL;
		}
	}

	fclose(fp);
	close(client_socket_fd);
	return NULL;
}

void * Synchronize_data(void *arg)
{
	struct thread_argu_struc *Sync_argu = (struct thread_argu_struc*)arg;
	int thread_data_num[cli_num];
        int i,sig;
	char buffer[512];
	sig = 1;
	FILE *file;
	Queue *thread_Q[cli_num];

	for (i = 0; i < cli_num; i++)
		thread_Q[i] = Sync_argu->Client_thread[i]->thread_Q;
	
	//csv file to collect client data
	file = fopen("./client_data_value.csv","wb");
	if (file == NULL)
		printf("the error number:%d\n",errno);
	
	int time = 0;
	while(run_flag){
		for(i = 0; i< cli_num; i++){
			if (thread_Q[i]->size == thread_Q[i]->capacity)
				sig = 0; //check whetehr all thread data has reaches its capacity
			else 
				sig = 1;
		}

		memset(buffer, 0, sizeof(buffer));
		
		if (sig == 0){
		for (i=0; i< cli_num; i++){
				if(i == 0)
					strcpy(buffer,front(thread_Q[i]));
				else {
					strcat(buffer," ");
					strcat(buffer,front(thread_Q[i]));	
				}
				Dequeue(thread_Q[i]);	
			}	
			printf("%s\n",buffer);
			fprintf(file,"%s\n",buffer);
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

	pthread_t tids[256];

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
				printf("All clients connected, start to handle their data.\n");
			
//				pthread_create(&tids[max_connections], NULL, Synchronize_data, arg); 
				for (n = 0; n < max_connections; n++)
					pthread_create(&tids[n], NULL, handle_client, (void *)Client_thread[n]);		
				
			}
		}
	}

	if (i > max_connections) {
		printf("Max number of connections reached. No longer accepting connections. Continuing to service old connections.\n");
	}

	for (n = 0; n != i ; n++)
		pthread_join(tids[n],NULL);
	
//	pthread_join(tids[max_connections],NULL);
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t manage_9dof_tid, manage_server_tid, Synchronize_data_tid;
	int rc;

	signal(SIGINT, do_when_interrupted);
	struct thread_argu_struc *data_thread;
	
	data_thread = (struct thread_argu_struc*)malloc(sizeof(struct thread_argu_struc));

	int i;
	for (i = 0; i < cli_num; i++){
		data_thread->Client_thread[i] = (struct thread_argu*)malloc(sizeof(struct thread_argu)); 
		data_thread->Client_thread[i]->Client = malloc(sizeof(CONNECTION*));
		data_thread->Client_thread[i]->client_n = i;
		data_thread->Client_thread[i]->thread_Q = createQueue(queue_cap);
	}	
	
/*	rc = pthread_create(&manage_9dof_tid, NULL, manage_9dof, NULL);
	if (rc != 0) {
		fprintf(stderr, "Failed to create manage_9dof thread. Exiting Program.\n");
		exit(0);
	}
*/
	rc = pthread_create(&manage_server_tid, NULL, manage_server, (void*)data_thread);
	if (rc != 0) {
		fprintf(stderr, "Failed to create manage_server thread. Exiting program.\n");
		fprintf(stderr, "Failed to create manage_server thread. Exiting program.\n");
		exit(0);
	}

	rc = pthread_create(&Synchronize_data_tid, NULL, Synchronize_data, (void*)data_thread);
	if (rc != 0) {
		fprintf(stderr, "Failed to create Synchronize_data thread. Exiting program.\n");
		exit(0);
	}

//	pthread_join(manage_9dof_tid, NULL);
	pthread_join(manage_server_tid, NULL);
	pthread_join(Synchronize_data_tid, NULL);
	
	FILE* f_p;
	printf("\n...cleanup operations complete. Exiting main.\n");

	return 0;
}
