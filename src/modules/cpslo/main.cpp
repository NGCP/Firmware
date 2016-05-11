/*
 * Make sure to add the following to the SD card for autostart:
 * etc/extras.txt -> cpslo start
 * 
 */

// Standard Library Imports
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

// PX4 API Library Imports
#include <px4_posix.h>
#include <px4_config.h>
#include <px4_tasks.h>
#include <nuttx/config.h>

// Module Imports
#include "Search.h"

// Expose our main function to the PX4 API so that the commander app can call it
extern "C" __EXPORT int cpslo_main(int argc, char *argv[]);

// Daemon Main Loop
int cpslo_thread_main(int argc, char *argv[]);

// Function Prototypes
void cp_usage(const char *reason);

// Daemon Globals
static bool cp_thread_should_exit = false;
static bool cp_thread_running = false;
static int cp_daemon_task;

int cpslo_main(int argc, char *argv[]) {
	if (argc < 2) {
		cp_usage("missing command");
		return 1;
	}

	if (!strcmp(argv[1], "start")) {
		if (cp_thread_running) {
			warnx("CPSLO already running");
			return 0;
		}

		cp_thread_should_exit = false;
		cp_daemon_task = px4_task_spawn_cmd("cpuav", SCHED_RR, SCHED_PRIORITY_DEFAULT, 1024, cpslo_thread_main, NULL);

		unsigned constexpr max_wait_us = 1000000;
		unsigned constexpr max_wait_steps = 2000;
		unsigned i;
		for (i = 0; i < max_wait_steps; i++) {
			usleep(max_wait_us / max_wait_steps);
			if (cp_thread_running)
				break;
		}
		return !(i < max_wait_steps);
	}

	if (!strcmp(argv[1], "stop")) {
		if (!cp_thread_running) {
			warnx("commander already stopped");
			return 0;
		}
		cp_thread_should_exit = true;
		while (cp_thread_running) {
			usleep(200000);
			//warnx(".");
		}
		warnx("cpslo terminated");
		return 0;
	}

	if (!strcmp(argv[1], "status")) {
		if (cp_thread_running)
			warnx("cpslo is running as a daemon!");
		else
			warnx("cpslo is not running as a daemon!");
		return 0;
	}
	
	if (!strcmp(argv[1], "debug")) {
		Search test;
		test.printDebug();
		return 0;
	}
	
	if (!strcmp(argv[1], "set")) {
		Search test;
		// test.SetMission(.0000001);
		test.SetExample();
		return 0;
	}
	
	if (!strcmp(argv[1], "clear")) {
		Search test;
		test.ClearAll();
		return 0;
	}
	
	cp_usage("Unrecognized Command");
	return 1;
}

int cpslo_thread_main(int argc, char *argv[]) {
	cp_thread_running = true;
	warnx("cpslo module starting!");
	while (!cp_thread_should_exit)
		usleep(10000);
	cp_thread_running = false;
	return 0;
}

void cp_usage(const char *reason) {
	if (reason && *reason > 0)
		PX4_INFO("cpslo: %s", reason);
	PX4_INFO("usage: cpslo {start|stop|status|debug|set|clear}\n");
}