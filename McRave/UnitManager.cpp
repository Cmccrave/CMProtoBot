#include "McRave.h"

void UnitTrackerClass::update()
{
	Display().startClock();
	updateUnits();
	updateGlobalSimulation();
	Display().performanceTest(__FUNCTION__);
}

void UnitTrackerClass::updateUnits()
{
	// Reset global strengths
	globalEnemyGroundStrength = 0.0;
	globalAllyGroundStrength = 0.0;
	allyDefense = 0.0;
	enemyDefense = 0.0;

	// Latch based engagement decision making based on what race we are playing	
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		if (Players().getNumberZerg() > 0 || Players().getNumberProtoss() > 0) minThreshold = 0.8, maxThreshold = 1.2;
		if (Players().getNumberTerran() > 0) minThreshold = 0.6, maxThreshold = 1.0;
		if (Players().getNumberRandom() > 0 && Broodwar->enemy()->getRace() == Races::Terran) minThreshold = 0.6, maxThreshold = 1.0;
		if (Players().getNumberRandom() > 0 && Broodwar->enemy()->getRace() != Races::Terran) minThreshold = 0.8, maxThreshold = 1.2;
	}
	else
	{
		if (Players().getNumberTerran() > 0) minThreshold = 0.8, maxThreshold = 1.2;
		if (Players().getNumberZerg() > 0 || Players().getNumberProtoss() > 0) minThreshold = 1.0, maxThreshold = 1.4;
	}

	// Update Enemy Units
	for (auto &u : enemyUnits)
	{
		UnitInfo &unit = u.second;
		if (!unit.unit()) continue; // Ignore improper storage if it happens		
		if (unit.unit()->exists())	updateEnemy(unit); // If unit is visible, update it
		if (!unit.unit()->exists() && unit.getPosition().isValid() && Broodwar->isVisible(TilePosition(unit.getPosition()))) unit.setPosition(Positions::None); // If unit is not visible but his position is, move it
		if (unit.getType().isValid()) enemyComposition[unit.getType()] += 1; // If unit has a valid type, update enemy composition tracking
		if (!unit.getType().isWorker() && !unit.getType().isBuilding()) unit.getType().isFlyer() ? globalEnemyAirStrength += unit.getVisibleAirStrength() : globalEnemyGroundStrength += unit.getVisibleGroundStrength(); // If unit is not a worker or building, add it to global strength	
		if (unit.getType().isBuilding() && unit.getGroundDamage() > 0 && unit.unit()->isCompleted()) enemyDefense += unit.getVisibleGroundStrength(); // If unit is a building and deals damage, add it to global defense		
	}

	// Update Ally Defenses
	for (auto &u : allyDefenses)
	{
		UnitInfo &unit = u.second;
		updateAlly(unit);
		allyDefense += unit.getMaxGroundStrength();
	}

	// Update Ally Units
	for (auto &u : allyUnits)
	{
		UnitInfo &unit = u.second;
		if (!unit.unit()) continue;
		updateAlly(unit);
		updateLocalSimulation(unit);
		updateStrategy(unit);

		if (!unit.getType().isBuilding()) unit.getType().isFlyer() ? globalAllyAirStrength += unit.getVisibleAirStrength() : globalAllyGroundStrength += unit.getVisibleGroundStrength();
		if (unit.getType().isWorker() && Workers().getMyWorkers().find(unit.unit()) != Workers().getMyWorkers().end()) Workers().removeWorker(unit.unit()); // Remove the worker role if needed			
		if (unit.getType().isWorker() && ((Grids().getResourceGrid(unit.getTilePosition()) == 0 || Grids().getEGroundThreat(unit.getWalkPosition()) == 0.0) || (BuildOrder().getCurrentBuild() == "Sparks" && Units().getGlobalGroundStrategy() != 1) || (Units().getGlobalEnemyGroundStrength() <= Units().getGlobalAllyGroundStrength() + Units().getAllyDefense()))) Workers().storeWorker(unit.unit()); // If this is a worker and is ready to go back to being a worker
	}
}

