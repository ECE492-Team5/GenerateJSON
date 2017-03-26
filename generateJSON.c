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

#include <signal.h>
#include <sys/stat.h>
#include <string.h>

// SIGNAL FLAGS
static volatile sig_atomic_t REREAD_CONFIG = 0;
static volatile sig_atomic_t GRACEFUL_EXIT = 0;

// FUNCTION SIGNATURES
void        fork_child_kill_parent();
void        generateJSON(int channel, int value);
int         get_date(char *date_buffer, size_t buffer_size);
void        init_signals();
void        inititalize();
int         read_adc(int channel);
static void sig_handler(int signo, siginfo_t *si, void *unused);

#define NUM_CHANNELS 8

int main() {
	int channel = 0;
	int millivolts = 0;

	// daemonize the program
	inititalize();

	while(1) {
		if (REREAD_CONFIG) {
			load_config();
			REREAD_CONFIG = 0;
		}

		if (GRACEFUL_EXIT) {
			break;
		}

		// Read Sensor Value from ADC
		millivolts = read_adc(channel);

		if (GRACEFUL_EXIT) {
			break;
		}

		// Output Read Value into JSON File
		if (channel < 4) {
			generateJSON(channel, get_current(channel, millivolts));
		} else {
			generateJSON(channel, millivolts);
		}

		if (GRACEFUL_EXIT) {
			break;
		}
#ifdef SLOWREADS
		sleep(10);
#else
		sleep(1);
#endif

		channel += 1;
		channel %= NUM_CHANNELS;
	}

	free_memory();
	return EXIT_SUCCESS;
}


void inititalize() {

	// Reference http://stackoverflow.com/a/17955149/6248563
	// StackOverflow Answer for: Creating a daemon in Linux
	// Answer By: Pascal Werkl on Jul 30 '13 (Edited: Dec 29 '16)
	// Accessed by: Satyen Akolkar on Mar 12 '17

	// Fork off the parent process to run in background
	fork_child_kill_parent();

	// create new session with child as leader without a controlling terminal
	if (setsid() < 0) {
#ifdef DEBUG
		fprintf(stdout, "Failed to create new session.\n");
#endif
		perror("setsid()");
		exit(EXIT_FAILURE);
	}

	init_signals();

	// Fork again so that session leader is killed. Can't be reassociated with
	// a terminal.
	fork_child_kill_parent();

	//XXX: might need to adjust for proper permissions with created files
	umask(0);
	chdir("/");

	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>=0; x--) {
		close (x);
	}

#ifdef DEBUG
	fprintf(stdout, "Process daemonized.");
#endif
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
	if (get_date(date_buffer, 30) < 0) {
#ifdef DEBUG
		fprintf(stdout, "Failed get_date() while generatingJSON for channel %d\n", channel);
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

void fork_child_kill_parent() {
	pid_t pid;
	pid = fork();

	// fork fails parent runs check
	if (pid < 0) {
#ifdef DEBUG
		fprintf(stdout, "Failed to fork child. Returned PID: %d\n", pid);
#endif
		perror("fork()");
		exit(EXIT_FAILURE);
	}

	// fork succeeds parent runs check
	if (pid > 0) {
#ifdef DEBUG
		fprintf(stdout, "Success: child process forked. Killing parent.\n");
#endif
		exit(EXIT_SUCCESS);
	}

	// only child survives till here pid=0 in child
}

void init_signals() {
	sigaction sa;
	// initialize sigaction struct and signal handling
	sa.sa_flags = SA_SIGINFO;
	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = sig_handler;

	if (sigaction(SIGHUP, &sa, NULL) == -1) {
		perror("Failed to setup signal handler for SIGHUP.");
		exit(EXIT_FAILURE);
	}

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("Failed to setup signal handler for SIGINT.");
		exit(EXIT_FAILURE);
	}

	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("Failed to setup signal handler for SIGTERM.");
		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	fprintf(stdout, "Signal handlers set.\n");
#endif
}

static void sig_handler(int signo, siginfo_t *si, void *unused) {
	switch (signo) {
		case SIGHUP:
			REREAD_CONFIG = 1;
			break;

		case SIGINT:
			GRACEFUL_EXIT = 1;
			break;

		case SIGTERM:
			GRACEFUL_EXIT = 1;
			break;

		default:
			break;
	}
}


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

	return 0;
}
