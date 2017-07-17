#include "McRave.h"

Unit TargetTrackerClass::getTarget(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Protoss_Observer || unit.getType() == UnitTypes::Protoss_Shuttle)
	{
		return nullptr;
	}
	else if (unit.getType() == UnitTypes::Terran_Medic)
	{
		return allyTarget(unit);
	}
	else
	{
		return enemyTarget(unit);
	}
	return nullptr;
}

Unit TargetTrackerClass::enemyTarget(UnitInfo& unit)
{
	double highest = 0.0, thisUnit = 0.0;
	Unit target = nullptr;
	Position targetPosition;

	for (auto &e : Units().getEnUnits())
	{
		UnitInfo enemy = e.second;
		if (!enemy.unit())
		{
			continue;
		}

		// If unit is dead or unattackable
		if (enemy.getDeadFrame() > 0 || (enemy.getType().isFlyer() && unit.getAirRange() == 0) || (!enemy.getType().isFlyer() && unit.getGroundRange() == 0))
		{
			continue;
		}

		if (unit.getType().isFlyer())
		{
			thisUnit = enemy.getPriority() / (1.0 + (unit.getPosition().getDistance(enemy.getPosition())));
		}
		else
		{
			// If the unit has higher range and is faster
			if (enemy.getType() == UnitTypes::Terran_Vulture && unit.getGroundRange() < enemy.getGroundRange())
			{
				continue;
			}

			// If the enemy is stasised, ignore it
			if (enemy.unit()->exists() && enemy.unit()->isStasised())
			{
				continue;
			}

			// If the unit is invis and undetected, ignore it
			if (enemy.unit()->exists() && (enemy.unit()->isCloaked() || enemy.unit()->isBurrowed()) && !enemy.unit()->isDetected())
			{
				continue;
			}

			double distance = pow(1.0 + unit.getPosition().getDistance(enemy.getPosition()), 2.0);
			double threat = Grids().getEGroundDistanceGrid(enemy.getWalkPosition());

			// Reavers and Tanks target highest priority units with clusters around them
			if (unit.getType() == UnitTypes::Protoss_Reaver || unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
			{
				thisUnit = (enemy.getPriority() * Grids().getEGroundCluster(enemy.getWalkPosition())) / distance;
			}

			// Arbiters only target tanks
			else if (unit.getType() == UnitTypes::Protoss_Arbiter)
			{
				if (enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || enemy.getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode)
				{
					thisUnit = (enemy.getPriority() * Grids().getStasisCluster(enemy.getWalkPosition())) / distance;
				}
			}

			// High Templars target the highest priority with the largest cluster
			else if (unit.getType() == UnitTypes::Protoss_High_Templar)
			{
				if (Grids().getACluster(enemy.getWalkPosition()) < Grids().getEAirCluster(enemy.getWalkPosition()) + Grids().getEGroundCluster(enemy.getWalkPosition()) && !enemy.getType().isBuilding())
				{
					thisUnit = (enemy.getPriority() * Grids().getEGroundCluster(enemy.getWalkPosition()) * Grids().getEAirCluster(enemy.getWalkPosition())) / distance;
				}
			}

			// All other units target highest priority units with slight emphasis on lower health units
			else
			{
				thisUnit = (enemy.getPriority() * (1 + 0.1 *(1 - enemy.getPercentHealth))) / distance;
			}

			// If the unit doesn't exist, it's not a suitable target usually (could be removed?)
			if (!enemy.unit()->exists())
			{
				thisUnit = thisUnit * 0.1;
			}
		}

		// If this is the strongest enemy around, target it
		if (thisUnit > highest || highest == 0.0)
		{
			target = enemy.unit();
			highest = thisUnit;
			targetPosition = enemy.getPosition();
		}
	}
	if (target)
	{
		unit.setTargetPosition(targetPosition);
	}
	return target;
}

Unit TargetTrackerClass::allyTarget(UnitInfo& unit)
{
	double highest = 0.0;
	Unit target = nullptr;
	Position targetPosition;

	// Search for an ally target that needs healing for medics
	for (auto &a : Units().getMyUnits())
	{
		UnitInfo ally = a.second;
		if (!ally.unit() || ally.getDeadFrame() != 0 || !ally.getType().isOrganic())
		{
			continue;
		}

		if (ally.unit()->isBeingHealed() && unit.getTarget() != ally.unit())
		{
			continue;
		}

		double distance = unit.getPosition().getDistance(ally.getPosition());

		if (ally.unit()->exists() && ally.unit()->getHitPoints() < ally.getType().maxHitPoints() && (distance < highest || highest == 0.0))
		{
			highest = distance;
			target = ally.unit();
			targetPosition = ally.getPosition();
		}
	}

	// If we found an ally target, store the position
	if (target)
	{
		unit.setTargetPosition(targetPosition);
	}
	return target;
}