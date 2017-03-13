/*
file: generateJSON.c

Written by: Jiawei Wu of ECE492 group 5
Date: 02/03/2017

Modified by: Satyen Akolkar of ECE492 group 5
Date: 12/03/2017

Description:
	Native C file that generates the JSON file and saves it to disk,
	allowing the server to access said files.

	At the moment, this file simply generates random values.
	Once the sensors have been set up, it will use the sensor
	value and output it into a JSON format.

	New files are generated every second, file creation
	is atomic and thread safe.
*/

#define DEBUG

#include <json/json.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int get_date(char *date_buffer, size_t buffer_size) {
	int ERR = -1;

	time_t timer;
	struct tm* tm_info;
	const char format[] = "%Y-%m-%dT%H:%M:%S";

	if (time(&timer) < 0) {
#ifdef DEBUG
		perror("Failed to get current time.");
#endif
		return ERR;
	}

	tm_info = localtime(&timer);
	if (tm_info == NULL) {
#ifdef DEBUG
		perror("Failed to derive localtime from current time.");
#endif
		return ERR;
	}

	if (strftime(date_buffer, buffer_size, format, tm_info) == 0) {
#ifdef DEBUG
		perror("Failed to format localtime. Indeterminate Result.");
#endif
	}
}

//Generates the JSON file and outputs it to current directory
void generateJSON(int channel, int value) {

    //Buffer to hold the date
    char date_buffer[30];
    //Buffer to hold temporary path name
    char path_buffer_temp[30];
    //Buffer to hold actual path name
    char path_buffer[30];

    //Get the date
    if (get_date(date_buffer) < 0) {
#ifdef DEBUG
		fprintf(stderr, "Failed get_date() while generatingJSON for channel %d", channel);
#endif
		exit(EXIT_FAILURE);
	}

    //Initialize new libjson JSON object
    json_object *j_sensor_obj         = json_object_new_object();
    json_object *j_sensor_value_int   = json_object_new_int(value);
    json_object *j_sensor_channel_int = json_object_new_int(channel);
    json_object *j_date_string        = json_object_new_string(date_buffer);

    //Add the INT and String objects to JSON object
    json_object_object_add(j_sensor_obj, "Channel", j_sensor_channel_int);
    json_object_object_add(j_sensor_obj, "Voltage", j_sensor_value_int);
    json_object_object_add(j_sensor_obj, "Date", j_date_string);

    //Generate path buffers (temp has a ~)
    snprintf(path_buffer_temp, 30, "./sensor_%d~.json", channel);
    snprintf(path_buffer, 30, "./sensor_%d.json", channel);

    //All the file IO stuff...
    FILE *fp_sensor;
    fp_sensor = fopen(path_buffer, "w");
    if (fp_sensor == NULL) {
        fprintf(stderr, "Can't Open File Sensor_%d\n", channel);
        exit(EXIT_FAILURE);
    }

    fprintf(fp_sensor, "%s\n",
        json_object_to_json_string_ext(j_sensor_obj, JSON_C_TO_STRING_PRETTY));
    fclose(fp_sensor);

    //Rename (This is atomic)
    rename(path_buffer_temp, path_buffer);
}
