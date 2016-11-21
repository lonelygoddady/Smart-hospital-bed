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

void* manage_server(void *arg)
{
	CONNECTION *server;
	CONNECTION *client;
	int max_connections;
	int i;
	pthread_t tids[256];

	max_connections = 10;

	server = (CONNECTION *) server_init(PORTNO, 10);
	if ((int) server == -1){
		run_flag = 0;
	}

	i = -1;

	while(i < max_connections && run_flag) {
		client = (CONNECTION*) server_accept_connection(server->sockfd);
		if ((int) client == -1) {
			printf("Latest child process is waiting for an incoming client connection.\n");
		}
		else {
			i++;
			pthread_create(&tids[i], NULL, handle_client, (void *)client);
		}
	}

	if (i >= max_connections) {
		printf("Max number of connections reached. No longer accepting connections. Continuing to service old connections.\n");
	}


	for(; i >= 0; i--) {
		pthread_join(tids[i], NULL);
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


  int n,file_n;
	char file[2][256]; //use to store two csv file name 
	char line[2][64]; //to store the line data read from each file
	FILE *fp[2]; //suppose there is only two csv file

	DIR *dp;
	struct dirent *ent;
	size_t len;

	file_n = -1; 

	memset(file[0], 0, 512);
	memset(file[1], 0, 512);
	memset(line[0], 0, 512);
	memset(line[1], 0, 512);

	//loop through current directory and seek for csv files 
	if((dp = opendir("./output")) == NULL)
		return 0;

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
	

  fp[0] = fopen(file[0], "r");
	fp[1] = fopen(file[1], "r");	
 
	float time, pitch, roll;
 
  while( fgets(line[0],512,fp[0]))
  {
      printf("%s", line[0]);
      sscanf(line[0], "%f,%f,%f", time, pitch, roll);
      printf("time is %s\n", time);
      printf("pitch is %s\n", pitch);
      printf("roll is %s\n", roll);
  } 
 

	int i;
	for(i = 0; i < file_n; i++)
	fclose(fp[i]);

	//pthread_join(manage_9dof_tid, NULL);
	//pthread_join(manage_server_tid, NULL);

	printf("\n...cleanup operations complete. Exiting main.\n");

	return 0;
}
