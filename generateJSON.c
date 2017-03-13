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

#include <json/json.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

void get_date(char *date_buffer, size_t buffer_size) {
	time_t timer;
	struct tm* tm_info;
	const char format[] = "%Y-%m-%dT%H:%M:%S";

	if (time(&timer) > 0) {
		perror("Failed to get current time.");
	}

	tm_info = localtime(&timer);
	strftime(date_buffer, buffer_size, format, tm_info);
}

int randomInt(int start_value) {

	int random_int = rand() % 2;

	if (random_int && start_value < 15) {
		start_value++;
	}
	else if (!random_int && start_value > 0) {
		start_value--;
	}

	return start_value;
}

int main() {

	int sensor_1_data = 5;
	int sensor_2_data = 5;

	char date_buffer[30];
	srand(time(NULL));

	FILE *fp_sensor_1;
	FILE *fp_sensor_2;

	json_object *j_sensor_1_ID_int = json_object_new_int(1);
	json_object *j_sensor_2_ID_int = json_object_new_int(2);

	while(1) {
		get_date(date_buffer);
		sensor_1_data = randomInt(sensor_1_data);
		sensor_2_data = randomInt(sensor_2_data);

		json_object *j_sensor_1_obj = json_object_new_object();
		json_object *j_sensor_2_obj = json_object_new_object();

		json_object *j_sensor_1_current_int = json_object_new_int(sensor_1_data);
		json_object *j_sensor_2_current_int = json_object_new_int(sensor_2_data);
		json_object *j_sensor_1_unit = json_object_new_string("Amp");
		json_object *j_sensor_2_unit = json_object_new_string("Amp");
		json_object *j_date_string = json_object_new_string(date_buffer);

		json_object_object_add(j_sensor_1_obj, "Sensor_ID" , j_sensor_1_ID_int);
		json_object_object_add(j_sensor_2_obj, "Sensor_ID" , j_sensor_2_ID_int);
		json_object_object_add(j_sensor_1_obj, "Current" , j_sensor_1_current_int);
		json_object_object_add(j_sensor_2_obj, "Current" , j_sensor_2_current_int);
		json_object_object_add(j_sensor_1_obj, "Date" , j_date_string);
		json_object_object_add(j_sensor_2_obj, "Date" , j_date_string);
		json_object_object_add(j_sensor_1_obj, "Unit" , j_sensor_1_unit);
		json_object_object_add(j_sensor_2_obj, "Unit" , j_sensor_2_unit);

		fp_sensor_1 = fopen("./sensors/sensor_1~.json", "w");
		if (fp_sensor_1 == NULL) {
			fprintf(stderr, "Can't Open File sensor_1\n");
			exit(1);
		}

		fprintf(fp_sensor_1, "%s\n",
			json_object_to_json_string_ext(j_sensor_1_obj, JSON_C_TO_STRING_PRETTY));
		fclose(fp_sensor_1);

		fp_sensor_2 = fopen("./sensors/sensor_2~.json", "w");
		if (fp_sensor_2 == NULL) {
			fprintf(stderr, "Can't Open File sensor_2\n");
			exit(1);
		}

		fprintf(fp_sensor_2, "%s\n",
			json_object_to_json_string_ext(j_sensor_2_obj, JSON_C_TO_STRING_PRETTY));
		fclose(fp_sensor_2);

		rename("./sensors/sensor_1~.json", "./sensors/sensor_1.json");
		rename("./sensors/sensor_2~.json", "./sensors/sensor_2.json");

		sleep(1);
	}
}
