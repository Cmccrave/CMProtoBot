#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

class WorkerInfo{
	int lastGatherFrame;
	Unit thisUnit, target, resource;
	UnitType unitType, buildingType;
	WalkPosition walkPosition;
	Position position, resourcePosition;
	TilePosition tilePosition, buildPosition;
public:
	WorkerInfo();

	int getLastGatherFrame() { return lastGatherFrame; }

	Unit unit() { return thisUnit; }
	Unit getTarget() { return target; }
	Unit getResource() { return resource; }
	UnitType getType() { return unitType; }
	UnitType getBuildingType() { return buildingType; }

	Position getPosition() { return position; }
	Position getResourcePosition() { return resourcePosition; }
	WalkPosition getWalkPosition() { return walkPosition; }
	TilePosition getTilePosition() { return tilePosition; }
	TilePosition getBuildPosition() { return buildPosition; }

	void setLastGatherFrame(int newFrame) { lastGatherFrame = newFrame; }

	void setUnit(Unit newUnit) { thisUnit = newUnit; }
	void setTarget(Unit newTarget) { target = newTarget; }
	void setResource(Unit newResource) { resource = newResource; }
	void setType(UnitType newType) { unitType = newType; }
	void setBuildingType(UnitType newBuildingType) { buildingType = newBuildingType; }

	void setPosition(Position newPosition) { position = newPosition; }
	void setResourcePosition(Position newPosition) { resourcePosition = newPosition; }
	void setWalkPosition(WalkPosition newPosition) { walkPosition = newPosition; }
	void setTilePosition(TilePosition newPosition) { tilePosition = newPosition; }
	void setBuildPosition(TilePosition newPosition) { buildPosition = newPosition; }
};