#pragma once
// Include API files
#include <BWAPI.h>
#include "Singleton.h"
#include "..\BWEM\bwem.h"
#include "..\BWEB\BWEB.h"

#define STORM_LIMIT 5.0
#define STASIS_LIMIT 8.0
#define LOW_SHIELD_PERCENT_LIMIT 0.2
#define LOW_MECH_PERCENT_LIMIT 0.2
#define MIN_THREAT 0.01f
#define SIM_RADIUS 640.0

#define MAX_SCARAB 5 + (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Reaver_Capacity) * 5)
#define MAX_INTERCEPTOR 4 + (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Carrier_Capacity) * 4)

// Namespaces
using namespace BWAPI;
using namespace BWEM;
using namespace std;
using namespace BWEB;

namespace McRave
{
    class UnitInfo;
    class ResourceInfo;

    struct Line {
        double yInt;
        double slope;
        double y(int x) { return (slope * double(x)) + yInt; }
        Line(double y, double s) {
            yInt = y, slope = s;
        }
    };

    enum class Role {
        None, Working, Fighting, Transporting, Scouting, Producing, Defending, Supporting
    };

    enum class TransportState {
        None, Loading, Unloading, Monitoring, Engaging, Retreating
    };

    enum class ResourceState {
        None, Assignable, Mineable
    };

    enum class GlobalState {
        None, Engaging, Retreating
    };

    enum class LocalState {
        None, Engaging, Retreating
    };

    enum class SimState {
        None, Win, Loss, HighWin, HighLoss
    };
}

namespace
{
    auto &mapBWEM = BWEM::Map::Instance();
}

// Include standard libraries that are needed
#include <set>
#include <ctime>
#include <chrono>

// Include other source files
#include "BuildingManager.h"
#include "BuildOrder.h"
#include "CommandManager.h"
#include "GoalManager.h"
#include "GridManager.h"
#include "StationManager.h"
#include "WorkerManager.h"
#include "PlayerManager.h"
#include "ProductionManager.h"
#include "PylonManager.h"
#include "ResourceManager.h"
#include "ScoutManager.h"
#include "StrategyManager.h"
#include "SupportManager.h"
#include "TargetManager.h"
#include "TerrainManager.h"
#include "TransportManager.h"
#include "UnitManager.h"
#include "Util.h"
#include "Visuals.h"

// Namespace to access all managers globally
namespace McRave
{
    inline StationManager& MyStations() { return StationSingleton::Instance(); }
    inline StrategyManager& Strategy() { return StrategySingleton::Instance(); }
    inline TargetManager& Targets() { return TargetSingleton::Instance(); }
    inline TerrainManager& Terrain() { return TerrainSingleton::Instance(); }
    inline TransportManager& Transport() { return TransportSingleton::Instance(); }
}
using namespace McRave;
