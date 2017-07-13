#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "WorkerInfo.h"

using namespace BWAPI;
using namespace std;

class WorkerTrackerClass
{
	map <Unit, WorkerInfo> myWorkers;
	map <WalkPosition, int> recentExplorations;
	Unit scout;
	bool scouting = true;
	int deadScoutFrame = 0;
public:

	bool isScouting() { return scouting; }
	map <Unit, WorkerInfo>& getMyWorkers() { return myWorkers; }
	Unit getScout() { return scout; }
	
	void update();
	void updateScout();
	void updateWorkers();
	void updateSituational(WorkerInfo&);
	void updateGathering(WorkerInfo&);
	void assignWorker(WorkerInfo&);
	void reAssignWorker(WorkerInfo&);
	void exploreArea(WorkerInfo&);

	void storeWorker(Unit);
	void removeWorker(Unit);


	Unit getClosestWorker(Position);
};

typedef Singleton<WorkerTrackerClass> WorkerTracker;
