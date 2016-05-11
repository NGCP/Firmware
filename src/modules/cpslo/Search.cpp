/*
 * For Now, we're making a dynamic mission from start position to end position.
 * Start Position = Current Location when search was invoked.
 * End Position = 15M north of Start Position.
 */

// Standard Library Imports
#include <string.h>
#include <cmath>

// PX4 Library Imports
#include <px4_posix.h>
#include <dataman/dataman.h>
#include <uORB/uORB.h>
#include <uORB/topics/home_position.h>
#include <dataman/dataman.h>

// Module Imports
#include "Search.h"

// Macro's
// Implement better way to do this later!
#define sind(x) (sin((x) * M_PI / 180))
#define cosd(x) (cos((x) * M_PI / 180))

/*
 * Possible Bug: We assume Home Position is already set by commander.
 */
Search::Search() {
	// Clear out stuct data
	memset(&search_mission, 0, sizeof (struct mission_s));
	memset(&next_point, 0, sizeof (struct mission_item_s));
	memset(&last, 0, sizeof (struct home_position_s));
	memset(&home, 0, sizeof (struct home_position_s));
	// Get Current Mission!
	int mission_sub = orb_subscribe(ORB_ID(offboard_mission));
	orb_copy(ORB_ID(offboard_mission), mission_sub, &search_mission);
	orb_unsubscribe(mission_sub);
	// Set Start Position to Home Position
	int home_sub = orb_subscribe(ORB_ID(home_position));
	orb_copy(ORB_ID(home_position), home_sub, &home);
	orb_unsubscribe(home_sub);
	// Read in the last waypoint on the offboard mission, store it to a local variable
	dm_read(DM_KEY_WAYPOINTS_OFFBOARD(offboard), 0, &last, sizeof (struct mission_item_s));
	offboard = 1;
	// Initialize Next Waypoint
	next_point.altitude_is_relative = true;
	next_point.altitude = home.alt + 7;
	next_point.autocontinue = true;
	next_point.nav_cmd = NAV_CMD_TAKEOFF;
	next_point.acceptance_radius = 5;
	next_point.lat = home.lat;
	next_point.lon = home.lon;
	next_point.time_inside = 0;
	next_point.frame = last.frame;
	mission_pub = orb_advertise(ORB_ID(offboard_mission), &search_mission);
}

Search::Search(const Search &orig) {
}

Search::~Search() {
}

void Search::SetMission(double search_radius) {
	// Get the amount of waypoints needed for mission.
	// Note: mission should already have a size of 1. For endpoint.
	// Note: we won't need to manage the home waypoint ourselves.
	// Note: We assume  the last waypoint is the upper left corner of the square search pattern.
	// Note: Only test on the field where the home position is correctly set
	// Home position is not part of mission, Index 0 = First new waypoint

	/* Waypoint ID 
	 * Waypoint Count = Waypoint ID + 1*/
	dm_clear(DM_KEY_WAYPOINTS_OFFBOARD(offboard));
	offboard = 0;
	int wp_id = 0;
	// Note: Acceptance Radius not accounted for (it's in meters, not degrees))
	while (next_point.lat - last.lat > search_radius && next_point.lon - last.lon > search_radius) {
		if (wp_id % 2 == 0) {
			// Do Latitudinal shifts here
			if (next_point.lat - last.lat < search_radius) {
				// Going Up
				next_point.lat = last.lat;
			} else {
				// Going down
				next_point.lat = home.lat;
			}
		} else {
			// Do longitudinal shifts here
			next_point.lon += -1 * search_radius;
		}
		if (wp_id)
			next_point.nav_cmd = NAV_CMD_WAYPOINT;
		dm_write(DM_KEY_WAYPOINTS_OFFBOARD(offboard), wp_id++, DM_PERSIST_POWER_ON_RESET, &next_point, sizeof(struct mission_item_s));
		offboard = (offboard == 0) ? 1 : 0;
	}
	search_mission.count = wp_id + 1;
	warnx("Waypoint count is: %d", search_mission.count);
	search_mission.current_seq = 0;
	orb_publish(ORB_ID(offboard_mission), mission_pub, &search_mission);
}

void Search::SetExample() {
	// Test changing the missions waypoint count?
	search_mission.count = 2;
	search_mission.current_seq = 0;
	getCoord(15.0);
	dm_write(DM_KEY_WAYPOINTS_OFFBOARD(offboard), 0, DM_PERSIST_POWER_ON_RESET, &next_point, sizeof(struct mission_item_s));
	offboard = (offboard == 0) ? 1 : 0;
	getCoord(15.0, 90);
	next_point.nav_cmd = NAV_CMD_WAYPOINT;
	dm_write(DM_KEY_WAYPOINTS_OFFBOARD(offboard), 1, DM_PERSIST_POWER_ON_RESET, &next_point, sizeof(struct mission_item_s));
	offboard = (offboard == 0) ? 1 : 0;
	orb_publish(ORB_ID(offboard_mission), mission_pub, &search_mission);
}