void UnitTrackerClass::updateLocalSimulation(UnitInfo& unit)
{
	// Variables for calculating local strengths
	double enemyLocalGroundStrength = 0.0, allyLocalGroundStrength = 0.0;
	double enemyLocalAirStrength = 0.0, allyLocalAirStrength = 0.0;
	double unitToEngage = max(0.0, unit.getPosition().getDistance(unit.getEngagePosition()) / unit.getSpeed());
	double simulationTime = unitToEngage + 5.0;
	UnitInfo &target = Units().getEnemyUnit(unit.getTarget());

	// Check every enemy unit being in range of the target
	for (auto &e : enemyUnits)
	{
		UnitInfo &enemy = e.second;
		double simRatio = 0.0;

		// Ignore workers and stasised units
		if (!enemy.unit() || enemy.getType().isWorker() || (enemy.unit() && enemy.unit()->exists() && enemy.unit()->isStasised())) continue;

		double widths = enemy.getType().tileWidth() * 16.0 + unit.getType().tileWidth() * 16.0;
		double enemyRange = widths + unit.getType().isFlyer() ? enemy.getAirRange() : enemy.getGroundRange();
		double unitRange = widths + enemy.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
		double enemyToEngage = 0.0;
		double distance = max(0.0, enemy.getPosition().getDistance(unit.getEngagePosition()) - enemyRange);

		if (enemy.getSpeed() > 0.0)
		{
			enemyToEngage = max(0.0, (distance - widths) / enemy.getSpeed());
			simRatio = max(0.0, simulationTime - enemyToEngage);
		}
		else if (distance - widths <= 0.0)
		{
			enemyToEngage = max(0.0, (unit.getPosition().getDistance(unit.getEngagePosition()) - enemyRange - widths) / unit.getSpeed());
			simRatio = max(0.0, simulationTime - enemyToEngage);
		}
		else
		{
			continue;
		}

		// Situations where an enemy should be treated as stronger than it actually is
		if (enemy.unit()->exists() && (enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && !enemy.unit()->isDetected()) simRatio = simRatio * 5.0;
		if (!enemy.getType().isFlyer() && Broodwar->getGroundHeight(enemy.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(unit.getEngagePosition()))) simRatio = simRatio * 2.0;
		if (enemy.getLastVisibleFrame() < Broodwar->getFrameCount()) simRatio = simRatio * (1.0 + ((Broodwar->getFrameCount() - enemy.getLastVisibleFrame()) / 5000));

		enemyLocalGroundStrength += enemy.getVisibleGroundStrength() * simRatio;
		enemyLocalAirStrength += enemy.getVisibleAirStrength() * simRatio;
	}

	// Check every ally being in range of the target
	for (auto &a : allyUnits)
	{
		UnitInfo &ally = a.second;
		if (ally.getPosition().getDistance(unit.getEngagePosition()) / ally.getSpeed() > simulationTime) continue;
		double distance = ally.getPosition().getDistance(ally.getEngagePosition());
		double speed = (unit.getTransport() && unit.getTransport()->exists()) ? unit.getTransport()->getType().topSpeed() * 24.0 : ally.getSpeed();
		double allyToEngage = max(0.0, distance / speed);
		double simRatio = max(0.0, simulationTime - allyToEngage);

		// Situations where an ally should be treated as stronger than it actually is
		if ((ally.unit()->isCloaked() || ally.unit()->isBurrowed()) && Grids().getEDetectorGrid(WalkPosition(ally.getEngagePosition())) == 0) simRatio = simRatio * 5.0;
		if (!ally.getType().isFlyer() && Broodwar->getGroundHeight(TilePosition(ally.getEngagePosition())) > Broodwar->getGroundHeight(TilePosition(ally.getTargetPosition())))	simRatio = simRatio * 2.0;

		allyLocalGroundStrength += ally.getVisibleGroundStrength() * simRatio;
		allyLocalAirStrength += ally.getVisibleAirStrength() * simRatio;
	}

	// Store the difference of strengths
	enemyLocalGroundStrength > 0.0 ? unit.setGroundLocal(allyLocalGroundStrength / enemyLocalGroundStrength) : unit.setGroundLocal(2.0);
	enemyLocalAirStrength > 0.0 ? unit.setAirLocal(allyLocalAirStrength / enemyLocalAirStrength) : unit.setAirLocal(2.0);
}

void UnitTrackerClass::updateStrategy(UnitInfo& unit)
{
	UnitInfo &target = unit.getType() == UnitTypes::Terran_Medic ? Units().getAllyUnit(unit.getTarget()) : Units().getEnemyUnit(unit.getTarget());
	double widths = target.getType().tileWidth() * 16.0 + unit.getType().tileWidth() * 16.0;
	double decisionLocal = unit.getType().isFlyer() ? unit.getAirLocal() : unit.getGroundLocal();
	double decisionGlobal = unit.getType().isFlyer() ? globalAirStrategy : globalGroundStrategy;
	double allyRange = widths + target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
	double enemyRange = widths + unit.getType().tileWidth() + unit.getType().isFlyer() ? target.getAirRange() : target.getGroundRange();

	if (!unit.getTarget() || (unit.getPosition().getDistance(unit.getSimPosition()) > 640.0 && decisionGlobal == 1)) unit.setStrategy(3);	 // If unit does not have a target or clearly out of range of sim target, set as "no local"
	else if (shouldAttack(unit)) unit.setStrategy(1);
	else if (shouldRetreat(unit)) unit.setStrategy(2);

	else if ((Terrain().isInAllyTerritory(unit.getTilePosition()) && (decisionGlobal == 0 || (!unit.getType().isFlyer() && unit.getGroundLocal() < minThreshold) || (unit.getType().isFlyer() && unit.getAirLocal() < minThreshold))) || (Terrain().isInAllyTerritory(target.getTilePosition()))) // If unit is in ally territory
	{
		if (!Strategy().isHoldChoke())
		{
			if (Grids().getBaseGrid(target.getTilePosition()) > 0 && (!target.getType().isWorker() || Broodwar->getFrameCount() - target.getLastAttackFrame() < 500)) unit.setStrategy(1);
			else unit.setStrategy(2);
		}
		else
		{
			if ((Terrain().isInAllyTerritory(target.getTilePosition()) && (!target.getType().isWorker() || Broodwar->getFrameCount() - target.getLastAttackFrame() < 500)) || unit.getPosition().getDistance(unit.getTargetPosition()) - (double(unit.getType().width()) / 2.0) - (double(target.getType().width()) / 2.0) <= unit.getGroundRange()) unit.setStrategy(1);
			else if (unit.getPosition().getDistance(unit.getTargetPosition()) > enemyRange || allyRange >= enemyRange) unit.setStrategy(2);
			else unit.setStrategy(0);
		}
	}
	else if (unit.getType().isFlyer() && Players().getNumberZerg() > 0 && globalEnemyAirStrength > 0.0 && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Air_Weapons) <= 0) unit.setStrategy(2);
	else if (!unit.getType().isFlyer() && Players().getNumberZerg() > 0 && BuildOrder().isForgeExpand() && globalEnemyGroundStrength > 0.0 && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) <= 0) unit.setStrategy(2);
	else if (Strategy().isPlayPassive() && !Terrain().isInAllyTerritory(unit.getTilePosition())) unit.setStrategy(2);

	else if (unit.getStrategy() == 1) // If last command was engage
	{
		if ((!unit.getType().isFlyer() && unit.getGroundLocal() < minThreshold) || (unit.getType().isFlyer() && unit.getAirLocal() < minThreshold)) unit.setStrategy(0);
		else unit.setStrategy(1);
	}
	else // If last command was disengage/no command
	{
		if ((!unit.getType().isFlyer() && unit.getGroundLocal() > maxThreshold) || (unit.getType().isFlyer() && unit.getAirLocal() > maxThreshold)) unit.setStrategy(1);
		else unit.setStrategy(0);
	}
}

