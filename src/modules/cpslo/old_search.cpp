/*things that could go wrong
coordinates not in order
square not perfect
comparing floats
 */

#include <px4_posix.h>
#include <px4_config.h>
#include <uORB/uORB.h>
#include <uORB/topics/vehicle_global_position.h>
#include <uORB/topics/mission.h>
#include <dataman/dataman.h>
#include <terrain_estimation/terrain_estimator.h>
#include <navigator/navigation.h>
#include <mavlink/mavlink_mission.h>

#include <math.h>
#include <float.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct gps_coord {
	int32_t lat; //y
	int32_t lon; //x
} gps_coord;

enum coordposition {
	topleft = 0,
	bottomleft = 1,
	topright = 2,
	bottomright = 3
};

//extern "C" __EXPORT int search_main();

static gps_coord mapgrid(float drs, int sleep, float lat, float lon) {
	struct gps_coord currentCoord = {0, 0};
	/* int direction = -1;
	 int finished = 0;
	 struct gps_coord coordinates[4];

	 struct vehicle_gps_position_s data;
	 memset(&data, 0, sizeof(data));

	 int gps_fd = orb_subscribe(ORB_ID(vehicle_gps_position));
	 orb_copy(ORB_ID(avehicle_gps_position), gps_fd, &data);

	 coordinates[topleft] = {data.lat, data.lon};
	 coordinates[bottomleft] = {lat*(int32_t)pow(10, 7), data.lon};
	 coordinates[topright] = {data.lat, lon*(int32_t)pow(10, 7)};
	 coordinates[bottomright] = {lat*(int32_t)pow(10,7), lon*(int32_t)pow(10,7)};

	 while (!finished) {
		if (direction == -1) {
		   while (data.lat + drs > coordinates[bottomleft].lat) {
			  //Move towards bottom, Waypoint or mission??
			  //Camera call
			  //sleep call?
			  orb_copy(ORB_ID(avehicle_gps_position), gps_fd, &data);
		   }
		}
		else {
		   while (data.lat - drs < coordinates[topleft].lat) {
			  //Move towards the top, Waypoint or mission??
			  //Camera Call?
			  //Sleep Call?
			  orb_copy(ORB_ID(avehicle_gps_position), gps_fd, &data);
		   }
		}

		direction *= -1;
		//move to the right, Waypoint or mission

		if(data.lon >= coordinates[bottomright].lon) //Might stop to soon
		   finished = 1;
	 }
	 */
	return currentCoord;
}

int search() {

	struct mission_item_s flight_vector_s {
	};

	struct mission_item_s flight_vector_e {
	};
	struct vehicle_gps_position_s gps;
	struct mission_s my_mission;
	orb_advert_t my_orb;

	int gps_fd;
	my_orb = orb_advertise(ORB_ID(onboard_mission), &my_mission);
	my_mission.count = 2;
	my_mission.current_seq = 0;

	//int mission_item_fd;

	memset(&gps, 0, sizeof (struct vehicle_gps_position_s));
	memset(&flight_vector_s, 0, sizeof (struct mission_item_s));
	memset(&flight_vector_e, 0, sizeof (struct mission_item_s));

	gps_fd = orb_subscribe(ORB_ID(vehicle_gps_position));
	orb_copy(ORB_ID(vehicle_gps_position), gps_fd, &gps);

	//mission_item_fd = orb_subscribe(ORB_ID());

	flight_vector_s.nav_cmd = NAV_CMD_WAYPOINT;
	flight_vector_s.acceptance_radius = 50; // TODO: make parameter
	flight_vector_s.autocontinue = true;
	flight_vector_s.altitude_is_relative = false;

	flight_vector_e.nav_cmd = NAV_CMD_WAYPOINT;
	flight_vector_e.acceptance_radius = 50; // TODO: make parameter
	flight_vector_e.autocontinue = true;
	flight_vector_e.altitude_is_relative = false;

	printf("Current GPS: lat: %d, lon: %d\n", gps.lat, gps.lon);

	flight_vector_s.lat = (double) gps.lat / pow(10, 7);
	flight_vector_s.lon = (double) gps.lon / pow(10, 7);

	printf("Start Vector: lat: %f, lon: %f\n", flight_vector_s.lat, flight_vector_s.lon);

	flight_vector_e.lat = flight_vector_s.lat + 2.0;
	flight_vector_e.lon = flight_vector_s.lon + 2.0;

	printf("Start Vector: lat: %f, lon: %f\n", flight_vector_e.lat, flight_vector_e.lon);

	dm_write(DM_KEY_WAYPOINTS_ONBOARD, 0, DM_PERSIST_IN_FLIGHT_RESET, &flight_vector_s, sizeof (struct mission_item_s));
	dm_write(DM_KEY_WAYPOINTS_ONBOARD, 1, DM_PERSIST_IN_FLIGHT_RESET, &flight_vector_e, sizeof (struct mission_item_s));

	orb_publish(ORB_ID(onboard_mission), my_orb, &my_mission);



	mapgrid(0, 1, 2.0, 3.0);

	/*scanf(%d, %d);
	Waypoints wai = new Waypoints();
	struct WP one = {0,0,1000,10000,10000};
	wai.set_waypoint_with_index(one);
	struct WP newwp = {0,0,0,0,0};
	newwp = wai.get_current_waypoint(void);
	cout >> newwp.id >> newwp.p1 >> newwp.alt >> newwp.lat >> newwp.lng*/
	return 0;
}