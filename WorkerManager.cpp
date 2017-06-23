#include "McRave.h"

void WorkerTrackerClass::update()
{
	for (auto &worker : myWorkers)
	{
		updateScout(worker.second);
		updateDecision(worker.second);
	}
}

void WorkerTrackerClass::updateScout(WorkerInfo& worker)
{
	if (!scouting)
	{
		return;
	}

	// Crappy scouting method
	if (!scout)
	{
		scout = worker.unit();
	}
	if (worker.unit() == scout)
	{
		if (Terrain().getEnemyBasePositions().size() == 0 && Units().getSupply() >= 18 && scouting)
		{
			for (auto start : getStartLocations())
			{
				if (Broodwar->isExplored(start->getTilePosition()) == false)
				{
					if (worker.unit()->getOrderTargetPosition() != start->getPosition())
					{
						worker.unit()->move(start->getPosition());
					}
					break;
				}
			}
		}
		if (Terrain().getEnemyBasePositions().size() > 0)
		{
			exploreArea(worker.unit());
		}
	}

}

void WorkerTrackerClass::exploreArea(Unit worker)
{
	WalkPosition start = myWorkers[worker].getWalkPosition();
	Position bestPosition = Terrain().getEnemyStartingPosition();
	double closestD = 1000;
	recentExplorations[start] = Broodwar->getFrameCount();

	for (int x = start.x - 10; x < start.x + 10 + worker->getType().tileWidth(); x++)
	{
		for (int y = start.y - 10; y < start.y + 10 + worker->getType().tileHeight(); y++)
		{
			if (Grids().getAntiMobilityGrid(x, y) == 0 && Grids().getMobilityGrid(x, y) > 0 && Grids().getEGroundDistanceGrid(x, y) == 0.0 && WalkPosition(x, y).isValid() && Broodwar->getFrameCount() - recentExplorations[WalkPosition(x, y)] > 500 && Position(WalkPosition(x, y)).getDistance(Terrain().getEnemyStartingPosition()) < 320)
			{
				if (Position(WalkPosition(x, y)).getDistance(Terrain().getEnemyStartingPosition()) < closestD)
				{
					bestPosition = Position(WalkPosition(x, y));
					closestD = Position(WalkPosition(x, y)).getDistance(Terrain().getEnemyStartingPosition());
				}
			}
		}
	}
	if (bestPosition.isValid())
	{
		worker->move(bestPosition);
		Broodwar->drawLineMap(worker->getPosition(), bestPosition, Colors::Red);
	}
	return;
}

