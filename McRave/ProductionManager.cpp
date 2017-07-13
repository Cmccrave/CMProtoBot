#include "McRave.h"

void ProductionTrackerClass::update()
{
	Display().startClock();
	updateReservedResources();
	updateProtoss();
	updateTerran();
	updateZerg();
	Display().performanceTest(__FUNCTION__);
	return;
}

bool ProductionTrackerClass::canAfford(UnitType unit)
{
	if (unit == UnitTypes::Protoss_Dragoon || unit == UnitTypes::Protoss_Zealot)
	{
		if (Broodwar->self()->minerals() >= unit.mineralPrice() + reservedMineral + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= unit.gasPrice() + reservedGas + Buildings().getQueuedGas())
		{
			return true;
		}
	}
	else
	{
		if (Broodwar->self()->minerals() >= unit.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= unit.gasPrice() + Buildings().getQueuedGas())
		{
			return true;
		}
	}
	return false;
}

void ProductionTrackerClass::updateReservedResources()
{
	// Reserved minerals for idle buildings, tech and upgrades
	reservedMineral = 0, reservedGas = 0;
	for (auto &b : idleHighProduction)
	{
		reservedMineral += b.second.mineralPrice();
		reservedGas += b.second.gasPrice();
	}
	for (auto &t : idleTech)
	{
		reservedMineral += t.second.mineralPrice();
		reservedGas += t.second.gasPrice();
	}
	for (auto &u : idleUpgrade)
	{
		reservedMineral += u.second.mineralPrice();
		reservedGas += u.second.gasPrice();
	}
	return;
}

