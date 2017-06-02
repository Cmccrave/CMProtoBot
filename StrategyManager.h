#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace BWTA;
using namespace std;
using namespace UnitTypes;

class StrategyTrackerClass
{
	map <UnitType, int> enemyComposition;
	map <UnitType, double> unitScore;
	double globalAllyStrength, globalEnemyStrength;
	bool fastExpand = false;
	bool invis = false;
	bool rush = false;
public:
	double globalAlly() { return globalAllyStrength; }
	double globalEnemy() {	return globalEnemyStrength;	}
	map <UnitType, double>& getUnitScore() { return unitScore; }
	bool isFastExpand() { return fastExpand; }
	bool setFastExpand(bool FE) { fastExpand = FE; }
	bool needDetection() { return invis; }	
	bool isRush() { return rush; }
	
	// Updating
	void update();
	void updateAlly();
	void updateEnemy();
	void updateComposition();
	void updateUnitScore(UnitType, int);	
};

typedef Singleton<StrategyTrackerClass> StrategyTracker;