void Search::ClearAll() {
	warnx("Clearing Onboard Waypoints!");
	dm_clear(DM_KEY_WAYPOINTS_ONBOARD);
	dm_clear(DM_KEY_WAYPOINTS_OFFBOARD_0);
	offboard = (offboard == 0) ? 1 : 0;
	/*dm_clear(DM_KEY_WAYPOINTS_OFFBOARD_0);
	dm_clear(DM_KEY_WAYPOINTS_OFFBOARD_1);*/
	warnx("Finished clearing all Waypoints!");
}

void Search::printDebug() {
	int mission_sub = orb_subscribe(ORB_ID(offboard_mission));
	orb_copy(ORB_ID(offboard_mission), mission_sub, &true_mission);
	orb_unsubscribe(mission_sub);
	// Timing for Navigator message to display properly
	usleep(20000);
	warnx("Mission count: %d", true_mission.count);
	warnx("Mission current sequence: %d", search_mission.current_seq);
	warnx("Start Point Latitude: %.7f", next_point.lat);
	warnx("Start Point Longitude: %.7f", next_point.lon);
	warnx("Last Point Frame is: %d", last.frame);
	warnx("Home Latitude: %.7f", home.lat);
	warnx("Home Longitude: %.7f", home.lon);
	if ((home.lat - next_point.lat) < 0.0001 && (home.lon - next_point.lon) < 0.0001)
		warnx("Start Point correctly copied Home's Lat/Lon!");
	else
		warnx("Start Point did not copy Home's Lat/Lon");
	warnx("========================================================");
	warnx("Last Waypoint Latitude: %.7f", last.lat);
	warnx("Last Waypoint Longitude: %.7f", last.lon);
}

void Search::getCoord(double disp, double heading) {
	/*
	Inputs: (pass by reference)
	 *  phi     = current latitude [deg]
	 *  lam     = current longitude [deg]
	 *  disp    = desired displacement distance [meters]
	 *  heading = desired direction of displacement [deg]
	 *              Pass in values for displacements in N, S, E, W
	 *              0   - displacement N
	 *              90  - displacement E
	 *              180 - displacement S
	 *              270 - displacement W
	 * 
	Outputs:
	 *  phi and lam will be changed to reflect new long and lat location after displacement in specified heading direction
	 */
	//FCC cites that Flat earth approximation breaks down past 475km (CFR 73.208) but autonomous mission shouldnt span more than 100 km range
	if (disp > 100000) {
		return;
	}

	//declare parameters to store linear distance per degree of longitude and latitude
	double Lphi, Llam;
	double phi = next_point.lat;

	// PHI = degrees of LATITUDE
	// LAM = degrees of LONGITUDE

	//uncomment section below for speed at cost of accuracy
	/*
	// <TODO: Implement smaller ranges of phi values to improve accuracy of estimation>
	// current accuracy within +/- 0.3 meters (~1ft)
	//    if (33.5 <= phi && phi < 34.5) {
	//        Lphi = 110922; // [m]
	//        Llam = 92385;
	//
	//    } else if (34.5 <= phi && phi < 35.5) {
	//        Lphi = 110941; // [m]
	//        Llam = 91288.4;
	//
	//    } else if (35.5 <= phi && phi < 36.5) {
	//        Lphi = 110959; // [m]
	//        Llam = 90163.9;
	//
	//    } else {
	// default case if not within a predetermined latitude angle        
	//        Lphi = 111132.92 - 559.82 * cosd((2 * phi)) + 1.175 * cosd((4 * phi))
	//                - 0.0023 * cosd((6 * phi));
	//        Llam = 111412.84 * cosd(phi) - 93.5 * cos((3 * phi)) - 0.118 * cosd((5 * phi));
	//    }
	 */

	//distance per degree of lat and long calculated each time
	Lphi = 111132.92 - 559.82 * cosd((2 * phi)) + 1.175 * cosd((4 * phi))
		- 0.0023 * cosd((6 * phi));
	Llam = 111412.84 * cosd(phi) - 93.5 * cos((3 * phi)) - 0.118 * cosd((5 * phi));

	next_point.lat += disp * cosd(heading) / Lphi; //[deg]
	next_point.lon += disp * sind(heading) / Llam; //[deg]
}