void ProductionTrackerClass::updateProtoss()
{
	// Gateway saturation - max of 12 so the bot can exceed 4 bases
	if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Gateway) >= min(12, (2 * Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus))))
	{
		gateSat = true;
	}

	// TEMP Production testing
	// Goal here is to check of all the units that are possible for a building, which is the best to build that we can afford based on resources, score and priority
	// Could become a much simpler approach to production than what is currently implemented
	bool test = false;
	if (test)
	{
		for (auto &b : Buildings().getMyBuildings())
		{
			BuildingInfo building = b.second;
			double highestPriority = 0.0;
			UnitType highestType;
			if (building.unit()->isIdle())
			{
				for (auto &unit : building.getType().buildsWhat())
				{
					if (Strategy().getUnitScore()[unit] > highestPriority)
					{
						highestPriority = Strategy().getUnitScore()[unit];
						highestType = unit;
					}
					// Setup a high/med/low priority for units and reserved minerals
					// Make a function that checks for canBuild first (tech requirements)					
				}

				if (canAfford(highestType))
				{
					building.unit()->train(highestType);
				}
				// If a unit is desired and we can afford it, train it, else, emplace


				for (auto &research : building.getType().researchesWhat())
				{
					// Researches
				}


				for (auto &upgrade : building.getType().upgradesWhat())
				{
					// Upgrades
				}

			}
		}
	}

	// Production
	for (auto &thisBuilding : Buildings().getMyBuildings())
	{
		BuildingInfo &building = thisBuilding.second;
		if (building.unit() && building.unit()->isIdle())
		{
			// Forge
			if (building.getType() == UnitTypes::Protoss_Forge && Units().getSupply() > 100)
			{
				if (Broodwar->self()->minerals() >= UpgradeTypes::Protoss_Ground_Weapons.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Protoss_Ground_Weapons.gasPrice() + Buildings().getQueuedGas() + reservedGas)
				{
					building.unit()->upgrade(UpgradeTypes::Protoss_Ground_Weapons);
				}
				if (Broodwar->self()->minerals() >= UpgradeTypes::Protoss_Ground_Armor.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Protoss_Ground_Armor.gasPrice() + Buildings().getQueuedGas() + reservedGas)
				{
					building.unit()->upgrade(UpgradeTypes::Protoss_Ground_Armor);
				}
				if (Broodwar->self()->minerals() >= UpgradeTypes::Protoss_Plasma_Shields.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Protoss_Plasma_Shields.gasPrice() + Buildings().getQueuedGas() + reservedGas)
				{
					building.unit()->upgrade(UpgradeTypes::Protoss_Plasma_Shields);
				}
			}

			// Cybernetics Core
			else if (building.getType() == UnitTypes::Protoss_Cybernetics_Core)
			{
				if (!Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) && idleLowProduction.size() == 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon) >= 3)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Singularity_Charge.mineralPrice() && Broodwar->self()->gas() >= UpgradeTypes::Singularity_Charge.gasPrice())
					{
						building.unit()->upgrade(UpgradeTypes::Singularity_Charge);
						idleUpgrade.erase(building.unit());
					}
					else
					{
						idleUpgrade.emplace(building.unit(), UpgradeTypes::Singularity_Charge);
					}
				}
			}

			// Robotics Support Bay
			else if (building.getType() == UnitTypes::Protoss_Robotics_Support_Bay)
			{
				if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Reaver) > 2)
				{
					if (Broodwar->enemy()->getRace() != Races::Zerg && Broodwar->self()->minerals() >= UpgradeTypes::Scarab_Damage.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Scarab_Damage.gasPrice() + Buildings().getQueuedGas() + reservedGas)
					{
						building.unit()->upgrade(UpgradeTypes::Scarab_Damage);
					}
				}
				if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Shuttle) >= 2)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Gravitic_Drive.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Gravitic_Drive.gasPrice() + Buildings().getQueuedGas() + reservedGas)
					{
						building.unit()->upgrade(UpgradeTypes::Gravitic_Drive);
					}
				}
			}

			// Fleet Beacon
			else if (building.getType() == UnitTypes::Protoss_Fleet_Beacon)
			{
				if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Carrier) > 2)
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Carrier_Capacity.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::Carrier_Capacity.gasPrice() + Buildings().getQueuedGas() + reservedGas)
					{
						building.unit()->upgrade(UpgradeTypes::Carrier_Capacity);
					}
				}
			}

			// Citadel Of Adun
			else if (building.getType() == UnitTypes::Protoss_Citadel_of_Adun)
			{
				if (Broodwar->self()->minerals() >= UpgradeTypes::Leg_Enhancements.mineralPrice() && Broodwar->self()->gas() >= UpgradeTypes::Leg_Enhancements.gasPrice())
				{
					building.unit()->upgrade(UpgradeTypes::Leg_Enhancements);
					idleUpgrade.erase(building.unit());
				}
				else
				{
					idleUpgrade.emplace(building.unit(), UpgradeTypes::Leg_Enhancements);
				}
			}

			// Gateway
			else if (building.getType() == UnitTypes::Protoss_Gateway)
			{
				if (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm) && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Templar_Archives) >= 1 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_High_Templar) < 5)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_High_Templar.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_High_Templar.gasPrice() + Buildings().getQueuedGas() + reservedGas && Units().getSupply() + UnitTypes::Protoss_High_Templar.supplyRequired() <= Broodwar->self()->supplyTotal())
					{
						building.unit()->train(UnitTypes::Protoss_High_Templar);
						idleHighProduction.erase(building.unit());
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_High_Templar);
					}
				}
				if ((Strategy().isZealotsLocked() || Strategy().getUnitScore()[UnitTypes::Protoss_Dragoon] >= Strategy().getUnitScore()[UnitTypes::Protoss_Zealot]) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Dragoon.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UnitTypes::Protoss_Dragoon.gasPrice() + Buildings().getQueuedGas() + reservedGas && Units().getSupply() + UnitTypes::Protoss_Dragoon.supplyRequired() <= Broodwar->self()->supplyTotal())
					{
						building.unit()->train(UnitTypes::Protoss_Dragoon);
						idleLowProduction.erase(building.unit());
					}
					else
					{
						idleLowProduction.emplace(building.unit(), UnitTypes::Protoss_Dragoon);
					}
				}
				if (!Strategy().isZealotsLocked() && (Strategy().getUnitScore()[UnitTypes::Protoss_Dragoon] < Strategy().getUnitScore()[UnitTypes::Protoss_Zealot] || ((Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) < 1) || Broodwar->self()->gas() < UnitTypes::Protoss_Dragoon.gasPrice() + Buildings().getQueuedGas() + reservedGas)))
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Zealot.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Units().getSupply() + UnitTypes::Protoss_Zealot.supplyRequired() <= Broodwar->self()->supplyTotal())
					{
						building.unit()->train(UnitTypes::Protoss_Zealot);
						idleLowProduction.erase(building.unit());
					}
					else
					{
						idleLowProduction.emplace(building.unit(), UnitTypes::Protoss_Zealot);
					}
				}
				return;
			}

			// Stargate
			else if (building.getType() == UnitTypes::Protoss_Stargate)
			{
				// Set as visible so it saves resources for Arbiters if we're teching to them
				if ((Broodwar->self()->isUpgrading(UpgradeTypes::Khaydarin_Core) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core)) && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter_Tribunal) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Arbiter) < 3)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Arbiter.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Arbiter.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->train(UnitTypes::Protoss_Arbiter);
						idleHighProduction.erase(building.unit());
						return;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Arbiter);
					}
				}
				// Only build corsairs against Zerg
				if (Broodwar->enemy()->getRace() == Races::Zerg && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Corsair) < 5)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Corsair.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Corsair.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->train(UnitTypes::Protoss_Corsair);
					}
				}
			}

			// Robotics Facility
			else if (building.getType() == UnitTypes::Protoss_Robotics_Facility)
			{
				// If detection is absolutely needed, cancel anything in queue and get the Observer immediately
				if (Strategy().needDetection() && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observer) == 0)
				{
					if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observatory) > 0)
					{
						if (building.unit()->isTraining())
						{
							for (auto &unit : building.unit()->getTrainingQueue())
							{
								if (unit == UnitTypes::Protoss_Reaver || unit == UnitTypes::Protoss_Shuttle)
								{
									building.unit()->cancelTrain();
								}
							}
						}

						if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Observer.mineralPrice() && Broodwar->self()->gas() >= UnitTypes::Protoss_Observer.gasPrice())
						{
							building.unit()->train(UnitTypes::Protoss_Observer);
							idleHighProduction.erase(building.unit());
							return;
						}
						else
						{
							idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Observer);
						}
					}
				}

				// If we need an Observer
				if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observatory) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Observer) < (floor(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) / 3) + 1))
				{
					// If we can afford an Observer, train, otherwise, add to priority
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Observer.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Observer.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->train(UnitTypes::Protoss_Observer);
						idleHighProduction.erase(building.unit());
						return;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Observer);
					}
				}

				// If we need a Shuttle
				else if ((Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) / 2 > Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle)) || (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Shuttle) <= 0))
				{
					// If we can afford a Shuttle, train, otherwise, add to priority
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Shuttle.mineralPrice() + Buildings().getQueuedMineral())
					{
						building.unit()->train(UnitTypes::Protoss_Shuttle);
						idleHighProduction.erase(building.unit());
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Shuttle);
					}
				}

				// If we need a Reaver			
				else if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Support_Bay) > 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Reaver) < 10)
				{
					// If we can afford a Reaver, train, otherwise, add to priority
					if (Broodwar->self()->minerals() >= UnitTypes::Protoss_Reaver.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Protoss_Reaver.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->train(UnitTypes::Protoss_Reaver);
						idleHighProduction.erase(building.unit());
						return;
					}
					else
					{
						idleHighProduction.emplace(building.unit(), UnitTypes::Protoss_Reaver);
					}
				}
			}

			// Templar Archives
			else if (building.getType() == UnitTypes::Protoss_Templar_Archives)
			{
				if (Strategy().getNumberTerran() == 0 || (Strategy().getNumberTerran() > 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Arbiter) > 0))
				{
					if (!Broodwar->self()->hasResearched(TechTypes::Psionic_Storm))
					{
						if (Broodwar->self()->minerals() >= TechTypes::Psionic_Storm.mineralPrice() && Broodwar->self()->gas() >= TechTypes::Psionic_Storm.gasPrice())
						{
							building.unit()->research(TechTypes::Psionic_Storm);
							idleTech.erase(building.unit());
						}
						else
						{
							idleTech.emplace(building.unit(), TechTypes::Psionic_Storm);
						}
					}
					else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Amulet) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_High_Templar) > 2)
					{
						if (Broodwar->self()->minerals() >= UpgradeTypes::Khaydarin_Amulet.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UpgradeTypes::Khaydarin_Amulet.gasPrice())
						{
							building.unit()->upgrade(UpgradeTypes::Khaydarin_Amulet);
							idleUpgrade.erase(building.unit());
						}
						else
						{
							idleUpgrade.emplace(building.unit(), UpgradeTypes::Khaydarin_Amulet);
						}
					}
				}
			}

			// Arbiter Tribunal
			else if (building.getType() == UnitTypes::Protoss_Arbiter_Tribunal)
			{
				if (!Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core))
				{
					if (Broodwar->self()->minerals() >= UpgradeTypes::Khaydarin_Core.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UpgradeTypes::Khaydarin_Core.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->upgrade(UpgradeTypes::Khaydarin_Core);
						idleUpgrade.erase(building.unit());
					}
					else
					{
						idleUpgrade.emplace(building.unit(), UpgradeTypes::Khaydarin_Core);
					}
				}
				else if (!Broodwar->self()->hasResearched(TechTypes::Stasis_Field))
				{
					if (Broodwar->self()->minerals() >= TechTypes::Stasis_Field.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= TechTypes::Stasis_Field.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->research(TechTypes::Stasis_Field);
						idleTech.erase(building.unit());
					}
					else
					{
						idleTech.emplace(building.unit(), TechTypes::Stasis_Field);
					}
				}
				else if (!Broodwar->self()->hasResearched(TechTypes::Recall))
				{
					if (Broodwar->self()->minerals() >= TechTypes::Recall.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= TechTypes::Recall.gasPrice() + Buildings().getQueuedGas())
					{
						building.unit()->research(TechTypes::Recall);
						idleTech.erase(building.unit());
					}
					else
					{
						idleTech.emplace(building.unit(), TechTypes::Recall);
					}
				}
			}
		}
	}
	return;
}

