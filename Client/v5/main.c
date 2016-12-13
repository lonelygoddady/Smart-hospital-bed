/*
UCLA EE202C Smart Hospital Bed Project
Edison 9DOF Client Code
Josh Rooks
v0.5
*/

#include "client.h"
#include "LSM9DS0.h"

#include <math.h>
#include <mraa.h>
#include <mraa/gpio.h>
#include <mraa/i2c.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>

#define XM_ADDR 0x1d
#define M_PI 3.14159265358979323846

typedef unsigned long long llu;

static volatile int run_flag = 1;

data_t calibrate();
double timestamp();
mraa_i2c_context accelInit();

void do_when_interrupted()
{
	run_flag = 0;
}

int client_handle_connection(int client_socket_fd)
{
	char buffer[256];
    
    data_t accel_data[30];
    data_t acal;
    float a_res = 0.061 / 1000;
	mraa_i2c_context accel;
    mraa_gpio_context intr1;
    mraa_gpio_context intr2;
    
    int i, j, n, r;
    int pr = 0;
    
    uint8_t fifoStatus = 0;
    uint8_t fifoLevel = 0;
    uint8_t fifoOvf = 0;
    uint8_t watermark = 0;
    
    uint8_t int1;
    uint8_t int2;
    
    double q;
    double time;
    double ts;
    
    float cx, cy, cz;
    float dx, dy, dz;
    float cPitch, cYaw, cRoll;
    float pitch, yaw , roll;
    float xSum, ySum, zSum;
    float xAvg, yAvg, zAvg;
    float cxAvg[4], cyAvg[4], czAvg[4];

	accel = accelInit();
    intr1 = mraa_gpio_init(33);
    intr2 = mraa_gpio_init(47);
    
    mraa_gpio_dir(intr1, MRAA_GPIO_IN);
    mraa_gpio_dir(intr2, MRAA_GPIO_IN);
	
    j = 0;
    
    //get reference orientation angles
    printf("Calibrating...\n");
    while(j < 4)
    {
        int2 = mraa_gpio_read(intr2);
        if(int2)
        {
            fifoStatus = mraa_i2c_read_byte_data(accel, FIFO_SRC_REG);
            fifoLevel = fifoStatus & 0x1F;
            
            xSum = 0;
            ySum = 0;
            zSum = 0;
            
            for(i = 0; i < 25; i++)
            {
                accel_data[i] = read_accel(accel, a_res);
                xSum = xSum + accel_data[i].x;
                ySum = ySum + accel_data[i].y;
                zSum = zSum + accel_data[i].z;
            }
            
            cxAvg[j] = xSum / 25;
            cyAvg[j] = ySum / 25;
            czAvg[j] = zSum / 25;
           
            j++;
        }
        usleep(100000);
    }
    
    cx = (cxAvg[0] + cxAvg[1] + cxAvg[2] + cxAvg[3]) / 4;
    cy = (cyAvg[0] + cyAvg[1] + cyAvg[2] + cyAvg[3]) / 4;
    cz = (czAvg[0] + czAvg[1] + czAvg[2] + czAvg[3]) / 4;
    
    cRoll  = atan2(cx, sqrt(cy*cy + cz*cz))*180/M_PI;
    cPitch = atan2(cy, sqrt(cx*cx + cz*cz))*180/M_PI;
    cYaw   = atan2(sqrt(cx*cx + cy*cy), cz)*180/M_PI;
    
    printf("cAngle = X: %f\t Y: %f\t Z: %f\t\n", dx, dy, dz);

    /*
	memset(buffer, 0, 256);
	sprintf(buffer, "time (epoch), angle_x, angle_y, angle_z");

	n = write(client_socket_fd, buffer, strlen(buffer));
	if (n < 0) {
		return client_error("ERROR writing to socket");
	}

	memset(buffer, 0, 256);

	n = read(client_socket_fd, buffer, 255);
	if (n < 0) {
		return client_error("ERROR reading from socket");
	}

	printf("msg from server: %s\n", buffer);
    */
    
	while (run_flag) 
    {
        
        time = timestamp();
        q = (time - (int)time);
        r = (int)(q * 4);
        
        if(r != pr)
        {
            fifoStatus = mraa_i2c_read_byte_data(accel, FIFO_SRC_REG);
            //printf("Fifo Status = %02X\n", fifoStatus);
            watermark = fifoStatus >> 7;
            fifoLevel = fifoStatus & 0x1F;
            fifoOvf = (fifoStatus >> 6) & 0x01;
            pr = r;
        
            xSum = 0;
            ySum = 0;
            zSum = 0;
            
            for(i = 0; i < fifoLevel; i++)
            {
                accel_data[i] = read_accel(accel, a_res);
                xSum = xSum + accel_data[i].x;
                ySum = ySum + accel_data[i].y;
                zSum = zSum + accel_data[i].z;
            }
            
            xAvg = xSum / fifoLevel;
            yAvg = ySum / fifoLevel;
            zAvg = zSum / fifoLevel;
            
            roll  = atan2(xAvg, sqrt(yAvg*yAvg + zAvg*zAvg))*180/M_PI;
            pitch = atan2(yAvg, sqrt(xAvg*xAvg + zAvg*zAvg))*180/M_PI;
            yaw   = atan2(sqrt(xAvg*xAvg + yAvg*yAvg), zAvg)*180/M_PI;
            
            dx = roll - cRoll;
            dy = pitch - cPitch;
            dz = yaw - cYaw;
            
            memset(buffer, 0, 256);

            // write 9DOF reading to buffer
            sprintf(buffer, "%10.2f,%f,%f,%f,", time, dx, dy, dz);

            // send 9DOF reading to server
            n = write(client_socket_fd, buffer, strlen(buffer));
            if (n < 0) 
            {
                return client_error("ERROR writing to socket");
            }
            
            printf("dAngle = X: %f\t Y: %f\t Z: %f\t\n", dx, dy, dz);
            printf("Time = %f\n", time);
        }


	}
	close(client_socket_fd);
	return 1;
}