void WorkerTrackerClass::updateDecision(WorkerInfo& worker)
{
	// Reassign workers if we need gas
	if (Resources().getGasNeeded() > 0 && !Strategy().isRush())
	{
		reAssignWorker(worker.unit());
		Resources().setGasNeeded(Resources().getGasNeeded() - 1);
	}

	// If worker has no shields left and on a shield battery grid
	if (worker.unit()->getShields() <= 5 && Grids().getBatteryGrid(worker.unit()->getTilePosition()) > 0)
	{
		if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Right_Click_Unit && worker.unit()->getClosestUnit(Filter::IsAlly && Filter::IsCompleted  && Filter::GetType == UnitTypes::Protoss_Shield_Battery && Filter::Energy > 10))
		{
			worker.unit()->rightClick(worker.unit()->getClosestUnit(Filter::IsAlly && Filter::GetType == UnitTypes::Protoss_Shield_Battery));
			return;
		}		
	}

	// If no valid target, try to get a new one
	if (!worker.getResource())
	{
		assignWorker(worker.unit());
		// Any idle workers can gather closest mineral field until they are assigned again
		if (worker.unit()->isIdle() && worker.unit()->getClosestUnit(Filter::IsMineralField))
		{
			worker.unit()->gather(worker.unit()->getClosestUnit(Filter::IsMineralField));
			worker.setTarget(nullptr);
			return;
		}
		return;
	}

	// Attack units in mineral line
	if (Broodwar->getFrameCount() - worker.getLastGatherFrame() <= 25 && Grids().getEGroundDistanceGrid(worker.getWalkPosition()) > 0)
	{
		if (!worker.getTarget() || (worker.getTarget() && !worker.getTarget()->exists()))
		{
			worker.setTarget(worker.unit()->getClosestUnit(Filter::IsEnemy && !Filter::IsFlying, 320));
		}
		else if (worker.getTarget()->exists() && (Grids().getResourceGrid(worker.getTarget()->getTilePosition().x, worker.getTarget()->getTilePosition().y) > 0/* || (!worker.getTarget()->getType().isWorker() && Grids().getBaseGrid(worker.getTarget()->getTilePosition().x, worker.getTarget()->getTilePosition().y) > 0)*/))
		{
			if ((worker.unit()->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && worker.unit()->getLastCommand().getTarget() && !worker.unit()->getLastCommand().getTarget()->exists()) || (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Attack_Unit))
			{
				worker.unit()->attack(worker.getTarget());
			}
			return;
		}
	}

	// Else command worker
	if (worker.unit() && worker.unit()->exists())
	{
		// Draw on every frame
		if (worker.unit() && worker.getResource())
		{
			Broodwar->drawLineMap(worker.unit()->getPosition(), Resources().getMyMinerals()[worker.getResource()].getPosition(), Broodwar->self()->getColor());
		}

		// If we have been given a command this frame already, return
		if (!worker.unit()->isIdle() && (worker.unit()->getLastCommand().getType() == UnitCommandTypes::Move || worker.unit()->getLastCommand().getType() == UnitCommandTypes::Build))
		{
			return;
		}

		// If idle and carrying gas or minerals, return cargo			
		if (worker.unit()->isCarryingGas() || worker.unit()->isCarryingMinerals())
		{
			if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo)
			{
				worker.unit()->returnCargo();
			}
			return;
		}

		// If not scouting and there's boulders to remove	
		if (!scouting && Resources().getMyBoulders().size() > 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus) >= 2)
		{
			for (auto b : Resources().getMyBoulders())
			{
				if (b.first && b.first->exists() && !worker.unit()->isGatheringMinerals() && worker.unit()->getDistance(b.second.getPosition()) < 512)
				{
					worker.unit()->gather(b.first);
					break;
				}
			}
		}

		// If not targeting the mineral field the worker is mapped to
		if (!worker.unit()->isCarryingGas() && !worker.unit()->isCarryingMinerals())
		{
			if ((worker.unit()->isGatheringMinerals() || worker.unit()->isGatheringGas()) && worker.unit()->getTarget() == worker.getResource())
			{				
				worker.setLastGatherFrame(Broodwar->getFrameCount());
				return;
			}
			// If the workers current target has a resource count of 0 (mineral blocking a ramp), let worker continue mining it
			if (worker.unit()->getTarget() && worker.unit()->getTarget()->getType().isMineralField() && worker.unit()->getTarget()->getResources() == 0)
			{
				return;
			}
			// If the mineral field is in vision and no target, force to gather from the assigned mineral field
			if (worker.getResource())
			{
				if (worker.getResource()->exists())
				{
					worker.unit()->gather(worker.getResource());
					worker.setLastGatherFrame(Broodwar->getFrameCount());
					return;
				}
				else
				{
					worker.unit()->move(Resources().getMyMinerals()[worker.getResource()].getPosition());
					return;
				}
			}
		}
	}
}

Unit WorkerTrackerClass::getClosestWorker(Position here)
{
	// Currently gets the closest worker that doesn't mine gas
	Unit closestWorker = nullptr;
	double closestD = 0.0;
	for (auto worker : myWorkers)
	{
		if (worker.second.getResource() && worker.second.getResource()->exists() && !worker.second.getResource()->getType().isMineralField())
		{
			continue;
		}
		if (worker.first->getLastCommand().getType() == UnitCommandTypes::Move || worker.first->getLastCommand().getType() == UnitCommandTypes::Build)
		{
			continue;
		}
		if (worker.first->getLastCommand().getType() == UnitCommandTypes::Gather && worker.first->getLastCommand().getTarget()->exists() && worker.first->getLastCommand().getTarget()->getInitialResources() == 0)
		{
			continue;
		}
		if (worker.first->getDistance(here) < closestD || closestD == 0.0)
		{
			closestWorker = worker.first;
			closestD = worker.first->getDistance(here);
		}
	}
	return closestWorker;
}

