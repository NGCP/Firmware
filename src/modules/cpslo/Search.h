 #ifndef SEARCH_H
#define SEARCH_H

// PX4 Library Imports
#include <uORB/topics/mission.h>
#include <navigator/navigation.h>
#include <uORB/topics/home_position.h>

class Search {
public:
	// Public Members
	Search();
	void printDebug();
	Search(const Search &orig);
	virtual ~Search();
	/* Note: Search Radius should be entered in degrees */
	void SetMission(double search_radius);
	void SetExample();
	void ClearAll();
	
private:
	// Private Members
	struct mission_s search_mission;
	struct mission_item_s next_point;
	int offboard = 0;
	orb_advert_t mission_pub;
	// Debug Members
	struct home_position_s home;
	struct mission_item_s last;
	struct mission_s true_mission;
	void getCoord(double disp, double heading = 0);
};

#endif