int main(int argc, char *argv[])
{
    char buffer[64];
	int client_socket_fd;

	signal(SIGINT, do_when_interrupted);

	mraa_init();

	// client initialization
	client_socket_fd = client_init(argc, argv);
	if (client_socket_fd < 0) {
		return -1;
	}
    
    printf("Waiting for starting msg from server...\n"); 
	while(run_flag)
	{
		memset(buffer, 0, 64);
		read(client_socket_fd, buffer, 63); 
		if (buffer[0] == '!')
		{
			printf("Receive starting msg from server!\n");
			break;
		}
	}

	// run client, read 9DOF data, and send to server
	client_handle_connection(client_socket_fd);

	return 0;
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

mraa_i2c_context accelInit()
{
    mraa_i2c_context accel;
	accel = mraa_i2c_init(1);

	if (accel == NULL) {
		fprintf(stderr, "Failed to initialize I2C.\n");
		exit(1);
	}
	mraa_i2c_address(accel, XM_ADDR);
	
	uint8_t accel_id = mraa_i2c_read_byte_data(accel, WHO_AM_I_XM);
	if (accel_id != 0x49) {
		fprintf(stderr, "Accelerometer/Magnetometer ID does not match.\n");
	}
	
	mraa_i2c_write_byte_data(accel, 0x60, CTRL_REG0_XM);	//enable fifo with watermark	
	mraa_i2c_write_byte_data(accel, 0x67, CTRL_REG1_XM);	//100 Hz
	mraa_i2c_write_byte_data(accel, 0x00, CTRL_REG2_XM);		
	mraa_i2c_write_byte_data(accel, 0x04, CTRL_REG3_XM);		
    mraa_i2c_write_byte_data(accel, 0x01, CTRL_REG4_XM);	//watermark interrupt on int2
    mraa_i2c_write_byte_data(accel, 0x59, FIFO_CTRL_REG);   //FIFO mode, watermark level = 25

	return accel;   
}