bool UnitTrackerClass::shouldAttack(UnitInfo& unit)
{
	UnitInfo &target = unit.getType() == UnitTypes::Terran_Medic ? Units().getAllyUnit(unit.getTarget()) : Units().getEnemyUnit(unit.getTarget());

	if (unit.getType().isWorker() && unit.getTarget()->exists()) return true; // If unit is a worker, always fight
	else if ((unit.unit()->isCloaked() || unit.unit()->isBurrowed()) && Grids().getEDetectorGrid(WalkPosition(unit.getEngagePosition())) <= 0) return true;
	else if (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && Util().unitInRange(unit)) return true; // If a unit is a Reaver and within range of an enemy	
	else if (!unit.getType().isFlyer() && (target.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || target.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && unit.getPosition().getDistance(unit.getTargetPosition()) < 128) return true; // If unit is close to a tank, keep attacking it
	return false;
}

bool UnitTrackerClass::shouldRetreat(UnitInfo& unit)
{
	UnitInfo &target = unit.getType() == UnitTypes::Terran_Medic ? Units().getAllyUnit(unit.getTarget()) : Units().getEnemyUnit(unit.getTarget());

	if (unit.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && target.getType() == UnitTypes::Terran_Vulture) return true; // If unit is a Zealot, don't chase Vultures TODO -- check for mobility
	else if (unit.getType() == UnitTypes::Protoss_High_Templar && unit.unit()->getEnergy() < 75) return true; // If unit is a High Templar and low energy, retreat	
	else if (unit.getType() == UnitTypes::Terran_Medic && unit.unit()->getEnergy() <= 0) return true; // If unit is a Medic and no energy, retreat	
	return false;
}

void UnitTrackerClass::updateGlobalSimulation()
{
	if (Broodwar->self()->getRace() == Races::Protoss)
	{
		double offset = 1.0;
		if (Players().getNumberZerg() > 0) offset = 1.1;
		if (Players().getNumberTerran() > 0) offset = 0.8;

		if (Strategy().isPlayPassive())	globalGroundStrategy = 0;
		else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) >= 1)	globalGroundStrategy = 1;
		else if (globalAllyGroundStrength < globalEnemyGroundStrength * offset) globalGroundStrategy = 0;
		else globalGroundStrategy = 1;

		if (Strategy().isPlayPassive())	globalAirStrategy = 0;
		else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) >= 1)	globalAirStrategy = 1;
		else if (globalAllyAirStrength < globalEnemyAirStrength * offset) globalAirStrategy = 0;
		else globalAirStrategy = 1;
		return;
	}
	else if (Broodwar->self()->getRace() == Races::Terran)
	{
		double offset = 1.0;
		if (Players().getNumberZerg() > 0) offset = 1.4;
		if (Players().getNumberProtoss() > 0) offset = 1.4;

		if (Strategy().isPlayPassive())	globalGroundStrategy = 0;
		else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters)) globalGroundStrategy = 1;
		else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && BuildOrder().getCurrentBuild() == "Sparks") globalGroundStrategy = 1;
		else if (globalAllyGroundStrength < globalEnemyGroundStrength * offset) globalGroundStrategy = 0;
		else globalGroundStrategy = 1;

		if (Strategy().isPlayPassive())	globalAirStrategy = 0;
		else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters)) globalAirStrategy = 1;
		else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && BuildOrder().getCurrentBuild() == "Sparks") globalGroundStrategy = 1;
		else if (globalAllyAirStrength < globalEnemyAirStrength * offset) globalAirStrategy = 0;
		else globalAirStrategy = 1;
		return;
	}
}

