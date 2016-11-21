#include <unistd.h>
#include <mraa/aio.h>
#include <stdio.h>
#include "floatfann.h"

int main()
{
    int i,j;
    int location;
    float max;
    int time;
    fann_type *calc_out;
    fann_type input[12];
    struct fann *ann;
    int debug = 1;
    char filename[] = "data/test_sequence-config1.csv";
    int num_lines = 340;//325;//377;//489;
//    char *line = NULL;
    char line[256];
    
    ann = fann_create_from_file("TEST.net");
    
    FILE *file = fopen(filename, "r");

    //while (1) {
    for(j = 0; j < num_lines; j++) {
        for(i = 0; i < 12; i++) {
            input[i] = 0;
        }
        
        fgets(line, sizeof(line), file);
// sscanf(line, "%d,%f,%f,%f\n", &time, &input[0], &input[1], &input[2]);
        sscanf(line, "%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", &time, &input[0], &input[1], &input[2], &input[3], &input[4], &input[5], &input[6], &input[7], &input[8], &input[9], &input[10], &input[11]);
        
//        if(debug == 1) printf("%s", line);
        if(debug == 1) printf("%d,", time);
        if(debug == 1) {
            for (i = 0; i < 12; i++) {
                printf("%f,", input[i]);
            }
        }

        calc_out = fann_run(ann, input);

        max = calc_out[0];
        location = 0;
//        if(debug == 1) printf("\t%f", calc_out[0]);
        for (i = 1; i < 4; i++) {
            if(debug == 1) printf("\t%f", calc_out[i]);
            if (calc_out[i] > max) {
                max = calc_out[i];
                location = i;
            }
        }
        if(debug == 1) printf(",%d,", location);
        
//        printf("x: %f, y: %f, z: %f -> Output is %d\n", value0, value1, value2, location);
        switch(location) {
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
//        usleep(250000);
    }
    fclose(file);

    fann_destroy(ann);
    return 0;
}