void ProductionTrackerClass::updateTerran()
{
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Barracks) >= (3 * Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center)))
	{
		barracksSat = true;
	}

	for (auto &building : Buildings().getMyBuildings())
	{
		if (building.second.unit() && building.second.unit()->isIdle() && building.second.unit()->isCompleted())
		{

			// Barracks
			if (building.second.getType() == UnitTypes::Terran_Barracks)
			{
				if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Medic) < 6 && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Academy) >= 1)
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Terran_Medic.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UnitTypes::Terran_Medic.gasPrice() + Buildings().getQueuedGas() + reservedGas)
					{
						building.first->train(UnitTypes::Terran_Medic);
					}
					else
					{
						idleLowProduction.emplace(building.first, UnitTypes::Terran_Medic);
					}
				}
				else
				{
					if (Broodwar->self()->minerals() >= UnitTypes::Terran_Marine.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral)
					{
						building.first->train(UnitTypes::Terran_Marine);
					}
					else
					{
						idleLowProduction.emplace(building.first, UnitTypes::Terran_Marine);
					}
				}
			}

			// Academy
			if (building.second.getType() == UnitTypes::Terran_Academy)
			{
				if (!Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && Broodwar->self()->minerals() >= TechTypes::Stim_Packs.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= TechTypes::Stim_Packs.gasPrice() + Buildings().getQueuedGas())
				{
					building.first->research(TechTypes::Stim_Packs);
				}
				else
				{
					idleTech.emplace(building.first, TechTypes::Stim_Packs);
				}
				if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && !Broodwar->self()->getUpgradeLevel(UpgradeTypes::U_238_Shells) && Broodwar->self()->minerals() >= UpgradeTypes::U_238_Shells.mineralPrice() + Buildings().getQueuedMineral() + reservedMineral && Broodwar->self()->gas() >= UpgradeTypes::U_238_Shells.gasPrice() + Buildings().getQueuedGas() + reservedGas)
				{
					building.first->upgrade(UpgradeTypes::U_238_Shells);
				}
			}

			// Engineering Bay
			if (building.second.getType() == UnitTypes::Terran_Engineering_Bay)
			{
				if (Broodwar->self()->minerals() >= UpgradeTypes::Terran_Infantry_Armor.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UpgradeTypes::Terran_Infantry_Armor.gasPrice() + Buildings().getQueuedGas())
				{
					if (Broodwar->enemy()->getRace() == Races::Zerg)
					{
						building.first->upgrade(UpgradeTypes::Terran_Infantry_Armor);
					}
					else
					{
						building.first->upgrade(UpgradeTypes::Terran_Infantry_Weapons);
					}
				}
				else
				{
					idleUpgrade.emplace(building.first, UpgradeTypes::Terran_Infantry_Armor);
				}
			}

			// Factory
			if (building.second.getType() == UnitTypes::Terran_Factory)
			{
				if (!building.second.unit()->getAddon())
				{
					building.second.unit()->buildAddon(UnitTypes::Terran_Machine_Shop);
				}
				else if (Broodwar->self()->minerals() >= UnitTypes::Terran_Siege_Tank_Tank_Mode.mineralPrice() + Buildings().getQueuedMineral() && Broodwar->self()->gas() >= UnitTypes::Terran_Siege_Tank_Tank_Mode.gasPrice() + Buildings().getQueuedGas())
				{
					building.second.unit()->train(UnitTypes::Terran_Siege_Tank_Tank_Mode);
				}
				else
				{
					idleHighProduction.emplace(building.first, UnitTypes::Terran_Siege_Tank_Tank_Mode);
				}
			}

			// Machine Shop
			if (building.second.getType() == UnitTypes::Terran_Machine_Shop)
			{
				if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
				{
					building.second.unit()->research(TechTypes::Tank_Siege_Mode);
				}
				else
				{
					idleTech.emplace(building.second.unit(), TechTypes::Tank_Siege_Mode);
				}
			}
		}
		else
		{
			idleLowProduction.erase(building.second.unit());
			idleHighProduction.erase(building.second.unit());
			idleTech.erase(building.second.unit());
			idleUpgrade.erase(building.second.unit());
		}
	}
}

void ProductionTrackerClass::updateZerg()
{
}