UnitInfo& UnitTrackerClass::getAllyUnit(Unit unit)
{
	if (allyUnits.find(unit) != allyUnits.end()) return allyUnits[unit];
	assert(0);
	return UnitInfo();
}

UnitInfo& UnitTrackerClass::getEnemyUnit(Unit unit)
{
	if (enemyUnits.find(unit) != enemyUnits.end()) return enemyUnits[unit];
	assert(0);
	return UnitInfo();
}

set<Unit> UnitTrackerClass::getAllyUnitsFilter(UnitType type)
{
	returnValues.clear();
	for (auto &u : allyUnits)
	{
		UnitInfo &unit = u.second;
		if (unit.getType() == type)	returnValues.insert(unit.unit());
	}
	return returnValues;
}

void UnitTrackerClass::storeEnemy(Unit unit)
{
	enemyUnits[unit].setUnit(unit);
	enemySizes[unit->getType().size()] += 1;
	if (unit->getType().isResourceDepot()) Bases().storeBase(unit);
}

void UnitTrackerClass::updateEnemy(UnitInfo& unit)
{
	// Update units
	auto t = unit.unit()->getType();
	auto p = unit.unit()->getPlayer();

	// Update information
	unit.setType(t);
	unit.setPosition(unit.unit()->getPosition());
	unit.setWalkPosition(Util().getWalkPosition(unit.unit()));
	unit.setTilePosition(unit.unit()->getTilePosition());
	unit.setPlayer(unit.unit()->getPlayer());

	// Update statistics
	unit.setHealth(unit.unit()->getHitPoints());
	unit.setShields(unit.unit()->getShields());
	unit.setPercentHealth(Util().getPercentHealth(unit));
	unit.setGroundRange(Util().getTrueGroundRange(unit));
	unit.setAirRange(Util().getTrueAirRange(unit));
	unit.setGroundDamage(Util().getTrueGroundDamage(unit));
	unit.setAirDamage(Util().getTrueAirDamage(unit));
	unit.setSpeed(Util().getTrueSpeed(unit));
	unit.setMinStopFrame(Util().getMinStopFrame(t));
	unit.setLastVisibleFrame(Broodwar->getFrameCount());

	// Update sizes and strength
	unit.setVisibleGroundStrength(Util().getVisibleGroundStrength(unit));
	unit.setMaxGroundStrength(Util().getMaxGroundStrength(unit));
	unit.setVisibleAirStrength(Util().getVisibleAirStrength(unit));
	unit.setMaxAirStrength(Util().getMaxAirStrength(unit));
	unit.setPriority(Util().getPriority(unit));

	// Set last command frame
	if (unit.unit()->isStartingAttack()) unit.setLastAttackFrame(Broodwar->getFrameCount());
	if (unit.unit()->getTarget() && unit.unit()->getTarget()->exists()) unit.setTarget(unit.unit()->getTarget());
	else if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine) unit.setTarget(unit.unit()->getClosestUnit(Filter::GetPlayer == Broodwar->self(), 128));
}

