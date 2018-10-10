#include "McRave.h"

void WorkerManager::onFrame()
{
	// TODO: Move scouts to another manager system
	updateScouts();
	for (auto &w : myWorkers) {
		auto &worker = w.second;
		worker.updateWorkerUnit();

		if (worker.info()->getRole() == Role::Working) {
			updateDecision(worker);
		}
	}
}

void WorkerManager::updateScouts()
{
	scoutAssignments.clear();
	scoutCount = 1;

	// If we have seen an enemy Probe before we've scouted the enemy, follow it
	if (Units().getEnemyCount(UnitTypes::Protoss_Probe) == 1) {
		auto w = Util().getClosestUnit(mapBWEB.getMainPosition(), Broodwar->enemy(), UnitTypes::Protoss_Probe);
		proxyCheck = (w && !Terrain().getEnemyStartingPosition().isValid() && w->getPosition().getDistance(mapBWEB.getMainPosition()) < 640.0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1);
	}

	// If we know a proxy possibly exists, we need a second scout
	auto foundProxyGates = Strategy().enemyProxy() && Strategy().getEnemyBuild() == "P2Gate" && Units().getEnemyCount(UnitTypes::Protoss_Gateway) > 0;
	if (((Strategy().enemyProxy() && Strategy().getEnemyBuild() != "P2Gate") || proxyCheck || foundProxyGates) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) < 1)
		scoutCount++;

	if (BuildOrder().isRush() && Broodwar->self()->getRace() == Races::Zerg && Terrain().getEnemyStartingPosition().isValid())
		scoutCount = 0;
	if (Strategy().getEnemyBuild() == "Z5Pool" && Units().getEnemyCount(UnitTypes::Zerg_Zergling) >= 5)
		scoutCount = 0;

	// If we have too many scouts, remove closest to home
	double distBest = DBL_MAX;
	Unit removal;
	if ((int)scouts.size() > scoutCount) {
		for (auto &scout : scouts) {

			if (!scout || !scout->exists())
				continue;

			double dist = scout->getPosition().getDistance(mapBWEB.getMainPosition());
			if (dist < distBest) {
				distBest = dist;
				removal = scout;
			}
		}
		scouts.erase(removal);
	}

	// If we have too few scouts
	if (mapBWEB.getNaturalChoke() && BuildOrder().shouldScout() && (int)scouts.size() < scoutCount)
		scouts.insert(getClosestWorker(Position(mapBWEB.getNaturalChoke()->Center()), false));
}

void WorkerManager::updateDecision(WorkerUnit& worker)
{
	// Workers that have a transport coming to pick them up should not do anything other than returning cargo
	if (worker.getTransport() && !worker.info()->unit()->isCarryingMinerals() && !worker.info()->unit()->isCarryingGas()) {
		if (worker.info()->unit()->getLastCommand().getType() != UnitCommandTypes::Move)
			worker.info()->unit()->move(worker.getTransport()->getPosition());
		return;
	}

	// If worker is potentially stuck, try to find a manner pylon
	if (worker.framesHoldingResource() >= 100 || worker.framesHoldingResource() <= -200) {
		auto pylon = Util().getClosestUnit(worker.info()->getPosition(), Broodwar->enemy(), UnitTypes::Protoss_Pylon);
		if (pylon && pylon->unit() && pylon->unit()->exists()) {
			if (worker.info()->unit()->getLastCommand().getTarget() != pylon->unit())
				worker.info()->unit()->attack(pylon->unit());
			return;
		}
	}

	// Remove combat unit role
	if (Units().getMyUnits().find(worker.info()->unit()) != Units().getMyUnits().end())
		Units().getMyUnits().erase(worker.info()->unit());

	// Assign a resource
	if (shouldAssign(worker))
		assign(worker);

	// Choose a command
	if (shouldReturnCargo(worker))
		returnCargo(worker);
	else if (shouldClearPath(worker)) {
		Broodwar->drawCircleMap(worker.info()->getPosition(), 8, Colors::Purple, true);
		clearPath(worker);
	}
	else if (shouldBuild(worker))
		build(worker);
	else if (shouldScout(worker))
		scout(worker);
	else if (shouldFight(worker))
		fight(worker);
	else if (shouldGather(worker))
		gather(worker);
}

