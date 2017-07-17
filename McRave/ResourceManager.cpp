#include "McRave.h"

void ResourceTrackerClass::update()
{
	Display().startClock();
	updateResources();
	Display().performanceTest(__FUNCTION__);
	return;
}

void ResourceTrackerClass::updateResources()
{
	// Assume mineral saturation, will be changed to false if any mineral field has less than 2 gatherers
	minSat = true;
	for (auto &m : myMinerals)
	{
		ResourceInfo& resource = m.second;
		if (resource.unit()->exists())
		{
			resource.setRemainingResources(resource.unit()->getResources());
		}
		if (minSat && resource.getGathererCount() < 2)
		{
			minSat = false;
		}
	}

	// Assume gas saturation, will be changed to false if any gas geyser has less than 3 gatherers
	gasSat = true;
	for (auto &g : myGas)
	{
		if (g.first->exists())
		{
			g.second.setUnitType(g.first->getType());
			g.second.setRemainingResources(g.first->getResources());
		}
		if (g.second.getGathererCount() < 3 && g.second.getType() != UnitTypes::Resource_Vespene_Geyser && g.second.unit()->isCompleted())
		{
			gasNeeded = 3 - g.second.getGathererCount();
			gasSat = false;
		}
	}
	return;
}

void ResourceTrackerClass::storeResource(Unit resource)
{
	if (resource->getInitialResources() > 0)
	{
		if (resource->getType().isMineralField())
		{
			storeMineral(resource);
		}
		else
		{
			storeGas(resource);
		}
	}
	else
	{
		storeBoulder(resource);
	}
}

void ResourceTrackerClass::storeMineral(Unit resource)
{
	ResourceInfo& m = myMinerals[resource];
	m.setGathererCount(0);
	m.setRemainingResources(resource->getResources());
	m.setUnit(resource);
	m.setResourceClusterPosition(resourceClusterCenter(resource));
	m.setClosestBasePosition(Terrain().getClosestBaseCenter(resource));
	m.setUnitType(resource->getType());
	m.setPosition(resource->getPosition());
	m.setWalkPosition(Util().getWalkPosition(resource));
	m.setTilePosition(resource->getTilePosition());
	Grids().updateResourceGrid(m);
	return;
}

void ResourceTrackerClass::storeGas(Unit resource)
{
	ResourceInfo& g = myGas[resource];
	g.setGathererCount(0);
	g.setRemainingResources(resource->getResources());
	g.setUnit(resource);
	g.setResourceClusterPosition(resourceClusterCenter(resource));
	g.setClosestBasePosition(Terrain().getClosestBaseCenter(resource));
	g.setUnitType(resource->getType());
	g.setPosition(resource->getPosition());
	g.setWalkPosition(Util().getWalkPosition(resource));
	g.setTilePosition(resource->getTilePosition());
	Grids().updateResourceGrid(g);
	return;
}

void ResourceTrackerClass::storeBoulder(Unit resource)
{
	ResourceInfo& b = myBoulders[resource];
	b.setGathererCount(0);
	b.setRemainingResources(resource->getResources());
	b.setUnit(resource);
	b.setUnitType(resource->getType());
	b.setPosition(resource->getPosition());
	b.setWalkPosition(Util().getWalkPosition(resource));
	b.setTilePosition(resource->getTilePosition());
	return;
}

void ResourceTrackerClass::removeResource(Unit resource)
{
	// Remove dead resources
	if (myMinerals.find(resource) != myMinerals.end())
	{
		Grids().updateResourceGrid(myMinerals[resource]);
		myMinerals.erase(resource);

	}
	else if (myGas.find(resource) != myGas.end())
	{
		Grids().updateResourceGrid(myGas[resource]);
		myGas.erase(resource);
	}
	else if (myBoulders.find(resource) != myBoulders.end())
	{
		Grids().updateResourceGrid(myBoulders[resource]);
		myBoulders.erase(resource);
	}

	// Any workers that targeted that resource now have no target
	for (auto &worker : Workers().getMyWorkers())
	{
		if (worker.second.getResource() == resource)
		{
			worker.second.setResource(nullptr);
		}
	}
	return;
}

Position ResourceTrackerClass::resourceClusterCenter(Unit resource)
{
	// Get average of minerals	
	int avgX = 0, avgY = 0, size = 0;
	for (auto &m : Broodwar->getUnitsInRadius(resource->getPosition(), 320, Filter::IsMineralField))
	{
		avgX = avgX + m->getPosition().x;
		avgY = avgY + m->getPosition().y;
		size++;
	}
	Position base = Terrain().getClosestBaseCenter(resource);

	if (size == 0 || !base.isValid())
	{
		return Positions::None;
	}

	avgX = avgX / size;
	avgY = avgY / size;

	return (Position(avgX, avgY) + base) / 2;
}
