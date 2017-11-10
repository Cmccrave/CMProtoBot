#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "BuildingInfo.h"

using namespace BWAPI;
using namespace std;

class BuildingTrackerClass
{
	int queuedMineral, queuedGas;
	map <TilePosition, UnitType> buildingsQueued;
	map <Unit, BuildingInfo> myBuildings;
	set<Unit> returnValues;
	TilePosition currentExpansion;
	UnitType lastBadBuilding;
	int errorTime = 0, buildingOffset = 0;
public:
	map <Unit, BuildingInfo>& getMyBuildings() { return myBuildings; }
	map <TilePosition, UnitType>& getBuildingsQueued() { return buildingsQueued; }
	TilePosition getBuildLocation(UnitType);
	TilePosition getBuildLocationNear(UnitType, TilePosition, bool ignoreCond = false);

	bool isBuildable(UnitType, TilePosition);
	bool isSuitable(UnitType, TilePosition);
	bool isQueueable(UnitType, TilePosition);

	// Returns the minerals that are reserved for queued buildings
	int getQueuedMineral() { return queuedMineral; }

	// Returns the gas that is reserved for queued buildings
	int getQueuedGas() { return queuedGas; }

	// Returns a set of ally buildings of a certain type
	set<Unit> getAllyBuildingsFilter(UnitType);
	BuildingInfo& getAllyBuilding(Unit);

	// Get current expansion tile
	TilePosition getCurrentExpansion() { return currentExpansion; }

	void update();
	void updateBuildings();
	void queueBuildings();
	void constructBuildings();
	void storeBuilding(Unit);
	void removeBuilding(Unit);
};

typedef Singleton<BuildingTrackerClass> BuildingTracker;