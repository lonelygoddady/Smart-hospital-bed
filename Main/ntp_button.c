#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

#include "server.h"
#include "9DOF.h"
#include "LSM9DS0.h"

#include <unistd.h>
#include <mraa/gpio.h>

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

void write_timestamp() {
    double sec_since_epoch = timestamp();
    FILE *fp;
    
    printf("button pressed at %10.10f\n", sec_since_epoch);
    
    fp = fopen("./button_timestamps.csv", "a");
    fprintf(fp, "%10.10f\n", sec_since_epoch);
    fclose(fp);
}

void* manage_9dof(void *arg)
{
    
    mraa_gpio_context button;
    button = mraa_gpio_init(4);
    mraa_gpio_dir(button, MRAA_GPIO_IN);

    double sec_since_epoch;    
   
    FILE *fp;
    fp = fopen("./button_timestamps.csv", "w");
    fclose(fp);
   
    while(run_flag) {
       mraa_gpio_isr(button ,MRAA_GPIO_EDGE_RISING, &write_timestamp, NULL);
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
    pthread_t manage_9dof_tid, manage_server_tid;
    int rc;
    
    signal(SIGINT, do_when_interrupted);
    
    rc = pthread_create(&manage_9dof_tid, NULL, manage_9dof, NULL);
    if (rc != 0) {
        fprintf(stderr, "Failed to create manage_9dof thread. Exiting Program.\n");
        exit(0);
    }
    
    rc = pthread_create(&manage_server_tid, NULL, manage_server, NULL);
    if (rc != 0) {
        fprintf(stderr, "Failed to create thread. Exiting program.\n");
        exit(0);
    }
    
    pthread_join(manage_9dof_tid, NULL);
    pthread_join(manage_server_tid, NULL);
    
    printf("\n...cleanup operations complete. Exiting main.\n");
    
    return 0;
}