bool WorkerManager::shouldAssign(WorkerUnit& worker)
{
	if (find(scouts.begin(), scouts.end(), worker.info()->unit()) != scouts.end())
		return false;

	if (!worker.hasResource()
		|| needGas()
		|| (worker.hasResource() && !worker.getResource().getType().isMineralField() && (Broodwar->self()->visibleUnitCount(worker.info()->getType()) <= 10 || gasWorkers > BuildOrder().gasWorkerLimit())))
		return true;

	// HACK: Try to make Zerg gather less gas
	if (Broodwar->self()->gas() > Broodwar->self()->minerals() && Broodwar->self()->getRace() == Races::Zerg)
		return true;
	return false;
}

bool WorkerManager::shouldBuild(WorkerUnit& worker)
{
	if (worker.getBuildingType().isValid() && worker.getBuildPosition().isValid())
		return true;
	return false;
}

bool WorkerManager::shouldClearPath(WorkerUnit& worker)
{
	if (Broodwar->getFrameCount() < 10000)
		return false;

	for (auto &b : Resources().getMyBoulders()) {
		ResourceInfo &boulder = b.second;
		if (!boulder.unit() || !boulder.unit()->exists())
			continue;
		if (worker.info()->getPosition().getDistance(boulder.getPosition()) < 480.0)
			return true;
	}
	return false;
}

bool WorkerManager::shouldFight(WorkerUnit& worker)
{
	if (worker.getTransport())
		return false;
	if (Util().reactivePullWorker(worker.info()->unit()) || (Util().proactivePullWorker(worker.info()->unit()) && worker.info()->unit() == getClosestWorker(Terrain().getDefendPosition(), true)))
		return true;
	if (Util().pullRepairWorker(worker.info()->unit()) && worker.info()->unit() == getClosestWorker(Terrain().getDefendPosition(), true))
		return true;
	return false;
}

bool WorkerManager::shouldGather(WorkerUnit& worker)
{
	if (worker.hasResource() && (worker.info()->unit()->isGatheringMinerals() || worker.info()->unit()->isGatheringGas()) && worker.info()->unit()->getTarget() != worker.getResource().unit() && worker.getResource().getState() == 2)
		return true;
	else if (worker.info()->unit()->isIdle() || worker.info()->unit()->getLastCommand().getType() != UnitCommandTypes::Gather)
		return true;
	return false;
}

bool WorkerManager::shouldReturnCargo(WorkerUnit& worker)
{
	// Don't return cargo if we're on an island and just mined a blocking mineral
	if (worker.getBuildingType().isResourceDepot() && worker.getBuildPosition().isValid() && mapBWEM.GetArea(worker.getBuildPosition()) && !mapBWEM.GetArea(worker.getBuildPosition())->AccessibleFrom(mapBWEB.getMainArea()) && worker.info()->getPosition().getDistance((Position)worker.getBuildPosition()) < 160.0)
		return false;
	if (worker.info()->unit()->isCarryingGas() || worker.info()->unit()->isCarryingMinerals())
		return true;
	return false;
}

bool WorkerManager::shouldScout(WorkerUnit& worker)
{
	if (worker.getBuildPosition().isValid())
		return false;
	if (Units().getEnemyCount(UnitTypes::Terran_Vulture) >= 2 && !BuildOrder().firstReady())
		return false;
	else if (deadScoutFrame > 0 && Broodwar->getFrameCount() - deadScoutFrame < 2000)
		return false;
	else if (find(scouts.begin(), scouts.end(), worker.info()->unit()) != scouts.end() && worker.getBuildingType() == UnitTypes::None && (BuildOrder().shouldScout() || proxyCheck))
		return true;
	return false;
}

