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

static volatile int run_flag = 1;

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
	FILE *fp;
	
	client = (CONNECTION *)arg;
	printf("%d\n",(int)client);
	client_socket_fd = client->sockfd;

	memset(buffer, 0, 256);
	sprintf(buffer, "output_file_IP_%s_TIMESTAMP_%u.csv", client->ip_addr_str, (unsigned)time(NULL));
	printf("filename: %s", buffer);

	fp = fopen(buffer, "wb");

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

		// print the message to console
		printf("%s says: %s\n", client->ip_addr_str, buffer);
		fprintf(fp, "%s\n", buffer);

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

void* Synchronize_data(void *arg)
{
	int n,file_n;
	char file[2][256]; //use to store two csv file name 
	char line[2][64]; //to store the line data read from each file
	FILE *fp[2]; //suppose there is only two csv file

	DIR *dp;
	struct dirent *ent;
	size_t len;

	file_n = -1; 

	memset(file[0], 0, 256);
	memset(file[1], 0, 256);
	memset(line[0], 0, 64);
	memset(line[1], 0, 64);

	//loop through current directory and seek for csv files 
	if((dp = opendir(".")) == NULL)
		return NULL;

	while ((ent = readdir(dp)) != NULL)
	{
		//if not regular file
		if(ent->d_type != DT_REG)
			continue;
		
		len = strlen(ent->d_name);
		
		//store csv file names 
		if (len > 4 && strcmp(ent->d_name + len - 4, ".csv") == 0)
		{
			file_n ++;
			sprintf(file[file_n], "%s", ent->d_name); 	
		}
		
		if(file_n > 2)
		{
			printf("Too many files!\n");
			break;
		}
	}
	closedir(dp);
	
	if (file_n == 1){  
		fp[0] = fopen(file[0], "r");
		fp[1] = fopen(file[1], "r");	
	}
	else {
		printf("Cannot find enough .csv file\n");	
		return NULL; //no .csv file exists	
	}

	//variables to store angle values and for queue construction process	
	char sync_line[64];
	memset(sync_line, 0, 64);

	float TS1,P1,R1,TS2,P2,R2;
  	float temp_TS, temp_P, temp_R;
	int early_fid = -1; //index to label the early arrive data csv file 
	int lag_time; //time difference between two data arriving time
  	int counter0 = 0;
	int counter1 = 0;
	int counter = 0;

	if (file_n == 1){
		//read the first line of both csv files
		fgets(line[0],64,fp[0]);
		fgets(line[1],64,fp[1]);
		//extract the timestamp and angle values
		sscanf(line[0], "%f,%f,%f", TS1,P1,R1);
		sscanf(line[1], "%f,%f,%f", TS2,P2,R2);
	}
	//determine which file arrives early
	if ((lag_time = TS1 - TS2) > 0) 
		early_fid = 0;
	else 
		early_fid = 1;

	//store that early arrived data section to a queue structure
	while(!feof(fp[0]))
	{
		fgets(line[0],64,fp[0]);
    		sscanf(line[0],"%f,%f,%f", TS1, P1, R1);
   		counter0 ++;	
       	}

	while(!feof(fp[1]))
	{
		fgets(line[1],64,fp[1]);
    		sscanf(line[1],"%f,%f,%f", TS2, P2, R2);
   		counter1 ++;	
       	}

		
	//store the early arrived data to the queue
	Queue *Q = createQueue(counter0-counter1);
	while(!feof(fp[early_fid]))
	{
		fgets(line[early_fid],64,fp[early_fid]);
		Enqueue(Q,line[early_fid]);
		memset(line[early_fid],0,64);
		if (Q->size == counter)
			break;
	}
	
	float TS_early, P_early, R_early;
	FILE* sync_fp;
       	sync_fp = fopen("./sync_data.csv", "Wb");	
	//output the sync data to a new file
 	while(run_flag){
		if (file_n != -1) {
			fgets(line[0], 64, fp[0]);
			fgets(line[1], 64, fp[0]);
			sscanf(line[0],"%f,%f,%f",TS1,P1,R1);
			sscanf(line[1],"%f,%f,%f",TS2,P2,R2);
			switch(early_fid){
			case 0:
				sscanf(rear(Q),"%f,%f,%f",TS_early,P_early,R_early);	
				Dequeue(Q);
				Enqueue(Q,line[0]);
				sprintf(sync_line,"%f %f %f %f",P_early, R_early, P2, R2);
				
				fprintf(sync_fp,"%s",sync_line);//write to file 	
				printf("%s\n", sync_line);
				memset(line[0], 0, 64);
				break;
			case 1:
				sscanf(rear(Q),"%f,%f,%f",TS_early,P_early,R_early);	
				Dequeue(Q);
				Enqueue(Q,line[1]);
				sprintf(sync_line,"%f %f %f %f",P_early, R_early, P1, R1);
				
				fprintf(sync_fp,"%s",sync_line);//write to file 
				printf("%s\n", sync_line);
				memset(line[1], 0, 64);
				break;
			default:
				break;
			}
			 
		}	
	}
	
	int i;
	for(i = 0; i < file_n; i++)
		fclose(fp[i]);

}


void* manage_server(void *arg)
{
	int max_connections;
	int i,n;
	max_connections = 2; //it was 10

	CONNECTION *server;
	CONNECTION *client[max_connections];
	
	for (i = 0; i < max_connections; i++)
		client[i] = malloc(sizeof(CONNECTION));	
	
	pthread_t tids[256];

	server = (CONNECTION *) server_init(PORTNO, 10);
	if ((int) server == -1){
		run_flag = 0;
	}

	i = 0; //-1

	while(i < max_connections && run_flag) {	
		client[i] = (CONNECTION*) server_accept_connection(server->sockfd);
		printf("%d\n",(int)client[0]);
		printf("%d\n",(int)client[1]);
		if ((int) client[i] == -1) {
			printf("Latest child process is waiting for an incoming client connection.\n");
		}
		else {
			printf("i m here1\n");
			i++;
			if (i == max_connections) {
				printf("All clients connected, start to handle their data.\n");

				for (n = 0; n < max_connections; n++){
					pthread_create(&tids[n], NULL, handle_client, (void *)client[n]);		
					printf("%d %d,i m here2\n",i,n);
				}
			}
		}
	}

	if (i > max_connections) {
		printf("Max number of connections reached. No longer accepting connections. Continuing to service old connections.\n");
	}
	
	for(; i >= 0; i--) {
		pthread_join(tids[i-1], NULL);
	}

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t manage_9dof_tid, manage_server_tid, Synchronize_data_tid;
	int rc;

	signal(SIGINT, do_when_interrupted);

	rc = pthread_create(&manage_9dof_tid, NULL, manage_9dof, NULL);
	if (rc != 0) {
		fprintf(stderr, "Failed to create manage_9dof thread. Exiting Program.\n");
		exit(0);
	}

	rc = pthread_create(&manage_server_tid, NULL, manage_server, NULL);
	if (rc != 0) {
		fprintf(stderr, "Failed to create manage_server thread. Exiting program.\n");
		exit(0);
	}

/*	rc = pthread_create(&Synchronize_data_tid, NULL, Synchronize_data, NULL);
	if (rc != 0) {
		fprintf(stderr, "Failed to create Synchronize_data thread. Exiting program.\n");
		exit(0);
	}
*/

//	pthread_join(Synchronize_data_tid, NULL);
	pthread_join(manage_9dof_tid, NULL);
	pthread_join(manage_server_tid, NULL);

	printf("\n...cleanup operations complete. Exiting main.\n");

	return 0;
}