void UnitTrackerClass::storeAlly(Unit unit)
{
	if (unit->getType().isBuilding())
	{
		allyDefenses[unit].setUnit(unit);
		allyDefenses[unit].setType(unit->getType());
		allyDefenses[unit].setTilePosition(unit->getTilePosition());
		allyDefenses[unit].setPosition(unit->getPosition());
		Grids().updateDefenseGrid(allyDefenses[unit]);
	}
	else allyUnits[unit].setUnit(unit);
}

void UnitTrackerClass::updateAlly(UnitInfo& unit)
{
	auto t = unit.unit()->getType();
	auto p = unit.unit()->getPlayer();

	// Update information
	unit.setType(t);
	unit.setPosition(unit.unit()->getPosition());
	unit.setDestination(Positions::None);
	unit.setTilePosition(unit.unit()->getTilePosition());
	unit.setWalkPosition(Util().getWalkPosition(unit.unit()));
	unit.setPlayer(unit.unit()->getPlayer());
	unit.setLastCommand(unit.unit()->getLastCommand());

	// Update statistics
	unit.setHealth(unit.unit()->getHitPoints());
	unit.setShields(unit.unit()->getShields());
	unit.setPercentHealth(Util().getPercentHealth(unit));
	unit.setGroundRange(Util().getTrueGroundRange(unit));
	unit.setAirRange(Util().getTrueAirRange(unit));
	unit.setGroundDamage(Util().getTrueGroundDamage(unit));
	unit.setAirDamage(Util().getTrueAirDamage(unit));
	unit.setSpeed(Util().getTrueSpeed(unit));
	unit.setMinStopFrame(Util().getMinStopFrame(t));

	// Update sizes and strength
	unit.setVisibleGroundStrength(Util().getVisibleGroundStrength(unit));
	unit.setMaxGroundStrength(Util().getMaxGroundStrength(unit));
	unit.setVisibleAirStrength(Util().getVisibleAirStrength(unit));
	unit.setMaxAirStrength(Util().getMaxAirStrength(unit));
	unit.setPriority(Util().getPriority(unit));

	// Update target
	unit.setTarget(Targets().getTarget(unit));

	// Set last command frame
	if (unit.unit()->isStartingAttack()) unit.setLastAttackFrame(Broodwar->getFrameCount());
}