void WorkerManager::assign(WorkerUnit& worker)
{
	// Remove current assignment if it has one
	if (worker.hasResource()) {
		worker.getResource().setGathererCount(worker.getResource().getGathererCount() - 1);
		worker.getResource().getType().isMineralField() ? minWorkers-- : gasWorkers--;
	}

	double distBest = DBL_MAX;
	ResourceInfo* bestResource = nullptr;

	// Check if we need gas workers
	if (needGas()) {
		for (auto &g : Resources().getMyGas()) {
			ResourceInfo &gas = g.second;
			double dist = gas.getPosition().getDistance(worker.info()->getPosition());
			if (dist < distBest && gas.getType() != UnitTypes::Resource_Vespene_Geyser && gas.unit()->exists() && gas.unit()->isCompleted() && gas.getGathererCount() < 3 && gas.getState() > 0)
				bestResource = &gas, distBest = dist;
		}
		if (bestResource) {
			bestResource->setGathererCount(bestResource->getGathererCount() + 1);
			worker.setResource(bestResource);
			gasWorkers++;
			return;
		}
	}

	// Check if we need mineral workers
	else {

		// If worker is injured early on, send him to the furthest mineral field
		bool injured = (worker.info()->unit()->getHitPoints() + worker.info()->unit()->getShields() < worker.info()->getType().maxHitPoints() + worker.info()->getType().maxShields());
		if (injured)
			distBest = 0.0;

		for (int i = 1; i <= 2; i++) {
			for (auto &m : Resources().getMyMinerals()) {
				ResourceInfo &mineral = m.second;

				double dist = mineral.getPosition().getDistance(worker.info()->getPosition());
				if (((dist < distBest && !injured) || (dist > distBest && injured)) && mineral.getGathererCount() < i && mineral.getState() > 0)
					bestResource = &mineral, distBest = dist;
			}
			if (bestResource) {
				bestResource->setGathererCount(bestResource->getGathererCount() + 1);
				worker.setResource(bestResource);
				minWorkers++;
				return;
			}
		}
	}

	worker.setResource(nullptr);
}

void WorkerManager::build(WorkerUnit& worker)
{
	Position center = Position(worker.getBuildPosition()) + Position(worker.getBuildingType().tileWidth() * 16, worker.getBuildingType().tileHeight() * 16);

	const auto shouldMoveToBuild = [&]() {
		auto mineralIncome = (minWorkers - 1) * 0.045;
		auto gasIncome = (gasWorkers - 1) * 0.07;
		auto speed = worker.info()->getType().topSpeed();
		auto dist = mapBWEB.getGroundDistance(worker.info()->getPosition(), center);
		auto time = (dist / speed) + 50.0;
		auto enoughGas = worker.getBuildingType().gasPrice() > 0 ? Broodwar->self()->gas() + int(gasIncome * time) >= worker.getBuildingType().gasPrice() : true;
		auto enoughMins = worker.getBuildingType().mineralPrice() > 0 ? Broodwar->self()->minerals() + int(mineralIncome * time) >= worker.getBuildingType().mineralPrice() : true;

		return enoughGas && enoughMins;
	};

	// Spider mine removal
	if (worker.getBuildingType().isResourceDepot()) {
		if (Unit mine = Broodwar->getClosestUnit(Position(worker.getBuildPosition()), Filter::GetType == UnitTypes::Terran_Vulture_Spider_Mine, 128)) {
			if (worker.info()->unit()->getLastCommand().getType() != UnitCommandTypes::Attack_Unit || !worker.info()->unit()->getLastCommand().getTarget() || !worker.info()->unit()->getLastCommand().getTarget()->exists()) {
				worker.info()->unit()->attack(mine);
			}
			return;
		}

		if (UnitInfo* unit = Util().getClosestUnit(worker.info()->getPosition(), Broodwar->enemy())) {
			if (unit->getPosition().getDistance(worker.info()->getPosition()) < 160
				&& unit->getTilePosition().x >= worker.getBuildPosition().x
				&& unit->getTilePosition().x < worker.getBuildPosition().x + worker.getBuildingType().tileWidth()
				&& unit->getTilePosition().y >= worker.getBuildPosition().y
				&& unit->getTilePosition().y < worker.getBuildPosition().y + worker.getBuildingType().tileHeight())
			{
				worker.info()->unit()->attack(unit->unit());
				return;
			}
		}
	}

	// If our building desired has changed recently, remove TODO
	if ((BuildOrder().buildCount(worker.getBuildingType()) <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()) || !Buildings().isBuildable(worker.getBuildingType(), worker.getBuildPosition()))) {
		worker.setBuildingType(UnitTypes::None);
		worker.setBuildPosition(TilePositions::Invalid);
		worker.info()->unit()->stop();
	}

	else if (shouldMoveToBuild()) {
		if (worker.info()->getPosition().getDistance(Position(worker.getBuildPosition())) > 256.0)
			safeMove(worker);
		else if (worker.info()->unit()->getOrder() != Orders::PlaceBuilding || worker.info()->unit()->isIdle())
			worker.info()->unit()->build(worker.getBuildingType(), worker.getBuildPosition());
	}
}

