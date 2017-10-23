#include "McRave.h"

void BaseTrackerClass::update()
{
	Display().startClock();
	updateAlliedBases();
	Display().performanceTest(__FUNCTION__);
	return;
}

void BaseTrackerClass::updateAlliedBases()
{
	for (auto &b : myBases)
	{
		BaseInfo& base = b.second;
		if (base.unit() && base.unit()->exists())
		{
			if (Grids().getBaseGrid(base.getTilePosition()) == 1 && base.unit()->isCompleted())
			{
				Grids().updateBaseGrid(base);
			}
			updateProduction(base);
		}		
	}
	return;
}

void BaseTrackerClass::updateProduction(BaseInfo& base)
{
	if (base.getType() == UnitTypes::Terran_Command_Center && !base.unit()->getAddon())
	{
		base.unit()->buildAddon(UnitTypes::Terran_Comsat_Station);
	}

	if (base.unit() && base.unit()->isIdle() && ((!Resources().isMinSaturated() || !Resources().isGasSaturated()) || BuildOrder().getCurrentBuild() == "Sparks"))
	{		
		for (auto &unit : base.getType().buildsWhat())
		{			
			if (unit.isWorker())
			{
				if (Broodwar->self()->completedUnitCount(unit) < 60 && (Broodwar->self()->minerals() >= unit.mineralPrice() + Production().getReservedMineral() + Buildings().getQueuedMineral()))
				{
					base.unit()->train(unit);
				}
			}
			else
			{
				// Zerg production
			}
		}
	}
	return;
}

void BaseTrackerClass::storeBase(Unit base)
{
	BaseInfo& b = myBases[base];
	b.setUnit(base);
	b.setType(base->getType());
	b.setResourcesPosition(TilePosition(Resources().resourceClusterCenter(base)));
	b.setPosition(base->getPosition());
	b.setWalkPosition(Util().getWalkPosition(base));
	b.setTilePosition(base->getTilePosition());
	b.setPosition(base->getPosition());
	myOrderedBases[base->getPosition().getDistance(Terrain().getPlayerStartingPosition())] = base->getTilePosition();
	Terrain().getAllyTerritory().insert(theMap.GetArea(b.getTilePosition())->Id());
	Grids().updateBaseGrid(b);
	return;
}

void BaseTrackerClass::removeBase(Unit base)
{
	Terrain().getAllyTerritory().erase(theMap.GetArea(base->getTilePosition())->Id());
	Grids().updateBaseGrid(myBases[base]);
	myOrderedBases.erase(base->getPosition().getDistance(Terrain().getPlayerStartingPosition()));
	myBases.erase(base);
	return;
}