void WorkerTrackerClass::storeWorker(Unit unit)
{
	if (unit->exists() && unit->isCompleted())
	{
		myWorkers[unit].setUnit(unit);
		myWorkers[unit].setPosition(unit->getPosition());
		myWorkers[unit].setWalkPosition(Util().getWalkPosition(unit));
		myWorkers[unit].setTilePosition(unit->getTilePosition());
	}
	return;
}

void WorkerTrackerClass::removeWorker(Unit worker)
{
	if (Resources().getMyGas().find(myWorkers[worker].getResource()) != Resources().getMyGas().end())
	{
		Resources().getMyGas()[myWorkers[worker].getResource()].setGathererCount(Resources().getMyGas()[myWorkers[worker].getResource()].getGathererCount() - 1);
	}
	if (Resources().getMyMinerals().find(myWorkers[worker].getResource()) != Resources().getMyMinerals().end())
	{
		Resources().getMyMinerals()[myWorkers[worker].getResource()].setGathererCount(Resources().getMyMinerals()[myWorkers[worker].getResource()].getGathererCount() - 1);
	}
	myWorkers.erase(worker);

	// If scouting worker died, assume where enemy is based on death location
	if (worker == scout && isScouting())
	{
		scout = nullptr;
		scouting = false;
		if (Terrain().getEnemyBasePositions().size() == 0)
		{
			double closestD = 0.0;
			BaseLocation* closestB = getNearestBaseLocation(worker->getTilePosition());
			for (auto base : getStartLocations())
			{
				if (base == getStartLocation(Broodwar->self()))
				{
					continue;
				}
				if (worker->getDistance(base->getPosition()) < closestD || closestD == 0.0)
				{
					closestB = base;
				}
			}
			Terrain().getEnemyBasePositions().emplace(closestB->getPosition());
		}
	}
}

void WorkerTrackerClass::assignWorker(Unit worker)
{
	// Assign a task if none
	int cnt = 1;

	if (!Strategy().isRush() || Resources().isMinSaturated())
	{
		for (auto &gas : Resources().getMyGas())
		{
			if (gas.second.getUnitType() == UnitTypes::Protoss_Assimilator && gas.first->isCompleted() && gas.second.getGathererCount() < 3)
			{
				gas.second.setGathererCount(gas.second.getGathererCount() + 1);
				myWorkers[worker].setResource(gas.first);				
				return;
			}
		}
	}

	// First checks if a mineral field has 0 workers mining, if none, checks if a mineral field has 1 worker mining. Assigns to 0 first, then 1. Spreads saturation.
	while (cnt <= 2)
	{
		for (auto &mineral : Resources().getMyMinerals())
		{
			if (mineral.second.getGathererCount() < cnt)
			{
				mineral.second.setGathererCount(mineral.second.getGathererCount() + 1);
				myWorkers[worker].setResource(mineral.first);				
				return;
			}
		}
		cnt++;
	}
	return;
}

void WorkerTrackerClass::reAssignWorker(Unit worker)
{
	if (Resources().getMyGas().find(myWorkers[worker].getResource()) != Resources().getMyGas().end())
	{
		Resources().getMyGas()[myWorkers[worker].getResource()].setGathererCount(Resources().getMyGas()[myWorkers[worker].getResource()].getGathererCount() - 1);
	}
	if (Resources().getMyMinerals().find(myWorkers[worker].getResource()) != Resources().getMyMinerals().end())
	{
		Resources().getMyMinerals()[myWorkers[worker].getResource()].setGathererCount(Resources().getMyMinerals()[myWorkers[worker].getResource()].getGathererCount() - 1);
	}
	assignWorker(worker);
}