void WorkerManager::clearPath(WorkerUnit& worker)
{
	for (auto &b : Resources().getMyBoulders()) {
		ResourceInfo &boulder = b.second;
		if (!boulder.unit() || !boulder.unit()->exists())
			continue;
		if (worker.info()->getPosition().getDistance(boulder.getPosition()) >= 480.0)
			continue;

		if (!worker.info()->unit()->isGatheringMinerals()) {
			if (worker.info()->unit()->getOrderTargetPosition() != b.second.getPosition())
				worker.info()->unit()->gather(b.first);
			return;
		}
	}
}

void WorkerManager::fight(WorkerUnit& worker)
{
	//Units().storeAlly(worker.info()->unit());
}

void WorkerManager::gather(WorkerUnit& worker)
{
	if (worker.hasResource() && worker.getResource().getState() == 2) {
		if (worker.getResource().unit() && worker.getResource().unit()->exists() && (worker.info()->getPosition().getDistance(worker.getResource().getPosition()) < 320.0 || worker.info()->unit()->getShields() < 20))
			worker.info()->unit()->gather(worker.getResource().unit());
		else if (worker.getResource().getPosition().isValid())
			safeMove(worker);
	}
	else {
		auto closest = worker.info()->unit()->getClosestUnit(Filter::IsMineralField);
		if (closest && closest->exists())
			worker.info()->unit()->gather(closest);
	}
}

void WorkerManager::returnCargo(WorkerUnit& worker)
{
	if (worker.info()->unit()->getOrder() != Orders::ReturnMinerals && worker.info()->unit()->getOrder() != Orders::ReturnGas && worker.info()->unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo)
		worker.info()->unit()->returnCargo();
}

void WorkerManager::scout(WorkerUnit& worker)
{
	WalkPosition start = worker.info()->getWalkPosition();
	double distBest = DBL_MAX;
	Position posBest = worker.info()->getDestination();
	//Broodwar->setScreenPosition(worker.getPosition() - Position(320, 240));

	// If first not ready
	// Look at scout targets
	// 1) Get closest scout target that isn't assigned yet
	// 2) If no scout assignments, check if we know where the enemy is, if we do, scout their main
	// 3) If we don't know, scout any unexplored starting locations

	//worker.setDestination(Positions::Invalid);

	if (!BuildOrder().firstReady() || Strategy().getEnemyBuild() == "Unknown") {

		// If it's a proxy (maybe cannon rush), try to find the worker to kill
		if ((Strategy().enemyProxy() || proxyCheck) && scoutCount > 1 && scoutAssignments.find(mapBWEB.getMainPosition()) == scoutAssignments.end() && getClosestScout(mapBWEB.getMainPosition()) == &worker) {
			UnitInfo* enemyWorker = Util().getClosestUnit(worker.info()->getPosition(), Broodwar->enemy(), UnitTypes::Protoss_Probe);
			scoutAssignments.insert(mapBWEB.getMainPosition());

			if (enemyWorker && enemyWorker->getPosition().isValid() && enemyWorker->getPosition().getDistance(mapBWEB.getMainPosition()) < 640.0) {
				if (enemyWorker->unit() && enemyWorker->unit()->exists()) {
					worker.info()->unit()->attack(enemyWorker->unit());
					return;
				}
				else
					worker.info()->setDestination(enemyWorker->getPosition());
			}
			else if (Strategy().getEnemyBuild() == "P2Gate") {
				UnitInfo* enemyPylon = Util().getClosestUnit(worker.info()->getPosition(), Broodwar->enemy(), UnitTypes::Protoss_Pylon);
				if (enemyPylon && !Terrain().isInEnemyTerritory(enemyPylon->getTilePosition())) {
					if (enemyPylon->unit() && enemyPylon->unit()->exists()) {
						worker.info()->unit()->attack(enemyPylon->unit());
						return;
					}
					else
						worker.info()->unit()->move(enemyPylon->getPosition());
				}
			}
			else
				worker.info()->setDestination(mapBWEB.getMainPosition());
		}

		// If we have scout targets, find the closest target
		else if (!Strategy().getScoutTargets().empty()) {
			for (auto &target : Strategy().getScoutTargets()) {
				double dist = target.getDistance(worker.info()->getPosition());
				double time = 1.0 + (double(Grids().lastVisibleFrame((TilePosition)target)));
				double timeDiff = Broodwar->getFrameCount() - time;

				if (time < distBest && timeDiff > 500 && scoutAssignments.find(target) == scoutAssignments.end()) {
					distBest = time;
					posBest = target;
				}
			}
			if (posBest.isValid()) {
				worker.info()->setDestination(posBest);
				scoutAssignments.insert(posBest);
			}
		}

		if (worker.info()->getDestination().isValid())
			explore(worker);

		Broodwar->drawLineMap(worker.info()->getPosition(), worker.info()->getDestination(), Colors::Green);
	}
	else
	{
		int best = INT_MAX;
		for (auto &area : mapBWEM.Areas()) {
			for (auto &base : area.Bases()) {
				if (area.AccessibleNeighbours().size() == 0 || Terrain().isInEnemyTerritory(base.Location()) || Terrain().isInAllyTerritory(base.Location()))
					continue;

				int time = Grids().lastVisibleFrame(base.Location());
				if (time < best)
					best = time, posBest = Position(base.Location());
			}
		}
		if (posBest.isValid() && worker.info()->unit()->getOrderTargetPosition() != posBest)
			worker.info()->unit()->move(posBest);
	}
}

