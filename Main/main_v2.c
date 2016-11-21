#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <fann.h>
#include <dirent.h>

#include "floatfann.h"
#include "server.h"
#include "queue.h"

#define PORTNO 5000
#define cli_num 4
#define queue_cap 2
#define sync_time 20 //20*0.25 = 5 second 

//neural network trainning variables
const unsigned int num_input = 12;
const unsigned int num_output = 4;
const unsigned int num_layers = 3;
const unsigned int num_neurons_hidden = 12; //9;
const float desired_error = (const float) 0.0001;
const unsigned int max_epochs = 5000;
const unsigned int epochs_between_reports = 100;
    
static volatile int run_flag = 1;
struct thread_argu{
	CONNECTION *Client;
	int client_n;
	struct Queue *thread_Q;
};

struct thread_argu_struc{
	struct thread_argu *Client_thread[cli_num];
  struct Queue *Sync_Q;
};

void test_neural_network(char* filename)
{
    int i,j;
    int location;
    float max;
    float time;
    fann_type *calc_out;
    fann_type input[12];
    struct fann *ann;
    int debug = 0;

    char line[256];
    
    ann = fann_create_from_file("TEST.net");
    
    FILE *file = fopen(filename, "r");

    while(fgets(line, sizeof(line), file) != NULL) {
        memset(input, 0, num_input*sizeof(fann_type));
        
        sscanf(line, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", &time, &input[0], &input[1], &input[2], &input[3], &input[4], &input[5], &input[6], &input[7], &input[8], &input[9], &input[10], &input[11]);
        
        if(debug == 1)
        {
           printf("%f,", time);
        
           for (i = 0; i < num_input; i++)
                printf("%f,", input[i]);
        }
        
        calc_out = fann_run(ann, input);
        max = calc_out[0];
        
        location = 0;

        for (i = 1; i < num_output; i++) 
        {
            if(debug == 1) 
		printf("\t%f", calc_out[i]);

            if (calc_out[i] > max) 
            {
                max = calc_out[i];
            //    printf("%f",max);
                location = i;
            }
        }
        
        if(debug == 1) 
           printf(",%d,", location);
        
        switch(location) 
        {
            case 0:
                printf("\tNo patient on bed\n");
                break;
            case 1:
                printf("\tPatient lying down\n");
                break;
            case 2:
                printf("\tPatient sitting up\n");
                break;
            case 3:
                printf("\tPatient sitting on edge\n");
                break;
        }
    }
    
    fclose(file);
    fann_destroy(ann);
}

void display_file()
{
  DIR *d;
  struct dirent *dir;
  d = opendir("./data");
  if (d)
  {
    printf("Here is a list of files in the data folder:\n");
    while ((dir = readdir(d)) != NULL)
    {
     if (dir->d_type == DT_REG)
	    printf("%s\n", dir->d_name);
    }

    closedir(d);
  }
  else 
    printf("data folder does not exist!\n");
}

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

void train_neural_network(char* buffer)
{
    struct fann *ann = fann_create_standard(num_layers, num_input,
        num_neurons_hidden, num_output);

    fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
    fann_set_activation_function_output(ann, FANN_SIGMOID_SYMMETRIC);

    fann_train_on_file(ann, buffer, max_epochs,
        epochs_between_reports, desired_error);

    fann_save(ann, "TEST.net");

    fann_destroy(ann);

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
	file = fopen("./data/client_data_value.csv","wb");
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
				Enqueue(Sync_argu->Sync_Q,buffer);
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

void* Static_pose_detect(void *arg)
{
  int location;
  float time,max;
  fann_type *calc_out;
  fann_type input[12];
  char* line;
  int i;

  struct fann *ann;
  struct thread_argu_struc *Sync_thread_argu;
  struct Queue *Sync_Q; 

  Sync_thread_argu = (struct thread_argu_struc*)arg;
  Sync_Q = Sync_thread_argu->Sync_Q;   
	
  ann = fann_create_from_file("TEST.net");
     
  while(Sync_Q->size > 0 && run_flag) 
  {
        memset(input, 0, num_input*sizeof(fann_type));
        
	line = front(Sync_Q);
	Dequeue(Sync_Q);
        sscanf(line, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", &time, &input[0], &input[1], &input[2], &input[3], &input[4], &input[5], &input[6], &input[7], &input[8], &input[9], &input[10], &input[11]);
        
        printf("%f,", time);
        
        for (i = 0; i < num_input; i++)
           printf("%f,", input[i]);
        
        calc_out = fann_run(ann, input);
        max = calc_out[0];
        
        location = 0;

        for (i = 1; i < num_output; i++) 
        {
            printf("\t%f", calc_out[i]);

            if (calc_out[i] > max) 
            {
                max = calc_out[i];
            //    printf("%f",max);
                location = i;
            }
        }
        
        printf(",%d,", location);
        
        switch(location) 
        {
            case 0:
                printf("\tNo patient on bed\n");
                break;
            case 1:
                printf("\tPatient lying down\n");
                break;
            case 2:
                printf("\tPatient sitting up\n");
                break;
            case 3:
                printf("\tPatient sitting on edge\n");
                break;
        }
    }    
    fann_destroy(ann);
}

void* manage_server(void *arg)
{
	int max_connections;
	int i,n;
	max_connections = cli_num;
	struct thread_argu_struc *server_thread_argu; 
	struct thread_argu *Client_thread[cli_num];
	CONNECTION *server;
	char msg[2] = {'!','\0'};

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
				for (n = 0; n < max_connections; n++)
					write(Client_thread[n]->Client->sockfd, msg, strlen(msg));
			
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
	pthread_t manage_server_tid, Synchronize_data_tid, static_pose_detect_tid;
	int rc;
	char choice;
  char train_f[256];
  char test_f[256];

	signal(SIGINT, do_when_interrupted);
	struct thread_argu_struc *data_thread;
 
	//data initialization 	
	data_thread = (struct thread_argu_struc*)malloc(sizeof(struct thread_argu_struc));
  data_thread->Sync_Q = createQueue(queue_cap);

	int i;
	for (i = 0; i < cli_num; i++){
		data_thread->Client_thread[i] = (struct thread_argu*)malloc(sizeof(struct thread_argu)); 
		data_thread->Client_thread[i]->Client = (CONNECTION*)malloc(sizeof(CONNECTION*));
		data_thread->Client_thread[i]->client_n = i;
		data_thread->Client_thread[i]->thread_Q = createQueue(queue_cap);
	}	
	
	while(1)
	{
		printf("\nPlease specify which function to perform:\n");
		printf("1: Collect training data\n");
		printf("2: Perform Machine learning trainning\n");
		printf("3: Perform Real time pose detection\n");
		printf("4: Test trained neural network\n");
		printf("5: Exit program\n");
		scanf(" %c", &choice);
		
		switch(choice)
		{
			case '1': 	
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
        	
        			printf("\n...cleanup operations complete. Resume main menu.\n");
			break;

			case '2':
             memset(train_f, 0, 64);
             display_file();
		       	 printf("PLease specify training data file: (data/FILENAME.txt)\n");
             scanf("%s", &train_f);
             
             //check whether file exist 
             if (access(train_f, F_OK) != -1)
             {
               printf("File exist, perform trainning...\n");
               train_neural_network(train_f);
               printf("Trainning completed, resume main menu.\n"); 
             }
             else
               printf("File don't exist!\n"); 
			break;

			case '3':
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
        			
				      rc = pthread_create(&static_pose_detect_tid, NULL, Static_pose_detect, (void*)data_thread);
              if (rc != 0) {
        				fprintf(stderr, "Failed to create Static pose detection thread. Exiting program.\n");
        				exit(0);
        			}         
              
              pthread_join(static_pose_detect_tid, NULL);
        			pthread_join(manage_server_tid, NULL);
        			pthread_join(Synchronize_data_tid, NULL);
        	
        			printf("\n...cleanup operations complete. Resume main menu.\n");	
			break;
			
			case '4':
			       memset(test_f, 0, 64);
             display_file();
		       	 printf("PLease specify training data file: (data/FILENAME.csv)\n");
             scanf("%s", &test_f);
             
             //check whether file exist 
             if (access(test_f, F_OK) != -1)
             {
               printf("File exist, perform testing...\n");
               test_neural_network(test_f);
               printf("Testing completed, resume main menu.\n"); 
             }
             else
               printf("File don't exist!\n"); 
			break;

			case '5':
			printf("Exiting program....\n");
			return 0;
			break;

			default:
			printf("invalid input, try again\n");
			break; 
			}
		}
	return 0;
}
