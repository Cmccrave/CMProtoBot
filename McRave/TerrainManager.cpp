#include "McRave.h"

void TerrainTrackerClass::update()
{
	Display().startClock();
	updateAreas();
	updateChokes();
	Display().performanceTest(__FUNCTION__);
	return;
}

void TerrainTrackerClass::updateAreas()
{
	// If we see a building, check for closest starting location
	if (enemyBasePositions.size() <= 0)
	{
		for (auto &unit : Units().getEnUnits())
		{
			if (unit.second.getType().isBuilding() && Terrain().getEnemyBasePositions().size() == 0 && unit.second.getPosition().getDistance(Terrain().getPlayerStartingPosition()) > 1600)
			{
				double distance = 0.0;
				TilePosition closest;
				for (auto &base : theMap.StartingLocations())
				{
					if (unit.second.getPosition().getDistance(Position(base)) < distance || distance == 0.0)
					{
						distance = unit.second.getPosition().getDistance(Position(base));
						closest = base;
					}
				}
				if (closest.isValid())
				{
					enemyBasePositions.emplace(Position(closest));
					enemyStartingTilePosition = closest;
					enemyStartingPosition = Position(closest);
					path = theMap.GetPath(playerStartingPosition, enemyStartingPosition);
				}
			}
		}
	}

	for (auto &base : enemyBasePositions)
	{
		if (base.isValid() && Broodwar->isVisible(TilePosition(base)) && Broodwar->getUnitsInRadius(base, 128, Filter::IsEnemy).size() == 0)
		{
			enemyBasePositions.erase(base);
			break;
		}
	}
}

void TerrainTrackerClass::updateChokes()
{
	// Store non island bases	
	for (auto &area : theMap.Areas())
	{
		if (area.AccessibleNeighbours().size() > 0)
		{
			for (auto &base : area.Bases())
			{
				allBaseLocations.emplace(base.Location());
			}
		}
	}

	// Start location
	playerStartingTilePosition = Broodwar->self()->getStartLocation();
	playerStartingPosition = Position(playerStartingTilePosition);

	// Establish FFE position	
	if (Broodwar->getFrameCount() > 100)
	{
		int x = 0;
		int y = 0;
		const Area* closestA;
		double closestBaseDistance = 0.0, furthestChokeDistance = 0.0, closestChokeDistance = 0.0;
		TilePosition natural;
		for (auto &area : theMap.Areas())
		{
			for (auto &base : area.Bases())
			{
				if (base.Geysers().size() == 0 || area.AccessibleNeighbours().size() == 0)
				{
					continue;
				}

				if (Grids().getDistanceHome(WalkPosition(base.Location())) > 50 && (Grids().getDistanceHome(WalkPosition(base.Location())) < closestBaseDistance || closestBaseDistance == 0))
				{
					closestBaseDistance = Grids().getDistanceHome(WalkPosition(base.Location()));
					closestA = base.GetArea();
					natural = base.Location();
				}
			}
		}
		if (closestA)
		{
			double largest = 0.0;
			for (auto &choke : closestA->ChokePoints())
			{				
				if (choke && (Grids().getDistanceHome(choke->Center()) < closestChokeDistance || closestChokeDistance == 0.0))
				{
					firstChoke = TilePosition(choke->Center());
					closestChokeDistance = Grids().getDistanceHome(choke->Center());
				}				
			}

			for (auto &choke : closestA->ChokePoints())
			{
				if (choke && TilePosition(choke->Center()) != firstChoke && (Position(choke->Center()).getDistance(playerStartingPosition) / choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2)) < furthestChokeDistance || furthestChokeDistance == 0))
				{
					secondChoke = TilePosition(choke->Center());					
					furthestChokeDistance = Position(choke->Center()).getDistance(playerStartingPosition) / choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2));
				}
			}
			FFEPosition = TilePosition(int(secondChoke.x*0.35 + natural.x*0.65), int(secondChoke.y*0.35 + natural.y*0.65));
		}
	}

	Broodwar->drawCircleMap(Position(secondChoke), 32, Colors::Red);
	Broodwar->drawCircleMap(Position(firstChoke), 32, Colors::Blue);
}

void TerrainTrackerClass::onStart()
{
		
}

void TerrainTrackerClass::removeTerritory(Unit base)
{
	if (base)
	{
		if (enemyBasePositions.find(base->getPosition()) != enemyBasePositions.end())
		{
			enemyBasePositions.erase(base->getPosition());
		}

		if (theMap.GetArea(base->getTilePosition()))
		{
			if (allyTerritory.find(theMap.GetArea(base->getTilePosition())->Id()) != allyTerritory.end())
			{
				allyTerritory.erase(theMap.GetArea(base->getTilePosition())->Id());
			}
		}
	}
}

bool TerrainTrackerClass::isInAllyTerritory(Unit unit)
{
	if (unit && unit->exists() && unit->getTilePosition().isValid() && theMap.GetArea(unit->getTilePosition()))
	{
		if (allyTerritory.find(theMap.GetArea(unit->getTilePosition())->Id()) != allyTerritory.end())
		{
			return true;
		}
	}
	return false;
}

Position TerrainTrackerClass::getClosestEnemyBase(Position here)
{
	double closestD = 0.0;
	Position closestP;
	for (auto &base : Terrain().getEnemyBasePositions())
	{
		if (here.getDistance(base) < closestD || closestD == 0.0)
		{
			closestP = base;
			closestD = here.getDistance(base);
		}
	}
	return closestP;
}

Position TerrainTrackerClass::getClosestAllyBase(Position here)
{

}