void WorkerManager::explore(WorkerUnit& worker)
{
	WalkPosition start = worker.info()->getWalkPosition();
	Position bestPosition = worker.info()->getDestination();
	int longest = 0;
	UnitInfo* enemy = Util().getClosestUnit(worker.info()->getPosition(), Broodwar->enemy());
	double test = 0.0;

	// Determine highest threat possible here
	const auto highestThreat = [&](WalkPosition here, UnitType t) {
		double highest = 0.01;
		for (int x = here.x - 2; x < here.x + 2; x++) {
			for (int y = here.y - 2; y < here.y + 2; y++) {
				WalkPosition w(x, y);
				double current = Grids().getEGroundThreat(w);
				highest = (current > highest) ? current : highest;
			}
		}
		return highest;
	};

	if (worker.info()->getPosition().getDistance(worker.info()->getDestination()) > 256.0 && (!enemy || (enemy && (enemy->getType().isWorker() || enemy->getPosition().getDistance(worker.info()->getPosition()) > 320.0)))) {
		bestPosition = worker.info()->getDestination();
	}
	else
	{
		// Check a 8x8 walkposition grid for a potential new place to scout
		double best = 0.0;
		int radius = 8;
		auto enemyThreat = (enemy && enemy->getType().groundWeapon().damageAmount() > 0 && enemy->getPosition().getDistance(worker.info()->getPosition()) < 128.0);

		if (enemy && enemy->getPosition().isValid())
			radius = max(4, (320 - (int)enemy->getPosition().getDistance(worker.info()->getPosition())) / 32);

		for (int x = start.x - radius; x < start.x + radius + 4; x++) {
			for (int y = start.y - radius; y < start.y + radius + 4; y++) {
				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);
				TilePosition t(w);

				if (!w.isValid()
					|| !Util().isMobile(start, w, worker.info()->getType())
					|| p.getDistance(worker.info()->getPosition()) < 32.0
					|| p.getDistance(worker.info()->getDestination()) < 96.0)
					continue;

				double mobility = double(Grids().getMobility(w));
				double threat = exp(highestThreat(w, worker.info()->getType()));
				double distance = enemyThreat ? 1.0 / p.getDistance(enemy->getPosition()) : mapBWEB.getGroundDistance(p, worker.info()->getDestination());
				double time = double(Grids().lastVisibleFrame(t));

				if (worker.info()->getDestination() == mapBWEB.getMainPosition() && (mapBWEM.GetArea(t) == mapBWEB.getMainArea() || (Broodwar->getFrameCount() > 4000 && mapBWEM.GetTile(t).GroundHeight() > mapBWEM.GetTile(mapBWEB.getMainTile()).GroundHeight())))
					distance = 1.0;

				double score = mobility / (threat * distance * time);

				if (score >= best) {
					best = score;
					bestPosition = p;
					test = time;
				}
			}
		}
	}

	if (bestPosition.isValid() && bestPosition != Position(start) && worker.info()->unit()->getLastCommand().getTargetPosition() != bestPosition) {
		worker.info()->unit()->move(bestPosition);
		Broodwar->drawLineMap(worker.info()->getPosition(), bestPosition, Colors::Blue);
	}
}

void WorkerManager::safeMove(WorkerUnit& worker)
{
	UnitInfo* enemy = Util().getClosestUnit(worker.info()->getPosition(), Broodwar->enemy());
	Position bestPosition(worker.getBuildPosition());

	if (!worker.getBuildPosition().isValid())
		bestPosition = worker.getResource().getPosition();

	if (enemy && enemy->getPosition().getDistance(worker.info()->getPosition()) < 320.0 && enemy->getType().groundWeapon().damageAmount() > 0 && !enemy->getType().isWorker()) {
		WalkPosition start = worker.info()->getWalkPosition();
		WalkPosition destination(worker.getBuildPosition());
		double best = 0.0;

		if (!worker.getBuildPosition().isValid())
			destination = (WalkPosition)worker.getResource().getPosition();

		for (int x = start.x - 8; x < start.x + 12; x++) {
			for (int y = start.y - 8; y < start.y + 12; y++) {
				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);

				if (!w.isValid()
					|| !Util().isMobile(start, w, worker.info()->getType()))
					continue;

				double threat = max(0.01, Grids().getEGroundThreat(w));
				double distance = 1.0 + mapBWEB.getGroundDistance(p, worker.info()->getDestination());

				double score = 1.0 / (threat * distance);
				if (score >= best) {
					best = score;
					bestPosition = Position(w);
				}
			}
		}
	}

	if (worker.info()->unit()->getLastCommand().getTargetPosition() != bestPosition || worker.info()->unit()->getOrderTargetPosition() != bestPosition)
		worker.info()->unit()->move(bestPosition);
}

Unit WorkerManager::getClosestWorker(Position here, bool isRemoving)
{
	// Currently gets the closest worker that doesn't mine gas
	Unit closestWorker = nullptr;
	double closestD = DBL_MAX;
	for (auto &w : myWorkers) {
		WorkerUnit &worker = w.second;

		if (worker.hasResource() && !worker.getResource().getType().isMineralField()) continue;
		if (isRemoving && worker.getBuildPosition().isValid()) continue;
		if (find(scouts.begin(), scouts.end(), worker.info()->unit()) != scouts.end()) continue;
		if (worker.info()->unit()->getLastCommand().getType() == UnitCommandTypes::Gather && worker.info()->unit()->getLastCommand().getTarget()->exists() && worker.info()->unit()->getLastCommand().getTarget()->getInitialResources() == 0)	continue;
		if (worker.info()->getType() != UnitTypes::Protoss_Probe && worker.info()->unit()->isConstructing()) continue;

		double dist = mapBWEB.getGroundDistance(worker.info()->getPosition(), here);
		if (dist == DBL_MAX)
			dist = worker.info()->getPosition().getDistance(here);

		if (dist < closestD) {
			closestWorker = worker.info()->unit();
			closestD = dist;
		}
	}
	return closestWorker;
}

void WorkerManager::storeWorker(Unit unit)
{
	myWorkers[unit];
}

void WorkerManager::removeWorker(Unit worker)
{
	WorkerUnit &thisWorker = myWorkers[worker];
	if (thisWorker.hasResource()) {
		thisWorker.getResource().setGathererCount(thisWorker.getResource().getGathererCount() - 1);
		thisWorker.getResource().getType().isMineralField() ? minWorkers-- : gasWorkers--;
	}

	if (find(scouts.begin(), scouts.end(), worker) != scouts.end()) {
		deadScoutFrame = Broodwar->getFrameCount();
		scouts.erase(find(scouts.begin(), scouts.end(), worker));
	}
	myWorkers.erase(worker);
}

WorkerUnit* WorkerManager::getClosestScout(Position here)
{
	double distBest = DBL_MAX;
	WorkerUnit* closestWorker = nullptr;
	for (auto &worker : scouts) {
		if (!worker)
			continue;

		double dist = mapBWEB.getGroundDistance(myWorkers[worker].info()->getPosition(), here);
		if (dist < distBest) {
			closestWorker = &myWorkers[worker];
			distBest = dist;
		}
	}
	return closestWorker;
}

bool WorkerManager::needGas() {
	if (Broodwar->self()->gas() > Broodwar->self()->minerals() && Broodwar->self()->getRace() == Races::Zerg)
		return false;

	if (Broodwar->self()->visibleUnitCount(Broodwar->self()->getRace().getWorker()) > 10 && !Resources().isGasSaturated() && ((gasWorkers < BuildOrder().gasWorkerLimit() && BuildOrder().isOpener()) || !BuildOrder().isOpener() || Resources().isMinSaturated()))
		return true;
	return false;
}